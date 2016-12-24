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

#include "utility.hpp"

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <thread>

namespace threadpool11 {

class Pool;

class Worker {
  friend class Pool;

public:
  Worker(Pool& pool);
  ~Worker() = default;

private:
  Worker(Worker&&) = delete;
  Worker(Worker const&) = delete;
  Worker& operator=(Worker&&) = delete;
  Worker& operator=(Worker const&) = delete;

  void execute(Pool& pool);

private:
  /*!
  * This should always stay at bottom so that it is called at the most end.
  */
  std::thread thread_;
};
}
