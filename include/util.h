// 一些threadpool_level.cpp这个主程序里面用到的函数
// Created by zy on 22-7-14.
// 把定义和实现放在了一起

#ifndef THREADPOOL_UTIL_H
#define THREADPOOL_UTIL_H

// 一些常量
int key_size = 16;
int value_size = 64;

// 一些函数
std::string generate_key(const std::string& key);
std::string generate_key(const std::string& key) {
    std::string result = std::string(key_size - key.length(), '0') + key;
    return std::move(result);
}





#endif //THREADPOOL_UTIL_H