#include "FunapiTestScene.h"
#include "ui/CocosGUI.h"

#include "funapi/funapi_network.h"
#include "funapi/pb/test_messages.pb.h"
#include "funapi/pb/management/maintenance_message.pb.h"
#include "funapi/pb/service/multicast_message.pb.h"
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
  std::string label_string_server_hostname_or_ip = "[FunapiNetwork] - " + kServerIp;
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
  });
  layer_download->addChild(button_download);

  y += button_height + (button_height * 0.5);

  std::string label_string_download = "[Download] - " + kServerIp;
  auto label_download = Label::createWithTTF(label_string_download.c_str(), "arial.ttf", 10);
  label_download->setAnchorPoint(Vec2(0.5, 0.5));
  label_download->setPosition(Vec2(center_x, y));
  layer_download->addChild(label_download, 1);

  y += (button_height * 0.5);

  layer_download->setContentSize(Size(visibleSize.width, y));

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
    TestFunapiNetwork(false);
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
    TestFunapiNetwork(true);
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

  if (network_)
  {
    network_->Stop();
  }

  TestFunapiNetwork(false);
}

void FunapiTest::update(float delta)
{
  // fun::DebugUtils::Log("delta = %f", delta);

  if (network_)
  {
    network_->Update();
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

bool FunapiTest::IsConnected()
{
  if (network_) {
    return network_->IsConnected();
  }

  return false;
}

void FunapiTest::Disconnect()
{
  if (multicast_) {
    multicast_->LeaveAllChannels();
  }

  if (network_) {
    if (network_->IsStarted()) {
      network_->Stop();
      return;
    }
  }

  fun::DebugUtils::Log("You should connect first.");
}

void FunapiTest::SendEchoMessage()
{
  if (network_ == nullptr || (network_->IsStarted() == false && network_->IsReliableSession()))
  {
    fun::DebugUtils::Log("You should connect first.");
  }
  else {
    fun::FunEncoding encoding = network_->GetEncoding(network_->GetDefaultProtocol());
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
        network_->SendMessage(msg);
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

        network_->SendMessage("echo", json_string);
      }
    }
  }
}

void FunapiTest::Connect(const fun::TransportProtocol protocol)
{
  if (!network_ || !network_->IsReliableSession()) {
    network_ = std::make_shared<fun::FunapiNetwork>(with_session_reliability_);

    network_->AddSessionInitiatedCallback([this](const std::string &session_id){ OnSessionInitiated(session_id); });
    network_->AddSessionClosedCallback([this](){ OnSessionClosed(); });

    network_->AddStoppedAllTransportCallback([this]() { OnStoppedAllTransport(); });
    network_->AddTransportConnectFailedCallback([this](const fun::TransportProtocol p){ OnTransportConnectFailed(p); });
    network_->AddTransportConnectTimeoutCallback([this](const fun::TransportProtocol p){ OnTransportConnectTimeout(p); });
    network_->AddTransportStartedCallback([this](const fun::TransportProtocol p){ OnTransportStarted(p); });
    network_->AddTransportClosedCallback([this](const fun::TransportProtocol p){ OnTransportClosed(p); });

    network_->AddMaintenanceCallback([this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body){ OnMaintenanceMessage(p, type, v_body); });

    network_->RegisterHandler("echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoJson(p, type, v_body); });
    network_->RegisterHandler("pbuf_echo", [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoProto(p, type, v_body); });

    network_->AttachTransport(GetNewTransport(protocol));
  }
  else {
    if (!network_->HasTransport(protocol))
    {
      network_->AttachTransport(GetNewTransport(protocol));
    }

    network_->SetDefaultProtocol(protocol);
  }

  network_->Start();
}

std::shared_ptr<fun::FunapiTransport> FunapiTest::GetNewTransport(fun::TransportProtocol protocol)
{
  std::shared_ptr<fun::FunapiTransport> transport = nullptr;
  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;

  if (protocol == fun::TransportProtocol::kTcp) {
    transport = fun::FunapiTcpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8022 : 8012), encoding);

    // transport->SetAutoReconnect(true);
    // transport->SetEnablePing(true);
    // transport->SetDisableNagle(true);
    // transport->SetConnectTimeout(10);
    // transport->SetSequenceNumberValidation(true);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    transport = fun::FunapiUdpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8023 : 8013), encoding);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    transport = fun::FunapiHttpTransport::create(kServerIp, static_cast<uint16_t>(with_protobuf_ ? 8028 : 8018), false, encoding);
    // transport->SetSequenceNumberValidation(true);
  }

  return transport;
}

void FunapiTest::OnSessionInitiated(const std::string &session_id)
{
  fun::DebugUtils::Log("session initiated: %s", session_id.c_str());
}

void FunapiTest::OnSessionClosed()
{
  fun::DebugUtils::Log("session closed");

  network_ = nullptr;
  multicast_ = nullptr;
}

void FunapiTest::OnEchoJson(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  std::string body(v_body.begin(), v_body.end());

  fun::DebugUtils::Log("msg '%s' arrived.", type.c_str());
  fun::DebugUtils::Log("json: %s", body.c_str());
}

void FunapiTest::OnEchoProto(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  fun::DebugUtils::Log("msg '%s' arrived.", type.c_str());

  FunMessage msg;
  msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));
  PbufEchoMessage echo = msg.GetExtension(pbuf_echo);
  fun::DebugUtils::Log("proto: %s", echo.msg().c_str());
}

void FunapiTest::OnMaintenanceMessage(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body)
{
  fun::DebugUtils::Log("OnMaintenanceMessage");

  fun::FunEncoding encoding = with_protobuf_ ? fun::FunEncoding::kProtobuf : fun::FunEncoding::kJson;

  if (encoding == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("Maintenance message\n%s", body.c_str());
  }

  if (encoding == fun::FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    MaintenanceMessage maintenance = msg.GetExtension(pbuf_maintenance);
    std::string date_start = maintenance.date_start();
    std::string date_end = maintenance.date_end();
    std::string message = maintenance.messages();

    fun::DebugUtils::Log("Maintenance message\nstart: %s\nend: %s\nmessage: %s", date_start.c_str(), date_end.c_str(), message.c_str());
  }
}

void FunapiTest::OnStoppedAllTransport()
{
  fun::DebugUtils::Log("OnStoppedAllTransport called.");
}

void FunapiTest::OnTransportConnectFailed (const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectFailed(%d)", (int)protocol);
}

void FunapiTest::OnTransportConnectTimeout (const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportConnectTimeout called.");
}


void FunapiTest::OnTransportStarted (const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportStarted called.");
}


void FunapiTest::OnTransportClosed (const fun::TransportProtocol protocol)
{
  fun::DebugUtils::Log("OnTransportClosed called.");
}


void FunapiTest::CreateMulticast()
{
  fun::DebugUtils::Log("CreateMulticast");

  if (multicast_) {
    return;
  }

  if (network_) {
    auto transport = network_->GetTransport(fun::TransportProtocol::kTcp);
    if (transport) {
      multicast_ = std::make_shared<fun::FunapiMulticastClient>(network_, transport->GetEncoding());

      std::stringstream ss_temp;
      std::random_device rd;
      std::default_random_engine re(rd());
      std::uniform_int_distribution<int> dist(1,100);
      ss_temp << "player" << dist(re);
      std::string sender = ss_temp.str();

      fun::DebugUtils::Log("sender = %s", sender.c_str());

      multicast_encoding_ = transport->GetEncoding();

      multicast_->SetSender(sender);
      // multicast_->SetEncoding(multicast_encoding_);

      multicast_->AddJoinedCallback([](const std::string &channel_id, const std::string &sender) {
        fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
      });
      multicast_->AddLeftCallback([](const std::string &channel_id, const std::string &sender) {
        fun::DebugUtils::Log("LeftCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
      });
      multicast_->AddErrorCallback([](int error){
        // EC_ALREADY_JOINED = 1,
        // EC_ALREADY_LEFT,
        // EC_FULL_MEMBER
      });

      return;
    }
  }

  fun::DebugUtils::Log("You should connect to tcp transport first.");
}

void FunapiTest::JoinMulticastChannel()
{
  fun::DebugUtils::Log("JoinMulticastChannel");
  if (multicast_) {
    if (multicast_->IsConnected() && !multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->JoinChannel(kMulticastTestChannel, [this](const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body) {
        OnMulticastChannelSignalle(channel_id, sender, v_body);
      });
    }
  }
}

void FunapiTest::SendMulticastMessage()
{
  fun::DebugUtils::Log("SendMulticastMessage");
  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      if (multicast_encoding_ == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();

        rapidjson::Value channel_id_node(kMulticastTestChannel.c_str(), msg.GetAllocator());
        msg.AddMember(rapidjson::StringRef("_channel"), channel_id_node, msg.GetAllocator());

        rapidjson::Value bounce_node(true);
        msg.AddMember(rapidjson::StringRef("_bounce"), bounce_node, msg.GetAllocator());

        std::string temp_messsage = "multicast test message";
        rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
        msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        multicast_->SendToChannel(json_string);
      }

      if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {
        FunMessage msg;

        FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
        mcast_msg->set_channel(kMulticastTestChannel.c_str());
        mcast_msg->set_bounce(true);

        FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
        chat_msg->set_text("multicast test message");

        multicast_->SendToChannel(msg);
      }
    }
  }
}

void FunapiTest::LeaveMulticastChannel()
{
  fun::DebugUtils::Log("LeaveMulticastChannel");
  if (multicast_) {
    if (multicast_->IsConnected() && multicast_->IsInChannel(kMulticastTestChannel)) {
      multicast_->LeaveChannel(kMulticastTestChannel);
      multicast_ = nullptr;
    }
  }
}

void FunapiTest::OnMulticastChannelSignalle(const std::string &channel_id, const std::string &sender, const std::vector<uint8_t> &v_body)
{
  if (multicast_encoding_ == fun::FunEncoding::kJson) {
    std::string body(v_body.cbegin(), v_body.cend());
    fun::DebugUtils::Log("channel_id=%s, sender=%s, body=%s", channel_id.c_str(), sender.c_str(), body.c_str());
  }

  if (multicast_encoding_ == fun::FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
    FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
    std::string message = chat_msg->text();

    fun::DebugUtils::Log("channel_id=%s, sender=%s, message=%s", channel_id.c_str(), sender.c_str(), message.c_str());
  }
}

static bool g_bTestRunning = true;

void test_echo(const int index, std::string server_ip,
               const int server_port,
               fun::TransportProtocol protocol,
               fun::FunEncoding encoding,
               bool use_session_reliability) {
  std::string temp_session_id("");
  bool is_ok = true;

  std::shared_ptr<fun::FunapiNetwork> network = std::make_shared<fun::FunapiNetwork>(use_session_reliability);

  network->AddSessionInitiatedCallback([encoding, network,index, &temp_session_id](const std::string &session_id) {
    temp_session_id = session_id;
    printf("(%d) init session id = %s\n", index, session_id.c_str());

    // // // //
    // wait
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // // // //
    // send
    std::stringstream ss_temp;
    ss_temp << static_cast<int>(0);
    std::string temp_string = ss_temp.str();

    if (encoding == fun::FunEncoding::kJson) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      network->SendMessage("echo", json_string);
    }
    else if (encoding == fun::FunEncoding::kProtobuf) {
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(temp_string.c_str());
      network->SendMessage(msg);
    }
  });

  network->AddSessionClosedCallback([index, &is_ok, &temp_session_id]() {
    printf("(%d) close session id = %s\n", index, temp_session_id.c_str());
    is_ok = false;
  });

  network->AddStoppedAllTransportCallback([index, &is_ok]() {
    printf("(%d) stop transport\n", index);
    is_ok = false;
  });

  network->AddTransportConnectFailedCallback([index, &is_ok](const fun::TransportProtocol p) {
    printf("(%d) connect failed\n", index);
    is_ok = false;
  });

  network->AddTransportConnectTimeoutCallback([index, &is_ok](const fun::TransportProtocol p) {
    printf("(%d) connect timeout\n", index);
    is_ok = false;
  });

  network->AddTransportStartedCallback([index](const fun::TransportProtocol p){
    printf("(%d) transport started\n", index);
  });

  network->AddTransportClosedCallback([index](const fun::TransportProtocol p){
    printf("(%d) transport closed\n", index);
  });

  network->RegisterHandler("echo", [index, network](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) {
    std::string body(v_body.cbegin(), v_body.cend());
    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(body.c_str());

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

    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
    msg.AddMember("message", message_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network->SendMessage("echo", json_string);
  });

  network->RegisterHandler("pbuf_echo", [index, network](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) {
    FunMessage msg_recv;
    msg_recv.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));
    PbufEchoMessage echo_recv = msg_recv.GetExtension(pbuf_echo);

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

    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(temp_string.c_str());
    network->SendMessage(msg);
  });

  std::shared_ptr<fun::FunapiTransport> transport;
  if (protocol == fun::TransportProtocol::kTcp) {
    transport = fun::FunapiTcpTransport::create(server_ip, static_cast<uint16_t>(server_port), encoding);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    transport = fun::FunapiUdpTransport::create(server_ip, static_cast<uint16_t>(server_port), encoding);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    transport = fun::FunapiHttpTransport::create(server_ip, static_cast<uint16_t>(server_port), false, encoding);
  }

  network->AttachTransport(transport);
  network->Start();

  while (is_ok && g_bTestRunning) {
    network->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  network->Stop();
}


void test_session_reliability(const int index, std::string server_ip,
               const int server_port,
               fun::TransportProtocol protocol,
               fun::FunEncoding encoding,
               bool use_session_reliability) {
  std::string temp_session_id("");
  bool is_ok = true;

  std::shared_ptr<fun::FunapiNetwork> network = std::make_shared<fun::FunapiNetwork>(use_session_reliability);

  int packet_no = 0;
  std::function<void(int)> send_echo = [network, &packet_no, encoding, index](int count) {
    for (int i=0;i<count;++i) {
      ++packet_no;
      std::stringstream ss_temp;
      ss_temp << "(" << index << ")" << static_cast<int>(packet_no);
      std::string temp_string = ss_temp.str();

      if (encoding == fun::FunEncoding::kJson) {
        rapidjson::Document msg;
        msg.SetObject();
        rapidjson::Value message_node(temp_string.c_str(), msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());

        // Convert JSON document to string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        msg.Accept(writer);
        std::string json_string = buffer.GetString();

        network->SendMessage("echo", json_string);
      }
      else if (encoding == fun::FunEncoding::kProtobuf) {
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(temp_string.c_str());
        network->SendMessage(msg);
      }
    }
  };

  network->AddSessionInitiatedCallback([encoding, network,index, &temp_session_id](const std::string &session_id) {
    temp_session_id = session_id;
    printf("(%d) init session id = %s\n", index, session_id.c_str());
  });

  network->AddSessionClosedCallback([index, &is_ok, &temp_session_id]() {
    printf("(%d) close session id = %s\n", index, temp_session_id.c_str());
    // is_ok = false;
  });

  network->AddStoppedAllTransportCallback([index, &is_ok]() {
    printf("(%d) stop transport\n", index);
    // is_ok = false;
  });

  network->AddTransportConnectFailedCallback([index, &is_ok](const fun::TransportProtocol p) {
    printf("(%d) connect failed\n", index);
    // is_ok = false;
  });

  network->AddTransportConnectTimeoutCallback([index, &is_ok](const fun::TransportProtocol p) {
    printf("(%d) connect timeout\n", index);
    // is_ok = false;
  });

  network->AddTransportStartedCallback([index](const fun::TransportProtocol p){
    printf("(%d) transport started\n", index);
  });

  network->AddTransportClosedCallback([index](const fun::TransportProtocol p){
    printf("(%d) transport closed\n", index);
  });

  network->RegisterHandler("echo", [index, network](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) {
    std::string body(v_body.cbegin(), v_body.cend());
    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(body.c_str());

    if (msg_recv.HasMember("message")) {
      printf("(%d) echo - %s\n", index, msg_recv["message"].GetString());
    }
  });

  network->RegisterHandler("pbuf_echo", [index, network](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) {
    FunMessage msg_recv;
    msg_recv.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));
    PbufEchoMessage echo_recv = msg_recv.GetExtension(pbuf_echo);

    printf("(%d) echo - %s\n", index, echo_recv.msg().c_str());
  });

  std::shared_ptr<fun::FunapiTransport> transport;
  if (protocol == fun::TransportProtocol::kTcp) {
    transport = fun::FunapiTcpTransport::create(server_ip, static_cast<uint16_t>(server_port), encoding);
  }
  else if (protocol == fun::TransportProtocol::kUdp) {
    transport = fun::FunapiUdpTransport::create(server_ip, static_cast<uint16_t>(server_port), encoding);
  }
  else if (protocol == fun::TransportProtocol::kHttp) {
    transport = fun::FunapiHttpTransport::create(server_ip, static_cast<uint16_t>(server_port), false, encoding);
  }

  network->AttachTransport(transport);
  network->Start();

  while (g_bTestRunning) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    network->Update();
    if (!temp_session_id.empty()) break;
  }

  send_echo(10);

  network->Stop();

  send_echo(10);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  network->Start();

  while (g_bTestRunning) {
    network->Update();
  }

  network->Stop();
}


void FunapiTest::TestFunapiNetwork(bool bStart)
{
  const int kMaxThread = 1;

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
        test_echo(i, kServerIp, server_port, protocol, encoding, with_session_reliability_);
        // test_session_reliability(i, kServerIp, server_port, protocol, encoding, with_session_reliability_);
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}
