// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_network.h"


namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiNetworkImpl implementation.

class FunapiNetworkImpl : public std::enable_shared_from_this<FunapiNetworkImpl> {
 public:
  typedef FunapiTransport::HeaderType HeaderType;
  typedef FunapiNetwork::MessageHandler MessageHandler;
  typedef FunapiNetwork::OnSessionInitiated OnSessionInitiated;
  typedef FunapiNetwork::OnSessionClosed OnSessionClosed;

  FunapiNetworkImpl(int type,
                    OnSessionInitiated on_session_initiated,
                    OnSessionClosed on_session_closed);

  ~FunapiNetworkImpl();

  void RegisterHandler(const std::string msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol);
  // void SendMessage(const std::string &msg_type, FJsonObject &body, const TransportProtocol protocol);
  void SendMessage(FunMessage& message, const TransportProtocol protocol);
  bool Started() const;
  bool Connected(const TransportProtocol protocol);
  void Update();
  void AttachTransport(std::shared_ptr<FunapiTransport> transport, std::weak_ptr<FunapiNetwork> network);
  std::shared_ptr<FunapiTransport> GetTransport(const TransportProtocol protocol) const;
  void PushTaskQueue(std::function<void()> task);

  // Funapi message-related constants.
  const std::string kMsgTypeBodyField = "_msgtype";
  const std::string kSessionIdBodyField = "_sid";
  const std::string kNewSessionMessageType = "_session_opened";
  const std::string kSessionClosedMessageType = "_session_closed";

 private:
  void OnTransportReceived(const HeaderType &header, const std::vector<uint8_t> &body);
  void OnTransportStopped();
  void OnNewSession(const std::string &msg_type, const std::vector<uint8_t>&v_body);
  void OnSessionTimedout(const std::string &msg_type, const std::vector<uint8_t>&v_body);

  void Initialize();
  void Finalize();

  bool started_;
  int encoding_type_;
  std::map<TransportProtocol, std::shared_ptr<FunapiTransport>> transports_;
  OnSessionInitiated on_session_initiated_;
  OnSessionClosed on_session_closed_;
  std::string session_id_;
  typedef std::map<std::string, MessageHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_;
  std::mutex mutex_transports_;

  bool initialized_ = false;
  time_t epoch_ = 0;
  const time_t funapi_session_timeout_ = 3600;

  std::queue<std::function<void()>> tasks_queue_;
  std::mutex tasks_queue_mutex_;
};



FunapiNetworkImpl::FunapiNetworkImpl(int type,
                                     OnSessionInitiated on_session_initiated,
                                     OnSessionClosed on_session_closed)
    : started_(false), encoding_type_(type),
      on_session_initiated_(on_session_initiated),
      on_session_closed_(on_session_closed),
      session_id_(""), last_received_(0) {

  Initialize();
}


FunapiNetworkImpl::~FunapiNetworkImpl() {
  Finalize();
  Stop();
  Update();
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
    const std::string msg_type, const MessageHandler &handler) {
  FUNAPI_LOG("New handler for message type %s", msg_type.c_str());
  message_handlers_[msg_type] = handler;
}


void FunapiNetworkImpl::Start() {
  // Installs event handlers.
  message_handlers_[kNewSessionMessageType] =
    [this](const std::string&s, const std::vector<uint8_t>&v) { OnNewSession(s, v); };
  message_handlers_[kSessionClosedMessageType] =
    [this](const std::string&s, const std::vector<uint8_t>&v) { OnSessionTimedout(s, v); };

  // Then, asks the transport to work.
  FUNAPI_LOG("Starting a network module.");
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
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
    std::unique_lock<std::mutex> lock(mutex_transports_);
    for (auto iter : transports_) {
      if (iter.second->Started())
        iter.second->Stop();
    }
  }
}


void FunapiNetworkImpl::SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol = TransportProtocol::kDefault) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout_;

  if (last_received_ != epoch_ && last_received_ + delta < now) {
    FUNAPI_LOG("Session is too stale. The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  // Encodes a messsage type.
  rapidjson::Value msg_type_node;
  msg_type_node.SetString(rapidjson::StringRef(msg_type.c_str()));
  body.AddMember(rapidjson::StringRef(kMsgTypeBodyField.c_str()), msg_type_node, body.GetAllocator());

  // Encodes a session id, if any.
  if (!session_id_.empty()) {
    rapidjson::Value session_id_node;
    session_id_node.SetString(rapidjson::StringRef(session_id_.c_str()));
    body.AddMember(rapidjson::StringRef(kSessionIdBodyField.c_str()), session_id_node, body.GetAllocator());
  }

  // Sends the manipulated JSON object through the transport.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
    if (transport) {
      transport->SendMessage(body);
    }
    else {
      FUNAPI_LOG("Invaild Protocol - Transport is not founded");
    }
  }
}

void FunapiNetworkImpl::SendMessage(FunMessage& message, const TransportProtocol protocol = TransportProtocol::kDefault) {
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

  // Sends the manipulated Protobuf object through the transport.
  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);
    if (transport) {
      transport->SendMessage(message);
    }
    else {
      FUNAPI_LOG("Invaild Protocol - Transport is not founded");
    }
  }
}


bool FunapiNetworkImpl::Started() const {
  return started_;
}


bool FunapiNetworkImpl::Connected(TransportProtocol protocol = TransportProtocol::kDefault) {
  std::unique_lock<std::mutex> lock(mutex_transports_);
  std::shared_ptr<FunapiTransport> transport = GetTransport(protocol);

  if (transport)
    return transport->Started();

  return false;
}


void FunapiNetworkImpl::OnTransportReceived(
    const HeaderType &header, const std::vector<uint8_t> &body) {
  FUNAPI_LOG("OnReceived invoked");

  last_received_ = time(NULL);

  std::string msg_type;
  std::string session_id;

  if (encoding_type_ == kJsonEncoding) {
    // Parses the given json string.
    Json json;
    json.Parse<0>((char*)(body.data()));
    assert(json.IsObject());

    const rapidjson::Value &msg_type_node = json[kMsgTypeBodyField.c_str()];
    assert(msg_type_node.IsString());
    msg_type = msg_type_node.GetString();
    json.RemoveMember(kMsgTypeBodyField.c_str());

    const rapidjson::Value &session_id_node = json[kSessionIdBodyField.c_str()];
    assert(session_id_node.IsString());
    session_id = session_id_node.GetString();
    json.RemoveMember(kSessionIdBodyField.c_str());

  } else if (encoding_type_ == kProtobufEncoding) {
    FunMessage proto;
    proto.ParseFromArray(body.data(), (int)body.size());

    msg_type = proto.msgtype();
    session_id = proto.sid();
  }

  if (session_id_.empty()) {
    session_id_ = session_id;
    FUNAPI_LOG("New session id: %s", session_id.c_str());
    on_session_initiated_(session_id_);
  }

  if (session_id_ != session_id) {
    FUNAPI_LOG("Session id changed: %s => %s", session_id_.c_str(), session_id.c_str());
    session_id_ = session_id;
    on_session_closed_();
    on_session_initiated_(session_id_);
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it == message_handlers_.end()) {
    FUNAPI_LOG("No handler for message '%s'. Ignoring.", msg_type.c_str());
  } else {
    it->second(msg_type, body);
  }
}


void FunapiNetworkImpl::OnTransportStopped() {
  FUNAPI_LOG("Transport terminated. Stopping. You may restart again.");
  Stop();
}


void FunapiNetworkImpl::OnNewSession(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  // ignore
}


void FunapiNetworkImpl::OnSessionTimedout(
  const std::string &msg_type, const std::vector<uint8_t>&v_body) {
  FUNAPI_LOG("Session timed out. Resetting my session id. The server will send me another one next time.");

  session_id_ = "";
  on_session_closed_();
}

void FunapiNetworkImpl::Update() {
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  while (!tasks_queue_.empty())
  {
    (tasks_queue_.front())();
    tasks_queue_.pop();
  }
}

void FunapiNetworkImpl::AttachTransport(std::shared_ptr<FunapiTransport> transport, std::weak_ptr<FunapiNetwork> network) {
  transport->RegisterEventHandlers(
    [this](const HeaderType &header, const std::vector<uint8_t> &body){ OnTransportReceived(header, body); },
    [this](){ OnTransportStopped(); });
  transport->SetNetwork(network);

  {
    std::unique_lock<std::mutex> lock(mutex_transports_);
    if (GetTransport(transport->Protocol()))
    {
      FUNAPI_LOG("AttachTransport - transport of '%d' type already exists.", static_cast<int>(transport->Protocol()));
      FUNAPI_LOG(" You should call DetachTransport first.");
    } else {
      transports_[transport->Protocol()] = transport;
    }
  }
}

// The caller must lock mutex_transports_ before call this function.
std::shared_ptr<FunapiTransport> FunapiNetworkImpl::GetTransport(const TransportProtocol protocol) const {
  if (protocol == TransportProtocol::kDefault) {
    return transports_.cbegin()->second;
  }

  auto iter = transports_.find(protocol);
  if (iter != transports_.cend())
    return iter->second;

  return nullptr;
}


void FunapiNetworkImpl::PushTaskQueue(const std::function<void()> task)
{
  std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
  tasks_queue_.push(task);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiNetwork implementation.

void FunapiNetwork::Initialize(time_t session_timeout) {
  FUNAPI_LOG("This will be deprecated");
}


void FunapiNetwork::Finalize() {
  FUNAPI_LOG("This will be deprecated");
}


FunapiNetwork::FunapiNetwork(
    int type,
    const OnSessionInitiated &on_session_initiated,
    const OnSessionClosed &on_session_closed)
      : impl_(std::make_shared<FunapiNetworkImpl>(type,
        on_session_initiated, on_session_closed)) {
}


FunapiNetwork::~FunapiNetwork() {
}


void FunapiNetwork::RegisterHandler(
    const std::string msg_type, const MessageHandler &handler) {
  return impl_->RegisterHandler(msg_type, handler);
}


void FunapiNetwork::Start() {
  return impl_->Start();
}


void FunapiNetwork::Stop() {
  return impl_->Stop();
}


void FunapiNetwork::SendMessage(const std::string &msg_type, Json &body, const TransportProtocol protocol) {
  return impl_->SendMessage(msg_type, body, protocol);
}


void FunapiNetwork::SendMessage(FunMessage& message, const TransportProtocol protocol) {
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

void FunapiNetwork::AttachTransport(std::shared_ptr<FunapiTransport> transport) {
  return impl_->AttachTransport(transport, shared_from_this());
}

void FunapiNetwork::PushTaskQueue(const std::function<void()> task)
{
  return impl_->PushTaskQueue(task);
}

}  // namespace fun
