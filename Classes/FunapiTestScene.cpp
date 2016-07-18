#include "FunapiTestScene.h"
#include "ui/CocosGUI.h"

#include "funapi/funapi_session.h"
#include "funapi/funapi_multicasting.h"
#include "funapi/funapi_downloader.h"

#include "test_messages.pb.h"
#include "funapi/management/maintenance_message.pb.h"
#include "funapi/service/multicast_message.pb.h"
#include "funapi/funapi_utils.h"

#include "json/writer.h"
#include "json/stringbuffer.h"

USING_NS_CC;
using namespace cocos2d::ui;

Scene* FunapiTest::createScene()
{
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
  // funapi network
  auto layer_funapi_network = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_funapi_network->setAnchorPoint(Vec2(0.0,0.0));

  float y = gap_height;

  Button* button_send_a_message = Button::create("button.png", "buttonHighlighted.png");
  button_send_a_message->setScale9Enabled(true);
  button_send_a_message->setContentSize(Size(button_width, button_height));
  button_send_a_message->setPressedActionEnabled(true);
  button_send_a_message->setAnchorPoint(Vec2(0.5, 0.0));
  button_send_a_message->setPosition(Vec2(center_x, y));
  button_send_a_message->setTitleText("Send a message");
  button_send_a_message->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Send a message");
    SendEchoMessage();
  });
  layer_funapi_network->addChild(button_send_a_message);

  y += (button_height + gap_height);

  Button* button_disconnect = Button::create("button.png", "buttonHighlighted.png");
  button_disconnect->setScale9Enabled(true);
  button_disconnect->setContentSize(Size(button_width, button_height));
  button_disconnect->setPressedActionEnabled(true);
  button_disconnect->setAnchorPoint(Vec2(0.5, 0.0));
  button_disconnect->setPosition(Vec2(center_x, y));
  button_disconnect->setTitleText("Disconnect");
  button_disconnect->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Disconnect");
    Disconnect();
  });
  layer_funapi_network->addChild(button_disconnect);

  y += (button_height + gap_height);

  Button* button_connect_http = Button::create("button.png", "buttonHighlighted.png");
  button_connect_http->setScale9Enabled(true);
  button_connect_http->setContentSize(Size(button_width, button_height));
  button_connect_http->setPressedActionEnabled(true);
  button_connect_http->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_http->setPosition(Vec2(center_x, y));
  button_connect_http->setTitleText("Connect (HTTP)");
  button_connect_http->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (HTTP)");
    ConnectHttp();
  });
  layer_funapi_network->addChild(button_connect_http);

  y += (button_height + gap_height);

  Button* button_connect_udp = Button::create("button.png", "buttonHighlighted.png");
  button_connect_udp->setScale9Enabled(true);
  button_connect_udp->setContentSize(Size(button_width, button_height));
  button_connect_udp->setPressedActionEnabled(true);
  button_connect_udp->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_udp->setPosition(Vec2(center_x, y));
  button_connect_udp->setTitleText("Connect (UDP)");
  button_connect_udp->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (UDP)");
    ConnectUdp();
  });
  layer_funapi_network->addChild(button_connect_udp);

  y += (button_height + gap_height);

  Button* button_connect_tcp = Button::create("button.png", "buttonHighlighted.png");
  button_connect_tcp->setScale9Enabled(true);
  button_connect_tcp->setContentSize(Size(button_width, button_height));
  button_connect_tcp->setPressedActionEnabled(true);
  button_connect_tcp->setAnchorPoint(Vec2(0.5, 0.0));
  button_connect_tcp->setPosition(Vec2(center_x, y));
  button_connect_tcp->setTitleText("Connect (TCP)");
  button_connect_tcp->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Connect (TCP)");
    ConnectTcp();
  });
  layer_funapi_network->addChild(button_connect_tcp);

  y += button_height + (button_height * 0.5);

  // label : server hostname or ip
  std::string label_string_server_hostname_or_ip = "[FunapiSession] - " + kServerIp;
  auto label_server_hostname_or_ip = Label::createWithTTF(label_string_server_hostname_or_ip.c_str(), "arial.ttf", 10);
  label_server_hostname_or_ip->setAnchorPoint(Vec2(0.5, 0.5));
  label_server_hostname_or_ip->setPosition(Vec2(center_x,y));
  layer_funapi_network->addChild(label_server_hostname_or_ip, 1);

  y += (button_height * 0.5);

  layer_funapi_network->setContentSize(Size(visibleSize.width, y));

  // layer
  // multicast
  auto layer_multicast = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_multicast->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  Button* button_multicast_leave_all_channels = Button::create("button.png", "buttonHighlighted.png");
  button_multicast_leave_all_channels->setScale9Enabled(true);
  button_multicast_leave_all_channels->setContentSize(Size(button_width, button_height));
  button_multicast_leave_all_channels->setPressedActionEnabled(true);
  button_multicast_leave_all_channels->setAnchorPoint(Vec2(0.5, 0.0));
  button_multicast_leave_all_channels->setPosition(Vec2(center_x, y));
  button_multicast_leave_all_channels->setTitleText("Leave all channels");
  button_multicast_leave_all_channels->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Leave all channels");
    LeaveMulticastAllChannels();
  });
  layer_multicast->addChild(button_multicast_leave_all_channels);

  y += (button_height + gap_height);

  Button* button_request_multicast_list = Button::create("button.png", "buttonHighlighted.png");
  button_request_multicast_list->setScale9Enabled(true);
  button_request_multicast_list->setContentSize(Size(button_width, button_height));
  button_request_multicast_list->setPressedActionEnabled(true);
  button_request_multicast_list->setAnchorPoint(Vec2(0.5, 0.0));
  button_request_multicast_list->setPosition(Vec2(center_x, y));
  button_request_multicast_list->setTitleText("Request list");
  button_request_multicast_list->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Request channel list");
    RequestMulticastChannelList();
  });
  layer_multicast->addChild(button_request_multicast_list);

  y += (button_height + gap_height);

  Button* button_leave_multicast = Button::create("button.png", "buttonHighlighted.png");
  button_leave_multicast->setScale9Enabled(true);
  button_leave_multicast->setContentSize(Size(button_width, button_height));
  button_leave_multicast->setPressedActionEnabled(true);
  button_leave_multicast->setAnchorPoint(Vec2(0.5, 0.0));
  button_leave_multicast->setPosition(Vec2(center_x, y));
  button_leave_multicast->setTitleText("Leave a channel");
  button_leave_multicast->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Leave a channel");
    LeaveMulticastChannel();
  });
  layer_multicast->addChild(button_leave_multicast);

  y += (button_height + gap_height);

  Button* button_send_multicast = Button::create("button.png", "buttonHighlighted.png");
  button_send_multicast->setScale9Enabled(true);
  button_send_multicast->setContentSize(Size(button_width, button_height));
  button_send_multicast->setPressedActionEnabled(true);
  button_send_multicast->setAnchorPoint(Vec2(0.5, 0.0));
  button_send_multicast->setPosition(Vec2(center_x, y));
  button_send_multicast->setTitleText("Send a message");
  button_send_multicast->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Send a message");
    SendMulticastMessage();
  });
  layer_multicast->addChild(button_send_multicast);

  y += (button_height + gap_height);

  Button* button_join_multicast = Button::create("button.png", "buttonHighlighted.png");
  button_join_multicast->setScale9Enabled(true);
  button_join_multicast->setContentSize(Size(button_width, button_height));
  button_join_multicast->setPressedActionEnabled(true);
  button_join_multicast->setAnchorPoint(Vec2(0.5, 0.0));
  button_join_multicast->setPosition(Vec2(center_x, y));
  button_join_multicast->setTitleText("Join a channel");
  button_join_multicast->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Join a channel");
    JoinMulticastChannel();
  });
  layer_multicast->addChild(button_join_multicast);

  y += (button_height + gap_height);

  Button* button_create_multicast = Button::create("button.png", "buttonHighlighted.png");
  button_create_multicast->setScale9Enabled(true);
  button_create_multicast->setContentSize(Size(button_width, button_height));
  button_create_multicast->setPressedActionEnabled(true);
  button_create_multicast->setAnchorPoint(Vec2(0.5, 0.0));
  button_create_multicast->setPosition(Vec2(center_x, y));
  button_create_multicast->setTitleText("Create 'multicast'");
  button_create_multicast->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Create 'multicast'");
    CreateMulticast();
  });
  layer_multicast->addChild(button_create_multicast);

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

  Button* button_download = Button::create("button.png", "buttonHighlighted.png");
  button_download->setScale9Enabled(true);
  button_download->setContentSize(Size(button_width, button_height));
  button_download->setPressedActionEnabled(true);
  button_download->setAnchorPoint(Vec2(0.5, 0.0));
  button_download->setPosition(Vec2(center_x, y));
  button_download->setTitleText("Download");
  button_download->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Download");
    DownloaderTest();
  });
  layer_download->addChild(button_download);

  y += button_height + (button_height * 0.5);

  std::string label_string_download = "[Download] - " + kDownloadServerIp;
  auto label_download = Label::createWithTTF(label_string_download.c_str(), "arial.ttf", 10);
  label_download->setAnchorPoint(Vec2(0.5, 0.5));
  label_download->setPosition(Vec2(center_x, y));
  layer_download->addChild(label_download, 1);

  y += (button_height * 0.5);

  layer_download->setContentSize(Size(visibleSize.width, y));

  button_download->setEnabled(false);

  // layer
  // test
  auto layer_test = LayerColor::create(Color4B(0, 0, 0, 255), visibleSize.width, visibleSize.height/2.0);
  layer_test->setAnchorPoint(Vec2(0.0,0.0));

  y = gap_height;

  Button* button_test_stop = Button::create("button.png", "buttonHighlighted.png");
  button_test_stop->setScale9Enabled(true);
  button_test_stop->setContentSize(Size(button_width, button_height));
  button_test_stop->setPressedActionEnabled(true);
  button_test_stop->setAnchorPoint(Vec2(0.5, 0.0));
  button_test_stop->setPosition(Vec2(center_x, y));
  button_test_stop->setTitleText("Test - Stop");
  button_test_stop->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Test - Stop");
    TestFunapi(false);
  });
  layer_test->addChild(button_test_stop);

  y += (button_height + gap_height);

  Button* button_test_start = Button::create("button.png", "buttonHighlighted.png");
  button_test_start->setScale9Enabled(true);
  button_test_start->setContentSize(Size(button_width, button_height));
  button_test_start->setPressedActionEnabled(true);
  button_test_start->setAnchorPoint(Vec2(0.5, 0.0));
  button_test_start->setPosition(Vec2(center_x, y));
  button_test_start->setTitleText("Test - Start");
  button_test_start->addClickEventListener([this](Ref* sender) {
    fun::DebugUtils::Log("(Button)Test - Start");
    TestFunapi(true);
  });
  layer_test->addChild(button_test_start);

  y += button_height + (button_height * 0.5);

  std::string label_string_test = "[Test] - " + kServerIp;
  auto label_test = Label::createWithTTF(label_string_test.c_str(), "arial.ttf", 10);
  label_test->setAnchorPoint(Vec2(0.5, 0.5));
  label_test->setPosition(Vec2(center_x, y));
  layer_test->addChild(label_test, 1);

  y += (button_height * 0.5);

  layer_test->setContentSize(Size(visibleSize.width, y));

  // scroll view
  // layer set position
  std::vector<LayerColor*> layers;
  layers.push_back(layer_funapi_network);
  layers.push_back(layer_multicast);
  layers.push_back(layer_download);
  layers.push_back(layer_test);

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

  if (session_) {
    session_->Update();
  }

  if (multicast_) {
    multicast_->Update();
  }

  if (downloader_)
  {
    downloader_->Update();

    if (code_ != fun::DownloadResult::NONE) {
      code_ = fun::DownloadResult::NONE;
      downloader_ = nullptr;
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

void FunapiTest::Connect(const fun::TransportProtocol protocol)
{
  if (!session_) {
    // create
    session_ = fun::FunapiSession::create(kServerIp.c_str(), with_session_reliability_);

    // add callback
    session_->AddSessionEventCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                             const fun::TransportProtocol protocol,
                                             const fun::SessionEventType type,
                                             const std::string &session_id) {
      if (type == fun::SessionEventType::kOpened) {
        OnSessionInitiated(session_id);
      }
      else if (type == fun::SessionEventType::kChanged) {
        // session id changed
      }
      else if (type == fun::SessionEventType::kClosed) {
        OnSessionClosed();
      }
    });

    session_->AddTransportEventCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                           const fun::TransportProtocol protocol,
                                           const fun::TransportEventType type) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed(%d)", (int)protocol);
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called (%d)", (int)protocol);
        // session->Connect(protocol);
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
    });

    session_->AddProtobufRecvCallback([](const std::shared_ptr<fun::FunapiSession> &session,
                                         const fun::TransportProtocol protocol,
                                         const FunMessage &message) {
      if (message.msgtype().compare("pbuf_echo") == 0) {
        fun::DebugUtils::Log("msg '%s' arrived.", message.msgtype().c_str());

        PbufEchoMessage echo = message.GetExtension(pbuf_echo);
        fun::DebugUtils::Log("proto: %s", echo.msg().c_str());
      }
    });
  }

  // connect
  if (session_->HasTransport(protocol)) {
    session_->Connect(protocol);
  }
  else {
    fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
    uint16_t port;

    if (protocol == fun::TransportProtocol::kTcp) {
      port = with_protobuf_ ? 8022 : 8012;

      /*
       auto option = fun::FunapiTcpTransportOption::create();
       option->SetDisableNagle(true);
       option->SetEnablePing(true);
       session_->Connect(protocol, port, encoding, option);
       */
    }
    else if (protocol == fun::TransportProtocol::kUdp) {
      port = with_protobuf_ ? 8023 : 8013;

      /*
       auto option = fun::FunapiUdpTransportOption::create();
       session_->Connect(protocol, port, encoding, option);
       */
    }
    else if (protocol == fun::TransportProtocol::kHttp) {
      port = with_protobuf_ ? 8028 : 8018;

      /*
       auto option = fun::FunapiHttpTransportOption::create();
       option->SetSequenceNumberValidation(true);
       option->SetUseHttps(false);
       session_->Connect(protocol, port, encoding, option);
       */
    }

    session_->Connect(protocol, port, encoding);
  }

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

    fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;
    uint16_t port = with_protobuf_ ? 8022 : 8012;

    multicast_ = fun::FunapiMulticast::create(sender.c_str(), kServerIp.c_str(), port, encoding);

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
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED
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
                                           const std::string &session_id) {
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

    multicast_->AddTransportEventCallback([](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                                           const fun::TransportEventType type) {
      if (type == fun::TransportEventType::kStarted) {
        fun::DebugUtils::Log("Transport Started called.");
      }
      else if (type == fun::TransportEventType::kStopped) {
        fun::DebugUtils::Log("Transport Stopped called.");
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        fun::DebugUtils::Log("Transport Connection Failed");
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        fun::DebugUtils::Log("Transport Connection Timedout called");
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        fun::DebugUtils::Log("Transport Disconnected called");
      }
    });

    multicast_->Connect();
  }
}

void FunapiTest::JoinMulticastChannel()
{
  fun::DebugUtils::Log("(Button)JoinMulticastChannel");
  if (multicast_) {
    if (!multicast_->IsInChannel(kMulticastTestChannel)) {
      // add callback
      multicast_->AddJsonChannelMessageCallback(kMulticastTestChannel,
                                                [this](const std::shared_ptr<fun::FunapiMulticast>& funapi_multicast,
                                                       const std::string &channel_id,
                                                       const std::string &sender,
                                                       const std::string &json_string)
      {
        fun::DebugUtils::Log("channel_id=%s, sender=%s, body=%s", channel_id.c_str(), sender.c_str(), json_string.c_str());
      });

      multicast_->AddProtobufChannelMessageCallback(kMulticastTestChannel,
                                                    [this](const std::shared_ptr<fun::FunapiMulticast> &funapi_multicast,
                                                           const std::string &channel_id,
                                                           const std::string &sender,
                                                           const FunMessage& message)
      {
        FunMulticastMessage mcast_msg = message.GetExtension(multicast);
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);
        std::string text = chat_msg.text();

        fun::DebugUtils::Log("channel_id=%s, sender=%s, message=%s", channel_id.c_str(), sender.c_str(), text.c_str());
      });

      // join
      multicast_->JoinChannel(kMulticastTestChannel);
    }
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
        mcast_msg->set_channel(kMulticastTestChannel.c_str());
        mcast_msg->set_bounce(true);

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
    downloader_ = std::make_shared<fun::FunapiHttpDownloader>();

    downloader_->AddVerifyCallback([this](const std::string &path) {
      fun::DebugUtils::Log("Check file - %s", path.c_str());
    });
    downloader_->AddReadyCallback([this](int total_count, uint64_t total_size) {
      downloader_->StartDownload();
    });
    downloader_->AddUpdateCallback([this](const std::string &path, uint64_t bytes_received, uint64_t total_bytes, int percentage) {
      fun::DebugUtils::Log("Downloading - path:%s / received:%llu / total:%llu / %d", path.c_str(), bytes_received, total_bytes, percentage);
    });
    downloader_->AddFinishedCallback([this](fun::DownloadResult code) {
      fun::DebugUtils::Log("Downloader Finished - %d", code);
      code_ = code;
    });

    std::stringstream ss_temp;
    ss_temp << "http://" << kDownloadServerIp << ":" << kDownloadServerPort;
    std::string download_url = ss_temp.str();

    std::string target_path = fun::FunapiUtil::GetWritablePath();

    downloader_->GetDownloadList(download_url, target_path);
  }
}

static bool g_bTestRunning = true;

// test function
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

  auto session = fun::FunapiSession::create(server_ip.c_str(), use_session_reliability);
  bool is_ok = true;

  // add callback
  session->AddSessionEventCallback([index, &send_message](const std::shared_ptr<fun::FunapiSession> &s,
                                           const fun::TransportProtocol protocol,
                                           const fun::SessionEventType type,
                                           const std::string &session_id) {
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
                                                     const fun::TransportEventType type) {
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

  auto option = fun::FunapiTcpTransportOption::create();
  option->SetEnablePing(true);
  option->SetDisableNagle(true);
  session->Connect(fun::TransportProtocol::kTcp, server_port, encoding, option);

  // multi transport
  if (encoding == fun::FunEncoding::kProtobuf) {
    session->Connect(fun::TransportProtocol::kUdp, 8023, encoding);
    session->Connect(fun::TransportProtocol::kHttp, 8028, encoding);
  }
  else if (encoding == fun::FunEncoding::kJson) {
    session->Connect(fun::TransportProtocol::kUdp, 8013, encoding);
    session->Connect(fun::TransportProtocol::kHttp, 8018, encoding);
  }

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
        test_funapi_session(i, kServerIp, server_port, protocol, encoding, with_session_reliability_);
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}
