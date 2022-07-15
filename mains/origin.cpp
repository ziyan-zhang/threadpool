#include <thread>
#include <future>
#include <iostream>
#include <ostream>

int main() {
  // 封装一下函数
  std::packaged_task<int()> task([] { return 7; });
  // 获取返回值 place_holder
  std::future<int> f1 = task.get_future();
  // 执行函数
  std::thread th (std::move(task));

  f1.wait();

  std::cout << "packaged task return: " << f1.get() << std::endl;
  th.join();

  return 0;
}
