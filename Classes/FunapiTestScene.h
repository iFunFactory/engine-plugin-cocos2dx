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
  void SendEchoMessage();

  //
  bool IsConnected();

  // callback
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();
  void OnEchoJson(const std::string &type, const std::vector<uint8_t> &v_body);
  void OnEchoProto(const std::string &type, const std::vector<uint8_t> &v_body);

  void OnMaintenanceMessage(const std::string &type, const std::vector<uint8_t> &v_body);
  void OnStoppedAllTransport();

  void OnTransportConnectFailed (const fun::TransportProtocol protocol);
  void OnTransportDisconnected (const fun::TransportProtocol protocol);

  void OnTransportStarted (const fun::TransportProtocol protocol);
  void OnTransportClosed (const fun::TransportProtocol protocol);
  void OnTransportFailure (const fun::TransportProtocol protocol);
  void OnConnectTimeout (const fun::TransportProtocol protocol);

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
};

#endif // __FUNAPI_TEST_SCENE_H__
