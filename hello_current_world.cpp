//
// Created by ziyan on 22-5-2.
//
// 一个简单的hello concurrent world并发程序
#include <iostream>
#include <thread>  // 记得编译的时候要加参数-pthread

void f(std::thread t);

int main() {
    void some_function();
    f(std::thread(some_function));
    std::thread t(some_function);
    f(std::move(t));
}