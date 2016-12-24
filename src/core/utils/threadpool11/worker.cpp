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

#include "threadpool11/pool.hpp"
#include "threadpool11/worker.hpp"

#include <boost/lockfree/queue.hpp>

namespace threadpool11 {

Worker::Worker(Pool& pool)
    : thread_(std::bind(&Worker::execute, this, std::ref(pool))) {
  // std::cout << std::this_thread::get_id() << " Worker created" << std::endl;
  thread_.detach();
}

void Worker::execute(Pool& pool) {
  const std::unique_ptr<Worker> self(this); //! auto de-allocation when thread is terminated

  while (true) {
    Work::Callable* work_ptr;
    Work::Type work_type = Work::Type::STD;

    ++pool.active_worker_count_;

    while (work_type != Work::Type::TERMINAL
           && pool.work_queue_->pop(work_ptr)) {
      --pool.work_queue_size_;

      const std::unique_ptr<Work::Callable> work(work_ptr);

      work_type = (*work)();
    }

    std::unique_lock<std::mutex> work_signal_lock(pool.work_signal_mutex_);

    // work_queue_size is increased when work_signal_mutex locked
    // active_worker_count_ is only decreased here so it is also when mutex is locked

    // as far as I can see when there is a work, it is certain that either work_queue_size or active_worker counter
    // is greater than zero so I don't see any race condition
    if (--pool.active_worker_count_ == 0 && pool.work_queue_size_ == 0) {
      std::unique_lock<std::mutex> notify_all_finished_lock(pool.notify_all_finished_mutex_);

      pool.are_all_really_finished_ = true;
      pool.notify_all_finished_signal_.notify_all();
    }

    if (work_type == Work::Type::TERMINAL) {
      --pool.worker_count_;

      return;
    }

    // block here until signalled
    pool.work_signal_.wait(work_signal_lock, [&pool]() { return (pool.work_queue_size_ > 0); });
  }
}

}
