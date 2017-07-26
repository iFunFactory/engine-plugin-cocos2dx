// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace funapiplugincocos2dxTestInhouse
{
  static const std::string g_server_ip = "127.0.0.1";

  TEST_CLASS(FunapiHttpDownloader)
  {
  public:

    TEST_METHOD(testDownloader)
    {
      std::string kDownloadServer = g_server_ip;
      int kDownloadServerPort = 8020;
      bool is_ok = true;
      bool is_working = true;

      std::stringstream ss_temp;
      ss_temp << "http://" << kDownloadServer << ":" << kDownloadServerPort;
      std::string download_url = ss_temp.str();

      auto downloader = fun::FunapiHttpDownloader::Create(download_url, cocos2d::FileUtils::getInstance()->getWritablePath());

      downloader->AddReadyCallback
      ([]
      (const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
        const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info)
      {
        for (auto i : info) {
          std::stringstream ss_temp;
          ss_temp << i->GetUrl() << std::endl;
          printf("%s", ss_temp.str().c_str());
        }
      });

      downloader->AddProgressCallback
      ([]
      (const std::shared_ptr<fun::FunapiHttpDownloader> &downloader,
        const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
        const int index,
        const int max_index,
        const uint64_t received_bytes,
        const uint64_t expected_bytes)
      {
        auto i = info[index];

        std::stringstream ss_temp;
        ss_temp << index << "/" << max_index << " " << received_bytes << "/" << expected_bytes << " " << i->GetUrl() << std::endl;
        printf("%s", ss_temp.str().c_str());
      });

      downloader->AddCompletionCallback
      ([&is_working, &is_ok]
      (const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
        const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
        const fun::FunapiHttpDownloader::ResultCode result_code)
      {
        if (result_code == fun::FunapiHttpDownloader::ResultCode::kSucceed) {
          is_ok = true;
          for (auto i : info) {
            printf("file_path=%s", i->GetPath().c_str());
          }
        }
        else {
          is_ok = false;
        }

        is_working = false;
      });

      downloader->Start();

      while (is_working) {
        downloader->Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
      }

      Assert::IsTrue(is_ok);
    }
  };

}
