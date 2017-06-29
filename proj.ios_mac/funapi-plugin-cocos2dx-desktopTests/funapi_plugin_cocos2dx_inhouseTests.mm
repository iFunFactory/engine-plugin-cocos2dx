// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

// http://stackoverflow.com/questions/15759559/variable-named-type-boolc-code-is-conflicted-with-ios-macro
#include <ConditionalMacros.h>
#undef TYPE_BOOL
#include "funapi_session.h"
#include "funapi_multicasting.h"
#include "funapi_tasks.h"
#include "funapi_encryption.h"

#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

#include "test_messages.pb.h"
#include "funapi/service/multicast_message.pb.h"

#import <XCTest/XCTest.h>

static const std::string g_server_ip = "127.0.0.1";

@interface funapi_plugin_cocos2dx_inhouseTests : XCTestCase

@end

@implementation funapi_plugin_cocos2dx_inhouseTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testEchoJson_reconnect_2 {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
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

      s->SendMessage("echo", json_string);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kStopped) {
      is_ok = true;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);

  session->Close();

  session->Connect(fun::TransportProtocol::kTcp);

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

    session->SendMessage("echo", json_string);
  }

  is_working = true;

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJson {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      s->SendMessage("echo", json_string);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8512, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJsonQueue {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8512, fun::FunEncoding::kJson);

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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncProtobuf {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
      }
      else {
        is_ok = false;
        XCTAssert(is_ok);
      }
    }
    else {
      is_ok = false;
      XCTAssert(is_ok);
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8522, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncProtobufQueue {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
      }
      else {
        is_ok = false;
        XCTAssert(is_ok);
      }
    }
    else {
      is_ok = false;
      XCTAssert(is_ok);
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8522, fun::FunEncoding::kProtobuf);

  // send
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());
    session->SendMessage(msg);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityJson {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      rapidjson::Document msg;
      msg.SetObject();
      rapidjson::Value message_node(send_string.c_str(), msg.GetAllocator());
      msg.AddMember("message", message_node, msg.GetAllocator());

      // Convert JSON document to string
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      msg.Accept(writer);
      std::string json_string = buffer.GetString();

      s->SendMessage("echo", json_string);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityJsonQueue {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = true;
      is_working = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityJsonQueue10Times {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      // XCTAssert(send_string.compare(recv_string) == 0);
      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

  // send
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

      session->SendMessage("echo", json_string);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityProtobuf {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
      }
      else {
        is_ok = false;
        XCTAssert(is_ok);
      }
    }
    else {
      is_ok = false;
      XCTAssert(is_ok);
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8622, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityProtobufQueue {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
      }
      else {
        is_ok = false;
        XCTAssert(is_ok);
      }
    }
    else {
      is_ok = false;
      XCTAssert(is_ok);
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8622, fun::FunEncoding::kProtobuf);

  // send
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());
    session->SendMessage(msg);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncReliabilityProtobufQueue10Times {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype("pbuf_echo");
      PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
      echo->set_msg(send_string.c_str());
      s->SendMessage(msg);
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
    else {
      is_ok = false;
      is_working = false;
      XCTAssert(is_ok);
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8622, fun::FunEncoding::kProtobuf);

  // send
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

      session->SendMessage(msg);
    }
  }
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());
    session->SendMessage(msg);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJson_ife1_ife2 {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

  // send
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

      fun::EncryptionType enc_type = fun::EncryptionType::kDefaultEncryption;
      if (i%2 == 0) enc_type = fun::EncryptionType::kIFunEngine1Encryption;
      else enc_type = fun::EncryptionType::kIFunEngine2Encryption;

      session->SendMessage("echo", json_string, fun::TransportProtocol::kTcp, enc_type);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJson_ife1_ife2_reconnect {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kStopped) {
      is_ok = true;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

  // send
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

      fun::EncryptionType enc_type = fun::EncryptionType::kDefaultEncryption;
      if (i%2 == 0) enc_type = fun::EncryptionType::kIFunEngine1Encryption;
      else enc_type = fun::EncryptionType::kIFunEngine2Encryption;

      session->SendMessage("echo", json_string, fun::TransportProtocol::kTcp, enc_type);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);

  session->Close();

  is_working = true;

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Connect(fun::TransportProtocol::kTcp, 8612, fun::FunEncoding::kJson);

  // send
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

      fun::EncryptionType enc_type = fun::EncryptionType::kDefaultEncryption;
      if (i%2 == 0) enc_type = fun::EncryptionType::kIFunEngine1Encryption;
      else enc_type = fun::EncryptionType::kIFunEngine2Encryption;

      session->SendMessage("echo", json_string, fun::TransportProtocol::kTcp, enc_type);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}

- (void)testEncProtobuf_ife1_ife2 {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      if (send_string.compare(echo.msg()) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
    else {
      is_ok = false;
      is_working = false;
      XCTAssert(is_ok);
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8622, fun::FunEncoding::kProtobuf);

  // send
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

      fun::EncryptionType enc_type = fun::EncryptionType::kDefaultEncryption;
      if (i%2 == 0) enc_type = fun::EncryptionType::kIFunEngine1Encryption;
      else enc_type = fun::EncryptionType::kIFunEngine2Encryption;

      session->SendMessage(msg, fun::TransportProtocol::kTcp, enc_type);
    }
  }
  {
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(send_string.c_str());
    session->SendMessage(msg);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJsonUdp_ife2 {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  auto option = fun::FunapiUdpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kIFunEngine2Encryption);
  session->Connect(fun::TransportProtocol::kUdp, 8513, fun::FunEncoding::kJson, option);

  // send
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

      session->SendMessage("echo", json_string);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJsonHttp_ife2 {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  auto option = fun::FunapiHttpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kIFunEngine2Encryption);
  session->Connect(fun::TransportProtocol::kHttp, 8518, fun::FunEncoding::kJson, option);

  // send
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

      session->SendMessage("echo", json_string);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJson_all {
  std::string send_string = "Json Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), true);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      is_ok = false;

      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();

      if (send_string.compare(recv_string) == 0) {
        is_ok = true;
        is_working = false;
      }
    }
  });

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kIFunEngine1Encryption);
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 8912, fun::FunEncoding::kJson, option);

  // send
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

      session->SendMessage("echo", json_string);
    }
  }
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

    session->SendMessage("echo", json_string);
  }

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testTemp_RedirectJson {
  std::string send_string = "user1";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
  {
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

      s->SendMessage("echo", json_string);
    }

    if (type == fun::SessionEventType::kRedirectSucceeded) {
      is_ok = true;
      is_working = false;
    }
  });

  session->AddTransportEventCallback
  ([self, &is_ok, &is_working]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::TransportEventType type,
    const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::TransportEventType::kConnectionFailed) {
      is_ok = false;
      is_working = false;
    }
    else if (type == fun::TransportEventType::kConnectionTimedOut) {
      is_ok = false;
      is_working = false;
    }

    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_string]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
  });

  session->Connect(fun::TransportProtocol::kTcp, 8412, fun::FunEncoding::kJson);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

@end
