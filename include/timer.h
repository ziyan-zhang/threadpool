//
// Created by zy on 22-7-15.
//

#ifndef THREADPOOL_TIMER_H
#define THREADPOOL_TIMER_H

#include <cstdint>
#include <utility>
#include <cassert>
#include <x86intrin.h>
#include <mutex>


// Timer是一个计时用的线程安全向量
class Timer {
private:
    // 封装成私有的属性，更加安全
    std::vector<uint64_t> time_collected;
    // 线程安全需要锁来保证
    std::mutex log_mtx;

public:
    void push_back(uint64_t time_elapsed) {
        std::unique_lock<std::mutex> lck(log_mtx);
        time_collected.push_back(time_elapsed);
    }
    std::vector<uint64_t> Time();

    Timer() = default;
    ~Timer() = default;
};

// 使用范围解析运算符定义，跟push_back成员函数的类内定义是一样的
std::vector<uint64_t> Timer::Time() {
    return time_collected;
}






#endif //THREADPOOL_TIMER_H
