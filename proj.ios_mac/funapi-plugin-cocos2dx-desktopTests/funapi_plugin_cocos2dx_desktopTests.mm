// Copyright (C) 2013-2019 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_session.h"
#include "funapi_multicasting.h"
#include "funapi_socket.h"
#include "funapi_downloader.h"
#include "funapi_announcement.h"
#include "funapi_tasks.h"

#include <iostream>
#include "test_messages.pb.h"

#import <XCTest/XCTest.h>


// 테스트들은 아래 흐름을 가진다.
// 1. 필요한 객체들 생성 및 확인
// 2. 등록된 콜백으로 테스트의 흐름을 작성
// 3. XCTAssert 조건에 부합하지 않다면 실패 및 테스트 종료.

static const std::string g_server_ip = "plugin-docker.ifunfactory.com";

@interface funapi_plugin_cocos2dx_desktopTests : XCTestCase

@end

@implementation funapi_plugin_cocos2dx_desktopTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    self.continueAfterFailure = NO;
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}


// Create Session, Send Recv Test
- (void)testFunapiSession_Json {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &is_ok, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage("echo", json_string);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    XCTAssert(msg_type.compare("echo") == 0);

    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(json_string.c_str());

    XCTAssert(msg_recv.HasMember("message"));

    std::string recv_string = msg_recv["message"].GetString();

    XCTAssert(send_string.compare(recv_string) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10201, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_Protobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());

      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage(msg);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

    PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

    XCTAssert(send_string.compare(echo.msg()) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10204, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_ReliabilityJson {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;
  bool with_session_reliability = true;

  session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage("echo", json_string);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    XCTAssert(msg_type.compare("echo") == 0);

    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(json_string.c_str());

    XCTAssert(msg_recv.HasMember("message"));

    std::string recv_string = msg_recv["message"].GetString();

    XCTAssert(send_string.compare(recv_string) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10231, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_ReliabilityProtobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;
  bool with_session_reliability = true;

  session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());

    for (int i = 0; i < kSendCount; ++i) {
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

    PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

    XCTAssert(send_string.compare(echo.msg()) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10234, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_ProtobufMsgtypeInt {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      FunMessage msg;
      msg.set_msgtype2(pbuf_echo.number());
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage(msg);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    XCTAssert(fun_message.has_msgtype2());
    XCTAssert(fun_message.msgtype2() == pbuf_echo.number());

    PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

    XCTAssert(send_string.compare(echo.msg()) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10204, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_AutoReconnect {
  // std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kChanged);

  });

  session->AddTransportEventCallback(
  [self, &is_ok, &is_working]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    static int reconnect_try_count = 0;

    if (type == fun::TransportEventType::kReconnecting) {
      ++reconnect_try_count;
      // 최대 재시도 횟수의 기본값은 4 회로 4회 호출시 성공으로 간주.
      if (reconnect_try_count > 3) {
        is_ok = true;
        is_working = false;
      }
    }

    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kStarted);

  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetAutoReconnect(true);

  // 열려있지 않은 포트로 접속을 시도해 ConnectFail 이벤트 발생시켜 AutoReonnect 가 동작하게함.
  session->Connect(fun::TransportProtocol::kTcp, 80, fun::FunEncoding::kJson, option);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_JsonMultitransport {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;

  // 멀티 Transport 로 분리하여 계산하기 위해 콜백 안에서 선언 및 계산.
  // int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    static int tcp_recv_count = 0;
    static int udp_recv_count = 0;
    static int http_recv_count = 0;
    static int websocket_recv_count = 0;

    switch (protocol)
    {
      case fun::TransportProtocol::kTcp: {
        XCTAssert(msg_type.compare("echo") == 0);

        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();

        XCTAssert(send_string.compare(recv_string) == 0);

        ++tcp_recv_count;
        break;
      }
      case fun::TransportProtocol::kUdp: {
        XCTAssert(msg_type.compare("echo") == 0);

        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();

        XCTAssert(send_string.compare(recv_string) == 0);

        ++udp_recv_count;
        break;
      }
      case fun::TransportProtocol::kHttp: {
        XCTAssert(msg_type.compare("echo") == 0);

        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();

        XCTAssert(send_string.compare(recv_string) == 0);

        ++http_recv_count;
        break;
      }
      case fun::TransportProtocol::kWebsocket: {
        XCTAssert(msg_type.compare("echo") == 0);

        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();

        XCTAssert(send_string.compare(recv_string) == 0);

        ++websocket_recv_count;
        break;
      }
      default: {
        XCTAssert(false);
        break;
      }
    }

    if (tcp_recv_count == kSendCount &&
        udp_recv_count >= 1 &&
        http_recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10201, fun::FunEncoding::kJson);
  session->Connect(fun::TransportProtocol::kUdp, 11202, fun::FunEncoding::kJson);
  session->Connect(fun::TransportProtocol::kHttp, 10202, fun::FunEncoding::kJson);
  session->Connect(fun::TransportProtocol::kWebsocket, 10203, fun::FunEncoding::kJson);

  // send
  {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
    msg.AddMember("message", message_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    for (int i = 0; i < kSendCount; ++i) {
      session->SendMessage("echo", json_string, fun::TransportProtocol::kTcp);
      session->SendMessage("echo", json_string, fun::TransportProtocol::kUdp);
      session->SendMessage("echo", json_string, fun::TransportProtocol::kHttp);
      session->SendMessage("echo", json_string, fun::TransportProtocol::kWebsocket);
    }
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_ProtobufMultitransport {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;

  // 멀티 Transport 로 분리하여 계산하기 위해 콜백 안에서 선언 및 계산.
  // int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const FunMessage &fun_message)
  {
    static int tcp_recv_count = 0;
    static int udp_recv_count = 0;
    static int http_recv_count = 0;
    static int websocket_recv_count = 0;

    switch (protocol)
    {
    case fun::TransportProtocol::kTcp: {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++tcp_recv_count;
      break;
    }
    case fun::TransportProtocol::kUdp: {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++udp_recv_count;
      break;
    }
    case fun::TransportProtocol::kHttp: {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++http_recv_count;
      break;
    }
    case fun::TransportProtocol::kWebsocket: {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++websocket_recv_count;
      break;
    }
    default: {
      XCTAssert(false);
      break;
    }
    }

    if (tcp_recv_count == kSendCount &&
        udp_recv_count >= 1 &&
        http_recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 10204, fun::FunEncoding::kProtobuf);
  session->Connect(fun::TransportProtocol::kUdp, 11203, fun::FunEncoding::kProtobuf);
  session->Connect(fun::TransportProtocol::kHttp, 10205, fun::FunEncoding::kProtobuf);
  session->Connect(fun::TransportProtocol::kWebsocket, 10206, fun::FunEncoding::kProtobuf);

  // send
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());

    for (int i = 0; i < kSendCount; ++i) {
      session->SendMessage(msg, fun::TransportProtocol::kTcp);
      session->SendMessage(msg, fun::TransportProtocol::kUdp);
      session->SendMessage(msg, fun::TransportProtocol::kHttp);
      session->SendMessage(msg, fun::TransportProtocol::kWebsocket);
    }
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_EncJson {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 5;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &is_ok, &send_string]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      s->SendMessage("echo", json_string, protocol, fun::EncryptionType::kAes128Encryption);
      s->SendMessage("echo", json_string, protocol, fun::EncryptionType::kChacha20Encryption);
      s->SendMessage("echo", json_string, protocol, fun::EncryptionType::kIFunEngine1Encryption);
      s->SendMessage("echo", json_string, protocol, fun::EncryptionType::kIFunEngine1Encryption);
      s->SendMessage("echo", json_string, protocol, fun::EncryptionType::kDummyEncryption);
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    XCTAssert(msg_type.compare("echo") == 0);

    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(json_string.c_str());

    XCTAssert(msg_recv.HasMember("message"));

    std::string recv_string = msg_recv["message"].GetString();

    XCTAssert(send_string.compare(recv_string) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
    "3ad8edc23e6e6b7544a64e54afbb383b06caa7dd7b0aaccaa678ead0e0aa6831");
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
    "3ad8edc23e6e6b7544a64e54afbb383b06caa7dd7b0aaccaa678ead0e0aa6831");

  session->Connect(fun::TransportProtocol::kTcp, 10301, fun::FunEncoding::kJson, option);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_EncProtobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 5;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &send_string]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());

      s->SendMessage(msg, protocol, fun::EncryptionType::kAes128Encryption);
      s->SendMessage(msg, protocol, fun::EncryptionType::kChacha20Encryption);
      s->SendMessage(msg, protocol, fun::EncryptionType::kIFunEngine1Encryption);
      s->SendMessage(msg, protocol, fun::EncryptionType::kIFunEngine2Encryption);
      s->SendMessage(msg, protocol, fun::EncryptionType::kDummyEncryption);
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

    PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

    XCTAssert(send_string.compare(echo.msg()) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
    "a1f38ae4910dd67ff915e719a86529dce8a2e4efe3d9a42e5af9395422691c0e");
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
    "a1f38ae4910dd67ff915e719a86529dce8a2e4efe3d9a42e5af9395422691c0e");

  session->Connect(fun::TransportProtocol::kTcp, 10304, fun::FunEncoding::kProtobuf, option);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_CompressDeflateJson {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &is_ok, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();
      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage("echo", json_string);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    XCTAssert(msg_type.compare("echo") == 0);

    rapidjson::Document msg_recv;
    msg_recv.Parse<0>(json_string.c_str());

    XCTAssert(msg_recv.HasMember("message"));

    std::string recv_string = msg_recv["message"].GetString();

    XCTAssert(send_string.compare(recv_string) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetCompressionType(fun::CompressionType::kDeflate);
  session->Connect(fun::TransportProtocol::kTcp, 10101, fun::FunEncoding::kJson, option);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_CompressZstdProtobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiSession> session = nullptr;

  const int kSendCount = 50;
  int recv_count = 0;

  session = fun::FunapiSession::Create(server_ip.c_str(), false);
  XCTAssert(session);

  session->AddSessionEventCallback
  ([self, &send_string, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
    XCTAssert(type != fun::SessionEventType::kRedirectStarted);
    XCTAssert(type != fun::SessionEventType::kRedirectFailed);
    XCTAssert(type != fun::SessionEventType::kChanged);

    if (type == fun::SessionEventType::kOpened) {
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());

      for (int i = 0; i < kSendCount; ++i) {
        s->SendMessage(msg);
      }
    }
  });

  session->AddTransportEventCallback([self]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    XCTAssert(type != fun::TransportEventType::kDisconnected);
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
    XCTAssert(type != fun::TransportEventType::kReconnecting);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
  (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

    PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

    XCTAssert(send_string.compare(echo.msg()) == 0);

    ++recv_count;
    if (recv_count == kSendCount) {
      is_ok = true;
      is_working = false;
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetCompressionType(fun::CompressionType::kZstd);
  session->Connect(fun::TransportProtocol::kTcp, 10104, fun::FunEncoding::kProtobuf, option);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}


- (void)testFunapiSession_SessionMultithread {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;

  const int kMaxThread = 2;
  const int kSendCount = 50;
  std::vector<std::thread> temp_thread(kMaxThread);
  std::vector<bool> v_completed(kMaxThread);
  std::mutex complete_mutex;

  auto test_funapi_session =
    [self, &send_string, kSendCount, &complete_mutex, &v_completed]
  (const int index,
    const std::string &server_ip,
    const int server_port,
    const fun::TransportProtocol protocol,
    const fun::FunEncoding encoding,
    const bool use_session_reliability)
  {

    auto session = fun::FunapiSession::Create(server_ip.c_str(), use_session_reliability);
    XCTAssert(session);

    bool is_ok = false;
    bool is_working = true;

    int recv_count = 0;

    session->AddSessionEventCallback
    ([self, index, &send_string]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(send_string.c_str());

        s->SendMessage(msg);
      }
    });

    session->AddTransportEventCallback
    ([self, index, &is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    session->AddProtobufRecvCallback
    ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const FunMessage &fun_message)
    {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++recv_count;
      if (recv_count == kSendCount) {
        is_working = false;
        is_ok = true;
        return;
      }

      s->SendMessage(fun_message);
    });

    auto option = fun::FunapiTcpTransportOption::Create();
    option->SetEnablePing(true);
    option->SetDisableNagle(true);
    session->Connect(fun::TransportProtocol::kTcp, server_port, encoding, option);

    while (is_working) {
      session->Update();
      std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
    }

    session->Close();

    XCTAssert(is_ok);

    {
      std::unique_lock<std::mutex> lock(complete_mutex);
      v_completed[index] = true;
    }
  };

  for (int i = 0; i < kMaxThread; ++i) {
    fun::TransportProtocol protocol = fun::TransportProtocol::kTcp;
    fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
    std::string server_ip = g_server_ip;
    int server_port = 10204;
    bool with_session_reliability = false;

    v_completed[i] = false;

    temp_thread[i] =
      std::thread
      ([&test_funapi_session, i, server_ip, server_port, protocol, encoding, with_session_reliability]()
    {
      test_funapi_session(i, server_ip, server_port, protocol, encoding, with_session_reliability);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    {
      std::unique_lock<std::mutex> lock(complete_mutex);
      bool complete = true;
      for (int i = 0; i < kMaxThread; ++i) {
        if (v_completed[i] == false) {
          complete = false;
          break;
        }
      }

      if (complete) {
        break;
      }
    }
  }

  for (int i = 0; i < kMaxThread; ++i) {
    if (temp_thread[i].joinable()) {
      temp_thread[i].join();
    }
  }
}


- (void)testFunapiSession_SessionMultithread_UpdateAll {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;

  const int kMaxThread = 2;
  const int kSendCount = 50;
  std::vector<std::thread> temp_thread(kMaxThread);
  std::vector<bool> v_completed(kMaxThread);
  std::mutex complete_mutex;

  auto test_funapi_session =
    [self, &send_string, kSendCount, &complete_mutex, &v_completed]
  (const int index,
    const std::string &server_ip,
    const int server_port,
    const fun::TransportProtocol protocol,
    const fun::FunEncoding encoding,
    const bool use_session_reliability)
  {

    auto session = fun::FunapiSession::Create(server_ip.c_str(), use_session_reliability);
    XCTAssert(session);

    bool is_ok = false;
    bool is_working = true;

    int recv_count = 0;

    session->AddSessionEventCallback
    ([self, index, &send_string]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        FunMessage msg;
        msg.set_msgtype("pbuf_echo");
        PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
        echo->set_msg(send_string.c_str());

        s->SendMessage(msg);
      }
    });

    session->AddTransportEventCallback
    ([self, index, &is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    session->AddProtobufRecvCallback
    ([self, &is_working, &is_ok, &send_string, &recv_count, kSendCount]
    (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const FunMessage &fun_message)
    {
      XCTAssert(fun_message.msgtype().compare("pbuf_echo") == 0);

      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      XCTAssert(send_string.compare(echo.msg()) == 0);

      ++recv_count;
      if (recv_count == kSendCount) {
        is_working = false;
        is_ok = true;
      }

      s->SendMessage(fun_message);
    });

    auto option = fun::FunapiTcpTransportOption::Create();
    option->SetEnablePing(true);
    option->SetDisableNagle(true);
    session->Connect(fun::TransportProtocol::kTcp, server_port, encoding, option);

    while (is_working) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
    }

    session->Close();

    XCTAssert(is_ok);

    {
      std::unique_lock<std::mutex> lock(complete_mutex);
      v_completed[index] = true;
    }
  };

  for (int i = 0; i < kMaxThread; ++i) {
    fun::TransportProtocol protocol = fun::TransportProtocol::kTcp;
    fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
    std::string server_ip = g_server_ip;
    int server_port = 10204;
    bool with_session_reliability = false;

    v_completed[i] = false;

    temp_thread[i] =
      std::thread
      ([&test_funapi_session, i, server_ip, server_port, protocol, encoding, with_session_reliability]()
    {
      test_funapi_session(i, server_ip, server_port, protocol, encoding, with_session_reliability);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  while (true) {
    fun::FunapiSession::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    {
      std::unique_lock<std::mutex> lock(complete_mutex);
      bool complete = true;
      for (int i = 0; i < kMaxThread; ++i) {
        if (v_completed[i] == false) {
          complete = false;
          break;
        }
      }

      if (complete) {
        break;
      }
    }
  }

  for (int i = 0; i < kMaxThread; ++i) {
    if (temp_thread[i].joinable()) {
      temp_thread[i].join();
    }
  }
}



- (void)testFunapiMulticast_MulticastJson {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = false;
  bool is_working = true;

  const int kUserCount = 10;
  const int kSendCount = 50;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 10401;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;


  auto send_function =
    [](const std::shared_ptr<fun::FunapiMulticast>& m,
       const std::string &channel_id,
       int number)
  {
    rapidjson::Document msg;
    msg.SetObject();

    std::stringstream ss;
    ss << number;

    std::string temp_messsage = ss.str();
    rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    m->SendToChannel(channel_id, json_string);
  };

  for (int i = 0; i < kUserCount; ++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
      fun::FunapiMulticast::Create(user_name.c_str(),
                                   server_ip.c_str(),
                                   port,
                                   encoding,
                                   false);
    XCTAssert(funapi_multicast);

    funapi_multicast->AddJoinedCallback
    ([self, user_name, multicast_test_channel, &send_function]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      XCTAssert(channel_id == multicast_test_channel);
      if (sender == user_name) {
        send_function(m, channel_id, 0);
      }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback([self]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      XCTAssert(false);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        m->JoinChannel(multicast_test_channel);
      }
    });

    funapi_multicast->AddTransportEventCallback
    ([self, is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    funapi_multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
      [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &kSendCount]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender,
      const std::string &json_string)
    {
      if (sender == user_name) {
        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();
        int number = atoi(recv_string.c_str());

        if (number >= kSendCount) {
          is_ok = true;
          is_working = false;
        }
        else {
          send_function(m, channel_id, number+1);
        }
      }
    });

    funapi_multicast->Connect();

    v_multicast.push_back(funapi_multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


- (void)testFunapiMulticast_MulticastProtobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = false;
  bool is_working = true;

  const int kUserCount = 10;
  const int kSendCount = 50;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 10404;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;


  auto send_function =
    [](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      int number)
  {
    std::stringstream ss;
    ss << number;

    FunMessage msg;
    FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
    FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
    chat_msg->set_text(ss.str());

    m->SendToChannel(channel_id, msg, true);
  };

  for (int i = 0; i < kUserCount; ++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
      fun::FunapiMulticast::Create(user_name.c_str(),
        server_ip.c_str(),
        port,
        encoding,
        false);
    XCTAssert(funapi_multicast);

    funapi_multicast->AddJoinedCallback
    ([self, user_name, multicast_test_channel, &send_function]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      XCTAssert(channel_id == multicast_test_channel);
      if (sender == user_name) {
        send_function(m, channel_id, 0);
      }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback([self]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      XCTAssert(false);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        m->JoinChannel(multicast_test_channel);
      }
    });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
      [&is_ok, &is_working, multicast_test_channel, user_name, &send_function, &kSendCount]
    (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
    {
      if (sender == user_name) {
        FunMulticastMessage mcast_msg = message.GetExtension(multicast);
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

        int number = atoi(chat_msg.text().c_str());

        if (number >= kSendCount) {
          is_ok = true;
          is_working = false;
        }
        else {
          send_function(m, channel_id, number + 1);
        }
      }
    });

    funapi_multicast->Connect();

    v_multicast.push_back(funapi_multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


- (void)testFunapiMulticast_ReliabilityJson {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = false;
  bool is_working = true;
  bool with_session_reliability = true;

  const int kUserCount = 10;
  const int kSendCount = 50;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 10431;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;


  auto send_function =
    [](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      int number)
  {
    rapidjson::Document msg;
    msg.SetObject();

    std::stringstream ss;
    ss << number;

    std::string temp_messsage = ss.str();
    rapidjson::Value message_node(temp_messsage.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef("message"), message_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    m->SendToChannel(channel_id, json_string);
  };

  for (int i = 0; i < kUserCount; ++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
      fun::FunapiMulticast::Create(user_name.c_str(),
        server_ip.c_str(),
        port,
        encoding,
        with_session_reliability);
    XCTAssert(funapi_multicast);

    funapi_multicast->AddJoinedCallback
    ([self, user_name, multicast_test_channel, &send_function]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      XCTAssert(channel_id == multicast_test_channel);
      if (sender == user_name) {
        send_function(m, channel_id, 0);
      }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback([self]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      XCTAssert(false);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        m->JoinChannel(multicast_test_channel);
      }
    });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    funapi_multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
      [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &kSendCount]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender,
      const std::string &json_string)
    {
      if (sender == user_name) {
        rapidjson::Document msg_recv;
        msg_recv.Parse<0>(json_string.c_str());

        XCTAssert(msg_recv.HasMember("message"));

        std::string recv_string = msg_recv["message"].GetString();
        int number = atoi(recv_string.c_str());

        if (number >= kSendCount) {
          is_ok = true;
          is_working = false;
        }
        else {
          send_function(m, channel_id, number + 1);
        }
      }
    });

    funapi_multicast->Connect();

    v_multicast.push_back(funapi_multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


- (void)testFunapiMulticast_ReliabilityProtobuf {
  std::string send_string = "Echo Message";
  std::string server_ip = g_server_ip;
  bool is_ok = false;
  bool is_working = true;
  bool with_session_reliability = false;

  const int kUserCount = 10;
  const int kSendCount = 50;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 10404;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;


  auto send_function =
    [](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      int number)
  {
    std::stringstream ss;
    ss << number;

    FunMessage msg;
    FunMulticastMessage* mcast_msg = msg.MutableExtension(multicast);
    FunChatMessage *chat_msg = mcast_msg->MutableExtension(chat);
    chat_msg->set_text(ss.str());

    m->SendToChannel(channel_id, msg, true);
  };

  for (int i = 0; i < kUserCount; ++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
      fun::FunapiMulticast::Create(user_name.c_str(),
        server_ip.c_str(),
        port,
        encoding,
        with_session_reliability);
    XCTAssert(funapi_multicast);

    funapi_multicast->AddJoinedCallback
    ([self, user_name, multicast_test_channel, &send_function]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      XCTAssert(channel_id == multicast_test_channel);
      if (sender == user_name) {
        send_function(m, channel_id, 0);
      }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback([self]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      XCTAssert(false);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
    (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::SessionEventType::kRedirectSucceeded);
      XCTAssert(type != fun::SessionEventType::kRedirectStarted);
      XCTAssert(type != fun::SessionEventType::kRedirectFailed);
      XCTAssert(type != fun::SessionEventType::kChanged);

      if (type == fun::SessionEventType::kOpened) {
        m->JoinChannel(multicast_test_channel);
      }
    });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
    (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      XCTAssert(type != fun::TransportEventType::kDisconnected);
      XCTAssert(type != fun::TransportEventType::kConnectionFailed);
      XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
      XCTAssert(type != fun::TransportEventType::kReconnecting);
    });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
      [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &kSendCount]
    (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
    {
      if (sender == user_name) {
        FunMulticastMessage mcast_msg = message.GetExtension(multicast);
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

        int number = atoi(chat_msg.text().c_str());

        if (number >= kSendCount) {
          is_ok = true;
          is_working = false;
        }
        else {
          send_function(m, channel_id, number + 1);
        }
      }
    });

    funapi_multicast->Connect();

    v_multicast.push_back(funapi_multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


- (void)testFunapipDownloader_Downloader {
  std::string kDownloadServer = g_server_ip;
  int kDownloadServerPort = 8020;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiHttpDownloader> downloader = nullptr;

  std::stringstream ss_temp;
  ss_temp << "http://" << kDownloadServer << ":" << kDownloadServerPort;
  std::string download_url = ss_temp.str();
  std::cout <<  "cocos default deirectory : " << cocos2d::FileUtils::getInstance()->getWritablePath() << std::endl;
  downloader = fun::FunapiHttpDownloader::Create(download_url, cocos2d::FileUtils::getInstance()->getWritablePath());
  XCTAssert(downloader);

  downloader->AddReadyCallback
  ([self]
  (const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
    const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info)
  {
    XCTAssert(info.size() != 0);
  });

  downloader->AddProgressCallback
  ([self]
  (const std::shared_ptr<fun::FunapiHttpDownloader> &downloader,
    const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
    const int index,
    const int max_index,
    const uint64_t received_bytes,
    const uint64_t expected_bytes)
  {
    XCTAssert(info.size() != 0);
  });

  downloader->AddCompletionCallback
  ([self, &is_working, &is_ok]
  (const std::shared_ptr<fun::FunapiHttpDownloader>&downloader,
    const std::vector<std::shared_ptr<fun::FunapiDownloadFileInfo>>&info,
    const fun::FunapiHttpDownloader::ResultCode result_code)
  {
    XCTAssert(result_code == fun::FunapiHttpDownloader::ResultCode::kSucceed);
    XCTAssert(info.size() != 0);

    is_ok = true;
    is_working = false;
  });

  downloader->Start();

  while (is_working) {
    downloader->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


- (void)testFunapiAnnounce_Announcer {
  std::string kAnnounceServer = g_server_ip;
  int kAnnounceServerPort = 8080;
  bool is_ok = true;
  bool is_working = true;
  std::shared_ptr<fun::FunapiAnnouncement> announcement = nullptr;

  std::stringstream ss_temp;
  ss_temp << "http://" << kAnnounceServer << ":" << kAnnounceServerPort;
  std::string announce_url = ss_temp.str();

  announcement = fun::FunapiAnnouncement::Create(announce_url, cocos2d::FileUtils::getInstance()->getWritablePath());
  XCTAssert(announcement);

  announcement->AddCompletionCallback
  ([self, &is_ok, &is_working]
  (const std::shared_ptr<fun::FunapiAnnouncement> &announcement,
    const fun::vector<std::shared_ptr<fun::FunapiAnnouncementInfo>>&info,
    const fun::FunapiAnnouncement::ResultCode result) {

    XCTAssert(result != fun::FunapiAnnouncement::ResultCode::kSucceed);
    XCTAssert(info.size() != 0);

    is_ok = true;
    is_working = false;
  });

  announcement->RequestList(5);

  while (is_working) {
    announcement->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}


@end
