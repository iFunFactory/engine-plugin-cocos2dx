// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_DOWNLOADER_H_
#define SRC_FUNAPI_DOWNLOADER_H_

namespace fun {

enum class DownloadResult {
  NONE,
  SUCCESS,
  FAILED
};


class FunapiHttpDownloaderImpl;
class FunapiHttpDownloader : public std::enable_shared_from_this<FunapiHttpDownloader> {
 public:
  typedef std::function<void(const std::string&)> VerifyEventHandler;
  typedef std::function<void(int, uint64_t)> ReadyEventHandler;
  typedef std::function<void(const std::string&, uint64_t, uint64_t, int)> UpdateEventHandler;
  typedef std::function<void(DownloadResult code)> FinishEventHandler;

  FunapiHttpDownloader();
  ~FunapiHttpDownloader();

  void AddVerifyCallback(const VerifyEventHandler &handler);
  void AddReadyCallback(const ReadyEventHandler &handler);
  void AddUpdateCallback(const UpdateEventHandler &handler);
  void AddFinishedCallback(const FinishEventHandler &handler);

  void GetDownloadList(const std::string &download_url, const std::string &target_path);
  void StartDownload();

  void Update();

 private:
  std::shared_ptr<FunapiHttpDownloaderImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_DOWNLOADER_H_
