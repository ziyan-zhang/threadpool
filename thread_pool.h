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

class ThreadPool {
 public:
  ThreadPool(size_t);
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>;
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
  for (size_t i = 0; i < pool_size; ++i)
    workers.emplace_back([this] {
      while (true) {
        std::function<void()> task;
        {
          std::unique_lock<std::mutex> lock(this->queue_mutex);
          // [条件变量]等待主线程设置stop或者任务队列不空
          // 下面这行的语法，std::condition_variable::wait()第二种用法
          // template<class Predicate>
          // void wait (unique_lock<mutex>& lck, Predicate pred)
          // 在这种情况下（即设置了Predicate），只有当pred条件为false时调用wait()才会阻塞当前线程，
          // 并且在收到其他线程的通知后只有当pred为true时才会被解除阻塞。
          // 类似于 while(!pred()) wait(lck);
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });
          // 这里this->stop和任务队列是否为空将情况分为了四种。可以画图自己看一下。
          if (this->stop && this->tasks.empty()) return;  // 主线程设置停止执行，同时任务队列为空
          // 任务队列不为空，将队首的任务拿出来执行
          task = std::move(this->tasks.front());
          this->tasks.pop();
        }
        task();  // 执行任务
      }
    });
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
template <class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  // std::result_of<>::type是函数的返回值类型
  using return_type = typename std::result_of<F(Args...)>::type;
  auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  // make_shared，一个智能指针的模板，相比于用new初始化，提升性能，并且是异常安全的（具体先不管）
  // 原版new初始化   std::shared_ptr<Widget> spw(new Widget);
  // 新版std::make_shared来替换  auto spw = std::make_shared<Widget>();

  // std::bind的头文件时<functional>，它是一个函数适配器，接受一个可调用对象(callable object)，
  // 生成一个新的可调用对象来“适应”原对象的参数列表。

  // 分离返回值和函数体
  std::future<return_type> res = task_ptr->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (stop) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
    }
    // 这里*task就是std::packaged_task<return_type()>, (*task)() = f(args)，
    // 而tasks是std::functional，封装了这段执行的代码，真正的执行是在消费者线程中
    tasks.emplace([task_ptr]() { (*task_ptr)(); });
  }
  // 通知一个消费者线程，有新任务了
  condition.notify_one();
  return res;
}

#endif
