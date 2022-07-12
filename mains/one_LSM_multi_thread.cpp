//
// Created by ziyan on 22-4-26.
//
// 一个LSM，多个thread访问它。我想要的是多个LSM，多个thread访问他们。
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// 既然只有一个leveldb，那不妨直接定义成全局的。
leveldb::DB* db;


void func(int id) {

    // Batch atomic write.
    leveldb::WriteBatch batch;
    std::string key = "name" + std::to_string(id);
    std::string value = "lllaaa" + std::to_string(id);
    batch.Put(key.c_str(), value.c_str());

    auto status = db->Write(leveldb::WriteOptions(), &batch);
    assert(status.ok());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Read data.
    std::string val;
    status = db->Get(leveldb::ReadOptions(), key.c_str(), &val);
    assert(status.ok());
    std::cout << "thread " << std::this_thread::get_id() << " : " << val << std::endl;
}

int main()
{

    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(opts, "./testdb", &db);
    assert(status.ok());

    // Write data.
    status = db->Put(leveldb::WriteOptions(), "name", "jinhelin");
    assert(status.ok());

    // Read data.
    std::string val;
    status = db->Get(leveldb::ReadOptions(), "name", &val);
    assert(status.ok());
    std::cout << val << std::endl;

    std::vector<std::thread> threads(8);
    for (int i = 0; i < 8; i++) {
        threads[i] = std::thread(func, i);
    }

    for (int i = 0; i < 8; i++) {
        threads[i].join();
    }

    // Scan database.
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }
    assert(it->status().ok());
    delete it;

    // leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

    // Range scan, example: [name3, name8)
    for (it->Seek("name3"); it->Valid() && it->key().ToString() < "name8"; it->Next()) {
        std::cout << it->key().ToString() << ": " << it->value().ToString() << std::endl;
    }

    // Close a database.
    delete it;
    delete db;
}
