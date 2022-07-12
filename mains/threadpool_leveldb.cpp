//
// Created by zy on 22-7-11.
//
#include "thread_pool.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include <vector>
#include <iostream>
#include <thread>


std::mutex enqueue_print_mutex;
int POOL_SIZE = 2;


int main() {
    // 创建POOL_SIZE个进程，没有任务时，处于阻塞状态
    ThreadPool pool(POOL_SIZE);
    // leveldb运行结果是一个future对象包装下的Status(而非直接的Status), future对象通常在另一个线程中获取std::packaged_task的执行结果
    std::vector<std::future<leveldb::Status>> results;

    // 在主线程里面打开leveldb，产生读写请求。在线程池里面处理读写请求。
    leveldb::DB* db;
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(opts, "./testdb0", &db);
    assert(status.ok());

    std::string key1 = "k1";
    std::string value1 = "v1";

    std::vector<std::string> keys1;
    std::vector<std::string> values1;
    keys1.emplace_back(key1);
    values1.emplace_back(value1);
    keys1.emplace_back(key1);
    values1.emplace_back(value1);
    keys1.emplace_back(key1);
    values1.emplace_back(value1);
    keys1.emplace_back(key1);
    values1.emplace_back(value1);

    int trace_len = keys1.size();

    // 执行4个任务，这八个任务对应两个leveldb实例
    for (int i=0; i<trace_len; i++) {
        results.emplace_back(pool.enqueue([db, keys1, values1, i] {
            {
                // 打印这里加个全局唯一锁，看得清楚
                std::unique_lock<std::mutex> l(enqueue_print_mutex);
                std::cout << "task " << i << " getting onboard (enqueued)" << std::endl;
            }
            leveldb::Status status = db->Put(leveldb::WriteOptions(), keys1[i], values1[i]);
            return status;
        }));
    }

    for (auto&& result : results) {
        leveldb::Status status = result.get();
    }

    delete db;  // 这里不显式地删除db也不会报错，可能因为db上的操作都做完了？
    std::cout << "the end" << std::endl;

    return 0;
}