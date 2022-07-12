//
// Created by ziyan on 22-5-11.
//
#include <iostream>
#include <mutex>
#include <thread>

std::mutex gMutex;

int Reenter() {
    std::lock_guard<std::mutex> lLock(gMutex);
    return 10;
}

int Callback() {
    std::lock_guard<std::mutex> lLock(gMutex);
    return Reenter();
}

int main(int argc, char** argv) {
    std::cout << "hello cmake world" << std::endl;
    std::thread lThread(Callback);
    lThread.join();
    std::cout << "sub thread is gone" << std::endl;
    return 0;
}