#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>
#include <iostream>

class ThreadPool {
 public:
  ThreadPool(size_t);
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
  ~ThreadPool();

 private:
  std::vector<std::thread> workers;         // 执行任务的工作队列
  std::queue<std::function<void()>> tasks;  // 任务队列，worker所执行的函数

  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;  // ThreadPool通过stop终止所有子线程
};

/**
 * @brief Construct a new Thread Pool:: Thread Pool object
 *
 * @param pool_size
 */
inline ThreadPool::ThreadPool(size_t pool_size) : stop(false) {
  // 建立pool_size个线程，每个线程的任务是从任务队列中拉出一个任务并执行
  for (size_t i = 0; i < pool_size; ++i) {
      workers.emplace_back([i, this] {
          // 下面的 while (true) 后面的整段代码应该是作为一个thread初始化的函数，立即被执行的。
          while (true) {
              std::function<void()> task;
              {
                  std::unique_lock<std::mutex> lock(this->queue_mutex);
                  // template <class Predicate>，void wait (unique_lock<mutex>& lck, Predicate pred) = while(!pred()) wait(lck);
                  // 没拿到队列锁lck、并且pred为false时才阻塞；收到其他线程通知后（从wait走出来，拿到锁lck了），再次检查只有当pred为true时才解除阻塞。
                  // 这里是一个lambda函数，捕获this，拿到其成员属性stop和task的是否非空状态。
                  this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                  // pred为true: 拿到stop信号或者队列非空; pred为false: 没拿到stop信号、并且队列为空。
                  // 后者意味着：没拿到stop信号的大前提下：刚初始化线程池，故线程队列为空; 或者线程队列中的任务全被做完了，故线程队列变空。两种情况都会阻塞。

                  // 没有被阻塞的情况，代码走到这里。要么拿到了stop信号，要么队列不空。分为如下三种情况：
                  // 1. 拿到stop信号、队列为空：停止while，也即这个线程不再是永生（while (true)）的了，要结束了。
                  if (this->stop && this->tasks.empty()) return;
//                  {  // 主线程设置停止执行，同时任务队列为空
//                      std::cout << "主线程设置停止执行，同时任务队列为空，线程池即将返回" << std::endl;
//                      return;
//                  }
                  // 2. 3. 任务队列不空：那就把队列中的任务执行完：将队首任务拿出来执行。
                  task = std::move(this->tasks.front());
                  this->tasks.pop();
              }
              // 选出task之后，队列就不用被保护了。所以可以先交出队列锁，再执行任务。否则，就只剩线程安全，没有并发了。
              std::cout << "thread " << i << " is working" << std::endl;
              task();
              // 每完成一个任务，这个地方就通过一次。也即完成了多少个任务，task()执行多少次，这里就通过多少次。
          }
      });
      // 这个位置在任何任务被加进队列前(初始化线程池的时候，并没有办法同时或在此之前把任务加进去)，通过了pool_size次，并且只运行pool_size次。
  }
}

inline ThreadPool::~ThreadPool() {
  // 终止所有子线程
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for (std::thread& worker : workers) {
    worker.join();
  }
}

// 添加一个新任务，其函数类型为F，参数类型为Args，这里的具名右值引用是为了统一const
// T &和T &两种形参，算是形参的万能模板
// std::result_of，用于在编译的时候推导出一个可调用对象（函数，std::function或者重载了operator()操作的对象等）的返回值类型。主要用于模板编写中。
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  // std::result_of<>::type是函数的返回值类型
//  auto aa = std::result_of<F(Args...)>;
  using return_type = typename std::result_of<F(Args...)>::type;
  // std::make_shared，c++11，返回一个智能指针。
  // std::packaged_task，包装一个可调用的对象，并且允许一步获取该可调用对象产生的结果。与std::function类似，
  // 只不过将其包装的可调用对象的执行结果传递给一个std::future对象（该对象通常在另一个线程中获取std::packaged_task的执行结果）。
  // std::bind头文件是<functional>，它是一个函数适配器，接受一个可调用对象，生成一个新的可调用对象来“适应”原对象的参数列表。
  // std::forward，完美转发，使万能引用内部的形参保留原来的左右值属性。注意到f和args都被完美转发了。
  auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  // 分离返回值和函数体
  std::future<return_type> res = task_ptr->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (stop) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }
    // 这里*task就是std::packaged_task<return_type()>, (*task)() = f(args)，
    // 而tasks是std::functional，封装了这段执行的代码，真正的执行是在消费者线程中
    // 下面这个是lambda表达式，capture子句[]捕获了task_ptr，参数列表()为空，可变规范、异常规范、后续返回类型为空。
    tasks.emplace([task_ptr]() { (*task_ptr)(); });
  }
  // 通知一个消费者线程，有新任务了
  condition.notify_one();
  return res;
}

#endif
