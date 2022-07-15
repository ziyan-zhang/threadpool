//
// Created by ziyan on 22-4-28.
//
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
#include <mutex>

// Open a database.

void func(int id, leveldb::DB* db) {
    // 这里实现的是先写，写完睡一秒，然后再读。写入通过db->Write(options, updates)批量写。

    // Batch atomic write.
//    leveldb::DB* db;
    leveldb::WriteBatch batch;
//    std::string key = "name" + std::to_string(id);
    std::string key = "name";

    std::string value = "thread_val_" + std::to_string(id);
    batch.Put(key.c_str(), value.c_str());

    // write有两种方式：db->Write(options, updates)批量写入；db->Put(options, key, value)单点写入。
    auto status = db->Write(leveldb::WriteOptions(), &batch);
    assert(status.ok());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Read data.
    std::string val;
    status = db->Get(leveldb::ReadOptions(), key.c_str(), &val);
    assert(status.ok());
    std::cout << "thread " << std::this_thread::get_id() << " : " << val << std::endl;
}

int main()
{
    leveldb::DB* db;

    // leveldb::DB的指针open了才能用，用指的是传给threads用。
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(opts, "./testd", &db);
    assert(status.ok());

    std::vector<std::thread> threads(8);
    for (int i = 0; i < 8; i++) {
        threads[i] = std::thread(func, i, db);
    }

    for (int i = 0; i < 8; i++) {
        threads[i].join();
    }

    // Scan database.
    std::cout << "下面对db进行全局scan操作：" << std::endl;
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }
    assert(it->status().ok());
    delete it;

    delete db;  // 不open就delete就会报错
}
