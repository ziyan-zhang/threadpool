//
// Created by ziyan on 22-4-28.
//
// 一多个LSM，多个thread。
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// Open a database.

void func(int id, leveldb::DB* db2) {
    // 这里实现的是先写，写完睡一秒，然后再读。写入通过db->Write(options, updates)批量写。

    // Batch atomic write.
//    leveldb::DB* db2;
    leveldb::WriteBatch batch;
    std::string key = "name" + std::to_string(id);
    std::string value = "thread_val_" + std::to_string(id);
    batch.Put(key.c_str(), value.c_str());

    // write有两种方式：db->Write(options, updates)批量写入；db->Put(options, key, value)单点写入。
//    auto status = db2->Write(leveldb::WriteOptions(), &batch);
//    assert(status.ok());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Read data.
    std::string val;
    auto status = db2->Get(leveldb::ReadOptions(), key.c_str(), &val);
    assert(status.ok());
    std::cout << "thread " << std::this_thread::get_id() << " : " << val << std::endl;
}

int main()
{
    leveldb::DB* db1;
    leveldb::DB* db2;


    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(opts, "./testdb", &db1);
    leveldb::Status status_ = leveldb::DB::Open(opts, "./testdb", &db2);

    assert(status.ok());

    // Write data.
//    status = db1->Put(leveldb::WriteOptions(), "main_key", "main_value");
//    assert(status.ok());

    // Read data.
    std::string val;
    status = db1->Get(leveldb::ReadOptions(), "main_key", &val);
    assert(status.ok());
    std::cout << "主线程读出main_key的value：" << val << std::endl;

    std::vector<std::thread> threads(8);
    for (int i = 0; i < 8; i++) {
        threads[i] = std::thread(func, i, db2);
    }

    for (int i = 0; i < 8; i++) {
        threads[i].join();
    }

    // Scan database.
    std::cout << "下面对db进行全局scan操作：" << std::endl;
    leveldb::Iterator* it = db1->NewIterator(leveldb::ReadOptions());

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }
    assert(it->status().ok());

    // Close a database.
    delete it;
    delete db1;
    delete db2;  // 不open就delete就会报错
}
