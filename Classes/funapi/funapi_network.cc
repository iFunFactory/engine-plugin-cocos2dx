// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "pb/network/ping_message.pb.h"
#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_network.h"
#include "funapi_utils.h"
#include "funapi_manager.h"

namespace fun {

// Funapi message-related constants.
static const char* kMsgTypeBodyField = "_msgtype";
static const char* kSessionIdBodyField = "_sid";
static const char* kNewSessionMessageType = "_session_opened";
static const char* kSessionClosedMessageType = "_session_closed";

// Ping message-related constants.
static const char* kServerPingMessageType = "_ping_s";
static const char* kClientPingMessageType = "_ping_c";
static const char* kPingTimestampField = "timestamp";

////////////////////////////////////////////////////////////////////////////////
// FunapiNetworkImpl implementation.

class FunapiNetworkImpl : public std::enable_shared_from_this<FunapiNetworkImpl> {
 public:
  typedef FunapiTransport::HeaderType HeaderType;
  typedef FunapiNetwork::MessageEventHandler MessageEventHandler;
  typedef FunapiNetwork::SessionInitHandler SessionInitHandler;
  typedef FunapiNetwork::SessionCloseHandler SessionCloseHandler;
  typedef FunapiNetwork::NotifyHandler NotifyHandler;
  typedef FunapiTransport::TransportEventHandler TransportEventHandler;

  FunapiNetworkImpl(const bool &session_reliability = false);
  ~FunapiNetworkImpl();

  void RegisterHandler(const std::string &msg_type, const MessageEventHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol);
  void SendMessage(FunMessage& message, TransportProtocol protocol);
  void SendMessage(const char *body, TransportProtocol protocol);
  bool Started() const;
  bool Connected(const TransportProtocol protocol);
  void Update();
  void AttachTransport(const std::shared_ptr<FunapiTransport> &transport, const std::weak_ptr<FunapiNetwork> &network);
  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  void PushTaskQueue(const std::function<void()> &task);

  void AddSessionInitiatedCallback(const SessionInitHandler &handler);
  void AddSessionClosedCallback(const SessionCloseHandler &handler);

  void AddMaintenanceCallback(const MessageEventHandler &handler);
  void AddStoppedAllTransportCallback(const NotifyHandler &handler);
  void AddTransportConnectFailedCallback(const TransportEventHandler &handler);
  void AddTransportDisconnectedCallback(const TransportEventHandler &handler);

  bool HasTransport(const TransportProtocol protocol) const;
  void SetDefaultProtocol(const TransportProtocol protocol);

  FunEncoding GetEncoding(const TransportProtocol protocol) const;
  TransportProtocol GetDefaultProtocol() const;

  void SendEmptyMessage(const TransportProtocol protocol, const FunEncoding encoding);
  bool SendClientPingMessage(const TransportProtocol protocol);

 private:
  void OnSessionInitiated(const std::string &session_id);
  void OnSessionClosed();

  void OnMaintenance(const TransportProtocol, const std::string &, const std::vector<uint8_t> &);
  void OnStoppedAllTransport();
  void OnTransportConnectFailed(const TransportProtocol protocol);
  void OnTransportDisconnected(const TransportProtocol protocol);

  void OnTransportReceived(const TransportProtocol protocol, const FunEncoding encoding, const HeaderType &header, const std::vector<uint8_t> &body);
  void OnTransportStopped();
  void OnNewSession(const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnSessionTimedout(const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void OnServerPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnClientPingMessage(const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void Initialize();
  void Finalize();

  bool started_;

  // Transport-releated member variables.
  std::map<TransportProtocol, std::shared_ptr<FunapiTransport>> transports_;
  mutable std::mutex transports_mutex_;
  TransportProtocol default_protocol_ = TransportProtocol::kDefault;

  std::string session_id_;
  typedef std::map<std::string, MessageEventHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_;

  bool initialized_ = false;
  time_t epoch_ = 0;
  const time_t funapi_session_timeout_ = 3600;

  std::queue<std::function<void()>> tasks_queue_;
  std::mutex tasks_queue_mutex_;

  // Funapi message-related events.
  FEvent<SessionInitHandler> on_session_initiated_;
  FEvent<SessionCloseHandler> on_session_closed_;
  FEvent<MessageEventHandler> on_maintenance_;
  FEvent<NotifyHandler> on_stopped_all_transport_;
  FEvent<TransportEventHandler> on_transport_connect_failed_;
  FEvent<TransportEventHandler> on_transport_disconnected_;

  bool session_reliability_ = false;

  static std::shared_ptr<FunapiManager> GetManager();

  void InsertTransport(const TransportProtocol protocol);
  void EraseTransport(const TransportProtocol protocol);
};


FunapiNetworkImpl::FunapiNetworkImpl(const bool &session_reliability)
  : session_id_(""), last_received_(0), session_reliability_(session_reliability) {

  Initialize();
}


FunapiNetworkImpl::~FunapiNetworkImpl() {
  Finalize();
  Stop();
  Update();
}


std::shared_ptr<FunapiManager> FunapiNetworkImpl::GetManager() {
  static std::shared_ptr<FunapiManager> manager = std::make_shared<FunapiManager>();
  return manager;
}


void FunapiNetworkImpl::Initialize() {
  assert(!initialized_);

  // Now ready.
  initialized_ = true;
}


void FunapiNetworkImpl::Finalize() {
  assert(initialized_);

  // All set.
  initialized_ = false;
}


void FunapiNetworkImpl::RegisterHandler(
    const std::string &msg_type, const MessageEventHandler &handler) {
  FUNAPI_LOG("New handler for message type %s", msg_type.c_str());
  message_handlers_[msg_type] = handler;
}


void FunapiNetworkImpl::Start() {
  // Installs event handlers.
  message_handlers_[kNewSessionMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnNewSession(s, v); };
  message_handlers_[kSessionClosedMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnSessionTimedout(s, v); };

  // ping
  message_handlers_[kServerPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnServerPingMessage(p, s, v); };
  message_handlers_[kClientPingMessageType] =
    [this](const TransportProtocol &p, const std::string&s, const std::vector<uint8_t>&v) { OnClientPingMessage(p, s, v); };

  // Then, asks the transport to work.
  FUNAPI_LOG("Starting a network module.");
  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto iter : transports_) {
      if (!iter.second->Started())
        iter.second->Start();
    }
  }

  // Ok. We are ready.
  started_ = true;
  last_received_ = epoch_ = time(NULL);
}


void FunapiNetworkImpl::Stop() {
  FUNAPI_LOG("Stopping a network module.");

  if (!started_)
    return;

  // Turns off the flag.
  started_ = false;

  // Then, requests the transport to stop.
  {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    for (auto iter : transports_) {
      if (iter.second->Started())
        iter.second->Stop();
    }
  }
}


void FunapiNetworkImpl::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    FUNAPI_LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  rapidjson::Document body;
  body.Parse<0>(json_string.c_str());

  // Encodes a messsage type.
  rapidjson::Value msg_type_node;
  msg_type_node.SetString(rapidjson::StringRef(msg_type.c_str()));
  body.AddMember(rapidjson::StringRef(kMsgTypeBodyField), msg_type_node, body.GetAllocator());

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    rapidjson::Value session_id_node;
    session_id_node.SetString(rapidjson::StringRef(session_id_.c_str()));
    body.AddMember(rapidjson::StringRef(kSessionIdBodyField), session_id_node, body.GetAllocator());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated JSON object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport) {
    transport->SendMessage(body);
  }
  else {
    FUNAPI_LOG("Invaild Protocol - Transport is not founded");
  }
}


void FunapiNetworkImpl::SendMessage(FunMessage& message, TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    FUNAPI_LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    message.set_sid(session_id_.c_str());
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport) {
    transport->SendMessage(message);
  }
  else {
    FUNAPI_LOG("Invaild Protocol - Transport is not founded");
  }
}


void FunapiNetworkImpl::SendMessage(const char *body, TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    FUNAPI_LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  if (protocol == TransportProtocol::kDefault)
    protocol = default_protocol_;

  // Sends the manipulated Protobuf object through the transport.
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport) {
    transport->SendMessage(body);
  }
  else {
    FUNAPI_LOG("Invaild Protocol - Transport is not founded");
  }
}


bool FunapiNetworkImpl::Started() const {
  return started_;
}


bool FunapiNetworkImpl::Connected(const TransportProtocol protocol = TransportProtocol::kDefault) {
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);

  if (transport)
    return transport->Started();

  return false;
}


void FunapiNetworkImpl::OnTransportReceived(
  const TransportProtocol protocol, const FunEncoding encoding, const HeaderType &header, const std::vector<uint8_t> &body) {
  FUNAPI_LOG("OnReceived invoked");

  last_received_ = time(NULL);

  std::string msg_type;
  std::string session_id;

  if (encoding == FunEncoding::kJson) {
    // Parses the given json string.
    rapidjson::Document json;
    json.Parse<0>((char*)(body.data()));
    assert(json.IsObject());

    const rapidjson::Value &msg_type_node = json[kMsgTypeBodyField];
    assert(msg_type_node.IsString());
    msg_type = msg_type_node.GetString();
    json.RemoveMember(kMsgTypeBodyField);

    const rapidjson::Value &session_id_node = json[kSessionIdBodyField];
    assert(session_id_node.IsString());
    session_id = session_id_node.GetString();
    json.RemoveMember(kSessionIdBodyField);

  } else if (encoding == FunEncoding::kProtobuf) {
    FunMessage proto;
    proto.ParseFromArray(body.data(), (int)body.size());

    msg_type = proto.msgtype();
    session_id = proto.sid();
  }

  if (session_id_.empty()) {
    session_id_ = session_id;
    FUNAPI_LOG("New session id: %s", session_id.c_str());
    OnSessionInitiated(session_id);
  }

  if (session_id_.compare(session_id) != 0) {
    FUNAPI_LOG("Session id changed: %s => %s", session_id_.c_str(), session_id.c_str());
    session_id_ = session_id;
    OnSessionClosed();
    OnSessionInitiated(session_id);
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it == message_handlers_.end()) {
    FUNAPI_LOG("No handler for message '%s'. Ignoring.", msg_type.c_str());
  } else {
    it->second(protocol, msg_type, body);
  }
}


void FunapiNetworkImpl::OnTransportStopped() {
  FUNAPI_LOG("Transport terminated. Stopping. You may restart again.");
  Stop();
  OnStoppedAllTransport();
}


void FunapiNetworkImpl::OnNewSession(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  // ignore
}


void FunapiNetworkImpl::OnSessionTimedout(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  FUNAPI_LOG("Session timed out. Resetting my session id. The server will send me another one next time.");

  session_id_ = "";

  OnSessionClosed();
}


void FunapiNetworkImpl::OnServerPingMessage(
  const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body) {

  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  std::string body(v_body.cbegin(), v_body.cend());

  if (encoding == FunEncoding::kJson)
  {
    SendMessage(body.c_str(), protocol);
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage msg;
    msg.ParseFromString(body);

    SendMessage(msg, protocol);
  }
}


void FunapiNetworkImpl::OnClientPingMessage(
  const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t>&v_body) {

  fun::FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  std::string body(v_body.cbegin(), v_body.cend());
  int64_t timestamp_ms;

  if (encoding == FunEncoding::kJson)
  {
    rapidjson::Document document;
    document.Parse<0>(body.c_str());

    timestamp_ms = document[kPingTimestampField].GetInt64();
  }

  if (encoding == FunEncoding::kProtobuf)
  {
    FunMessage msg;
    msg.ParseFromString(body);

    FunPingMessage ping_message = msg.GetExtension(cs_ping);
    timestamp_ms = ping_message.timestamp();
  }

  int64_t ping_time_ms = (std::chrono::system_clock::now().time_since_epoch().count() - timestamp_ms) / 1000;

  FUNAPI_LOG("Receive %s ping - timestamp:%lld time=%lld ms", "TCP", (int64_t)timestamp_ms, ping_time_ms);

  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
  if (transport) {
    transport->ResetPingClientTimeout();
  }
  else {
    FUNAPI_LOG("Invaild Protocol - Transport is not founded");
  }
}


void FunapiNetworkImpl::Update() {
#ifndef FUNAPI_HAVE_THREAD
  GetManager()->Update();
#endif // FUNAPI_HAVE_THREAD

  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  while (!tasks_queue_.empty())
  {
    (tasks_queue_.front())();
    tasks_queue_.pop();
  }
}


void FunapiNetworkImpl::AttachTransport(const std::shared_ptr<FunapiTransport> &transport, const std::weak_ptr<FunapiNetwork> &network) {
  transport->RegisterEventHandlers(
    [this](const TransportProtocol protocol, const FunEncoding encoding, const HeaderType &header, const std::vector<uint8_t> &body){ OnTransportReceived(protocol, encoding, header, body); },
    [this](){ OnTransportStopped(); });
  transport->SetNetwork(network);

  if (transport->GetProtocol() == TransportProtocol::kHttp) {
    transport->AddStartedCallback([this](const TransportProtocol protocol){ InsertTransport(protocol); });
    transport->AddStoppedCallback([this](const TransportProtocol protocol){ EraseTransport(protocol); });
  }
  else {
    transport->AddInitSocketCallback([this](const TransportProtocol protocol){ InsertTransport(protocol); });
    transport->AddCloseSocketCallback([this](const TransportProtocol protocol){ EraseTransport(protocol); });
  }

  if (HasTransport(transport->GetProtocol()))
  {
    FUNAPI_LOG("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->GetProtocol()));
    FUNAPI_LOG(" You should call DetachTransport first.");
  }
  else {
    std::unique_lock<std::mutex> lock(transports_mutex_);
    transports_[transport->GetProtocol()] = transport;

    if (default_protocol_ == TransportProtocol::kDefault)
    {
      SetDefaultProtocol(transport->GetProtocol());
    }
  }
}


std::shared_ptr<FunapiTransport> FunapiNetworkImpl::GetTransport(const TransportProtocol protocol) const {
  std::unique_lock<std::mutex> lock(transports_mutex_);
  if (protocol == TransportProtocol::kDefault) {
    return transports_.cbegin()->second;
  }

  auto iter = transports_.find(protocol);
  if (iter != transports_.cend())
    return iter->second;

  return nullptr;
}


void FunapiNetworkImpl::PushTaskQueue(const std::function<void()> &task)
{
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  tasks_queue_.push(task);
}


void FunapiNetworkImpl::AddSessionInitiatedCallback(const SessionInitHandler &handler) {
  on_session_initiated_ += handler;
}


void FunapiNetworkImpl::AddSessionClosedCallback(const SessionCloseHandler &handler) {
  on_session_closed_ += handler;
}


void FunapiNetworkImpl::AddMaintenanceCallback(const MessageEventHandler &handler) {
  on_maintenance_ += handler;
}


void FunapiNetworkImpl::AddStoppedAllTransportCallback(const NotifyHandler &handler) {
  on_stopped_all_transport_ += handler;
}


void FunapiNetworkImpl::AddTransportConnectFailedCallback(const TransportEventHandler &handler) {
  on_transport_connect_failed_ += handler;
}


void FunapiNetworkImpl::AddTransportDisconnectedCallback(const TransportEventHandler &handler) {
  on_transport_disconnected_ += handler;
}


void FunapiNetworkImpl::OnSessionInitiated(const std::string &session_id) {
  on_session_initiated_(session_id_);
}


void FunapiNetworkImpl::OnSessionClosed() {
  on_session_closed_();
}


void FunapiNetworkImpl::OnMaintenance (const TransportProtocol protocol, const std::string &msg_type, const std::vector<uint8_t> &v_body) {
  on_maintenance_(protocol, msg_type, v_body);
}


void FunapiNetworkImpl::OnStoppedAllTransport() {
  on_stopped_all_transport_();
}


void FunapiNetworkImpl::OnTransportConnectFailed(const TransportProtocol protocol) {
  on_transport_connect_failed_(protocol);
}


void FunapiNetworkImpl::OnTransportDisconnected(const TransportProtocol protocol) {
  on_transport_disconnected_(protocol);
}


bool FunapiNetworkImpl::HasTransport(const TransportProtocol protocol) const {
  if (GetTransport(protocol)) {
    return true;
  }

  return false;
}


void FunapiNetworkImpl::SetDefaultProtocol(const TransportProtocol protocol) {
  default_protocol_ = protocol;
}


FunEncoding FunapiNetworkImpl::GetEncoding(const TransportProtocol protocol) const {
  auto transport = GetTransport(protocol);
  if (transport) {
    return transport->GetEncoding();
  }

  return FunEncoding::kNone;
}


TransportProtocol FunapiNetworkImpl::GetDefaultProtocol() const {
  return default_protocol_;
}


void FunapiNetworkImpl::SendEmptyMessage(const TransportProtocol protocol, const FunEncoding encoding) {
  if (session_id_.empty()) {
    assert(encoding!=FunEncoding::kNone);

    if (encoding == FunEncoding::kJson) {
      SendMessage("{}");
    }
    else if (encoding == FunEncoding::kProtobuf) {
      FunMessage msg;
      SendMessage(msg);
    }
  }
}


bool FunapiNetworkImpl::SendClientPingMessage(const TransportProtocol protocol) {
  assert(protocol==TransportProtocol::kTcp);

  if (session_id_.empty())
    return false;

  FunEncoding encoding = GetEncoding(protocol);
  assert(encoding!=FunEncoding::kNone);

  int64_t timestamp = std::chrono::system_clock::now().time_since_epoch().count();
  FUNAPI_LOG("Send TCP ping - timestamp: %lld", timestamp);

  if (encoding == FunEncoding::kProtobuf) {
    FunMessage msg;
    msg.set_msgtype(kClientPingMessageType);
    FunPingMessage *ping_message = msg.MutableExtension(cs_ping);
    ping_message->set_timestamp(timestamp);
    // ping_message->set_data("77777777");
    SendMessage(msg, protocol);
  }
  else if (encoding == fun::FunEncoding::kJson) {
    rapidjson::Document msg;
    msg.SetObject();
    rapidjson::Value key(kPingTimestampField, msg.GetAllocator());
    rapidjson::Value timestamp_node(rapidjson::kNumberType);
    timestamp_node.SetInt64(timestamp);
    msg.AddMember(key, timestamp_node, msg.GetAllocator());

    // Convert JSON document to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    msg.Accept(writer);
    std::string json_string = buffer.GetString();

    SendMessage(kClientPingMessageType, json_string, protocol);
  }

  return true;
}


void FunapiNetworkImpl::InsertTransport(const TransportProtocol protocol) {
  auto transport = GetTransport(protocol);
  if (transport) {
    GetManager()->InsertTransport(transport);
  }
}


void FunapiNetworkImpl::EraseTransport(const TransportProtocol protocol) {
  auto transport = GetTransport(protocol);
  if (transport) {
    GetManager()->EraseTransport(GetTransport(protocol));
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiNetwork implementation.

FunapiNetwork::FunapiNetwork(bool session_reliability)
  : impl_(std::make_shared<FunapiNetworkImpl>(session_reliability)) {
}


FunapiNetwork::~FunapiNetwork() {
}


void FunapiNetwork::RegisterHandler(
    const std::string msg_type, const MessageEventHandler &handler) {
  return impl_->RegisterHandler(msg_type, handler);
}


void FunapiNetwork::Start() {
  return impl_->Start();
}


void FunapiNetwork::Stop() {
  return impl_->Stop();
}


void FunapiNetwork::SendMessage(const std::string &msg_type, std::string &json_string, TransportProtocol protocol) {
  return impl_->SendMessage(msg_type, json_string, protocol);
}


void FunapiNetwork::SendMessage(FunMessage& message, TransportProtocol protocol) {
  return impl_->SendMessage(message, protocol);
}


bool FunapiNetwork::Started() const {
  return impl_->Started();
}


bool FunapiNetwork::Connected(const TransportProtocol protocol) const {
  return impl_->Connected(protocol);
}


void FunapiNetwork::Update() {
  return impl_->Update();
}


void FunapiNetwork::AttachTransport(const std::shared_ptr<FunapiTransport> &transport) {
  return impl_->AttachTransport(transport, shared_from_this());
}


void FunapiNetwork::PushTaskQueue(const std::function<void()> &task)
{
  return impl_->PushTaskQueue(task);
}


void FunapiNetwork::AddSessionInitiatedCallback(const SessionInitHandler &handler) {
  return impl_->AddSessionInitiatedCallback(handler);
}


void FunapiNetwork::AddSessionClosedCallback(const SessionCloseHandler &handler) {
  return impl_->AddSessionClosedCallback(handler);
}


void FunapiNetwork::AddMaintenanceCallback(const MessageEventHandler &handler) {
  return impl_->AddMaintenanceCallback(handler);
}


void FunapiNetwork::AddStoppedAllTransportCallback(const NotifyHandler &handler) {
  return impl_->AddStoppedAllTransportCallback(handler);
}


void FunapiNetwork::AddTransportConnectFailedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportConnectFailedCallback(handler);
}


void FunapiNetwork::AddTransportDisconnectedCallback(const TransportEventHandler &handler) {
  return impl_->AddTransportDisconnectedCallback(handler);
}


bool FunapiNetwork::HasTransport(const TransportProtocol protocol) const {
  return impl_->HasTransport(protocol);
}


void FunapiNetwork::SetDefaultProtocol(const TransportProtocol protocol) {
  return impl_->SetDefaultProtocol(protocol);
}


bool FunapiNetwork::IsSessionReliability() const {
  // return impl_->SessionReliability();
  return false;
}


FunEncoding FunapiNetwork::GetEncoding(const TransportProtocol protocol) const {
  return impl_->GetEncoding(protocol);
}


TransportProtocol FunapiNetwork::GetDefaultProtocol() const {
  return impl_->GetDefaultProtocol();
}


void FunapiNetwork::SendEmptyMessage(const TransportProtocol protocol, const FunEncoding encoding) {
  return impl_->SendEmptyMessage(protocol, encoding);
}


bool FunapiNetwork::SendClientPingMessage(const TransportProtocol protocol) {
  return impl_->SendClientPingMessage(protocol);
}

}  // namespace fun
