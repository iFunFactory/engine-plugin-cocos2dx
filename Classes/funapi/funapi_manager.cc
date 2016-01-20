// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_transport.h"
#include "funapi_manager.h"
#include "funapi_build_config.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiManagerImpl implementation.

class FunapiManagerImpl : public std::enable_shared_from_this<FunapiManagerImpl> {
 public:
  FunapiManagerImpl();
  ~FunapiManagerImpl();

  void Update();
  void InsertTransport(const std::shared_ptr<FunapiTransport> &transport);
  void EraseTransport(const std::shared_ptr<FunapiTransport> &transport);

 private:
  void Initialize();
  void Finalize();
  void JoinThread();
  void Thread();

  std::set<std::shared_ptr<FunapiTransport>> set_;
  std::mutex mutex_;
  std::condition_variable_any condition_;
  std::thread thread_;
  bool run_ = false;
};


FunapiManagerImpl::FunapiManagerImpl() {
  Initialize();
}


FunapiManagerImpl::~FunapiManagerImpl() {
  Finalize();
}


void FunapiManagerImpl::Initialize() {
#ifdef FUNAPI_HAVE_THREAD
  run_ = true;
  thread_ = std::thread([this](){ Thread(); });
#endif // FUNAPI_HAVE_THREAD
}


void FunapiManagerImpl::Finalize() {
#ifdef FUNAPI_HAVE_THREAD
  JoinThread();
#endif // FUNAPI_HAVE_THREAD
}


void FunapiManagerImpl::JoinThread() {
  run_ = false;
  condition_.notify_all();
  if (thread_.joinable())
    thread_.join();
}


void FunapiManagerImpl::Thread() {
  while (run_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (set_.empty()) {
        condition_.wait(mutex_);
      }
    }

    Update();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}


void FunapiManagerImpl::InsertTransport(const std::shared_ptr<FunapiTransport> &transport) {
  std::unique_lock<std::mutex> lock(mutex_);
  set_.insert(transport);

#ifdef FUNAPI_HAVE_THREAD
  condition_.notify_one();
#endif // FUNAPI_HAVE_THREAD
}


void FunapiManagerImpl::EraseTransport(const std::shared_ptr<FunapiTransport> &transport) {
  std::unique_lock<std::mutex> lock(mutex_);
  set_.erase(transport);
}


void FunapiManagerImpl::Update() {
  int max_fd = -1;

  fd_set rset;
  fd_set wset;

  FD_ZERO(&rset);
  FD_ZERO(&wset);

  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto transport : set_) {
      int fd = transport->GetSocket();
      if (fd > 0) {
        if (fd > max_fd) max_fd = fd;
        FD_SET(fd, &rset);
        FD_SET(fd, &wset);
      }

      transport->Update();
    }
  }

  struct timeval timeout = { 0, 1 };

  if (select(max_fd + 1, &rset, &wset, NULL, &timeout) > 0) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto transport : set_) {
      int fd = transport->GetSocket();
      if (fd > 0) {
        if (FD_ISSET(fd, &rset)) {
          transport->OnSocketRead();
        }

        if (FD_ISSET(fd, &wset)) {
          transport->OnSocketWrite();
        }
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiManager implementation.

FunapiManager::FunapiManager()
  : impl_(std::make_shared<FunapiManagerImpl>()) {
}


FunapiManager::~FunapiManager() {
}


void FunapiManager::Update() {
  impl_->Update();
}


void FunapiManager::InsertTransport(const std::shared_ptr<FunapiTransport> &transport) {
  impl_->InsertTransport(transport);
}


void FunapiManager::EraseTransport(const std::shared_ptr<FunapiTransport> &transport) {
  impl_->EraseTransport(transport);
}

}  // namespace fun
