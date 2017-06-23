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

@interface funapi_plugin_cocos2dx_desktopTests : XCTestCase

@end

@implementation funapi_plugin_cocos2dx_desktopTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testExample {
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

- (void)testEchoJson {
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

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEchoJson_reconnect {
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

  is_working = true;

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

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

- (void)testEchoProtobuf {
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

  session->Connect(fun::TransportProtocol::kTcp, 8022, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testMulticastJson {
  int user_count = 10;
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 8112;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "multicast";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      // send
      if (channel_id == multicast_test_channel) {
        if (sender == user_name) {
          send_function(m, channel_id, 0);
        }
      }
    });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id,
        const std::string &sender)
    {
    });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      is_ok = false;
      is_working = false;

      XCTAssert(is_ok);
    });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      if (type == fun::SessionEventType::kOpened) {
        m->JoinChannel(multicast_test_channel);
      }
    });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      if (type == fun::TransportEventType::kStarted) {
      }
      else if (type == fun::TransportEventType::kStopped) {
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
    });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

        if (number >= send_count) {
          is_ok = true;
          is_working = false;
        }
        else {
          send_function(m, channel_id, number+1);
        }
      }
    });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}

- (void)testMulticastProtobuf {
  int user_count = 10;
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8122;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "multicast";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
     // send
     if (channel_id == multicast_test_channel) {
       if (sender == user_name) {
         send_function(m, channel_id, 0);
       }
     }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      is_ok = false;
      is_working = false;

      XCTAssert(is_ok);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
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
      if (type == fun::TransportEventType::kStarted) {
      }
      else if (type == fun::TransportEventType::kStopped) {
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
    });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
    {
      if (sender == user_name) {
        FunMulticastMessage mcast_msg = message.GetExtension(multicast);
        FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

        int number = atoi(chat_msg.text().c_str());

        if (number >= send_count) {
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

- (void)testReliabilityJson {
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 8212;
  bool with_session_reliability = true;

  auto send_function =
  [](const std::shared_ptr<fun::FunapiSession>& s,
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

    s->SendMessage("echo", json_string);
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  bool is_ok = true;
  bool is_working = true;

  session->
  AddSessionEventCallback
  ([&send_function]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
   {
     if (type == fun::SessionEventType::kOpened) {
       send_function(s, 0);
     }
   });

  session->
  AddTransportEventCallback
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

  session->
  AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_function, &send_count]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
  {
    if (msg_type.compare("echo") == 0) {
      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();
      int number = atoi(recv_string.c_str());

      if (number >= send_count) {
        is_ok = true;
        is_working = false;
      }
      else {
        send_function(s, number+1);
      }
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, port, encoding);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testReliabilityJson_reconnect {
  int send_count = 10;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 8212;
  bool with_session_reliability = true;

  auto send_function =
  [](const std::shared_ptr<fun::FunapiSession>& s,
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

    s->SendMessage("echo", json_string);
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  bool is_ok = true;
  bool is_working = true;

  session->
  AddSessionEventCallback
  ([&send_function]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
   {
     if (type == fun::SessionEventType::kOpened) {
       send_function(s, 0);
     }
   });

  session->
  AddTransportEventCallback
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

  session->
  AddJsonRecvCallback
  ([self, &is_working, &is_ok, &send_function, &send_count]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const std::string &msg_type, const std::string &json_string)
   {
     if (msg_type.compare("echo") == 0) {
       rapidjson::Document msg_recv;
       msg_recv.Parse<0>(json_string.c_str());

       XCTAssert(msg_recv.HasMember("message"));

       std::string recv_string = msg_recv["message"].GetString();
       int number = atoi(recv_string.c_str());

       if (number >= send_count) {
         is_ok = true;
         is_working = false;
       }
       else {
         send_function(s, number+1);
       }
     }
   });

  session->Connect(fun::TransportProtocol::kTcp, port, encoding);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);

  is_working = true;

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  is_working = true;

  session->Connect(fun::TransportProtocol::kTcp, port, encoding);

  send_function(session, 0);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testReliabilityProtobuf {
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8222;
  bool with_session_reliability = true;

  auto send_function =
  [](const std::shared_ptr<fun::FunapiSession>& s,
     int number)
  {
    std::stringstream ss;
    ss << number;

    // send
    FunMessage msg;
    msg.set_msgtype("pbuf_echo");
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(ss.str().c_str());
    s->SendMessage(msg);
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  bool is_ok = true;
  bool is_working = true;

  session->
  AddSessionEventCallback
  ([&send_function]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
   {
     if (type == fun::SessionEventType::kOpened) {
       send_function(s, 0);
     }
   });

  session->
  AddTransportEventCallback
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

  session->
  AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_function, &send_count]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
  {
    if (fun_message.msgtype().compare("pbuf_echo") == 0) {
      PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

      int number = atoi(echo.msg().c_str());
      if (number >= send_count) {
        is_ok = true;
        is_working = false;
      }
      else {
        send_function(s, number+1);
      }
    }
    else {
      is_ok = false;
      is_working = false;
      XCTAssert(is_ok);
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, port, encoding);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testMulticastReliabilityJson {
  int user_count = 10;
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 8312;
  bool with_session_reliability = true;
  std::string multicast_test_channel = "MulticastReliability";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id,
        const std::string &sender)
     {
     });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel);
       }
     });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

         if (number >= send_count) {
           is_ok = true;
           is_working = false;
         }
         else {
           send_function(m, channel_id, number+1);
         }
       }
     });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}

- (void)testMulticastReliabilityProtobuf {
  int user_count = 10;
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8322;
  bool with_session_reliability = true;
  std::string multicast_test_channel = "MulticastReliability";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
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
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testEchoJsonQueue {
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

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

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

- (void)testEchoJsonQueue10times {
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

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

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
  // //

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEchoJsonQueueMultitransport {
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

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);
  session->Connect(fun::TransportProtocol::kUdp, 8013, fun::FunEncoding::kJson);
  session->Connect(fun::TransportProtocol::kHttp, 8018, fun::FunEncoding::kJson);

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

      session->SendMessage("echo", json_string, fun::TransportProtocol::kHttp);
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

    session->SendMessage("echo", json_string, fun::TransportProtocol::kHttp);
  }
  // //

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEchoProtobufMsgtypeInt {
  std::string send_string = "Protobuf Echo Message";
  std::string server_ip = g_server_ip;

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;
  bool is_working = true;

  session->AddSessionEventCallback
  ([&send_string](const std::shared_ptr<fun::FunapiSession> &s,
                  const fun::TransportProtocol protocol,
                  const fun::SessionEventType type,
                  const std::string &session_id,
                  const std::shared_ptr<fun::FunapiError> &error)
  {
    if (type == fun::SessionEventType::kOpened) {
      // send
      FunMessage msg;
      msg.set_msgtype2(pbuf_echo.number());
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
    if (fun_message.has_msgtype2()) {
      if (fun_message.msgtype2() == pbuf_echo.number()) {
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
    }
    else {
      is_ok = false;
      XCTAssert(is_ok);
    }

    is_working = false;
  });

  session->Connect(fun::TransportProtocol::kTcp, 8422, fun::FunEncoding::kProtobuf);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEchoProtobufMsgtypeInt_2 {
  int send_count = 100;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8422;
  bool with_session_reliability = false;

  auto send_function =
  [](const std::shared_ptr<fun::FunapiSession>& s,
     int number)
  {
    std::stringstream ss;
    ss << number;

    // send
    FunMessage msg;
    msg.set_msgtype2(pbuf_echo.number());
    PbufEchoMessage *echo = msg.MutableExtension(pbuf_echo);
    echo->set_msg(ss.str().c_str());
    s->SendMessage(msg);
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), with_session_reliability);
  bool is_ok = true;
  bool is_working = true;

  session->
  AddSessionEventCallback
  ([&send_function]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol protocol,
    const fun::SessionEventType type,
    const std::string &session_id,
    const std::shared_ptr<fun::FunapiError> &error)
   {
     if (type == fun::SessionEventType::kOpened) {
       send_function(s, 0);
     }
   });

  session->
  AddTransportEventCallback
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

  session->
  AddProtobufRecvCallback
  ([self, &is_working, &is_ok, &send_function, &send_count]
   (const std::shared_ptr<fun::FunapiSession> &s,
    const fun::TransportProtocol transport_protocol,
    const FunMessage &fun_message)
   {
     if (fun_message.msgtype2() == pbuf_echo.number()) {
       PbufEchoMessage echo = fun_message.GetExtension(pbuf_echo);

       int number = atoi(echo.msg().c_str());
       if (number >= send_count) {
         is_ok = true;
         is_working = false;
       }
       else {
         send_function(s, number+1);
       }
     }
     else {
       is_ok = false;
       is_working = false;
       XCTAssert(is_ok);
     }
   });

  session->Connect(fun::TransportProtocol::kTcp, port, encoding);

  while (is_working) {
    session->Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  session->Close();

  XCTAssert(is_ok);
}

- (void)testEncJson_sodium {
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
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 9012, fun::FunEncoding::kJson, option);

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

- (void)testEncJson_chacha20 {
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
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 8712, fun::FunEncoding::kJson, option);

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

- (void)testEncProtobuf_chacha20 {
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

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kChacha20Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 8722, fun::FunEncoding::kProtobuf, option);

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

- (void)testEncJson_aes128 {
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
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 8812, fun::FunEncoding::kJson, option);

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

- (void)testEncProtobuf_aes128 {
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

  auto option = fun::FunapiTcpTransportOption::Create();
  option->SetEncryptionType(fun::EncryptionType::kAes128Encryption,
                            "0b8504a9c1108584f4f0a631ead8dd548c0101287b91736566e13ead3f008f5d");
  session->Connect(fun::TransportProtocol::kTcp, 8822, fun::FunEncoding::kProtobuf, option);

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

- (void)testTemp_Multicast_Json_2 {
  int user_count = 20;
  int send_count = 50;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  uint16_t port = 8112;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "multicast";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    if (i%2 == 0) port = 8012;
    else port = 9012;

    if (i%3 == 0) multicast_test_channel = "channel1";
    else multicast_test_channel = "channel2";

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel);
       }
     });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

         if (number >= send_count) {
           is_ok = true;
           is_working = false;
         }
         else {
           send_function(m, channel_id, number+1);
         }
       }
     });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }
  
  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }
  
  XCTAssert(is_ok);
}

- (void)testTemp_MulticastProtobufWithToken_Users {
  int user_count = 10;
  int send_count = 50;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "test";
  std::string multicast_token = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    if (i%2 == 0) port = 8022;
    else port = 9022;

    if (i%3 == 0) {
      multicast_test_channel = "test";
      multicast_token = "test";
    }
    else {
      multicast_test_channel = "test1";
      multicast_token = "test1";
    }

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
    {
      // send
      if (channel_id == multicast_test_channel) {
       if (sender == user_name) {
         send_function(m, channel_id, 0);
       }
      }
    });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
    {
    });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
    {
      // EC_ALREADY_JOINED = 1,
      // EC_ALREADY_LEFT,
      // EC_FULL_MEMBER
      // EC_CLOSED

      is_ok = false;
      is_working = false;

      XCTAssert(is_ok);
    });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel, multicast_token]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      if (type == fun::SessionEventType::kOpened) {
       m->JoinChannel(multicast_test_channel, multicast_token);
      }
    });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
    {
      if (type == fun::TransportEventType::kStarted) {
      }
      else if (type == fun::TransportEventType::kStopped) {
      }
      else if (type == fun::TransportEventType::kConnectionFailed) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kConnectionTimedOut) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
      else if (type == fun::TransportEventType::kDisconnected) {
        is_ok = false;
        is_working = false;
        XCTAssert(is_ok);
      }
    });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testTemp_MulticastProtobufWithToken_Users2 {
  int user_count = 2;
  int send_count = 10;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "test";
  std::string multicast_token = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    if (i%2 == 0) port = 8022;
    else port = 9022;

//    if (i%3 == 0) {
//      multicast_test_channel = "test";
//      multicast_token = "test";
//    }
//    else {
//      multicast_test_channel = "test1";
//      multicast_token = "test1";
//    }

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel, multicast_token]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, multicast_token);
       }
     });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testTemp_DedicatedManager {
  int user_count = 2;
  std::string server_ip = g_server_ip;

  bool is_ok = true;
  bool is_working = true;
  std::vector<std::shared_ptr<fun::FunapiSession>> v_sessions;

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto session = fun::FunapiSession::Create(server_ip.c_str(), false);

    session->
    AddSessionEventCallback
    ([user_name]
     (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         rapidjson::Document msg;
         msg.SetObject();
         rapidjson::Value message_node(user_name.c_str(), msg.GetAllocator());
         msg.AddMember("name", message_node, msg.GetAllocator());

         // Convert JSON document to string
         rapidjson::StringBuffer buffer;
         rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
         msg.Accept(writer);
         std::string json_string = buffer.GetString();

         s->SendMessage("login", json_string);
       }
     });

    session->
    AddTransportEventCallback
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

    session->
    AddJsonRecvCallback
    ([self, &is_working, &is_ok, user_count]
     (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const std::string &msg_type,
      const std::string &json_string)
    {
      if (msg_type.compare("_sc_dedicated_server") == 0)
      {
        static int recv_count = 0;

        printf("json_string = '%s'\n", json_string.c_str());

        ++recv_count;
        if (recv_count == user_count) {
          std::this_thread::sleep_for(std::chrono::seconds(5));
          is_working = false;
          is_ok = true;
        }
      }
    });

    session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

    v_sessions.push_back(session);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}

- (void)testTemp_DedicatedManager_Delay {
  int user_count = 2;
  std::string server_ip = g_server_ip;

  bool is_ok = true;
  bool is_working = true;
  std::vector<std::shared_ptr<fun::FunapiSession>> v_sessions;

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto session = fun::FunapiSession::Create(server_ip.c_str(), false);

    session->
    AddSessionEventCallback
    ([user_name]
     (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         rapidjson::Document msg;
         msg.SetObject();
         rapidjson::Value message_node(user_name.c_str(), msg.GetAllocator());
         msg.AddMember("name", message_node, msg.GetAllocator());

         // Convert JSON document to string
         rapidjson::StringBuffer buffer;
         rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
         msg.Accept(writer);
         std::string json_string = buffer.GetString();

         s->SendMessage("login", json_string);
       }
     });

    session->
    AddTransportEventCallback
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

    session->
    AddJsonRecvCallback
    ([self, &is_working, &is_ok, user_count]
     (const std::shared_ptr<fun::FunapiSession> &s,
      const fun::TransportProtocol protocol,
      const std::string &msg_type,
      const std::string &json_string)
     {
       if (msg_type.compare("_sc_dedicated_server") == 0)
       {
         static int recv_count = 0;

         printf("json_string = '%s'\n", json_string.c_str());

         ++recv_count;
         if (recv_count == user_count) {
           std::this_thread::sleep_for(std::chrono::seconds(5));
           is_working = false;
           is_ok = true;
         }
       }
     });

    session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

    v_sessions.push_back(session);

    int temp_seconds = 0;
    while (is_working) {
      fun::FunapiTasks::UpdateAll();
      std::this_thread::sleep_for(std::chrono::seconds(1));
      ++temp_seconds;
      if (temp_seconds > 20) break;
    }
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }

  XCTAssert(is_ok);
}

- (void)testTemp_MulticastJsonWithToken_1 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  // uint16_t port = 8112;
  uint16_t port = 8012;
  bool with_session_reliability = false;
  // std::string multicast_test_channel = "multicast";
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id,
        const std::string &sender)
     {
     });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED
       // EC_INVALID_TOKEN

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "test");
       }
     });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

         if (number >= send_count) {
           is_ok = true;
           is_working = false;
         }
         else {
           send_function(m, channel_id, number+1);
         }
       }
     });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }
  
  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }
  
  XCTAssert(is_ok);
}

- (void)testTemp_MulticastJsonWithToken_2 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  // uint16_t port = 8112;
  uint16_t port = 8012;
  bool with_session_reliability = false;
  // std::string multicast_test_channel = "multicast";
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id,
        const std::string &sender)
     {
     });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED
       // EC_INVALID_TOKEN

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "wrong");
       }
     });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

         if (number >= send_count) {
           is_ok = true;
           is_working = false;
         }
         else {
           send_function(m, channel_id, number+1);
         }
       }
     });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }
  
  XCTAssert(is_ok);
}

- (void)testTemp_MulticastJsonWithToken_3 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kJson;
  // uint16_t port = 8112;
  uint16_t port = 8012;
  bool with_session_reliability = false;
  // std::string multicast_test_channel = "multicast";
  std::string multicast_test_channel = "error";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    multicast->AddJoinedCallback
    ([user_name, multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id,
        const std::string &sender)
     {
     });

    multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED
       // EC_INVALID_TOKEN

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "test");
       }
     });

    multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    multicast->AddJsonChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, multicast_test_channel, user_name, &send_function, &send_count]
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

         if (number >= send_count) {
           is_ok = true;
           is_working = false;
         }
         else {
           send_function(m, channel_id, number+1);
         }
       }
     });

    multicast->Connect();

    v_multicast.push_back(multicast);
  }

  while (is_working) {
    fun::FunapiTasks::UpdateAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60fps
  }
  
  XCTAssert(is_ok);
}

- (void)testTemp_MulticastProtobufWithToken_0 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 9022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "test");
       }
     });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testTemp_MulticastProtobufWithToken_1 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "test");
       }
     });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testTemp_MulticastProtobufWithToken_2 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "test";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "wrong");
       }
     });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

- (void)testTemp_MulticastProtobufWithToken_3 {
  int user_count = 1;
  int send_count = 1;
  std::string server_ip = g_server_ip;
  fun::FunEncoding encoding = fun::FunEncoding::kProtobuf;
  uint16_t port = 8022;
  bool with_session_reliability = false;
  std::string multicast_test_channel = "error";

  std::vector<std::shared_ptr<fun::FunapiMulticast>> v_multicast;
  bool is_ok = false;
  bool is_working = true;

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

  for (int i=0;i<user_count;++i) {
    std::stringstream ss_sender;
    ss_sender << "user" << i;
    std::string user_name = ss_sender.str();

    auto funapi_multicast =
    fun::FunapiMulticast::Create(user_name.c_str(),
                                 server_ip.c_str(),
                                 port,
                                 encoding,
                                 with_session_reliability);

    funapi_multicast->AddJoinedCallback
    ([user_name, &multicast_test_channel, &send_function]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const std::string &channel_id,
      const std::string &sender)
     {
       // send
       if (channel_id == multicast_test_channel) {
         if (sender == user_name) {
           send_function(m, channel_id, 0);
         }
       }
     });

    funapi_multicast->AddLeftCallback
    ([](const std::shared_ptr<fun::FunapiMulticast>& m,
        const std::string &channel_id, const std::string &sender)
     {
     });

    funapi_multicast->AddErrorCallback
    ([self, &is_working, &is_ok]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      int error)
     {
       // EC_ALREADY_JOINED = 1,
       // EC_ALREADY_LEFT,
       // EC_FULL_MEMBER
       // EC_CLOSED

       is_ok = false;
       is_working = false;

       XCTAssert(is_ok);
     });

    funapi_multicast->AddSessionEventCallback
    ([self, &is_ok, &is_working, &multicast_test_channel]
     (const std::shared_ptr<fun::FunapiMulticast>& m,
      const fun::SessionEventType type,
      const std::string &session_id,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::SessionEventType::kOpened) {
         m->JoinChannel(multicast_test_channel, "test");
       }
     });

    funapi_multicast->AddTransportEventCallback
    ([self, &is_ok, &is_working]
     (const std::shared_ptr<fun::FunapiMulticast>& multicast,
      const fun::TransportEventType type,
      const std::shared_ptr<fun::FunapiError> &error)
     {
       if (type == fun::TransportEventType::kStarted) {
       }
       else if (type == fun::TransportEventType::kStopped) {
       }
       else if (type == fun::TransportEventType::kConnectionFailed) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kConnectionTimedOut) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
       else if (type == fun::TransportEventType::kDisconnected) {
         is_ok = false;
         is_working = false;
         XCTAssert(is_ok);
       }
     });

    funapi_multicast->AddProtobufChannelMessageCallback
    (multicast_test_channel,
     [self, &is_ok, &is_working, &multicast_test_channel, user_name, &send_function, &send_count]
     (const std::shared_ptr<fun::FunapiMulticast> &m,
      const std::string &channel_id,
      const std::string &sender,
      const FunMessage& message)
     {
       if (sender == user_name) {
         FunMulticastMessage mcast_msg = message.GetExtension(multicast);
         FunChatMessage chat_msg = mcast_msg.GetExtension(chat);

         int number = atoi(chat_msg.text().c_str());

         if (number >= send_count) {
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

@end
