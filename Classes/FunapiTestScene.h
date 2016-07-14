#ifndef __FUNAPI_TEST_SCENE_H__
#define __FUNAPI_TEST_SCENE_H__

#include "cocos2d.h"
#include "funapi/funapi_downloader.h"

namespace fun {
  class FunapiSession;
  class FunapiMulticast;
  enum class TransportProtocol;
}

class FunapiTest : public cocos2d::Layer
{
public:
  static cocos2d::Scene* createScene();

  virtual bool init();
  virtual void cleanup();
  virtual void update(float delta);

  // UI
  void ConnectTcp();
  void ConnectUdp();
  void ConnectHttp();
  void Disconnect();
  void SendEchoMessage();

  void CreateMulticast();
  void JoinMulticastChannel();
  void SendMulticastMessage();
  void LeaveMulticastChannel();
  void RequestMulticastChannelList();
  void LeaveMulticastAllChannels();

  void DownloaderTest();

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

  // implement the "static create()" method manually
  CREATE_FUNC(FunapiTest);

private:
  void Connect(const fun::TransportProtocol protocol);
  void TestFunapi(bool bStart);

  std::shared_ptr<fun::FunapiSession> session_ = nullptr;

  // Please change this address for test.
  const std::string kServerIp = "127.0.0.1";
  const std::string kDownloadServerIp = "127.0.0.1";
  const int kDownloadServerPort = 8020;

  // member variables.
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  const std::string kMulticastTestChannel = "multicast";
  std::shared_ptr<fun::FunapiMulticast> multicast_ = nullptr;

  std::shared_ptr<fun::FunapiHttpDownloader> downloader_ = nullptr;
  fun::DownloadResult code_ = fun::DownloadResult::NONE;
};

#endif // __FUNAPI_TEST_SCENE_H__
