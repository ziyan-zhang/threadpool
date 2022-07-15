#include <chrono>
#include <iostream>
#include <vector>

#include "thread_pool.h"

int main() {
  // 创建4个线程，没有任务时，处于阻塞状态
  ThreadPool pool(4);
  std::vector<std::future<int>> results;
  // 执行8个任务
  for (int i = 0; i < 15; ++i) {
      //这里又是一个lambda，捕获i，没有输入参数，也没有可变规范、异常规范、拖尾的输出类型。
      results.emplace_back(pool.enqueue([i] {
      std::cout << "hello" << i << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "world" << i << std::endl;
      return i * i;
    }));
  }

  for (auto&& result : results) std::cout << result.get() << " ";
  std::cout << std::endl;

  return 0;
}
