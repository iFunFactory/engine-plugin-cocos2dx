// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_tasks.h"
#include "funapi_downloader.h"
#include "network/CCDownloader.h"
#include "md5/md5.h"

namespace fun {

class DownloadFileInfo : public std::enable_shared_from_this<DownloadFileInfo> {
 public:
  DownloadFileInfo() = delete;
  DownloadFileInfo(const std::string &url, const std::string &path, const uint64_t size, const std::string &hash, const std::string &hash_front);
  ~DownloadFileInfo();

  const std::string& GetUrl();
  const std::string& GetPath();
  uint64_t GetSize();
  const std::string& GetHash();
  const std::string& GetHashFront();

  fun::DownloadResult GetDownloadResult();
  void SetDownloadResult(fun::DownloadResult r);

 private:
  std::string url_;          //
  std::string path_;         // save file path
  uint64_t size_;            // file size
  std::string hash_;         // md5 hash
  std::string hash_front_;   // front part of file (1MB)

  fun::DownloadResult result_ = fun::DownloadResult::NONE;
};


DownloadFileInfo::DownloadFileInfo(const std::string &url, const std::string &path, const uint64_t size, const std::string &hash, const std::string &hash_front)
: url_(url), path_(path), size_(size), hash_(hash), hash_front_(hash_front)
{
}


DownloadFileInfo::~DownloadFileInfo() {
}


const std::string& DownloadFileInfo::GetUrl() {
  return url_;
}


const std::string& DownloadFileInfo::GetPath() {
  return path_;
}


uint64_t DownloadFileInfo::GetSize() {
  return size_;
}


const std::string& DownloadFileInfo::GetHash() {
  return hash_;
}


const std::string& DownloadFileInfo::GetHashFront() {
  return hash_front_;
}


DownloadResult DownloadFileInfo::GetDownloadResult() {
  return result_;
}


void DownloadFileInfo::SetDownloadResult(fun::DownloadResult r) {
  result_ = r;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloaderImpl implementation.

class FunapiHttpDownloaderImpl : public std::enable_shared_from_this<FunapiHttpDownloaderImpl> {
 public:
  typedef FunapiHttpDownloader::VerifyEventHandler VerifyEventHandler;
  typedef FunapiHttpDownloader::ReadyEventHandler ReadyEventHandler;
  typedef FunapiHttpDownloader::UpdateEventHandler UpdateEventHandler;
  typedef FunapiHttpDownloader::FinishEventHandler FinishEventHandler;

  FunapiHttpDownloaderImpl();
  ~FunapiHttpDownloaderImpl();

  void AddVerifyCallback(const VerifyEventHandler &handler);
  void AddReadyCallback(const ReadyEventHandler &handler);
  void AddUpdateCallback(const UpdateEventHandler &handler);
  void AddFinishedCallback(const FinishEventHandler &handler);

  void GetDownloadList(const std::string &download_url, const std::string &target_path);
  void StartDownload();

  void Update();

 private:
  FunapiEvent<VerifyEventHandler> on_download_verify_;
  FunapiEvent<ReadyEventHandler> on_download_ready_;
  FunapiEvent<UpdateEventHandler> on_download_update_;
  FunapiEvent<FinishEventHandler> on_download_finished_;

  void OnDownloadVerify(const std::string &path);
  void OnDownloadReady(int total_count, uint64_t total_size);
  void OnDownloadUpdate(const std::string &path, uint64_t bytes_received, uint64_t total_bytes, int percentage);
  void OnDownloadFinished(DownloadResult code);

  void CheckDownloadFileInfoResult();

  std::mutex result_mutex_;

  std::shared_ptr<FunapiTasks> tasks_;

  void PushTaskQueue(const std::function<void()> &task);

  std::vector<std::shared_ptr<DownloadFileInfo>> file_list_;
  std::unique_ptr<cocos2d::network::Downloader> downloader_;

  bool is_failed_ = false;
  int total_count_ = 0;
  std::mutex total_count_mutex_;

  std::thread thread_;
  bool run_ = false;

  bool IsDownloadFile(std::shared_ptr<DownloadFileInfo> info);
  bool MD5Compare(std::shared_ptr<DownloadFileInfo> info);
  std::string GetMD5String(std::shared_ptr<DownloadFileInfo> info, bool use_front);
};


FunapiHttpDownloaderImpl::FunapiHttpDownloaderImpl() {
  tasks_ = FunapiTasks::Create();
}


FunapiHttpDownloaderImpl::~FunapiHttpDownloaderImpl() {
  run_ = false;
  if (thread_.joinable())
    thread_.join();
}


void FunapiHttpDownloaderImpl::AddVerifyCallback(const VerifyEventHandler &handler) {
  on_download_verify_ += handler;
}


void FunapiHttpDownloaderImpl::AddReadyCallback(const ReadyEventHandler &handler) {
  on_download_ready_ += handler;
}


void FunapiHttpDownloaderImpl::AddUpdateCallback(const UpdateEventHandler &handler) {
  on_download_update_ += handler;
}


void FunapiHttpDownloaderImpl::AddFinishedCallback(const FinishEventHandler &handler) {
  on_download_finished_ += handler;
}


void FunapiHttpDownloaderImpl::OnDownloadVerify(const std::string &path) {
  PushTaskQueue([this, path]() {
    on_download_verify_(path);
  });
}


void FunapiHttpDownloaderImpl::OnDownloadReady(int total_count, uint64_t total_size) {
  PushTaskQueue([this, total_count, total_size]() {
    on_download_ready_(total_count, total_size);
  });
}


void FunapiHttpDownloaderImpl::OnDownloadUpdate(const std::string &path, uint64_t bytes_received, uint64_t total_bytes, int percentage) {
  PushTaskQueue([this, path, bytes_received, total_bytes, percentage]() {
    on_download_update_(path, bytes_received, total_bytes, percentage);
  });
}


void FunapiHttpDownloaderImpl::OnDownloadFinished(DownloadResult code) {
  PushTaskQueue([this, code]() {
    on_download_finished_(code);
  });
}


void FunapiHttpDownloaderImpl::GetDownloadList(const std::string &download_url, const std::string &target_path) {
  cocos2d::network::HttpRequest* request = new (std::nothrow) cocos2d::network::HttpRequest();
  request->setUrl(download_url.c_str());
  request->setRequestType(cocos2d::network::HttpRequest::Type::POST);

  request->setResponseCallback([this, download_url, target_path](cocos2d::network::HttpClient *sender, cocos2d::network::HttpResponse *response) {
    if (!response->isSucceed()) {
      DebugUtils::Log("Response was invalid!");
      OnDownloadFinished(DownloadResult::FAILED);
    }
    else {
      std::string body(response->getResponseData()->begin(), response->getResponseData()->end());
      DebugUtils::Log("body = %s", body.c_str());

      int total_count = 0;
      uint64_t total_size = 0;

      rapidjson::Document document;
      document.Parse<0>(body.c_str());

      if (document.HasMember("data")) {
        rapidjson::Value &d = document["data"];
        total_count = d.Size();

        DebugUtils::Log("total_size = %d", total_count);

        for (int i=0;i<total_count;++i) {
          rapidjson::Value &v = d[i];

          std::string url;
          std::string path;
          std::string md5;
          std::string md5_front;

          uint64_t size = 0;

          if (v.HasMember("path")) {
            url = download_url + "/" + v["path"].GetString();
            path = target_path + v["path"].GetString();
          }

          if (v.HasMember("md5")) {
            md5 = v["md5"].GetString();
          }

          if (v.HasMember("md5_front")) {
            md5_front = v["md5_front"].GetString();
          }

          if (v.HasMember("size")) {
            size = v["size"].GetUint64();
            total_size += size;
          }

          fun::DebugUtils::Log("index=%d path=%s size=%llu md5=%s md5_front=%s", i, path.c_str(), size, md5.c_str(), md5_front.c_str());

          file_list_.push_back(std::make_shared<DownloadFileInfo>(url, path, size, md5, md5_front));
        }

        {
          std::unique_lock<std::mutex> lock(total_count_mutex_);
          total_count_ = total_count;
        }

        OnDownloadReady(total_count, total_size);
      }
    }
  });
  cocos2d::network::HttpClient::getInstance()->send(request);
  request->release();
}


void FunapiHttpDownloaderImpl::StartDownload() {
  DebugUtils::Log("StartDownload");
  if (file_list_.size() == 0) {
    DebugUtils::Log("You must call GetDownloadList function first");
    return;
  }

  // http://www.cocos2d-x.org/wiki/Assets_manager
  // Known issue
  // Assets manager(cocos2d::network::Downloader) may fail to create and download files on windows and iOS simulator,
  // we will try to fix it very soon, please test on real iOS devices in the meantime.
  cocos2d::network::DownloaderHints hints =
  {
    1,
    10,
    ".tmp"
  };
  downloader_.reset(new cocos2d::network::Downloader(hints));

  downloader_->onTaskProgress = [this](const cocos2d::network::DownloadTask& task,
                                      int64_t bytesReceived,
                                      int64_t totalBytesReceived,
                                      int64_t totalBytesExpected)
  {
    float percent = float(totalBytesReceived * 100) / totalBytesExpected;
    DebugUtils::Log("%.1f%%[total %d KB]", percent, int(totalBytesExpected/1024));

    int index = atoi(task.identifier.c_str());
    auto info = file_list_[index];
    OnDownloadUpdate(info->GetPath(), totalBytesReceived, totalBytesExpected, static_cast<int>(percent));
  };

  downloader_->onFileTaskSuccess = [this](const cocos2d::network::DownloadTask& task)
  {
    DebugUtils::Log("Download [%s] success.", task.identifier.c_str());

    int index = atoi(task.identifier.c_str());
    auto info = file_list_[index];
    {
      std::unique_lock<std::mutex> lock(result_mutex_);
      info->SetDownloadResult(DownloadResult::SUCCESS);
    }

    CheckDownloadFileInfoResult();
  };

  downloader_->onTaskError = [this](const cocos2d::network::DownloadTask& task,
                                   int errorCode,
                                   int errorCodeInternal,
                                   const std::string& errorStr)
  {
    DebugUtils::Log("Failed to download : %s, identifier(%s) error code(%d), internal error code(%d) desc(%s)"
        , task.requestURL.c_str()
        , task.identifier.c_str()
        , errorCode
        , errorCodeInternal
        , errorStr.c_str());

    int index = atoi(task.identifier.c_str());
    auto info = file_list_[index];
    {
      std::unique_lock<std::mutex> lock(result_mutex_);
      info->SetDownloadResult(DownloadResult::FAILED);
    }

    is_failed_ = true;
    CheckDownloadFileInfoResult();
  };

  run_ = true;
  thread_ = std::thread([this]() {
    int index = 0;

    for (auto info : file_list_) {
      if (run_ == false) {
        return;
      }

      if (!IsDownloadFile(info)) {
        info->SetDownloadResult(DownloadResult::SUCCESS);
        CheckDownloadFileInfoResult();
      }
      else {
        std::stringstream ss;
        ss << index;
        std::string identifier = ss.str();

        if (info->GetDownloadResult() != DownloadResult::SUCCESS) {
          /*
          // Create Directory
          std::string dir;
          unsigned long found = info->GetPath().find_last_of("/\\");
          if (found == std::string::npos)
          {
            DebugUtils::Log("Can't find dirname in storagePath: %s", info->GetPath().c_str());
            break;
          }

          // ensure directory is exist
          auto util = cocos2d::FileUtils::getInstance();
          dir = info->GetPath().substr(0, found+1);
          if (false == util->isDirectoryExist(dir))
          {
            if (false == util->createDirectory(dir))
            {
              DebugUtils::Log("Can't create dir: %s", dir.c_str());
              break;
            }
          }
          */

          // Create Task
          downloader_->createDownloadFileTask(info->GetUrl(), info->GetPath(), identifier);
        }
      }

      ++index;
    }
  });
}


bool FunapiHttpDownloaderImpl::IsDownloadFile(std::shared_ptr<DownloadFileInfo> info) {
  if (!cocos2d::FileUtils::getInstance()->isFileExist(info->GetPath())) {
    return true;
  }

  if (cocos2d::FileUtils::getInstance()->getFileSize(info->GetPath()) != info->GetSize()) {
    return true;
  }

  if (!MD5Compare(info)) {
    return true;
  }

  return false;
}


std::string FunapiHttpDownloaderImpl::GetMD5String(std::shared_ptr<DownloadFileInfo> info, bool use_front) {
  const size_t read_buffer_size = 1048576; // 1024*1024
  const size_t md5_buffer_size = 16;
  std::vector<unsigned char> buffer(read_buffer_size);
  std::vector<unsigned char> md5(md5_buffer_size);
  std::string ret;
  size_t length;

  FILE *fp = fopen(cocos2d::FileUtils::getInstance()->getSuitableFOpen(info->GetPath()).c_str(), "rb");
  if (!fp) {
    return std::string("");
  }

  MD5_CTX ctx;
  MD5_Init(&ctx);
  if (use_front) {
    length = fread(buffer.data(), 1, read_buffer_size, fp);
    MD5_Update(&ctx, buffer.data(), length);
  }
  else {
    while ((length = fread(buffer.data(), 1, read_buffer_size, fp)) != 0) {
      MD5_Update(&ctx, buffer.data(), length);
    }
  }
  MD5_Final(md5.data(), &ctx);
  fclose(fp);

  char c[3];
  for (int i = 0; i<md5_buffer_size; ++i) {
    sprintf(c, "%02x", md5[i]);
    ret.append(c);
  }

  return ret;
}


bool FunapiHttpDownloaderImpl::MD5Compare(std::shared_ptr<DownloadFileInfo> info) {
  if (info->GetHashFront().length() > 0) {
    std::string md5_string = GetMD5String(info, true);
    if (info->GetHashFront().compare(md5_string) == 0) {
      return true;
    }
  }

  if (info->GetHash().length() > 0) {
    std::string md5_string = GetMD5String(info, false);
    if (info->GetHash().compare(md5_string) == 0) {
      return true;
    }
  }

  return false;
}


void FunapiHttpDownloaderImpl::CheckDownloadFileInfoResult() {
  {
    std::unique_lock<std::mutex> lock(total_count_mutex_);
    --total_count_;
    if (total_count_ > 0) {
      return;
    }
  }

  if (is_failed_) {
    OnDownloadFinished(DownloadResult::FAILED);
  }
  else {
    OnDownloadFinished(DownloadResult::SUCCESS);
  }
}


void FunapiHttpDownloaderImpl::Update() {
  tasks_->Update();
}


void FunapiHttpDownloaderImpl::PushTaskQueue(const std::function<void()> &task)
{
  tasks_->Push([task]() { task(); return true; });
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpDownloader implementation.

FunapiHttpDownloader::FunapiHttpDownloader()
  : impl_(std::make_shared<FunapiHttpDownloaderImpl>()) {
}


FunapiHttpDownloader::~FunapiHttpDownloader() {
}


void FunapiHttpDownloader::AddVerifyCallback(const VerifyEventHandler &handler) {
  return impl_->AddVerifyCallback(handler);
}


void FunapiHttpDownloader::AddReadyCallback(const ReadyEventHandler &handler) {
  return impl_->AddReadyCallback(handler);
}


void FunapiHttpDownloader::AddUpdateCallback(const UpdateEventHandler &handler) {
  return impl_->AddUpdateCallback(handler);
}


void FunapiHttpDownloader::AddFinishedCallback(const FinishEventHandler &handler) {
  return impl_->AddFinishedCallback(handler);
}


void FunapiHttpDownloader::GetDownloadList(const std::string &download_url, const std::string &target_path) {
  return impl_->GetDownloadList(download_url, target_path);
}


void FunapiHttpDownloader::StartDownload() {
  return impl_->StartDownload();
}


void FunapiHttpDownloader::Update() {
  return impl_->Update();
}


}  // namespace fun
