#ifndef __FUNAPI_TEST_SCENE_H__
#define __FUNAPI_TEST_SCENE_H__

#include "cocos2d.h"
#include "funapi/funapi_network.h"
#include "funapi/funapi_multicasting.h"
#include "funapi/funapi_downloader.h"

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

  //
  void CreateMulticast();
  void JoinMulticastChannel();
  void SendMulticastMessage();
  void LeaveMulticastChannel();
  void RequestMulticastChannelList();

  //
  bool IsConnected();

  //
  void DownloaderTest();

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();
  void OnEchoJson(const fun::TransportProtocol protocol,const std::string &type, const std::vector<uint8_t> &v_body);
  void OnEchoProto(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);

  void OnMaintenanceMessage(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);
  void OnStoppedAllTransport();

  void OnTransportConnectFailed (const fun::TransportProtocol protocol);
  void OnTransportConnectTimeout (const fun::TransportProtocol protocol);

  void OnTransportStarted (const fun::TransportProtocol protocol);
  void OnTransportClosed (const fun::TransportProtocol protocol);

  // implement the "static create()" method manually
  CREATE_FUNC(FunapiTest);

private:
  void Connect(const fun::TransportProtocol protocol);
  std::shared_ptr<fun::FunapiTransport> GetNewTransport(const fun::TransportProtocol protocol);

  // Please change this address for test.
  const std::string kServerIp = "127.0.0.1";

  // member variables.
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  std::shared_ptr<fun::FunapiNetwork> network_ = nullptr;

  const std::string kMulticastTestChannel = "multicast";
  std::shared_ptr<fun::FunapiMulticastClient> multicast_ = nullptr;
  fun::FunEncoding multicast_encoding_;

  void OnMulticastChannelSignalle(const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body);

  void TestFunapiNetwork(bool bStart);

  // Please change this address for test.
  const std::string kDownloadServerIp = "127.0.0.1";
  const int kDownloadServerPort = 8020;

  std::shared_ptr<fun::FunapiHttpDownloader> downloader_ = nullptr;
  fun::DownloadResult code_ = fun::DownloadResult::NONE;
};

#endif // __FUNAPI_TEST_SCENE_H__
