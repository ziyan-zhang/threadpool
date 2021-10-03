#include <future>
#include <iostream>
#include <ostream>

int main() {
  // 封装一下函数
  std::packaged_task<int()> task([] { return 7 });
  // 获取返回值place_holder
  std::future<int> f1 = task.get_future();
  // 执行函数
  task();
  f1.wait();

  std::cout << f1.get() << std::endl;

  return 0;
}
