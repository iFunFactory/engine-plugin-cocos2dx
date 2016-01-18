// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_UTILS_H_
#define SRC_FUNAPI_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#if FUNAPI_PLATFORM_WINDOWS
#include <stdint.h>
#pragma warning(disable:4996)
#endif

namespace fun {

#if FUNAPI_PLATFORM_WINDOWS
#define ssize_t   size_t
#define access    _access
#define snprintf  _snprintf
#define F_OK      0
#define mkdir(path, mode)   _mkdir(path)
#endif


// Format string
typedef std::string string;

template <typename ... Args>
string FormatString (const char* fmt, Args... args)
{
  size_t length = snprintf(nullptr, 0, fmt, args...) + 1;
  std::unique_ptr<char[]> buf(new char[length]);
  snprintf(buf.get(), length, fmt, args...);
  return string(buf.get(), buf.get() + length - 1);
}


// Function event
template <typename T>
class FEvent
{
 public:
  void operator+= (const T &handler) { std::unique_lock<std::mutex> lock(mutex_); vector_.push_back(handler); }
  template <typename... ARGS>
  void operator() (const ARGS&... args) { std::unique_lock<std::mutex> lock(mutex_); for (const auto &f : vector_) f(args...); }
  bool empty() { return vector_.empty(); }

 private:
  std::vector<T> vector_;
  std::mutex mutex_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_UTILS_H_
