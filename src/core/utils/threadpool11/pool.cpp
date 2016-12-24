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

#include <boost/lockfree/queue.hpp>

#include <algorithm>
#include <future>
#include <vector>

namespace threadpool11 {

Pool::Pool(std::size_t worker_count)
    : worker_count_(0)
    , active_worker_count_(0)
    , are_all_really_finished_{true}
    , work_queue_(new boost::lockfree::queue<Work::Callable*>(0))
    , work_queue_size_(0) {
  spawnWorkers(worker_count);
}

Pool::~Pool() { joinAll(); }

void Pool::waitAll() {
  std::unique_lock<std::mutex> notify_all_finished_lock(notify_all_finished_mutex_);

  notify_all_finished_signal_.wait(notify_all_finished_lock, [this]() { return are_all_really_finished_.load(); });
}

void Pool::joinAll() { decWorkerCountBy(std::numeric_limits<size_t>::max(), Method::SYNC); }

size_t Pool::getWorkerCount() const { return worker_count_.load(); }

void Pool::setWorkerCount(std::size_t n, Method method) {
  if (getWorkerCount() < n)
    incWorkerCountBy(n - getWorkerCount());
  else
    decWorkerCountBy(getWorkerCount() - n, method);
}

size_t Pool::getWorkQueueSize() const { return work_queue_size_.load(); }

size_t Pool::getActiveWorkerCount() const { return active_worker_count_.load(); }

size_t Pool::getInactiveWorkerCount() const { return worker_count_.load() - active_worker_count_.load(); }

void Pool::incWorkerCountBy(std::size_t n) { spawnWorkers(n); }

void Pool::decWorkerCountBy(size_t n, Method method) {
  n = std::min(n, getWorkerCount());
  if (method == Method::SYNC) {
    std::vector<std::future<void>> futures;
    futures.reserve(n);
    while (n-- > 0)
      futures.emplace_back(postWork<void>([]() {}, Work::Type::TERMINAL));
    for (auto& it : futures)
      it.get();
  } else {
    while (n-- > 0)
      postWork<void>([]() {}, Work::Type::TERMINAL);
  }
}

void Pool::spawnWorkers(std::size_t n) {
  //'OR' makes sure the case where one of the expressions is zero, is valid.
  assert(static_cast<size_t>(worker_count_ + n) > n ||
         static_cast<size_t>(worker_count_ + n) > worker_count_);
  while (n-- > 0) {
    new Worker(*this); //! Worker class takes care of its de-allocation itself after here
    ++worker_count_;
  }
}

void Pool::push(Work::Callable* workFunc) {
  are_all_really_finished_ = false;

  std::unique_lock<std::mutex> work_signal_lock(work_signal_mutex_);
  ++work_queue_size_;
  work_queue_->push(workFunc);
  work_signal_.notify_one();
}
}
