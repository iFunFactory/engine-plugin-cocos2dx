// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_MANAGER_H_
#define SRC_FUNAPI_MANAGER_H_

namespace fun {

class FunapiTransport;
class FunapiManagerImpl;
class FunapiManager : public std::enable_shared_from_this<FunapiManager> {
 public:
  FunapiManager();
  ~FunapiManager();

  void Update();
  void InsertTransport(const std::shared_ptr<FunapiTransport> &transport);
  void EraseTransport(const std::shared_ptr<FunapiTransport> &transport);

 private:
  std::shared_ptr<FunapiManagerImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_MANAGER_H_
