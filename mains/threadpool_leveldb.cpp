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
#include <sstream>
#include <random>
#include "util.h"

std::mutex enqueue_print_mutex;
int POOL_SIZE = 2;

// 主线程里面定义一些输入文件路径
std::string input_filename1 = "../data/ebs_segment_io_records/AY283G_2022-01-01_00:00:00_2022-01-02_00:00:00/device/111969901601639.csv";
std::string values(1024*1024, '0');
std::default_random_engine e2(255);
std::uniform_int_distribution<uint64_t> uniform_dist_value(0, (uint64_t)values.size() - value_size - 1);

// 我们希望将一些数据最合成单一对象，但又不想麻烦地定义一个新数据结构来表示这些数据时，可以使用std::tuple这一“快速而随意”的数据结构。
std::tuple<std::vector<std::string>, std::vector<char>> get_keys1_and_operations1(std::string input_filename) {
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
//    std::vector<std::string> keys_all_truncated(keys_all.begin(), keys_all.end());
//    std::vector<char> ops_all_truncated(ops_all.begin(), ops_all.end());

    std::tuple<std::vector<std::string>, std::vector<char>> keys_and_ops;
    keys_and_ops = std::make_tuple(keys_all, ops_all);
    return keys_and_ops;
}

int main() {
    // 创建POOL_SIZE个进程，没有任务时，处于阻塞状态
    ThreadPool pool(POOL_SIZE);
    // leveldb运行结果是一个future对象包装下的Status(而非直接的Status), future对象通常在另一个线程中获取std::packaged_task的执行结果
    std::vector<std::future<int>> results;

    // 在主线程里面打开leveldb，产生读写请求。在线程池里面处理读写请求。
    leveldb::DB* db0;
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status;
    std::string db_location("./db_dir/testdb0");
    status = leveldb::DB::Open(opts, db_location, &db0);
    assert(status.ok());

    // 读取读keys1和operations1
    std::tuple<std::vector<std::string>, std::vector<char>> keys1_and_vals1 = get_keys1_and_operations1(input_filename1);
    std::vector<std::string> keys1 = std::get<0>(keys1_and_vals1);
    std::vector<char> ops1 = std::get<1>(keys1_and_vals1);
    int trace1_len = keys1.size();
    std::cout << "keys1长度：" << trace1_len << std::endl;

    // 执行4个任务，这八个任务对应两个leveldb实例
    for (int i=0; i<trace1_len; i++) {
        // pool.enqueue返回的是一个std::future<leveldb::Status>类型。
        results.emplace_back(pool.enqueue([&db0, keys1, ops1, i, &status] {
            const std::string& key = keys1[i];
            const char& op_flag = ops1[i];
            std::string value;
            if (op_flag == 'R') {
                status = db0->Get(leveldb::ReadOptions(), key, &value);
                if (!status.ok()) {
                    std::cout << key << " Not Found" << std::endl;
                }
            } else if (op_flag == 'W') {
                status = db0->Put(leveldb::WriteOptions(), key, {values.data() + uniform_dist_value(e2), (uint64_t)64});
                assert(status.ok() && "Put Error");
            } else throw "error";
            return 0;
        }));
    }

    for (auto&& result : results) {
        result.get();
    }

    delete db0;  // 这里不显式地删除db也不会报错，可能因为db上的操作都做完了？
    std::cout << "the end" << std::endl;

    return 0;
}