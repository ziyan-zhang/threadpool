//
// Created by zy on 22-7-11.
//
#include "thread_pool.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include <vector>
#include <iostream>
#include <thread>
#include <fstream>
#include <random>
#include <x86intrin.h>
#include "util.h"
#include "timer.h"
#include "stats.h"

std::mutex print_mutex;

int POOL_SIZE = 4;
int NUM_LSMTs = 1;

// 主线程里面定义一些输入文件路径
std::string input_filename1 = "../data/ebs_segment_io_records/AY283G_2022-01-01_00:00:00_2022-01-02_00:00:00/device/111969901601639.csv";
std::string values(1024*1024, '0');
std::default_random_engine e2(255);
std::uniform_int_distribution<uint64_t> uniform_dist_value(0, (uint64_t)values.size() - value_size - 1);

// 我们希望将一些数据最合成单一对象，但又不想麻烦地定义一个新数据结构来表示这些数据时，可以使用std::tuple这一“快速而随意”的数据结构。
std::tuple<std::vector<std::string>, std::vector<char>, std::vector<int>> get_kv(std::string input_filename, int trace_id) {
    std::ifstream input(input_filename);
    assert(input.good());
    std::string dev_id;
    char* op_flag;
    std::string key;
    std::vector<std::string> keys_all;
    std::vector<char> ops_all;

    std::string line;
    while (getline(input, line)) {
        std::vector<std::string> data_line;
        std::string number;
        std::istringstream readstr(line);
        // csv中每行的数据使用逗号分割
        for (int j=0; j<5; j++) {
            getline(readstr, number, ',');
            data_line.push_back(number);
        }
        op_flag = (char*) (data_line[1]).data();
        key = data_line[2];
        std::string the_key = generate_key(key);
        keys_all.push_back(std::move(the_key));
        ops_all.push_back(std::move(*op_flag));
    }

    std::cout << "这里是为了调试，减少了实际的key的数量" << std::endl;
    int trunc_size = 1000000;
    if (trunc_size > keys_all.size()) trunc_size = keys_all.size();

    std::vector<std::string> keys_all_truncated(keys_all.begin(), keys_all.begin()+trunc_size);
    std::vector<char> ops_all_truncated(ops_all.begin(), ops_all.begin()+trunc_size);

    std::vector<int> db_serial(trunc_size, trace_id);
    auto keys_and_ops = std::make_tuple(keys_all_truncated, ops_all_truncated, db_serial);  // auto还是牛逼
    return keys_and_ops;
}

int main() {
    // 创建POOL_SIZE个进程，没有任务时，处于阻塞状态
    ThreadPool pool(POOL_SIZE);
    // leveldb运行结果是一个future对象包装下的Status(而非直接的Status), future对象通常在另一个线程中获取std::packaged_task的执行结果
    std::vector<std::future<int>> results;

    // 在主线程里面打开leveldb，产生读写请求。在线程池里面处理读写请求。
    std::vector<leveldb::DB*> dbs(NUM_LSMTs);
    std::vector<std::string> db_locations(NUM_LSMTs);
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status;

    for (int i=0; i<NUM_LSMTs; i++) {
        db_locations[i] = "./db_dir/testdb" + std::to_string(i);
        status = leveldb::DB::Open(opts, db_locations[i], &dbs[i]);
    }
    assert(status.ok());

    // 一一些options
    leveldb::ReadOptions read_options = leveldb::ReadOptions();
    leveldb::WriteOptions write_options = leveldb::WriteOptions();
    write_options.sync=false;

    // 读取读keys1和operations1
    std::tuple<std::vector<std::string>, std::vector<char>, std::vector<int>> kv_0 = get_kv(input_filename1, 0);
    std::vector<std::string> keys_0 = std::get<0>(kv_0);
    std::vector<char> ops_0 = std::get<1>(kv_0);
    std::vector<int> dbSerial_0 = std::get<2>(kv_0);



    int trace1_len = keys_0.size();
    std::cout << "keys1长度：" << trace1_len << std::endl;

    int NUM_TRACES = 1;
    std::vector<std::mutex> mtxs(NUM_TRACES);

    std::vector<int> db_serials = dbSerial_0;

    // 设置一个times变量，用于统计时间，不能直接用std::vector，因其不是线程安全的. 这里实现了一个线程安全队列Timer.
    Timer timer_all;
    Timer timer_read;
    Timer timer_write;
    Timer timer_all_inner;

    unsigned int dummy1=0;
    unsigned int dummy2=0;
    unsigned int dummy3=0;
    unsigned int dummy4=0;

    uint64_t all_st =  __rdtscp(&dummy1);
    // 执行4个任务，这八个任务对应两个leveldb实例
    for (int i=0; i<trace1_len; i++) {
        // pool.enqueue返回的是一个std::future<leveldb::Status>类型。
        // lambda捕获值的话，每次入队任务都要拷贝，会耗尽内存
        // 这里的i要值捕获，引用捕获会报错，因为引用捕获是运行的时候才拿值，不同线程拿的时候，i可能变化了，导致报错([&]是不行的)。
        results.emplace_back(pool.enqueue([&, i] {
            const std::string& key = keys_0[i];
            const char& op_flag = ops_0[i];
            const int& serial = db_serials[i];
            std::string value;
            uint64_t all_inner_st =  __rdtscp(&dummy2);
            if (op_flag == 'R') {
                uint64_t read_st = __rdtscp(&dummy3);
                status = dbs[serial]->Get(read_options, key, &value);
                uint64_t read_elapsed = __rdtscp(&dummy3) - read_st;
                timer_read.push_back(read_elapsed);
                if (!status.ok()) {
                    std::unique_lock<std::mutex> lock(print_mutex);  // 用个打印锁保护一下
//                    std::cout << "第 " << i << " 个key: " << key << " Not Found" << std::endl;
                }
            } else if (op_flag == 'W') {
                uint64_t write_st = __rdtscp(&dummy4);
                status = dbs[serial]->Put(write_options, key, {values.data() + uniform_dist_value(e2), (uint64_t)64});
                uint64_t write_elapsed = __rdtscp(&dummy4) - write_st;
                timer_write.push_back(write_elapsed);
                if (!status.ok()) {
                    std::cout << key << std::endl;
                    printf("here");
                }
                assert(status.ok() && "Put Error");
            } else {
                printf("here");
            }
            uint64_t all_inner_elapsed = __rdtscp(&dummy2) - all_inner_st;
            timer_all_inner.push_back(all_inner_elapsed);
            return 0;
        }));
    }

    // 通过get来进行同步，保证线程结束
    for (auto&& result : results) {
        result.get();
    }

    uint64_t all_elapsed = __rdtscp(&dummy1) - all_st;
    uint64_t all_elapsed_uSec = all_elapsed / (2.9 * 1000000000) * 1000000;
    // all如果上面的result.get()之前的话，前面的任务并不一定（而且很可能没有）结束，因为都是异步的。包括read、write的统计也不一定结束了。
    timer_all.push_back(all_elapsed_uSec);

    // 把所有的timer记录的东西打印出来
    uint64_t timer_avg = (uint64_t) timer_all.Time()[0]/trace1_len;
    std::ofstream out("out.txt", std::ofstream::app);
    out << "\nPOOL_SIZE: " << POOL_SIZE << ", NUM_LSMTs: " << NUM_LSMTs << ", trace len: " << trace1_len << std::endl;
    out << "Timer all, sum: " << timer_all.Time()[0] << ", MEAN: " << timer_avg << std::endl;
    out.close();
    print_mean_and_std(timer_all_inner.Time(), "all inner");
    print_mean_and_std(timer_read.Time(), "read");
    print_mean_and_std(timer_write.Time(), "write");

    for (int i=0; i<NUM_LSMTs; i++) {
        delete dbs[i];
    }

    std::cout << "the end" << std::endl;
    return 0;
}
