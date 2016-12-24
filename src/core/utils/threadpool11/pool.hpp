/*!
Copyright (c) 2013, 2014, 2015 Tolga HOŞGÖR
All rights reserved.

This file is part of threadpool11.

    threadpool11 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    threadpool11 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with threadpool11.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "worker.hpp"
#include "utility.hpp"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>

#if defined(WIN32) && defined(threadpool11_DLL)
#ifdef threadpool11_EXPORTING
#define threadpool11_EXPORT __declspec(dllexport)
#else
#define threadpool11_EXPORT __declspec(dllimport)
#endif
#else
#define threadpool11_EXPORT
#endif

namespace boost {
namespace parameter {
struct void_;
}
namespace lockfree {
template <typename T,
          class A0,
          class A1,
          class A2>
class queue;
}
}

namespace threadpool11 {

class Pool {
  friend class Worker;

public:
  enum class Method { SYNC, ASYNC };

public:
  threadpool11_EXPORT Pool(std::size_t worker_count = std::thread::hardware_concurrency());
  ~Pool();

  /*!
   * \brief postWork Posts a work to the pool for getting processed.
   *
   * If there are no threads left (i.e. you called Pool::joinAll(); prior to
   * this function) all the works you post gets enqueued. If you spawn new threads in
   * future, they will be executed then.
   *
   * Properties: thread-safe.
   */
  template <typename T>
  threadpool11_EXPORT std::future<T> postWork(std::function<T()> callable, Work::Type type = Work::Type::STD);
  // TODO: convert 'type' above to const& when MSVC fixes that bug.

  /**
   * \brief waitAll Blocks the calling thread until all posted works are finished.
   *
   * This function suspends the calling thread until all posted works are finished and, therefore, all worker
   * threads are free. It guarantees you that before the function returns, all queued works are finished.
   */
  threadpool11_EXPORT void waitAll();

  /*!
   * \brief joinAll Joins the worker threads.
   *
   * This function joins all the threads in the thread pool as fast as possible.
   * All the posted works are NOT GUARANTEED to be finished before the worker threads
   * are destroyed and this function returns.
   *
   * However, ongoing works in the threads in the pool are guaranteed
   * to finish before that threads are terminated.
   *
   * Properties: NOT thread-safe.
   */
  threadpool11_EXPORT void joinAll();

  /*!
   * \brief getWorkerCount
   *
   * Properties: thread-safe.
   *
   * \return The number of worker threads.
   */
  threadpool11_EXPORT size_t getWorkerCount() const;

  /*!
   * \brief setWorkerCount
   * \param n
   *
   * WARNING: This function behaves different based on second parameter. (Only if decreasing)
   *
   * Method::ASYNC: It will return before the threads are joined. It will just post
   *  'n' requests for termination. This means that if you call this function multiple times,
   *  worker termination requests will pile up. It can even kill the newly
   *  created workers if all workers are removed before all requests are processed.
   *
   * Method::SYNC: It won't return until the specified number of workers are actually destroyed.
   *  There still may be a few milliseconds delay before value returned by Pool::getWorkerCount is updated.
   *  But it will be more accurate compared to ASYNC one.
   */
  threadpool11_EXPORT void setWorkerCount(std::size_t n, Method method = Method::ASYNC);

  /*!
   * \brief getWorkQueueSize
   *
   * Properties: thread-safe.
   *
   * \return The number of work items that has not been acquired by workers.
   */
  threadpool11_EXPORT size_t getWorkQueueSize() const;

  /*!
    * \brief getActiveWorkerCount Gets the number of active workers when the function is called.
    *
    * The information this function returns does not really mean much. The worker may be starting to execute a work from queue,
    * it may be executing a work or it may have just executed a work.
    *
    * \return The number of active workers.
    */
  threadpool11_EXPORT size_t getActiveWorkerCount() const;

  /**
   * \brief getInactiveWorkerCount Gets the number of the inactive worker count.
   *
    * The information this function returns does not really mean much. The worker may be starting to execute a work from queue,
    * it may be executing a work or it may have just executed a work.
    *
    * \return The number of active workers.
   */
  threadpool11_EXPORT size_t getInactiveWorkerCount() const;

  /*!
   * \brief incWorkerCountBy Increases the number of threads in the pool by n.
   *
   * Properties: thread-safe.
   */
  threadpool11_EXPORT void incWorkerCountBy(std::size_t n);

  /*!
   * \brief decWorkerCountBy Tries to decrease the number of threads in the pool by n.
   *
   * Setting 'n' higher than the number of workers has no effect.
   * Calling without arguments asynchronously terminates all workers.
   *
   * \warning This function behaves different based on second parameter.
   *
   * Method::ASYNC: It will return before the threads are joined. It will just post
   *  'n' requests for termination. This means that if you call this function multiple times,
   *  worker termination requests will pile up. It can even kill the newly
   *  created workers if all workers are removed before all requests are processed.
   *
   * Method::SYNC: It won't return until the specified number of workers are actually destroyed.
   *  There still may be a few milliseconds delay before value returned by Pool::getWorkerCount is updated.
   *  But it will be more accurate compared to ASYNC one.
   *
   * Properties: thread-safe.
   */
  threadpool11_EXPORT void decWorkerCountBy(std::size_t n = std::numeric_limits<size_t>::max(),
                                            Method method = Method::ASYNC);

private:
  Pool(Pool&&) = delete;
  Pool(Pool const&) = delete;
  Pool& operator=(Pool&&) = delete;
  Pool& operator=(Pool const&) = delete;

  void spawnWorkers(std::size_t n);

  /*!
   * \brief executor
   * This is run by different threads to do necessary operations for queue processing.
   */
  void executor(std::unique_ptr<std::thread> self);

  /**
   * @brief push Internal usage.
   * @param workFunc
   */
  void push(Work::Callable* workFunc);

private:
  std::atomic<size_t> worker_count_;
  std::atomic<size_t> active_worker_count_;

  mutable std::mutex notify_all_finished_mutex_;
  std::condition_variable notify_all_finished_signal_;
  std::atomic<bool> are_all_really_finished_;

  mutable std::mutex work_signal_mutex_;
  // bool work_signal_prot; //! wake up protection // <- work_queue_size is used instead of this
  std::condition_variable work_signal_;

  std::unique_ptr<
      boost::lockfree::
          queue<Work::Callable*, boost::parameter::void_, boost::parameter::void_, boost::parameter::void_>>
      work_queue_;
  std::atomic<size_t> work_queue_size_;
};

template <typename T>
threadpool11_EXPORT inline std::future<T> Pool::postWork(std::function<T()> callable, Work::Type type) {
  std::promise<T> promise;
  auto future = promise.get_future();

  /* TODO: how to avoid copy of callable into this lambda and the ones below? In a decent way... */
  /* evil move hack */
  auto move_callable = make_move_on_copy(std::move(callable));
  /* evil move hack */
  auto move_promise = make_move_on_copy(std::move(promise));

  auto workFunc = new Work::Callable([move_callable, move_promise, type]() mutable {
    move_promise.value().set_value((move_callable.value())());
    return type;
  });

  push(workFunc);

  return future;
}

template <>
threadpool11_EXPORT inline std::future<void> Pool::postWork(std::function<void()> callable,
                                                            Work::Type const type) {
  std::promise<void> promise;
  auto future = promise.get_future();

  /* evil move hack */
  auto move_callable = make_move_on_copy(std::move(callable));
  /* evil move hack */
  auto move_promise = make_move_on_copy(std::move(promise));

  auto workFunc = new Work::Callable([move_callable, move_promise, type]() mutable {
    (move_callable.value())();
    move_promise.value().set_value();
    return type;
  });

  push(workFunc);

  return future;
}

#undef threadpool11_EXPORT
#undef threadpool11_EXPORTING
}
