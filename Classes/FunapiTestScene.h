#ifndef __FUNAPI_TEST_SCENE_H__
#define __FUNAPI_TEST_SCENE_H__

#include "cocos2d.h"

namespace fun {
  class FunapiSession;
  class FunapiMulticast;
  class FunapiHttpDownloader;
  class FunapiHttp;
  class FunapiAnnouncement;
  enum class TransportProtocol;
}

namespace cocos2d {
  namespace ui {
    class EditBox;
    class CheckBox;
    class Button;
  }
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
  void RequestAnnouncements();

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

  // implement the "static create()" method manually
  CREATE_FUNC(FunapiTest);

private:
  void Connect(const fun::TransportProtocol protocol);
  void TestFunapi(bool bStart);
  void SendRedirectTestMessage();
  void UpdateUI();

  std::shared_ptr<fun::FunapiSession> session_ = nullptr;

  // Please change this address for test.
  std::string kServer = "127.0.0.1";
  bool with_protobuf_ = false;
  bool with_session_reliability_ = false;

  std::shared_ptr<fun::FunapiMulticast> multicast_ = nullptr;
  const std::string kMulticastTestChannel = "multicast";

  // Please change this address for test.
  const std::string kDownloadServer = "127.0.0.1";
  const int kDownloadServerPort = 8020;
  std::shared_ptr<fun::FunapiHttpDownloader> downloader_ = nullptr;

  // Please change this address for test.
  const std::string kAnnouncementServer = "127.0.0.1";
  const int kAnnouncementServerPort = 8080;
  std::shared_ptr<fun::FunapiAnnouncement> announcement_ = nullptr;

  // UI controls
  cocos2d::ui::EditBox* editbox_sernvername_;
  cocos2d::ui::CheckBox* checkbox_protobuf_;
  cocos2d::ui::CheckBox* checkbox_reliability_;
  cocos2d::ui::Button* button_send_a_message_;
  cocos2d::ui::Button* button_disconnect_;
  cocos2d::ui::Button* button_connect_http_;
  cocos2d::ui::Button* button_connect_udp_;
  cocos2d::ui::Button* button_connect_tcp_;
  cocos2d::ui::Button* button_multicast_leave_all_channels_;
  cocos2d::ui::Button* button_multicast_request_list_;
  cocos2d::ui::Button* button_multicast_leave_;
  cocos2d::ui::Button* button_multicast_send_;
  cocos2d::ui::Button* button_multicast_join_;
  cocos2d::ui::Button* button_multicast_create_;
  cocos2d::ui::Button* button_download_;
  cocos2d::ui::Button* button_announcement_;
  cocos2d::ui::Button* button_test_stop_;
  cocos2d::ui::Button* button_test_start_;
};

#endif // __FUNAPI_TEST_SCENE_H__
