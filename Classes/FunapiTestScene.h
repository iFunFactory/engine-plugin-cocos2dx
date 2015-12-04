#ifndef __FUNAPI_TEST_SCENE_H__
#define __FUNAPI_TEST_SCENE_H__

#include "cocos2d.h"
#include "funapi/funapi_network.h"

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
  bool SendEchoMessage();
  
  //
  bool IsConnected();
  
  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();
  void OnEchoJson(const std::string &type, const std::vector<uint8_t> &v_body);
  void OnEchoProto(const std::string &type, const std::vector<uint8_t> &v_body);
  
  // implement the "static create()" method manually
  CREATE_FUNC(FunapiTest);
  
private:
  void Connect(const fun::TransportProtocol protocol);
  std::shared_ptr<fun::FunapiTransport> GetNewTransport(fun::TransportProtocol protocol);
  
  const std::string kServerIp = "127.0.0.1";
  std::shared_ptr<fun::FunapiNetwork> network_;
  int8_t msg_type_ = fun::kJsonEncoding;
  fun::TransportProtocol protocol_ = fun::TransportProtocol::kDefault;
};

#endif // __FUNAPI_TEST_SCENE_H__
