#include "FunapiTestScene.h"
#include "ui/UIButton.h"

#include "funapi/funapi_network.h"
#include "funapi/pb/test_messages.pb.h"

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
  
  Size visibleSize = Director::getInstance()->getVisibleSize();
  Vec2 origin = Director::getInstance()->getVisibleOrigin();
  
  /////////////////////////////
  // 2. add a menu items
  
  const float gap_height = 1.0;
  const float button_width = visibleSize.width * 0.5;
  const float button_height = 30.0;
  const float center_x = origin.x + (visibleSize.width * 0.5);
  
  // label : server hostname or ip
  std::string label_string_server_hostname_or_ip = "[FunapiNetwork] - " + kServerIp;
  auto label = Label::createWithTTF(label_string_server_hostname_or_ip.c_str(), "arial.ttf", 10);
  label->setPosition(Vec2(center_x,
                          origin.y + visibleSize.height - label->getContentSize().height));
  this->addChild(label, 1);
  
  // Create the buttons
  Button* button_connect_tcp = Button::create("button.png", "buttonHighlighted.png");
  button_connect_tcp->setScale9Enabled(true);
  button_connect_tcp->setContentSize(Size(button_width, button_height));
  button_connect_tcp->setPressedActionEnabled(true);
  button_connect_tcp->setAnchorPoint(Vec2(0.5, 1.0));
  button_connect_tcp->addClickEventListener([this](Ref* sender) {
    // FUNAPI_LOG("Connect (TCP)");
    ConnectTcp();
  });
  button_connect_tcp->setTitleText("Connect (TCP)");
  button_connect_tcp->setPosition(Vec2(center_x, label->getPositionY() - label->getContentSize().height - gap_height));
  this->addChild(button_connect_tcp);

  Button* button_connect_udp = Button::create("button.png", "buttonHighlighted.png");
  button_connect_udp->setScale9Enabled(true);
  button_connect_udp->setContentSize(Size(button_width, button_height));
  button_connect_udp->setPressedActionEnabled(true);
  button_connect_udp->setAnchorPoint(Vec2(0.5, 1.0));
  button_connect_udp->addClickEventListener([this](Ref* sender) {
    // FUNAPI_LOG("Connect (UDP)");
    ConnectUdp();
  });
  button_connect_udp->setTitleText("Connect (UDP)");
  button_connect_udp->setPosition(Vec2(center_x, button_connect_tcp->getPositionY() - button_connect_tcp->getContentSize().height - gap_height));
  this->addChild(button_connect_udp);

  Button* button_connect_http = Button::create("button.png", "buttonHighlighted.png");
  button_connect_http->setScale9Enabled(true);
  button_connect_http->setContentSize(Size(button_width, button_height));
  button_connect_http->setPressedActionEnabled(true);
  button_connect_http->setAnchorPoint(Vec2(0.5, 1.0));
  button_connect_http->addClickEventListener([this](Ref* sender) {
    // FUNAPI_LOG("Connect (HTTP)");
    ConnectHttp();
  });
  button_connect_http->setTitleText("Connect (HTTP)");
  button_connect_http->setPosition(Vec2(center_x, button_connect_udp->getPositionY() - button_connect_udp->getContentSize().height - gap_height));
  this->addChild(button_connect_http);

  Button* button_disconnect = Button::create("button.png", "buttonHighlighted.png");
  button_disconnect->setScale9Enabled(true);
  button_disconnect->setContentSize(Size(button_width, button_height));
  button_disconnect->setPressedActionEnabled(true);
  button_disconnect->setAnchorPoint(Vec2(0.5, 1.0));
  button_disconnect->addClickEventListener([this](Ref* sender) {
    // FUNAPI_LOG("Disconnect");
    Disconnect();
  });
  button_disconnect->setTitleText("Disconnect");
  button_disconnect->setPosition(Vec2(center_x, button_connect_http->getPositionY() - button_connect_http->getContentSize().height - gap_height));
  this->addChild(button_disconnect);

  Button* button_send_a_message = Button::create("button.png", "buttonHighlighted.png");
  button_send_a_message->setScale9Enabled(true);
  button_send_a_message->setContentSize(Size(button_width, button_height));
  button_send_a_message->setPressedActionEnabled(true);
  button_send_a_message->setAnchorPoint(Vec2(0.5, 1.0));
  button_send_a_message->addClickEventListener([this](Ref* sender) {
    // FUNAPI_LOG("Send a message");
    SendEchoMessage();
  });
  button_send_a_message->setTitleText("Send a message");
  button_send_a_message->setPosition(Vec2(center_x, button_disconnect->getPositionY() - button_disconnect->getContentSize().height - gap_height));
  this->addChild(button_send_a_message);
  
  return true;
}

void FunapiTest::cleanup()
{
  // FUNAPI_LOG("cleanup");

  if (network_)
  {
    network_->Stop();
  }

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
  std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
}

void FunapiTest::update(float delta)
{
  // FUNAPI_LOG("delta = %f", delta);
  
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
  return network_ != nullptr && network_->Connected();
}

void FunapiTest::Disconnect()
{
  if (network_ == nullptr || network_->Started() == false)
  {
    FUNAPI_LOG("You should connect first.");
    return;
  }
  
  network_->Stop();
}

bool FunapiTest::SendEchoMessage()
{
  if (network_ == NULL || network_->Started() == false)
  {
    FUNAPI_LOG("You should connect first.");
    return false;
  }
  
  if (msg_type_ == fun::kJsonEncoding)
  {
    fun::Json msg;
    msg.SetObject();
    rapidjson::Value message_node("hello world", msg.GetAllocator());
    msg.AddMember("message", message_node, msg.GetAllocator());
    network_->SendMessage("echo", msg, protocol_);
    return true;
  }
  else if (msg_type_ == fun::kProtobufEncoding)
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg("hello proto");
    network_->SendMessage(msg, protocol_);
    return true;
  }
  
  return false;
}

void FunapiTest::Connect(const fun::TransportProtocol protocol)
{
  if (!network_) {
    network_ = std::make_shared<fun::FunapiNetwork>(msg_type_,
                                                    [this](const std::string &session_id){ OnSessionInitiated(session_id); },
                                                    [this]{ OnSessionClosed(); });
    
    network_->RegisterHandler("echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoJson(type, v_body); });
    network_->RegisterHandler("pbuf_echo", [this](const std::string &type, const std::vector<uint8_t> &v_body){ OnEchoProto(type, v_body); });
    
    network_->AttachTransport(GetNewTransport(protocol));
    network_->Start();
  }
  else {
    network_->AttachTransport(GetNewTransport(protocol));
    network_->Start();
  }
  
  protocol_ = protocol;
}

std::shared_ptr<fun::FunapiTransport> FunapiTest::GetNewTransport(fun::TransportProtocol protocol)
{
  std::shared_ptr<fun::FunapiTransport> transport;
  
  if (protocol == fun::TransportProtocol::kTcp)
    transport = std::make_shared<fun::FunapiTcpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8022 : 8012));
  else if (protocol == fun::TransportProtocol::kUdp)
    transport = std::make_shared<fun::FunapiUdpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8023 : 8013));
  else if (protocol == fun::TransportProtocol::kHttp)
    transport = std::make_shared<fun::FunapiHttpTransport>(kServerIp, static_cast<uint16_t>(msg_type_ == fun::kProtobufEncoding ? 8028 : 8018), false);
  
  return transport;
}

void FunapiTest::OnSessionInitiated(const std::string &session_id)
{
  FUNAPI_LOG("session initiated: %s", session_id.c_str());
}

void FunapiTest::OnSessionClosed()
{
  FUNAPI_LOG("session closed");
}

void FunapiTest::OnEchoJson(const std::string &type, const std::vector<uint8_t> &v_body)
{
  std::string body(v_body.begin(), v_body.end());

  FUNAPI_LOG("msg '%s' arrived.", type.c_str());
  FUNAPI_LOG("json: %s", body.c_str());
}

void FunapiTest::OnEchoProto(const std::string &type, const std::vector<uint8_t> &v_body)
{
  FUNAPI_LOG("msg '%s' arrived.", type.c_str());

  std::string body(v_body.begin(), v_body.end());

  FunMessage msg;
  msg.ParseFromString(body);
  PbufEchoMessage echo = msg.GetExtension(pbuf_echo);
  FUNAPI_LOG("proto: %s", echo.msg().c_str());
}
