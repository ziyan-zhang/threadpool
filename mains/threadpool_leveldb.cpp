//
// Created by zy on 22-7-11.
//
#include "thread_pool.h"

int main() {
    // 创建四个进程，没有任务时，处于阻塞状态
    ThreadPool pool(4);
}