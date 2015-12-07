#include "FunapiTestScene.h"
#include "ui/UIButton.h"

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
    CCLOG("Connect (TCP)");
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
    CCLOG("Connect (UDP)");
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
    CCLOG("Connect (HTTP)");
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
    CCLOG("Disconnect");
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
    CCLOG("Send a message");
  });
  button_send_a_message->setTitleText("Send a message");
  button_send_a_message->setPosition(Vec2(center_x, button_disconnect->getPositionY() - button_disconnect->getContentSize().height - gap_height));
  this->addChild(button_send_a_message);
  
  return true;
}
