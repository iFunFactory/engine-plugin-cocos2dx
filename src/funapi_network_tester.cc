// Copyright (C) 2013 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include <unistd.h>

#include <iostream>
#include <string>

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "./funapi_network.h"

const char kServerIp[] = "127.0.0.1";


void on_session_initiated(const std::string &session_id, void *ctxt) {
  std::cout << "session initiated: " << session_id << std::endl;
}


void on_session_closed(void *ctxt) {
  std::cout << "session closed" << std::endl;
}


void on_echo(
    const std::string &msg_type, const rapidjson::Document &body, void *ctxt) {
  std::cout << "msg '" << msg_type << "' arrived." << std::endl;

  rapidjson::StringBuffer string_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
  body.Accept(writer);
  std::cout << "json: " << string_buffer.GetString() << std::endl;
}


int main(int argc, char **argv) {
  fun::FunapiNetwork::Initialize(3600);
  fun::FunapiNetwork *network = NULL;

  while (true) {
    std::cout << "** Select number" << std::endl;
    std::cout << "1. connect tcp" << std::endl;
    std::cout << "2. connect udp" << std::endl;
    std::cout << "3. echo message" << std::endl;
    std::cout << "4. disconnect" << std::endl;

    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
      std::cout << "EOF reached. Quitting." << std::endl;
      break;
    }

    if (input == "1" || input == "2") {
      if (network != NULL && network->Started()) {
        std::cout << "Already connected. Disconnect first." << std::endl;
        continue;
      }

      fun::FunapiTransport *transport = NULL;
      if (input == "1") {
        transport = new fun::FunapiTcpTransport(kServerIp, 8012);
      } else if (input == "2") {
        transport = new fun::FunapiUdpTransport(kServerIp, 8013);
      }

      network = new fun::FunapiNetwork(transport,
        fun::FunapiNetwork::OnSessionInitiated(on_session_initiated, NULL),
        fun::FunapiNetwork::OnSessionClosed(on_session_closed, NULL));
        network->RegisterHandler("echo", fun::FunapiNetwork::MessageHandler(on_echo, NULL));

      network->Start();
      // network->Start() works asynchronously.
      // So, we can proceeed to handle other requests and
      // check network->Started() later.
      // In this example, however, we will do polling for brevity.
      time_t started = time(NULL);
      while (started + 5 >= time(NULL)) {
        std::cout << "Waiting for connect() to complete." << std::endl;
        sleep(1);
        if (network->Connected()) {
          std::cout << "Connected" << std::endl;
          break;
        }
      }
      if (network->Connected() == false) {
        std::cout << "Connection failed. Stopping." << std::endl;
        network->Stop();
      }
    } else if (input == "4") {
      if (network->Started() == false) {
        std::cout << "You should connect first." << std::endl;
        continue;
      }
      network->Stop();
    } else if (input.compare(0, 1, "3") == 0) {
      if (network->Started() == false) {
        std::cout << "You should connect first." << std::endl;
      } else {
        rapidjson::Document msg;
        msg.SetObject();
        rapidjson::Value message_node("hello world", msg.GetAllocator());
        msg.AddMember("message", message_node, msg.GetAllocator());
        network->SendMessage("echo", msg);
      }
    } else {
      std::cout << "Select one of connect, disconnect, or echo" << std::endl;
    }
  }

  delete network;
  fun::FunapiNetwork::Finalize();

  return 0;
}

