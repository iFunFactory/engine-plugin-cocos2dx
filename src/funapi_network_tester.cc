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
#include "./funapi_downloader.h"
#include "./pbuf_echo.pb.h"


const char kServerIp[] = "127.0.0.1";
bool is_downloading = false;


void on_session_initiated(const std::string &session_id, void *ctxt) {
  std::cout << "session initiated: " << session_id << std::endl;
}


void on_session_closed(void *ctxt) {
  std::cout << "session closed" << std::endl;
}


void on_echo_json(const std::string &msg_type, const std::string &body, void *ctxt) {
  std::cout << "msg '" << msg_type << "' arrived." << std::endl;
  std::cout << "json: " << body << std::endl;
}


void on_download_update(const std::string &path, long receive, long total, int percent, void *ctxt) {
}


void on_download_finished(int result, void *ctxt) {
  is_downloading = false;
}


int main(int argc, char **argv) {
  fun::FunapiNetwork::Initialize(3600);
  fun::FunapiNetwork *network = NULL;
  fun::FunapiTransport *transport = NULL;
  int msg_type = fun::kJsonEncoding;

  while (true) {
    std::cout << "** Select number" << std::endl;
    std::cout << "1. connect tcp" << std::endl;
    std::cout << "2. connect udp" << std::endl;
    std::cout << "3. connect http" << std::endl;
    std::cout << "4. download test" << std::endl;
    std::cout << "e. echo message" << std::endl;
    std::cout << "q. disconnect" << std::endl;

    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
      std::cout << "EOF reached. Quitting." << std::endl;
      break;
    }

    if (input == "1" || input == "2" || input == "3") {
      if (network != NULL && network->Started()) {
        std::cout << "Already connected. Disconnect first." << std::endl;
        continue;
      }

      while (true) {
        std::cout << "** Select encoding" << std::endl;
        std::cout << "1. Json" << std::endl;
        std::cout << "2. Protobuf" << std::endl;
        std::string input2;
        std::getline(std::cin, input2);
        if (input2.empty()) {
          std::cout << "EOF reached. Quitting." << std::endl;
          break;
        }

        if (input2 == "1")
          msg_type = fun::kJsonEncoding;
        else if (input2 == "2")
          msg_type = fun::kProtobufEncoding;
        else {
          std::cout << "Select one of Json or Protobuf" << std::endl;
          continue;
        }
        break;
      }

      if (input == "1") {
        transport = new fun::FunapiTcpTransport(kServerIp, 8012);
      } else if (input == "2") {
        transport = new fun::FunapiUdpTransport(kServerIp, 8013);
      } else if (input == "3") {
        transport = new fun::FunapiHttpTransport(kServerIp, 8018);
      }

      network = new fun::FunapiNetwork(transport, msg_type,
          fun::FunapiNetwork::OnSessionInitiated(on_session_initiated, NULL),
          fun::FunapiNetwork::OnSessionClosed(on_session_closed, NULL));
      network->RegisterHandler("echo", fun::FunapiNetwork::MessageHandler(on_echo_json, NULL));

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
      // The folder that contains the resource to download.
      // Please change the path of the resource to download.
      fun::FunapiHttpDownloader downloader("/home/anonymous/resource",
          fun::FunapiHttpDownloader::OnUpdate(on_download_update, NULL),
          fun::FunapiHttpDownloader::OnFinished(on_download_finished, NULL));

      is_downloading = true;
      downloader.StartDownload(kServerIp, 8020, "list");

      while (is_downloading) {
        sleep(1);
      }
      break;
    } else if (input == "e") {
      if (network == NULL || network->Started() == false) {
        std::cout << "You should connect first." << std::endl;
      } else {
        if (msg_type == fun::kJsonEncoding) {
          rapidjson::Document msg;
          msg.SetObject();
          rapidjson::Value message_node("hello world", msg.GetAllocator());
          msg.AddMember("message", message_node, msg.GetAllocator());
          network->SendMessage("echo", msg, encryption);
        } else if (msg_type == fun::kProtobufEncoding) {
          FunMessage example;
          example.set_msgtype("pbuf_echo");
          PbufEchoMessage *echo = example.MutableExtension(pbuf_echo);
          echo->set_message("hello proto");
          network->SendMessage(example, encryption);
        }
     }
    } else if (input == "q") {
      if (network == NULL || network->Started() == false) {
        std::cout << "You should connect first." << std::endl;
        continue;
      }

      network->Stop();
      break;

    } else {
      std::cout << "Select one of connect, disconnect, or echo" << std::endl;
    }
  }

  fun::FunapiNetwork::Finalize();

  return 0;
}
