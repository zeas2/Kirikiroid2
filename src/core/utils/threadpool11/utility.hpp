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

#include <functional>

namespace threadpool11 {

namespace Work {
enum class Type { STD, TERMINAL };
enum class Prio { DEFERRED, IMMIDIATE };

typedef std::function<Work::Type()> Callable;
}

template <typename T>
class move_on_copy {
public:
  move_on_copy(T&& aValue)
      : value_(std::move(aValue)) {}
  move_on_copy(const move_on_copy& other)
      : value_(std::move(other.value_)) {}

  move_on_copy& operator=(move_on_copy&& aValue) = delete;      // not needed here
  move_on_copy& operator=(const move_on_copy& aValue) = delete; // not needed here

  T& value() { return value_; }
  const T& value() const { return value_; }

private:
  mutable T value_;
};

template <typename T>
inline move_on_copy<T> make_move_on_copy(T&& aValue) {
  return move_on_copy<T>(std::move(aValue));
}
}
