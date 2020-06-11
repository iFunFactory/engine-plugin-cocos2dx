// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "FunapiTestScene.h"
#include "ui/CocosGUI.h"

#include "funapi/funapi_session.h"
#include "funapi/funapi_multicasting.h"
#include "funapi/funapi_downloader.h"
#include "funapi/funapi_announcement.h"
#include "funapi/funapi_send_flag_manager.h"

#include "test_messages.pb.h"
#include "funapi/management/maintenance_message.pb.h"
#include "funapi/service/multicast_message.pb.h"
#include "funapi/funapi_utils.h"
#include "funapi/funapi_tasks.h"
#include "funapi/funapi_encryption.h"

#include "json/writer.h"
#include "json/document.h"
#include "json/stringbuffer.h"

USING_NS_CC;
using namespace cocos2d::ui;

class LambdaEditBoxDelegate : public EditBoxDelegate {
 public:
  typedef std::function<void(EditBox*,const std::string&)> ChangedHandler;
  typedef std::function<void(EditBox*)> ReturnHandler;

  LambdaEditBoxDelegate() = delete;
  LambdaEditBoxDelegate(ChangedHandler changed_hanlder, ReturnHandler return_hanlder)
  : changed_handler_(changed_hanlder), return_handler_(return_hanlder) {
  }
  ~LambdaEditBoxDelegate() = default;

  void editBoxEditingDidBegin(EditBox* editBox) {};
  void editBoxEditingDidEnd(EditBox* editBox) {};
  void editBoxTextChanged(EditBox* editBox, const std::string& text) {
    changed_handler_(editBox, text);
  };
  void editBoxReturn(EditBox* editBox) {
    return_handler_(editBox);
  }

 private:
  ChangedHandler changed_handler_;
  ReturnHandler return_handler_;
};

Scene* FunapiTest::createScene()
{
  // funapi plugin's manager init
  fun::FunapiSendFlagManager::Init();

  // 'scene' is an autorelease object
  auto scene = Scene::create();

  // 'layer' is an autorelease object
  auto layer = FunapiTest::create();

  // add layer as a child to scene
  scene->addChild(layer);

  // return the scene
  return scene;
}

// on "init" you need to initialize your instance
bool FunapiTest::init()
{
  scheduleUpdate();

  Director::getInstance()->setDisplayStats(false);

  //////////////////////////////
  // 1. super init first
  if ( !Layer::init() )
  {
    return false;
  }

  /////////////////////////////
  // 2. add a menu items

  Size visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 visibleOrigin = Director::getInstance()->getVisibleOrigin();

  const float button_width = visibleSize.width * 0.5;
  const float button_height = visibleSize.height * 0.1;
  const float gap_height = button_height * 0.1;
  const float center_x = visibleSize.width * 0.5;

  // scroll view
  cocos2d::ui::ScrollView* scrollView = cocos2d::ui::ScrollView::create();
  scrollView->setContentSize(visibleSize);
  scrollView->setPosition(visibleOrigin);
  this->addChild(scrollView);

  // layer
  // setting
  auto layer_setting = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_setting->setAnchorPoint(Vec2(0.0,0.0));

  float y = gap_height;

  static LambdaEditBoxDelegate session_ip_editbox_delegate([this](EditBox *editBox, const std::string &text) {
  },[this](EditBox *editBox) {
    kServer = editBox->getText();
  });

  editbox_sernvername_ = ui::EditBox::create(Size(visibleSize.width, button_height*0.7), "edit_back.png");
  editbox_sernvername_->setText(kServer.c_str());
  editbox_sernvername_->setAnchorPoint(Vec2(0.5, 0.0));
  editbox_sernvername_->setPosition(Vec2(center_x,y));
  editbox_sernvername_->setInputMode(ui::EditBox::InputMode::URL);
  editbox_sernvername_->setInputFlag(ui::EditBox::InputFlag::INITIAL_CAPS_SENTENCE);
  editbox_sernvername_->setReturnType(ui::EditBox::KeyboardReturnType::DONE);
  editbox_sernvername_->setDelegate(&session_ip_editbox_delegate);
  editbox_sernvername_->setFontSize(9);
  editbox_sernvername_->setVisible(true);
  layer_setting->addChild(editbox_sernvername_, 1);

  y += button_height;

  Text* text_protobuf = Text::create("protobuf", "arial.ttf", 10);
  text_protobuf->setPosition(Vec2(center_x, y));
  text_protobuf->setAnchorPoint(Vec2(0.5, 0.0));
  layer_setting->addChild(text_protobuf);

  checkbox_protobuf_ = CheckBox::create("check_box_normal.png","",
                                        "check_box_active.png","",
                                        "check_box_active_disable.png");
  checkbox_protobuf_->setAnchorPoint(Vec2(0.0, 0.0));
  checkbox_protobuf_->setPosition(Vec2(0.0, y));
  checkbox_protobuf_->ignoreContentAdaptWithSize(false);
  checkbox_protobuf_->setContentSize(Size(20,20));
  checkbox_protobuf_->setSelected(with_protobuf_);
  checkbox_protobuf_->addEventListener([this](Ref* ref,CheckBox::EventType type) {
    if (type == CheckBox::EventType::SELECTED) {
      with_protobuf_ = true;
    }
    else {
      with_protobuf_ = false;
    }
  });
  layer_setting->addChild(checkbox_protobuf_);

  y += button_height;

  Text* text_reliability = Text::create("session reliability", "arial.ttf", 10);
  text_reliability->setPosition(Vec2(center_x, y));
  text_reliability->setAnchorPoint(Vec2(0.5, 0.0));
  layer_setting->addChild(text_reliability);

  checkbox_reliability_ = CheckBox::create("check_box_normal.png","",
                                           "check_box_active.png","",
                                           "check_box_active_disable.png");
  checkbox_reliability_->setAnchorPoint(Vec2(0.0, 0.0));
  checkbox_reliability_->setPosition(Vec2(0, y));
  checkbox_reliability_->ignoreContentAdaptWithSize(false);
  checkbox_reliability_->setContentSize(Size(20,20));
  checkbox_reliability_->setSelected(with_session_reliability_);
  checkbox_reliability_->addEventListener([this](Ref* ref,CheckBox::EventType type) {
    if (type == CheckBox::EventType::SELECTED) {
      with_session_reliability_ = true;
    }
    else {
      with_session_reliability_ = false;
    }
  });
  layer_setting->addChild(checkbox_reliability_);

  y += button_height;

  layer_setting->setContentSize(Size(visibleSize.width, y));

  // layer
  // funapi session
  auto layer_funapi_session = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_funapi_session->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  button_send_a_message_ = Button::create("button.png", "buttonHighlighted.png");
  button_send_a_message_->setScale9Enabled(true);
  button_send_a_message_->setContentSize(Size(button_width, button_height));
  button_send_a_message_->setPressedActionEnabled(true);
  button_send_a_message_->setAnchorPoint(Vec2(0.5, 0.0));
  button_send_a_message_->setPosition(Vec2(center_x, y));
  button_send_a_message_->setTitleText("Send a message");
  button_send_a_message_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Send a message");
    SendEchoMessage();
  });
  layer_funapi_session->addChild(button_send_a_message_);

  y += (button_height + gap_height);

  button_disconnect_ = Button::create("button.png", "buttonHighlighted.png");
  button_disconnect_->setScale9Enabled(true);
  button_disconnect_->setContentSize(Size(button_width, button_height));
  button_disconnect_->setPressedActionEnabled(true);
  button_disconnect_->setAnchorPoint(Vec2(0.5, 0.0));
  button_disconnect_->setPosition(Vec2(center_x, y));
  button_disconnect_->setTitleText("Disconnect");
  button_disconnect_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Disconnect");
    Disconnect();
  });
  layer_funapi_session->addChild(button_disconnect_);

  y += (button_height + gap_height);

  button_connect_http_ = Button::create("button.png", "buttonHighlighted.png");
  button_connect_http_->setScale9Enabled(true);
  button_connect_http_->setContentSize(Size(button_width, button_height));
  button_connect_http_->setPressedActionEnabled(true);
  button_connect_http_->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_http_->setPosition(Vec2(center_x, y));
  button_connect_http_->setTitleText("Connect (HTTP)");
  button_connect_http_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (HTTP)");
    ConnectHttp();
  });
  layer_funapi_session->addChild(button_connect_http_);

  y += (button_height + gap_height);

  button_connect_udp_ = Button::create("button.png", "buttonHighlighted.png");
  button_connect_udp_->setScale9Enabled(true);
  button_connect_udp_->setContentSize(Size(button_width, button_height));
  button_connect_udp_->setPressedActionEnabled(true);
  button_connect_udp_->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_udp_->setPosition(Vec2(center_x, y));
  button_connect_udp_->setTitleText("Connect (UDP)");
  button_connect_udp_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (UDP)");
    ConnectUdp();
  });
  layer_funapi_session->addChild(button_connect_udp_);

  y += (button_height + gap_height);

  button_connect_tcp_ = Button::create("button.png", "buttonHighlighted.png");
  button_connect_tcp_->setScale9Enabled(true);
  button_connect_tcp_->setContentSize(Size(button_width, button_height));
  button_connect_tcp_->setPressedActionEnabled(true);
  button_connect_tcp_->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_tcp_->setPosition(Vec2(center_x, y));
  button_connect_tcp_->setTitleText("Connect (TCP)");
  button_connect_tcp_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (TCP)");
    ConnectTcp();
  });
  layer_funapi_session->addChild(button_connect_tcp_);

  y += (button_height + gap_height);

  // label
  std::string label_string_session = "[FunapiSession]";
  auto label_session = Label::createWithTTF(label_string_session.c_str(), "arial.ttf", 10);
  label_session->setAnchorPoint(Vec2(0.5, 0.0));
  label_session->setPosition(Vec2(center_x,y));
  layer_funapi_session->addChild(label_session, 1);

  y += (button_height * 0.5);

  layer_funapi_session->setContentSize(Size(visibleSize.width, y));

  // layer
  // multicast
  auto layer_multicast = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_multicast->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  button_multicast_leave_all_channels_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_leave_all_channels_->setScale9Enabled(true);
  button_multicast_leave_all_channels_->setContentSize(Size(button_width, button_height));
  button_multicast_leave_all_channels_->setPressedActionEnabled(true);
  button_multicast_leave_all_channels_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_leave_all_channels_->setPosition(Vec2(center_x, y));
  button_multicast_leave_all_channels_->setTitleText("Leave all channels");
  button_multicast_leave_all_channels_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Leave all channels");
    LeaveMulticastAllChannels();
  });
  layer_multicast->addChild(button_multicast_leave_all_channels_);

  y += (button_height + gap_height);

  button_multicast_request_list_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_request_list_->setScale9Enabled(true);
  button_multicast_request_list_->setContentSize(Size(button_width, button_height));
  button_multicast_request_list_->setPressedActionEnabled(true);
  button_multicast_request_list_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_request_list_->setPosition(Vec2(center_x, y));
  button_multicast_request_list_->setTitleText("Request list");
  button_multicast_request_list_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Request channel list");
    RequestMulticastChannelList();
  });
  layer_multicast->addChild(button_multicast_request_list_);

  y += (button_height + gap_height);

  button_multicast_leave_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_leave_->setScale9Enabled(true);
  button_multicast_leave_->setContentSize(Size(button_width, button_height));
  button_multicast_leave_->setPressedActionEnabled(true);
  button_multicast_leave_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_leave_->setPosition(Vec2(center_x, y));
  button_multicast_leave_->setTitleText("Leave a channel");
  button_multicast_leave_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Leave a channel");
    LeaveMulticastChannel();
  });
  layer_multicast->addChild(button_multicast_leave_);

  y += (button_height + gap_height);

  button_multicast_send_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_send_->setScale9Enabled(true);
  button_multicast_send_->setContentSize(Size(button_width, button_height));
  button_multicast_send_->setPressedActionEnabled(true);
  button_multicast_send_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_send_->setPosition(Vec2(center_x, y));
  button_multicast_send_->setTitleText("Send a message");
  button_multicast_send_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Send a message");
    SendMulticastMessage();
  });
  layer_multicast->addChild(button_multicast_send_);

  y += (button_height + gap_height);

  button_multicast_join_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_join_->setScale9Enabled(true);
  button_multicast_join_->setContentSize(Size(button_width, button_height));
  button_multicast_join_->setPressedActionEnabled(true);
  button_multicast_join_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_join_->setPosition(Vec2(center_x, y));
  button_multicast_join_->setTitleText("Join a channel");
  button_multicast_join_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Join a channel");
    JoinMulticastChannel();
  });
  layer_multicast->addChild(button_multicast_join_);

  y += (button_height + gap_height);

  button_multicast_create_ = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_create_->setScale9Enabled(true);
  button_multicast_create_->setContentSize(Size(button_width, button_height));
  button_multicast_create_->setPressedActionEnabled(true);
  button_multicast_create_->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_create_->setPosition(Vec2(center_x, y));
  button_multicast_create_->setTitleText("Create 'multicast'");
  button_multicast_create_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Create 'multicast'");
    CreateMulticast();
  });
  layer_multicast->addChild(button_multicast_create_);

  y += button_height + (button_height * 0.5);

  std::string label_string_multicast = "[Multicasting]";
  auto label_multicast = Label::createWithTTF(label_string_multicast.c_str(), "arial.ttf", 10);
  label_multicast->setAnchorPoint(Vec2(0.5, 0.5));
  label_multicast->setPosition(Vec2(center_x, y));
  layer_multicast->addChild(label_multicast, 1);

  y += (button_height * 0.5);

  layer_multicast->setContentSize(Size(visibleSize.width, y));

  // layer
  // download
  auto layer_download = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_download->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  button_download_ = Button::create("button.png", "buttonHighlighted.png");
  button_download_->setScale9Enabled(true);
  button_download_->setContentSize(Size(button_width, button_height));
  button_download_->setPressedActionEnabled(true);
  button_download_->setAnchorPoint(Vec2(0.5, 0.0));
  button_download_->setPosition(Vec2(center_x, y));
  button_download_->setTitleText("Download");
  button_download_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Download");
    DownloaderTest();
  });
  layer_download->addChild(button_download_);

  y += button_height + (button_height * 0.5);

  std::stringstream ss_download;
  ss_download << "[Download] - " << kDownloadServer << ":" << kDownloadServerPort;
  auto label_download = Label::createWithTTF(ss_download.str().c_str(), "arial.ttf", 10);
  label_download->setAnchorPoint(Vec2(0.5, 0.5));
  label_download->setPosition(Vec2(center_x, y));
  layer_download->addChild(label_download, 1);

  y += (button_height * 0.5);

  layer_download->setContentSize(Size(visibleSize.width, y));

  // layer
  // announcement
  auto layer_announcement = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_announcement->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  button_announcement_ = Button::create("button.png", "buttonHighlighted.png");
  button_announcement_->setScale9Enabled(true);
  button_announcement_->setContentSize(Size(button_width, button_height));
  button_announcement_->setPressedActionEnabled(true);
  button_announcement_->setAnchorPoint(Vec2(0.5, 0.0));
  button_announcement_->setPosition(Vec2(center_x, y));
  button_announcement_->setTitleText("Announcement");
  button_announcement_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Announcement");
    RequestAnnouncements();
  });
  layer_announcement->addChild(button_announcement_);

  y += button_height + (button_height * 0.5);

  std::stringstream ss_announcement;
  ss_announcement << "[Announcement] - " << kAnnouncementServer << ":" << kAnnouncementServerPort;
  auto label_announcement = Label::createWithTTF(ss_announcement.str().c_str(), "arial.ttf", 10);
  label_announcement->setAnchorPoint(Vec2(0.5, 0.5));
  label_announcement->setPosition(Vec2(center_x, y));
  layer_announcement->addChild(label_announcement, 1);

  y += (button_height * 0.5);

  layer_announcement->setContentSize(Size(visibleSize.width, y));

  /*
  // layer
  // test
  auto layer_test = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_test->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  button_test_stop_ = Button::create("button.png", "buttonHighlighted.png");
  button_test_stop_->setScale9Enabled(true);
  button_test_stop_->setContentSize(Size(button_width, button_height));
  button_test_stop_->setPressedActionEnabled(true);
  button_test_stop_->setAnchorPoint(Vec2(0.5, 0.0));
  button_test_stop_->setPosition(Vec2(center_x, y));
  button_test_stop_->setTitleText("Test - Stop");
  button_test_stop_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Test - Stop");
    TestFunapi(false);
  });
  layer_test->addChild(button_test_stop_);

  y += (button_height + gap_height);

  button_test_start_ = Button::create("button.png", "buttonHighlighted.png");
  button_test_start_->setScale9Enabled(true);
  button_test_start_->setContentSize(Size(button_width, button_height));
  button_test_start_->setPressedActionEnabled(true);
  button_test_start_->setAnchorPoint(Vec2(0.5, 0.0));
  button_test_start_->setPosition(Vec2(center_x, y));
  button_test_start_->setTitleText("Test - Start");
  button_test_start_->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Test - Start");
    TestFunapi(true);
  });
  layer_test->addChild(button_test_start_);

  y += button_height + (button_height * 0.5);

  std::string label_string_test = "[Test]";
  auto label_test = Label::createWithTTF(label_string_test.c_str(), "arial.ttf", 10);
  label_test->setAnchorPoint(Vec2(0.5, 0.5));
  label_test->setPosition(Vec2(center_x, y));
  layer_test->addChild(label_test, 1);

  y += (button_height * 0.5);

  layer_test->setContentSize(Size(visibleSize.width, y));
  */

  // scroll view
  // layer set position
  std::vector<LayerColor*> layers;
  layers.push_back(layer_setting);
  layers.push_back(layer_funapi_session);
  layers.push_back(layer_multicast);
  layers.push_back(layer_download);
  layers.push_back(layer_announcement);
  // layers.push_back(layer_test);

  y = 0;
  float scroll_view_height = 0;
  int count = static_cast<int>(layers.size());

  for (int i=count-1;i>=0;--i) {
    auto layer = layers[i];
    layer->setPosition(0,y);
    scrollView->addChild(layer);

    Size size = layer->getContentSize();
    scroll_view_height += size.height;
    y += size.height;
  }

  scrollView->setInnerContainerSize(Size(scrollView->getContentSize().width, scroll_view_height));

  return true;
}

void FunapiTest::cleanup()
{
  // fun::DebugUtils::Log("cleanup");

  if (session_) {
    session_->Close();
  }

  if (multicast_) {
    multicast_->Close();
  }

  TestFunapi(false);
}

void FunapiTest::update(float delta)
{
  // fun::DebugUtils::Log("delta = %f", delta);

  // fun::FunapiTasks::UpdateAll();

  fun::FunapiSession::UpdateAll();

  if (announcement_) {
    fun::FunapiAnnouncement::UpdateAll();
  }

  if (downloader_) {
    fun::FunapiHttpDownloader::UpdateAll();
  }

  {
    static time_t last_time = 0;
    time_t now = time(NULL);
    if (now != last_time) {
      last_time = now;
      UpdateUI();
    }
  }
}

void FunapiTest::ConnectTcp()
{
  Connect(fun::TransportProtocol::kTcp);
}

void FunapiTest::ConnectUdp()
{
  Connect(fun::TransportProtocol::kUdp);
}

void FunapiTest::ConnectHttp()
{
  Connect(fun::TransportProtocol::kHttp);
}

void FunapiTest::ConnectWebsocket()
{
#if FUNAPI_HAVE_WEBSOCKET
  Connect(fun::TransportProtocol::kWebsocket);
#endif
}

void FunapiTest::Disconnect()
{
  if (session_) {
    session_->Close();
    return;
  }

  fun::DebugUtils::Log("You should connect first.");
}

void FunapiTest::SendEchoMessage()
{
  if (session_ == nullptr)
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else {
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return;
    }

    if (encoding == fun::FunEncoding::kProtobuf)
    {
      for (int i = 0; i < 10; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp << "hello proto - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(temp_string.c_str());

        session_->SendMessage(msg);
      }
    }

    if (encoding == fun::FunEncoding::kJson)
    {
      for (int i = 0; i < 10; ++i) {
        // std::to_string is not supported on android, using std::stringstream instead.
        std::stringstream ss_temp;
        ss_temp <<  "hello world - " << static_cast<int>(i);
        std::string temp_string = ss_temp.str();

        rapidjson::Document msg;
        msg.SetObject();
        rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        session_->SendMessage("echo", json_string);
      }
    }
  }
}

void FunapiTest::SendRedirectTestMessage()
{
  if (session_ == nullptr)
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else {
    fun::FunEncoding encoding = session_->GetEncoding(session_->GetDefaultProtocol());
    if (encoding == fun::FunEncoding::kNone)
    {
      fun::DebugUtils::Log("You should attach transport first.");
      return;
    }

    std::stringstream ss_temp;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1,0xffff);
    ss_temp << "name" << dist(re);
    std::string name = ss_temp.str();

    if (encoding == fun::FunEncoding::kProtobuf)
    {
      /*
      FunMessage msg;

      msg.set_msgtype("cs_hello");
      Hello *hello = msg.MutableExtension(cs_hello);
      // hello->set_name("hello-name");
      hello->set_name(name.c_str());

      session_->SendMessage(msg);
      */
    }

    if (encoding == fun::FunEncoding::kJson)
    {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(name.c_str(), msg.GetAllocator());
      msg.AddMember("name", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      session_->SendMessage("hello", json_string);
    }
  }
}

void FunapiTest::Connect(const fun::TransportProtocol protocol)
{
  if (!session_) {
    // create
    session_ = fun::FunapiSession::Create(kServer.c_str(), with_session_reliability_);

    // add callback
    session_->AddSessionEventCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                             const fun::TransportProtocol protocol,
                                             const fun::SessionEventType type,
                                             const std::string &session_id,
                                             const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::SessionEventType::kOpened) {
        OnSessionInitiated(session_id);
      }
      else if (type == fun::SessionEventType::kChanged) {
        // session id changed
      }
      else if (type == fun::SessionEventType::kClosed) {
        OnSessionClosed();
        session_ = nullptr;
      }
    });

    session_->AddTransportEventCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                           const fun::TransportProtocol protocol,
                                           const fun::TransportEventType type,
                                           const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed (%s)", fun::TransportProtocolToString(protocol).c_str());
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
        session_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called (%s)", fun::TransportProtocolToString(protocol).c_str());
      }
    });

    session_->AddJsonRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                     const fun::TransportProtocol protocol,
                                     const std::string &msg_type,
                                     const std::string &json_string) {
      if (msg_type.compare("echo") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", msg_type.c_str());
        fun::DebugUtils::Log("json: %s", json_string.c_str());
      }

      if (msg_type.compare("_maintenance") == 0) {
        fun::DebugUtils::Log("Maintenance message : %s", json_string.c_str());
      }
    });

    session_->AddProtobufRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                         const fun::TransportProtocol protocol,
                                         const FunMessage &message) {
      if (message.msgtype().compare("pbuf_echo") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", message.msgtype().c_str());

        PbufEchoMessage echo = message.GetExtension(pbuf_echo);
        fun::DebugUtils::Log("proto: %s", echo.msg().c_str());
      }

      if (message.msgtype().compare("_maintenance") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", message.msgtype().c_str());

        MaintenanceMessage maintenance = message.GetExtension(pbuf_maintenance);
        std::string date_start = maintenance.date_start();
        std::string date_end = maintenance.date_end();
        std::string message = maintenance.messages();

        fun::DebugUtils::Log("Maintenance message:\nstart: %s\nend: %s\nmessage: %s", date_start.c_str(), date_end.c_str(), message.c_str());
      }
    });

    /*
    session_->SetTransportOptionCallback([](const fun::TransportProtocol protocol,
                                            const std::string &flavor) -> std::shared_ptr<fun::FunapiTransportOption> {
      if (protocol == fun::TransportProtocol::kTcp) {
        auto option = fun::FunapiTcpTransportOption::Create();
        option->SetDisableNagle(true);
        return option;
      }

      return nullptr;
    });
    */
  }

  // connect
  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
  uint16_t port = 0;

  if (protocol == fun::TransportProtocol::kTcp) {
    port = with_protobuf_ ? 8022 : 8012;

    // auto option = fun::FunapiTcpTransportOption::Create();
    // option->SetDisableNagle(true);
    // option->SetEnablePing(true);
    // option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption, "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
    // session_->Connect(protocol, port, encoding, option);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    port = with_protobuf_ ? 8023 : 8013;

    // auto option = fun::FunapiUdpTransportOption::Create();
    // session_->Connect(protocol, port, encoding, option);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    port = with_protobuf_ ? 8028 : 8018;

    // auto option = fun::FunapiHttpTransportOption::Create();
    // option->SetSequenceNumberValidation(true);
    // option->SetUseHttps(true);
    // option->SetCACertFilePath(cocos2d::FileUtils::getInstance()->fullPathForFilename("cacert.pem"));
    // session_->Connect(protocol, port, encoding, option);
  }
#if FUNAPI_HAVE_WEBSOCKET
  else if (protocol == fun::TransportProtocol::kWebsocket) {
    port = with_protobuf_ ? 18022 : 18012;
  }
#endif

  if (with_session_reliability_) {
    port += 200;
  }

  session_->Connect(protocol, port, encoding);

  session_->SetDefaultProtocol(protocol);
}

void FunapiTest::OnSessionInitiated(const std::string &session_id)
{
  fun::DebugUtils::Log("session opened: %s", session_id.c_str());
}

void FunapiTest::OnSessionClosed()
{
  fun::DebugUtils::Log("session closed");
}

void FunapiTest::CreateMulticast()
{
  fun::DebugUtils::Log("(Button)CreateMulticast");

  if (!multicast_) {
    std::stringstream ss_temp;
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> dist(1,100);
    ss_temp << "player" << dist(re);
    std::string sender = ss_temp.str();

    fun::DebugUtils::Log("sender = %s", sender.c_str());

    if (!multicast_) {
      fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
      uint16_t port = with_protobuf_ ? 8122 : 8112;

      if (with_session_reliability_) {
        port += 200;
      }

      multicast_ = fun::FunapiMulticast::Create(sender.c_str(), kServer.c_str(), port, encoding, with_session_reliability_);
    }

    multicast_->AddJoinedCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                     const std::string &channel_id, const std::string &sender) {
      fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
    });
    multicast_->AddLeftCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                   const std::string &channel_id, const std::string &sender) {
      fun::DebugUtils::Log("LeftCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
    });
    multicast_->AddErrorCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                    int error) {
      // EC_ALREADY_JOINED = 1
      // EC_ALREADY_LEFT
      // EC_FULL_MEMBER
      // EC_CLOSED
      // EC_INVALID_TOKEN
      // EC_CANNOT_CREATE_CHANNEL
    });
    multicast_->AddChannelListCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                          const std::map<std::string, int> &cl){
      // fun::DebugUtils::Log("[channel list]");
      for (auto i : cl) {
        fun::DebugUtils::Log("%s - %d", i.first.c_str(), i.second);
      }
    });

    multicast_->AddSessionEventCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                           const fun::SessionEventType type,
                                           const std::string &session_id,
                                           const std::shared_ptr<fun::FunapiError> &error) {
      /*
      if (type == fun::SessionEventType::kOpened) {
      }
      else if (type == fun::SessionEventType::kChanged) {
        // session id changed
      }
      else if (type == fun::SessionEventType::kClosed) {
      }
      */
    });

    multicast_->AddTransportEventCallback([this](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                             const fun::TransportEventType type,
                                             const std::shared_ptr<fun::FunapiError> &error) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed");
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
        multicast_ = nullptr;
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called");
        multicast_ = nullptr;
      }
    });
  }

  multicast_->Connect();
}

void FunapiTest::JoinMulticastChannel()
{
  auto json_msg_handler =
    [this](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
      const fun::string &channel_id,
      const fun::string &sender_string,
      const fun::string &json_string)
  {
    fun::DebugUtils::Log("Arrived the chatting message. channel_id = %s, sender = %s, body = %s",
      channel_id.c_str(), sender_string.c_str(), json_string.c_str());
  };

  auto protobuf_msg_handler =
    [this](const std::shared_ptr<fun::FunapiMulticast> &funapi_multicast,
      const fun::string &channel_id,
      const fun::string &sender_string,
      const FunMessage& message)
  {
    if (message.HasExtension(multicast))
    {
      FunMulticastMessage mcast_msg = message.GetExtension(multicast);
      if (mcast_msg.HasExtension(chat)) {
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);
        fun::string text = chat_msg.text();
        fun::DebugUtils::Log("Arrived the chatting message. channel_id = %s, sender = %s, message = %s", channel_id.c_str(), sender_string.c_str(), text.c_str());
      }
    }
  };

  fun::DebugUtils::Log("JoinMulticastChannel button was clicked.");
  if (multicast_ == nullptr)
  {
    fun::DebugUtils::Log("Afunapi_tester::JoinMulticastChannel was called, but the FunapiMulticast instance is not created yet");
    return;
  }

  if (!multicast_->IsConnected())
  {
    fun::DebugUtils::Log("Afunapi_tester::JoinMulticastChannel was called, but the FunapiMulticast instance is not connected yet");
    return;
  }

  if (multicast_->IsInChannel(kMulticastTestChannel))
  {
    fun::DebugUtils::Log(
      "Afunapi_tester::JoinMulticastChannel was called, but the FunapiMulticast instance was already joined %s channel",
      kMulticastTestChannel.c_str());
    return;
  }

  auto encoding = multicast_->GetEncoding();
  if (encoding == fun::FunEncoding::kJson)
  {
    multicast_->JoinChannel(kMulticastTestChannel, json_msg_handler);
  }
  else if (encoding == fun::FunEncoding::kProtobuf)
  {
    multicast_->JoinChannel(kMulticastTestChannel, protobuf_msg_handler);
  }
}

void FunapiTest::SendMulticastMessage()
{
  fun::DebugUtils::Log("(Button)SendMulticastMessage");
  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      if (multicast_->GetEncoding() == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();

        std::string temp_messsage = "multicast test message";
        rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
        msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        multicast_->SendToChannel(kMulticastTestChannel, json_string);
      }

      if (multicast_->GetEncoding() == fun::FunEncoding::kProtobuf) {
        FunMessage msg;

        FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
        FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
        chat_msg->set_text("multicast test message");

        multicast_->SendToChannel(kMulticastTestChannel, msg);
      }
    }
  }
}

void FunapiTest::LeaveMulticastChannel()
{
  fun::DebugUtils::Log("(Button)LeaveMulticastChannel");
  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->LeaveChannel(kMulticastTestChannel);
    }
  }
}

void FunapiTest::RequestMulticastChannelList()
{
  fun::DebugUtils::Log("(Button)RequestMulticastChannelList");
  if (multicast_) {
    if (multicast_->IsConnected()) {
      multicast_->RequestChannelList();
    }
  }
}

void FunapiTest::LeaveMulticastAllChannels()
{
  // fun::DebugUtils::Log("(Button)LeaveMulticastAllChannels");
  if (multicast_) {
    multicast_->LeaveAllChannels();
  }
}

void FunapiTest::DownloaderTest()
{
  if (!downloader_) {
    std::stringstream ss_temp;
    ss_temp << "http://" << kDownloadServer << ":" << kDownloadServerPort;
    std::string download_url = ss_temp.str();

    downloader_ = fun::FunapiHttpDownloader::Create(download_url, cocos2d::FileUtils::getInstance()->getWritablePath());

    downloader_->AddReadyCallback([](const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
                                     const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info) {
      /*
      fun::DebugUtils::Log("ready download");
      for (auto i : info) {
        std::stringstream ss_temp;
        ss_temp << i->GetUrl() << std::endl;
        fun::DebugUtils::Log("%s", ss_temp.str().c_str());
      }
      */
    });

    downloader_->AddProgressCallback([](const std::shared_ptr<fun::FunapiHttpDownloader> &downloader,
                                        const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
                                        const int index,
                                        const int max_index,
                                        const uint64_t received_bytes,
                                        const uint64_t expected_bytes) {
      auto i = info[index];

      std::stringstream ss_temp;
      ss_temp << index << "/" << max_index << " " << received_bytes << "/" << expected_bytes << " " << i->GetUrl() << std::endl;
      fun::DebugUtils::Log("%s", ss_temp.str().c_str());
    });

    downloader_->AddCompletionCallback([this](const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
                                       const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
                                       const fun::FunapiHttpDownloader::ResultCode result_code) {
      if (result_code == fun::FunapiHttpDownloader::ResultCode::kSucceed) {
        for (auto i : info) {
          fun::DebugUtils::Log("file_path=%s", i->GetPath().c_str());
        }
      }

      downloader_ = nullptr;
    });
    downloader_->SetTimeoutPerFile(5);

    downloader_->Start();
  }
}

void FunapiTest::RequestAnnouncements()
{
  if (announcement_ == nullptr) {
    std::stringstream ss_url;
    ss_url << "http://" << kAnnouncementServer << ":" << kAnnouncementServerPort;

    announcement_ = fun::FunapiAnnouncement::Create(ss_url.str(), cocos2d::FileUtils::getInstance()->getWritablePath());

    announcement_->AddCompletionCallback([this](const std::shared_ptr<fun::FunapiAnnouncement> &announcement,
                                                const std::vector<std::shared_ptr<fun::FunapiAnnouncementInfo>>&info,
                                                const fun::FunapiAnnouncement::ResultCode result){
      if (result == fun::FunapiAnnouncement::ResultCode::kSucceed) {
        for (auto &i : info) {

          fun::stringstream ss;

          ss << "FunapiAnnounce reponse : " << "data=" << i->GetDate() << " ";
          ss << "message=" << i->GetMessageText() << " ";
          ss << "subject=" << i->GetSubject() << " ";
          ss << "file_path=" << i->GetFilePath() << " ";
          ss << "kind=" << i->GetKind() << " ";
          ss << "extra_image_path={";

          auto extra_image_infos = i->GetExtraImageInfos();
          for (auto &extra_info : extra_image_infos)
          {
            ss << extra_info->GetFilePath() << " ";
          }

          ss << "}";

          fun::DebugUtils::Log("%s", ss.str().c_str());
        }
      }

      announcement_ = nullptr;
    });
  }

  announcement_->RequestList(5);
}

static bool g_bTestRunning = false;

void FunapiTest::UpdateUI()
{
  bool is_connected_session = false;
  if (session_) {
    if (session_->IsConnected()) {
      is_connected_session = true;
    }
  }

  if (is_connected_session) {
    button_connect_tcp_->setEnabled(false);
    button_connect_udp_->setEnabled(false);
    button_connect_http_->setEnabled(false);
    button_disconnect_->setEnabled(true);
    button_send_a_message_->setEnabled(true);
  }
  else {
    button_connect_tcp_->setEnabled(true);
    button_connect_udp_->setEnabled(true);
    button_connect_http_->setEnabled(true);
    button_disconnect_->setEnabled(false);
    button_send_a_message_->setEnabled(false);
  }

  bool is_connected_multicast = false;
  if (multicast_) {
    if (multicast_->IsConnected()) {
      is_connected_multicast = true;
    }
  }

  if (is_connected_multicast) {
    button_multicast_create_->setEnabled(false);
    if (multicast_->IsInChannel(kMulticastTestChannel)) {
      button_multicast_join_->setEnabled(false);
      button_multicast_send_->setEnabled(true);
      button_multicast_leave_->setEnabled(true);
      button_multicast_leave_all_channels_->setEnabled(true);
    }
    else {
      button_multicast_join_->setEnabled(true);
      button_multicast_send_->setEnabled(false);
      button_multicast_leave_->setEnabled(false);
      button_multicast_leave_all_channels_->setEnabled(false);
    }
    button_multicast_request_list_->setEnabled(true);
  }
  else {
    button_multicast_create_->setEnabled(true);
    button_multicast_join_->setEnabled(false);
    button_multicast_send_->setEnabled(false);
    button_multicast_leave_->setEnabled(false);
    button_multicast_request_list_->setEnabled(false);
    button_multicast_leave_all_channels_->setEnabled(false);
  }

  if (is_connected_session || is_connected_multicast) {
    editbox_sernvername_->setEnabled(false);
    checkbox_protobuf_->setEnabled(false);
    checkbox_reliability_->setEnabled(false);
  }
}


void test_funapi_session(const int index, std::string server_ip,
                          const int server_port,
                          fun::TransportProtocol protocol,
                          fun::FunEncoding encoding,
                          bool use_session_reliability) {

  std::function<void(const std::shared_ptr<fun::FunapiSession>&,
                     const fun::TransportProtocol,
                     const std::string&)> send_message =
  [](const std::shared_ptr<fun::FunapiSession>&s,
     const fun::TransportProtocol protocol,
     const std::string &temp_string) {
    if (s->GetEncoding(protocol) == fun::FunEncoding::kJson) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      s->SendMessage("echo", json_string);
    }
    else if (s->GetEncoding(protocol) == fun::FunEncoding::kProtobuf) {
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(temp_string.c_str());

      s->SendMessage(msg);
    }
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), use_session_reliability);
  bool is_ok = true;

  // add callback
  session->AddSessionEventCallback([index, &send_message](const std::shared_ptr<fun::FunapiSession> &s,
                                                          const fun::TransportProtocol protocol,
                                                          const fun::SessionEventType type,
                                                          const std::string &session_id,
                                                          const std::shared_ptr<fun::FunapiError> &error) {
    if (type == fun::SessionEventType::kOpened) {
      // // // //
      // send
      std::stringstream ss_temp;
      ss_temp << static_cast<int>(0);
      std::string temp_string = ss_temp.str();

      send_message(s, protocol, temp_string);
    }
  });

  session->AddTransportEventCallback([index, &is_ok](const std::shared_ptr<fun::FunapiSession> &s,
                                                     const fun::TransportProtocol protocol,
                                                     const fun::TransportEventType type,
                                                     const std::shared_ptr<fun::FunapiError> &error) {
    if (type == fun::TransportEventType::kConnectionFailed ||
        type == fun::TransportEventType::kConnectionTimedOut ||
        type == fun::TransportEventType::kDisconnected) {
      is_ok = false;
    }
  });

  session->AddJsonRecvCallback([index, &send_message](const std::shared_ptr<fun::FunapiSession> &s,
                                       const fun::TransportProtocol protocol,
                                       const std::string &msg_type, const std::string &json_string) {
    if (msg_type.compare("echo") == 0) {
      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      int count = 0;
      if (msg_recv.HasMember("message")) {
        count = atoi(msg_recv["message"].GetString());
        printf("(%d) echo - %d\n", index, count);
        ++count;
      }

      // // // //
      // wait
      std::this_thread::sleep_for(std::chrono::seconds(1));

      // // // //
      // send
      std::stringstream ss_temp;
      ss_temp << static_cast<int>(count);
      std::string temp_string = ss_temp.str();

      send_message(s, protocol, temp_string);
    }
  });

  session->AddProtobufRecvCallback([index, &send_message](const std::shared_ptr<fun::FunapiSession> &s,
                                      const fun::TransportProtocol protocol,
                                      const FunMessage &message) {
    if (message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo_recv = message.GetExtension(pbuf_echo);

      int count = 0;
      count = atoi(echo_recv.msg().c_str());
      printf("(%d) echo - %d\n", index, count);
      ++count;

      // // // //
      // wait
      std::this_thread::sleep_for(std::chrono::seconds(1));

      // // // //
      // send
      std::stringstream ss_temp;
      ss_temp << static_cast<int>(count);
      std::string temp_string = ss_temp.str();

      send_message(s, protocol, temp_string);
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEnablePing(true);
  option->SetDisableNagle(true);
  session->Connect(fun::TransportProtocol::kTcp, server_port, encoding, option);

  /*
  // multi transport
  if (encoding == fun::FunEncoding::kProtobuf) {
    session->Connect(fun::TransportProtocol::kUdp, 8023, encoding);
    session->Connect(fun::TransportProtocol::kHttp, 8028, encoding);
  }
  else if (encoding == fun::FunEncoding::kJson) {
    session->Connect(fun::TransportProtocol::kUdp, 8013, encoding);
    session->Connect(fun::TransportProtocol::kHttp, 8018, encoding);
  }
  */

  while (g_bTestRunning && is_ok) {
    session->Update();
  }

  session->Close();
}

void FunapiTest::TestFunapi(bool bStart)
{
  const int kMaxThread = 2;

  fun::TransportProtocol protocol = fun::TransportProtocol::kTcp;

  fun::FunEncoding encoding = fun::FunEncoding::kNone;
  if (with_protobuf_) {
    encoding = fun::FunEncoding::kProtobuf;
  }
  else {
    encoding = fun::FunEncoding::kJson;
  }

  int server_port = 0;
  if (encoding == fun::FunEncoding::kJson) {
    if (protocol == fun::TransportProtocol::kTcp) server_port = 8012;
    else if (protocol == fun::TransportProtocol::kUdp) server_port = 8013;
    else if (protocol == fun::TransportProtocol::kHttp) server_port = 8018;
  }
  else if (encoding == fun::FunEncoding::kProtobuf) {
    if (protocol == fun::TransportProtocol::kTcp) server_port = 8022;
    else if (protocol == fun::TransportProtocol::kUdp) server_port = 8023;
    else if (protocol == fun::TransportProtocol::kHttp) server_port = 8028;
  }

  if (with_session_reliability_) {
    server_port += 200;
  }

  static std::vector<std::thread> temp_thread(kMaxThread);

  g_bTestRunning = false;
  for (int i = 0; i < kMaxThread; ++i) {
    if (temp_thread[i].joinable()) {
      temp_thread[i].join();
    }
  }

  if (bStart) {
    g_bTestRunning = true;
    for (int i = 0; i < kMaxThread; ++i) {
      temp_thread[i] = std::thread([this, i, server_port, protocol, encoding](){
        test_funapi_session(i, kServer, server_port, protocol, encoding, with_session_reliability_);
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}
