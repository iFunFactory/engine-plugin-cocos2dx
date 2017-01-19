// Copyright (C) 2013-2017 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

// http://stackoverflow.com/questions/15759559/variable-named-type-boolc-code-is-conflicted-with-ios-macro
#include <ConditionalMacros.h>
#undef TYPE_BOOL
#include "funapi_session.h"
#include "test_messages.pb.h"
#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

#import <XCTest/XCTest.h>

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
  std::string send_string;
  std::string server_ip = "127.0.0.1";

  std::function<void(const std::shared_ptr<fun::FunapiSession>&,
                     const std::string&)> send_message =
  [&send_string](const std::shared_ptr<fun::FunapiSession>&s,
                 const std::string &temp_string) {
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

      send_string = temp_string;
  };

  auto session = fun::FunapiSession::Create(server_ip.c_str(), false);
  bool is_ok = true;

  session->AddSessionEventCallback([&send_message](const std::shared_ptr<fun::FunapiSession> &s,
                                                          const fun::TransportProtocol protocol,
                                                          const fun::SessionEventType type,
                                                          const std::string &session_id,
                                                          const std::shared_ptr<fun::FunapiError> &error) {
    if (type == fun::SessionEventType::kOpened) {
      send_message(s, "Json Echo Message");
    }
  });

  session->AddTransportEventCallback([self](const std::shared_ptr<fun::FunapiSession> &s,
                                            const fun::TransportProtocol protocol,
                                            const fun::TransportEventType type,
                                            const std::shared_ptr<fun::FunapiError> &error) {
    XCTAssert(type != fun::TransportEventType::kConnectionFailed);
    XCTAssert(type != fun::TransportEventType::kConnectionTimedOut);
  });

  session->AddJsonRecvCallback([self, &is_ok, &send_string](const std::shared_ptr<fun::FunapiSession> &s,
                                              const fun::TransportProtocol protocol,
                                              const std::string &msg_type, const std::string &json_string) {
    if (msg_type.compare("echo") == 0) {
      rapidjson::Document msg_recv;
      msg_recv.Parse<0>(json_string.c_str());

      XCTAssert(msg_recv.HasMember("message"));

      std::string recv_string = msg_recv["message"].GetString();
      printf("echo - %s\n", recv_string.c_str());

      XCTAssert(send_string.compare(recv_string) == 0);

      is_ok = false;
    }
  });

  session->Connect(fun::TransportProtocol::kTcp, 8012, fun::FunEncoding::kJson);

  while (is_ok) {
    session->Update();
  }

  session->Close();
}

@end
