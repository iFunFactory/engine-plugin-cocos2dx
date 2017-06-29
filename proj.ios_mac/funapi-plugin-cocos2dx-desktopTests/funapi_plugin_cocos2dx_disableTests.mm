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

@interface funapi_plugin_cocos2dx_disableTests : XCTestCase

@end

@implementation funapi_plugin_cocos2dx_disableTests

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

    session->AddSessionEventCallback
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
  uint16_t port = 8012;
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
  uint16_t port = 8012;
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
  uint16_t port = 8012;
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
