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
  std::vector<std::thread> workers;         // 执行任务
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
          this->condition.wait(
              lock, [this] { return this->stop || !this->tasks.empty(); });
          if (this->stop && this->tasks.empty()) return;  // 主线程设置停止执行，同时任务队列为空
          // 将队首的任务拿出来执行
          task = std::move(this->tasks.front());
          this->tasks.pop();
        }
        task();
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
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  // 分离返回值和函数体
  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if (stop) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
      // 这里*task就是std::packaged_task<return_type()>, (*task)() = f(args)，
      // 而tasks是std::functional，封装了这段执行的代码，真正的执行是在消费者线程中
      tasks.emplace([task]() { (*task)(); });
    }
  }
  // 通知一个消费者线程，有新任务了
  condition.notify_one();
  return res;
}

#endif
