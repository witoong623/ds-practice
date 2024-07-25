#include "ThreadPool.h"

#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <functional>
#include <vector>
#include <deque>
#include <type_traits>


ThreadPool::ThreadPool(int num_threads) 
{
  for (int i = 0; i < num_threads; i++) {
    pool.emplace_back(&ThreadPool::run, this);
  }
}

ThreadPool::~ThreadPool() {
  is_active = false;
  cv.notify_all();
  for (auto& th : pool) {
    th.join();
  }
}

void ThreadPool::post(std::packaged_task<void()> job) {
  std::unique_lock lock(guard);
  pending_jobs.emplace_back(std::move(job));
  cv.notify_one();
}

void ThreadPool::run() noexcept
{
  while (is_active)
  {
    thread_local std::packaged_task<void()> job;
    {
      std::unique_lock lock(guard);
      cv.wait(lock, [&]{ return !pending_jobs.empty() || !is_active; });
      if (!is_active) break;
      job.swap(pending_jobs.front());
      pending_jobs.pop_front();
    }
    job();
  }
}
