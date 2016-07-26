// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_network.h"
#include "funapi_session.h"
#include "funapi_multicasting.h"
#include "service/multicast_message.pb.h"

namespace fun {

static const char* kMulticastMsgType = "_multicast";
static const char* kChannelId = "_channel";
static const char* kSender = "_sender";
static const char* kJoin = "_join";
static const char* kLeave = "_leave";
static const char* kErrorCode = "_error_code";
static const char* kChannelListId = "_channels";

////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastClientImpl implementation.

class FunapiMulticastClientImpl : public std::enable_shared_from_this<FunapiMulticastClientImpl> {
 public:
  typedef FunapiMulticastClient::ChannelNotify ChannelNotify;
  typedef FunapiMulticastClient::ChannelMessage ChannelMessage;
  typedef FunapiMulticastClient::ErrorNotify ErrorNotify;
  typedef FunapiMulticastClient::ChannelListNotify ChannelListNotify;

  FunapiMulticastClientImpl(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding);
  ~FunapiMulticastClientImpl();

  void SetSender(const std::string &sender);
  void SetEncoding(const FunEncoding encodig);

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;

  bool JoinChannel(const std::string &channel_id, const ChannelMessage &handler);
  bool LeaveChannel(const std::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(FunMessage &msg);
  bool SendToChannel(std::string &json_string);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);

  void OnReceived(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body);

  bool RequestChannelList();

 private:
  std::shared_ptr<fun::FunapiNetwork> network_ = nullptr;

  FunEncoding encoding_ = FunEncoding::kNone;
  std::string sender_;

  FunapiEvent<ChannelNotify> on_joined_;
  FunapiEvent<ChannelNotify> on_left_;
  FunapiEvent<ErrorNotify> on_error_;
  FunapiEvent<ChannelListNotify> on_channel_list_;

  void OnJoined(const std::string &channel_id, const std::string &sender);
  void OnLeft(const std::string &channel_id, const std::string &sender);
  void OnError(int error);

  void OnChannelList(const rapidjson::Document &msg);
  void OnChannelList(FunMulticastMessage *msg);
  void OnChannelList(const std::map<std::string, int> &cl);

  std::map<std::string, ChannelMessage> channels_;

  void SendLeaveMessage(const std::string &channel_id);
};


FunapiMulticastClientImpl::FunapiMulticastClientImpl(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding)
  : network_(network), encoding_(encoding) {
  network_->RegisterHandler(kMulticastMsgType, [this](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body){ OnReceived(p, type, v_body); });
}


FunapiMulticastClientImpl::~FunapiMulticastClientImpl() {
  network_->RegisterHandler(kMulticastMsgType, [](const fun::TransportProtocol p, const std::string &type, const std::vector<uint8_t> &v_body) { });
}


void FunapiMulticastClientImpl::SetSender(const std::string &sender) {
  sender_ = sender;
}


void FunapiMulticastClientImpl::SetEncoding(const FunEncoding encodig) {
  encoding_ = encodig;
}


void FunapiMulticastClientImpl::AddJoinedCallback(const ChannelNotify &handler) {
  on_joined_ += handler;
}


void FunapiMulticastClientImpl::AddLeftCallback(const ChannelNotify &handler) {
  on_left_ += handler;
}


void FunapiMulticastClientImpl::AddErrorCallback(const ErrorNotify &handler) {
  on_error_ += handler;
}


void FunapiMulticastClientImpl::AddChannelListCallback(const ChannelListNotify &handler) {
  on_channel_list_ += handler;
}


void FunapiMulticastClientImpl::OnJoined(const std::string &channel_id, const std::string &sender) {
  on_joined_(channel_id, sender);
}


void FunapiMulticastClientImpl::OnLeft(const std::string &channel_id, const std::string &sender) {
  on_left_(channel_id, sender);
}


void FunapiMulticastClientImpl::OnError(const int error) {
  on_error_(error);
}


void FunapiMulticastClientImpl::OnChannelList(const std::map<std::string, int> &cl) {
  on_channel_list_(cl);
}


void FunapiMulticastClientImpl::OnChannelList(FunMulticastMessage *msg) {
  std::map<std::string, int> cl;

  for (int i=0;i<msg->channels_size();++i) {
    cl[msg->channels(i).channel_name()] = msg->channels(i).num_members();
  }

  OnChannelList(cl);
}


void FunapiMulticastClientImpl::OnChannelList(const rapidjson::Document &msg) {
  const rapidjson::Value &d = msg[kChannelListId];

  std::map<std::string, int> cl;

  for (unsigned int i=0;i<d.Size();++i) {
    const rapidjson::Value &v = d[i];
    if (v.HasMember("_name") && v.HasMember("_members")) {
      cl[v["_name"].GetString()] = v["_members"].GetInt();
    }
  }

  OnChannelList(cl);
}


bool FunapiMulticastClientImpl::IsConnected() const {
  if (network_) {
    if (network_->IsConnected()) {
      return true;
    }
  }

  return false;
}


bool FunapiMulticastClientImpl::IsInChannel(const std::string &channel_id) const {
  if (channels_.find(channel_id) != channels_.end()) {
    return true;
  }

  return false;
}


bool FunapiMulticastClientImpl::JoinChannel(const std::string &channel_id, const ChannelMessage &handler) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. First connect before join a multicast channel.");
    return false;
  }

  if (IsInChannel(channel_id)) {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  channels_[channel_id] = handler;

  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value join_node(true);
    msg.AddMember(rapidjson::StringRef(kJoin), join_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_join(true);
    mcast_msg->set_sender(sender_.c_str());

    network_->SendMessage(msg);
  }

  return true;
}


bool FunapiMulticastClientImpl::LeaveChannel(const std::string &channel_id) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  SendLeaveMessage(channel_id);
  OnLeft(channel_id, sender_);

  channels_.erase(channel_id);

  return true;
}


bool FunapiMulticastClientImpl::LeaveAllChannels() {
  if (!IsConnected()) {
    return false;
  }

  if (channels_.size() <= 0)
    return false;

  for (auto c : channels_) {
    SendLeaveMessage(c.first);
    OnLeft(c.first, sender_);
  }

  channels_.clear();

  return true;
}


bool FunapiMulticastClientImpl::SendToChannel(FunMessage &msg) {
  FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
  std::string channel_id = mcast_msg->channel();

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  msg.set_msgtype(kMulticastMsgType);
  mcast_msg->set_sender(sender_.c_str());

  network_->SendMessage(msg);

  return true;
}


bool FunapiMulticastClientImpl::SendToChannel(std::string &json_string) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  rapidjson::Document msg;
  msg.Parse<0>(json_string.c_str());

  std::string channel_id = msg[kChannelId].GetString();

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

  // Convert JSON document to string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg.Accept(writer);
  std::string send_json_string = buffer.GetString();

  network_->SendMessage(kMulticastMsgType, send_json_string);

  return true;
}


void FunapiMulticastClientImpl::SendLeaveMessage(const std::string &channel_id) {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value leave_node(true);
    msg.AddMember(rapidjson::StringRef(kLeave), leave_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_leave(true);
    mcast_msg->set_sender(sender_.c_str());

    network_->SendMessage(msg);
  }
}

void FunapiMulticastClientImpl::OnReceived(const fun::TransportProtocol protocol, const std::string &type, const std::vector<uint8_t> &v_body) {
  // fun::DebugUtils::Log("OnReceived");

  std::string channel_id = "";
  std::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  if (encoding_ == fun::FunEncoding::kJson) {
    // log
    // std::string body(v_body.cbegin(), v_body.cend());
    // fun::DebugUtils::Log("multicast message\n%s", body.c_str());
    // //

    rapidjson::Document msg;
    msg.Parse<0>(reinterpret_cast<char*>(const_cast<uint8_t*>(v_body.data())));

    if (msg.HasMember(kChannelListId)) {
      OnChannelList(msg);
      return;
    }

    if (msg.HasMember(kChannelId)) {
      channel_id = msg[kChannelId].GetString();
    }

    if (msg.HasMember(kSender)) {
      sender = msg[kSender].GetString();
    }

    if (msg.HasMember(kErrorCode)) {
      error_code = msg[kErrorCode].GetInt();
    }

    if (msg.HasMember(kJoin)) {
      join = msg[kJoin].GetBool();
    }

    if (msg.HasMember(kLeave)) {
      leave = msg[kLeave].GetBool();
    }
  }

  if (encoding_ == fun::FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.ParseFromArray(v_body.data(), static_cast<int>(v_body.size()));

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);

    channel_id = mcast_msg->channel();

    if (mcast_msg->has_sender()) {
      sender = mcast_msg->sender();
    }

    if (mcast_msg->has_error_code()) {
      error_code = mcast_msg->error_code();
    }

    if (mcast_msg->has_join()) {
      join = mcast_msg->join();
    }

    if (mcast_msg->has_leave()) {
      leave = mcast_msg->leave();
    }

    if (channel_id.length() == 0 && sender.length() == 0) {
      OnChannelList(mcast_msg);
      return;
    }
  }

  if (error_code != 0) {
    DebugUtils::Log("Multicast error - code: %d", error_code);

    if (error_code == FunMulticastMessage_ErrorCode_EC_ALREADY_LEFT ||
        error_code == FunMulticastMessage_ErrorCode_EC_FULL_MEMBER ) {

      if (IsInChannel(channel_id)) {
        channels_.erase(channel_id);
      }
    }

    OnError(error_code);
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return;
  }

  if (join) {
    DebugUtils::Log("%s joined the '%s' channel", sender.c_str(), channel_id.c_str());
    OnJoined(channel_id, sender);
  }
  else if (leave) {
    DebugUtils::Log("%s left the '%s' channel", sender.c_str(), channel_id.c_str());
    OnLeft(channel_id, sender);
  }
  else {
    channels_[channel_id](channel_id, sender, v_body);
  }
}


bool FunapiMulticastClientImpl::RequestChannelList() {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    network_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_sender(sender_.c_str());

    network_->SendMessage(msg);
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastClient implementation.

FunapiMulticastClient::FunapiMulticastClient(std::shared_ptr<FunapiNetwork> network, FunEncoding encoding)
  : impl_(std::make_shared<FunapiMulticastClientImpl>(network, encoding)) {
  /*** DEPRECATED: The FunapiMulticastClient class should no longer be used.  FunapiMulticast is the replacement for FunapiMulticastClient ***/
  DebugUtils::Log("/*** DEPRECATED: The FunapiMulticastClient class should no longer be used.  FunapiMulticast is the replacement for FunapiMulticastClient ***/");
}


FunapiMulticastClient::~FunapiMulticastClient() {
}


void FunapiMulticastClient::SetSender(const std::string &sender) {
  impl_->SetSender(sender);
}


void FunapiMulticastClient::SetEncoding(const FunEncoding encodig) {
  impl_->SetEncoding(encodig);
}


void FunapiMulticastClient::AddJoinedCallback(const ChannelNotify &handler) {
  impl_->AddJoinedCallback(handler);
}


void FunapiMulticastClient::AddLeftCallback(const ChannelNotify &handler) {
  impl_->AddLeftCallback(handler);
}


void FunapiMulticastClient::AddErrorCallback(const ErrorNotify &handler) {
  impl_->AddErrorCallback(handler);
}


void FunapiMulticastClient::AddChannelListCallback(const ChannelListNotify &handler) {
  impl_->AddChannelListCallback(handler);
}


bool FunapiMulticastClient::IsConnected() const {
  return impl_->IsConnected();
}


bool FunapiMulticastClient::IsInChannel(const std::string &channel_id) const {
  return impl_->IsInChannel(channel_id);
}


bool FunapiMulticastClient::JoinChannel(const std::string &channel_id, const ChannelMessage &handler) {
  return impl_->JoinChannel(channel_id, handler);
}


bool FunapiMulticastClient::LeaveChannel(const std::string &channel_id) {
  return impl_->LeaveChannel(channel_id);
}


bool FunapiMulticastClient::LeaveAllChannels() {
  return impl_->LeaveAllChannels();
}


bool FunapiMulticastClient::SendToChannel(FunMessage &msg) {
  return impl_->SendToChannel(msg);
}


bool FunapiMulticastClient::SendToChannel(std::string &json_string) {
  return impl_->SendToChannel(json_string);
}


bool FunapiMulticastClient::RequestChannelList() {
  return impl_->RequestChannelList();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMulticastImpl implementation.

class FunapiMulticastImpl : public std::enable_shared_from_this<FunapiMulticastImpl> {
 public:
  typedef FunapiMulticast::ChannelNotify ChannelNotify;
  typedef FunapiMulticast::ErrorNotify ErrorNotify;
  typedef FunapiMulticast::ChannelListNotify ChannelListNotify;

  typedef FunapiMulticast::JsonChannelMessageHandler JsonChannelMessageHandler;
  typedef FunapiMulticast::ProtobufChannelMessageHandler ProtobufChannelMessageHandler;

  FunapiMulticastImpl() = delete;
  FunapiMulticastImpl(const char* sender, const char* hostname_or_ip, uint16_t port, FunEncoding encoding, bool reliability);
  ~FunapiMulticastImpl();

  bool IsConnected() const;
  bool IsInChannel(const std::string &channel_id) const;
  bool JoinChannel(const std::string &channel_id);
  bool LeaveChannel(const std::string &channel_id);
  bool LeaveAllChannels();

  bool SendToChannel(const std::string &channel_id, FunMessage &msg, const bool bounce = true);
  bool SendToChannel(const std::string &channel_id, std::string &json_string, const bool bounce = true);

  void AddJoinedCallback(const ChannelNotify &handler);
  void AddLeftCallback(const ChannelNotify &handler);
  void AddErrorCallback(const ErrorNotify &handler);
  void AddChannelListCallback(const ChannelListNotify &handler);

  void AddProtobufChannelMessageCallback(const std::string &channel_id, const ProtobufChannelMessageHandler &handler);
  void AddJsonChannelMessageCallback(const std::string &channel_id, const JsonChannelMessageHandler &handler);

  bool RequestChannelList();

  void Update();

  FunEncoding GetEncoding();

  void AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler);
  void AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler);

  void Connect(const std::weak_ptr<FunapiMulticast>& multicast);
  void Close();

 private:
  void OnReceived(const std::string &json_string);
  void OnReceived(const FunMessage &message);
  bool OnReceived(const std::string &channel_id, const std::string &sender, const bool join, const bool leave, const int error_code);


  std::shared_ptr<fun::FunapiSession> session_ = nullptr;
  std::weak_ptr<fun::FunapiMulticast> multicast_;

  FunEncoding encoding_ = FunEncoding::kNone;
  std::string sender_;
  uint16_t port_;

  FunapiEvent<ChannelNotify> on_joined_;
  FunapiEvent<ChannelNotify> on_left_;
  FunapiEvent<ErrorNotify> on_error_;
  FunapiEvent<ChannelListNotify> on_channel_list_;

  void OnJoined(const std::string &channel_id, const std::string &sender);
  void OnLeft(const std::string &channel_id, const std::string &sender);
  void OnError(int error);

  void OnChannelList(const rapidjson::Document &msg);
  void OnChannelList(FunMulticastMessage *msg);
  void OnChannelList(const std::map<std::string, int> &cl);

  std::unordered_set<std::string> channels_;
  std::unordered_map<std::string, FunapiEvent<JsonChannelMessageHandler>> json_channel_handlers_;
  std::unordered_map<std::string, FunapiEvent<ProtobufChannelMessageHandler>> protobuf_channel_handlers_;
  std::mutex channels_mutex_;

  void SendLeaveMessage(const std::string &channel_id);
};


FunapiMulticastImpl::FunapiMulticastImpl(const char* sender,
                                         const char* hostname_or_ip,
                                         uint16_t port,
                                         FunEncoding encoding,
                                         bool reliability)
: encoding_(encoding), sender_(sender), port_(port) {
  session_ = FunapiSession::create(hostname_or_ip, reliability);
  session_->AddJsonRecvCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                       const fun::TransportProtocol protocol,
                                       const std::string &msg_type,
                                       const std::string &json_string) {
    if (msg_type.compare(kMulticastMsgType) == 0) {
      OnReceived(json_string);
    }
  });
  session_->AddProtobufRecvCallback([this](const std::shared_ptr<fun::FunapiSession> &session,
                                       const fun::TransportProtocol protocol,
                                       const FunMessage &message) {
    if (message.msgtype().compare(kMulticastMsgType) == 0) {
      OnReceived(message);
    }
  });

  AddJoinedCallback([this](const std::shared_ptr<fun::FunapiMulticast>& multicast,
                           const std::string &channel_id, const std::string &multicast_sender) {
    // fun::DebugUtils::Log("JoinedCallback called. channel_id:%s player:%s", channel_id.c_str(), sender.c_str());
    if (sender_.compare(multicast_sender) == 0) {
      std::unique_lock<std::mutex> lock(channels_mutex_);
      channels_.insert(channel_id);
    }
  });
}


FunapiMulticastImpl::~FunapiMulticastImpl() {
}


void FunapiMulticastImpl::AddJoinedCallback(const ChannelNotify &handler) {
  on_joined_ += handler;
}


void FunapiMulticastImpl::AddLeftCallback(const ChannelNotify &handler) {
  on_left_ += handler;
}


void FunapiMulticastImpl::AddErrorCallback(const ErrorNotify &handler) {
  on_error_ += handler;
}


void FunapiMulticastImpl::AddChannelListCallback(const ChannelListNotify &handler) {
  on_channel_list_ += handler;
}


void FunapiMulticastImpl::OnJoined(const std::string &channel_id, const std::string &sender) {
  if (auto m = multicast_.lock()) {
    on_joined_(m, channel_id, sender);
  }
}


void FunapiMulticastImpl::OnLeft(const std::string &channel_id, const std::string &sender) {
  if (auto m = multicast_.lock()) {
    on_left_(m, channel_id, sender);
  }
}


void FunapiMulticastImpl::OnError(const int error) {
  if (auto m = multicast_.lock()) {
    on_error_(m, error);
  }
}


void FunapiMulticastImpl::OnChannelList(const std::map<std::string, int> &cl) {
  if (auto m = multicast_.lock()) {
    on_channel_list_(m, cl);
  }
}


void FunapiMulticastImpl::OnChannelList(FunMulticastMessage *msg) {
  std::map<std::string, int> cl;

  for (int i=0;i<msg->channels_size();++i) {
    cl[msg->channels(i).channel_name()] = msg->channels(i).num_members();
  }

  OnChannelList(cl);
}


void FunapiMulticastImpl::OnChannelList(const rapidjson::Document &msg) {
  const rapidjson::Value &d = msg[kChannelListId];

  std::map<std::string, int> cl;

  for (unsigned int i=0;i<d.Size();++i) {
    const rapidjson::Value &v = d[i];
    if (v.HasMember("_name") && v.HasMember("_members")) {
      cl[v["_name"].GetString()] = v["_members"].GetInt();
    }
  }

  OnChannelList(cl);
}


bool FunapiMulticastImpl::IsConnected() const {
  if (session_) {
    if (session_->IsConnected()) {
      return true;
    }
  }

  return false;
}


bool FunapiMulticastImpl::IsInChannel(const std::string &channel_id) const {
  if (channels_.find(channel_id) != channels_.end()) {
    return true;
  }

  return false;
}


bool FunapiMulticastImpl::JoinChannel(const std::string &channel_id) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. First connect before join a multicast channel.");
    return false;
  }

  if (IsInChannel(channel_id)) {
    DebugUtils::Log("Already joined the channel: %s", channel_id.c_str());
    return false;
  }

  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value join_node(true);
    msg.AddMember(rapidjson::StringRef(kJoin), join_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_join(true);
    mcast_msg->set_sender(sender_.c_str());

    session_->SendMessage(msg);
  }

  return true;
}


bool FunapiMulticastImpl::LeaveChannel(const std::string &channel_id) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  SendLeaveMessage(channel_id);
  OnLeft(channel_id, sender_);

  {
    std::unique_lock<std::mutex> lock(channels_mutex_);
    channels_.erase(channel_id);
  }

  json_channel_handlers_.erase(channel_id);
  protobuf_channel_handlers_.erase(channel_id);

  return true;
}


bool FunapiMulticastImpl::LeaveAllChannels() {
  auto channels_set(channels_);
  for (auto c : channels_set) {
    LeaveChannel(c);
  }

  return true;
}


bool FunapiMulticastImpl::SendToChannel(const std::string &channel_id, FunMessage &msg, const bool bounce) {
  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
  mcast_msg->set_channel(channel_id.c_str());
  mcast_msg->set_bounce(bounce);
  mcast_msg->set_sender(sender_.c_str());

  msg.set_msgtype(kMulticastMsgType);

  session_->SendMessage(msg);

  return true;
}


bool FunapiMulticastImpl::SendToChannel(const std::string &channel_id, std::string &json_string, const bool bounce) {
  if (!IsConnected()) {
    DebugUtils::Log("Not connected. If you are trying to leave a channel in which you were, connect first while preserving the session id you used for join.");
    return false;
  }

  rapidjson::Document msg;
  msg.Parse<0>(json_string.c_str());

  if (!IsInChannel(channel_id)) {
    DebugUtils::Log("You are not in the channel: %s", channel_id.c_str());
    return false;
  }

  rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef("_channel"), channel_id_node, msg.GetAllocator());

  rapidjson::Value bounce_node(bounce);
  msg.AddMember(rapidjson::StringRef("_bounce"), bounce_node, msg.GetAllocator());

  rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
  msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

  // Convert JSON document to string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  msg.Accept(writer);
  std::string send_json_string = buffer.GetString();

  session_->SendMessage(kMulticastMsgType, send_json_string);

  return true;
}


void FunapiMulticastImpl::SendLeaveMessage(const std::string &channel_id) {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value channel_id_node(channel_id.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kChannelId), channel_id_node, msg.GetAllocator());

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    rapidjson::Value leave_node(true);
    msg.AddMember(rapidjson::StringRef(kLeave), leave_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_channel(channel_id.c_str());
    mcast_msg->set_leave(true);
    mcast_msg->set_sender(sender_.c_str());

    session_->SendMessage(msg);
  }
}


void FunapiMulticastImpl::OnReceived(const std::string &json_string) {
  std::string channel_id = "";
  std::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  if (encoding_ == fun::FunEncoding::kJson) {
    // log
    // std::string body(v_body.cbegin(), v_body.cend());
    // fun::DebugUtils::Log("multicast message\n%s", body.c_str());
    // //

    rapidjson::Document msg;
    msg.Parse<0>(json_string.c_str());

    if (msg.HasMember(kChannelListId)) {
      OnChannelList(msg);
      return;
    }

    if (msg.HasMember(kChannelId)) {
      channel_id = msg[kChannelId].GetString();
    }

    if (msg.HasMember(kSender)) {
      sender = msg[kSender].GetString();
    }

    if (msg.HasMember(kErrorCode)) {
      error_code = msg[kErrorCode].GetInt();
    }

    if (msg.HasMember(kJoin)) {
      join = msg[kJoin].GetBool();
    }

    if (msg.HasMember(kLeave)) {
      leave = msg[kLeave].GetBool();
    }
  }

  if (OnReceived(channel_id, sender, join, leave, error_code) == false) {
    if (json_channel_handlers_.find(channel_id) != json_channel_handlers_.end()) {
      if (auto m = multicast_.lock()) {
        json_channel_handlers_[channel_id](m, channel_id, sender, json_string);
      }
    }
  }
}


void FunapiMulticastImpl::OnReceived(const FunMessage &message) {
  std::string channel_id = "";
  std::string sender = "";
  bool join = false;
  bool leave = false;
  int error_code = 0;

  FunMulticastMessage mcast_msg = message.GetExtension(multicast);

  channel_id = mcast_msg.channel();

  if (mcast_msg.has_sender()) {
    sender = mcast_msg.sender();
  }

  if (mcast_msg.has_error_code()) {
    error_code = mcast_msg.error_code();
  }

  if (mcast_msg.has_join()) {
    join = mcast_msg.join();
  }

  if (mcast_msg.has_leave()) {
    leave = mcast_msg.leave();
  }

  if (channel_id.length() == 0 && sender.length() == 0) {
    OnChannelList(&mcast_msg);
    return;
  }

  if (OnReceived(channel_id, sender, join, leave, error_code) == false) {
    if (protobuf_channel_handlers_.find(channel_id) != protobuf_channel_handlers_.end()) {
      if (auto m = multicast_.lock()) {
        protobuf_channel_handlers_[channel_id](m, channel_id, sender, message);
      }
    }
  }
}


bool FunapiMulticastImpl::OnReceived(const std::string &channel_id, const std::string &sender, const bool join, const bool leave, const int error_code) {
  if (error_code != 0) {
    OnError(error_code);
  }

  if (join) {
    // DebugUtils::Log("%s joined the '%s' channel", sender.c_str(), channel_id.c_str());
    OnJoined(channel_id, sender);
  }
  else if (leave) {
    // DebugUtils::Log("%s left the '%s' channel", sender.c_str(), channel_id.c_str());
    OnLeft(channel_id, sender);
  }
  else {
    return false;
  }

  return true;
}


bool FunapiMulticastImpl::RequestChannelList() {
  if (encoding_ == FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();

    rapidjson::Value sender_node(sender_.c_str(), msg.GetAllocator());
    msg.AddMember(rapidjson::StringRef(kSender), sender_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    session_->SendMessage(kMulticastMsgType, json_string);
  }

  if (encoding_ == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kMulticastMsgType);

    FunMulticastMessage *mcast_msg = msg.MutableExtension(multicast);
    mcast_msg->set_sender(sender_.c_str());

    session_->SendMessage(msg);
  }

  return true;
}


void FunapiMulticastImpl::Update() {
  if (session_) {
    session_->Update();
  }
}


FunEncoding FunapiMulticastImpl::GetEncoding() {
  return encoding_;
}


void FunapiMulticastImpl::AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler) {
  if (session_) {
    session_->AddSessionEventCallback([this, handler](const std::shared_ptr<fun::FunapiSession> &session,
                                                      const fun::TransportProtocol protocol,
                                                      const fun::SessionEventType type,
                                                      const std::string &session_id) {
      if (auto m = multicast_.lock()) {
        handler(m, type, session_id);
      }
    });
  }
}


void FunapiMulticastImpl::AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler) {
  if (session_) {
    session_->AddTransportEventCallback([this, handler](const std::shared_ptr<fun::FunapiSession> &session,
                                                        const fun::TransportProtocol protocol,
                                                        const fun::TransportEventType type) {
      if (auto m = multicast_.lock()) {
        handler(m, type);
      }
    });
  }
}


void FunapiMulticastImpl::Connect(const std::weak_ptr<FunapiMulticast>& multicast) {
  multicast_ = multicast;

  if (session_) {
    session_->Connect(TransportProtocol::kTcp, port_, encoding_);
  }
}


void FunapiMulticastImpl::Close() {
  if (session_) {
    session_->Close();
  }
}


void FunapiMulticastImpl::AddProtobufChannelMessageCallback(const std::string &channel_id,
                                                        const ProtobufChannelMessageHandler &handler) {
  protobuf_channel_handlers_[channel_id] += handler;
}


void FunapiMulticastImpl::AddJsonChannelMessageCallback(const std::string &channel_id,
                                                    const JsonChannelMessageHandler &handler) {
  json_channel_handlers_[channel_id] += handler;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiMulticast implementation.

FunapiMulticast::FunapiMulticast(const char* sender, const char* hostname_or_ip, uint16_t port, FunEncoding encoding, bool reliability)
: impl_(std::make_shared<FunapiMulticastImpl>(sender, hostname_or_ip, port, encoding, reliability)) {
}


FunapiMulticast::~FunapiMulticast() {
}


std::shared_ptr<FunapiMulticast> FunapiMulticast::create(const char* sender, const char* hostname_or_ip, uint16_t port, FunEncoding encoding, bool reliability) {
  return std::make_shared<FunapiMulticast>(sender, hostname_or_ip, port, encoding, reliability);
}


void FunapiMulticast::AddJoinedCallback(const ChannelNotify &handler) {
  impl_->AddJoinedCallback(handler);
}


void FunapiMulticast::AddLeftCallback(const ChannelNotify &handler) {
  impl_->AddLeftCallback(handler);
}


void FunapiMulticast::AddErrorCallback(const ErrorNotify &handler) {
  impl_->AddErrorCallback(handler);
}


void FunapiMulticast::AddChannelListCallback(const ChannelListNotify &handler) {
  impl_->AddChannelListCallback(handler);
}


bool FunapiMulticast::IsConnected() const {
  return impl_->IsConnected();
}


bool FunapiMulticast::IsInChannel(const std::string &channel_id) const {
  return impl_->IsInChannel(channel_id);
}


bool FunapiMulticast::LeaveChannel(const std::string &channel_id) {
  return impl_->LeaveChannel(channel_id);
}


bool FunapiMulticast::LeaveAllChannels() {
  return impl_->LeaveAllChannels();
}


bool FunapiMulticast::SendToChannel(const std::string &channel_id, FunMessage &msg, const bool bounce) {
  return impl_->SendToChannel(channel_id, msg);
}


bool FunapiMulticast::SendToChannel(const std::string &channel_id, std::string &json_string, const bool bounce) {
  return impl_->SendToChannel(channel_id, json_string);
}


bool FunapiMulticast::RequestChannelList() {
  return impl_->RequestChannelList();
}


void FunapiMulticast::Update() {
  auto self = shared_from_this();
  impl_->Update();
}


FunEncoding FunapiMulticast::GetEncoding() {
  return impl_->GetEncoding();
}


void FunapiMulticast::AddSessionEventCallback(const FunapiMulticast::SessionEventHandler &handler) {
  impl_->AddSessionEventCallback(handler);
}


void FunapiMulticast::AddTransportEventCallback(const FunapiMulticast::TransportEventHandler &handler) {
  impl_->AddTransportEventCallback(handler);
}


void FunapiMulticast::Connect() {
  impl_->Connect(shared_from_this());
}


void FunapiMulticast::Close() {
  impl_->Close();
}


bool FunapiMulticast::JoinChannel(const std::string &channel_id) {
  return impl_->JoinChannel(channel_id);
}


void FunapiMulticast::AddProtobufChannelMessageCallback(const std::string &channel_id,
                                                        const ProtobufChannelMessageHandler &handler) {
  impl_->AddProtobufChannelMessageCallback(channel_id, handler);
}


void FunapiMulticast::AddJsonChannelMessageCallback(const std::string &channel_id,
                                                    const JsonChannelMessageHandler &handler) {
  impl_->AddJsonChannelMessageCallback(channel_id, handler);
}

}  // namespace fun
