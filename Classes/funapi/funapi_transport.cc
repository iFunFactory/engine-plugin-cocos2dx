// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_network.h"
#include "funapi_manager.h"

namespace fun {

// Funapi header-related constants.
static const char* kHeaderDelimeter = "\n";
static const char* kHeaderFieldDelimeter = ":";
static const char* kVersionHeaderField = "VER";
static const char* kPluginVersionHeaderField = "PVER";
static const char* kLengthHeaderField = "LEN";
// static const char* kEncryptionHeaderField = "ENC";

////////////////////////////////////////////////////////////////////////////////
// FunapiTransportBase implementation.

class FunapiTransportBase : public std::enable_shared_from_this<FunapiTransportBase> {
 public:
  typedef FunapiTransport::TransportEventHandler TransportEventHandler;
  typedef FunapiTransport::OnReceived OnReceived;
  typedef FunapiTransport::OnStopped OnStopped;

  // Buffer-related constants.
  static const int kUnitBufferSize = 65536;

  FunapiTransportBase(TransportProtocol type, FunEncoding encoding);
  virtual ~FunapiTransportBase();

  bool Started();
  virtual void Start() = 0;
  virtual void Stop() = 0;

  void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);

  void SendMessage(rapidjson::Document &message);
  void SendMessage(FunMessage &message);
  void SendMessage(const char *body);

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);

  virtual TransportProtocol GetProtocol() { return TransportProtocol::kDefault; };
  FunEncoding GetEncoding() { return encoding_; };

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddFailureCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void ResetPingClientTimeout() {};

 protected:
  typedef std::map<std::string, std::string> HeaderFields;

  virtual bool EncodeThenSendMessage(std::vector<uint8_t> body) = 0;

  void PushSendQueue(std::function<bool()> task);
  void Send();
  void PushTaskQueue(std::function<void()> task);
  void PushStopTask();

  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool TryToDecodeBody(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields);
  bool EncodeMessage(std::vector<uint8_t> &body);
  bool DecodeMessage(int nRead, std::vector<uint8_t> &receiving);
  std::string MakeHeaderString(const std::vector<uint8_t> &body);

  // Registered event handlers.
  OnReceived on_received_;
  OnStopped on_stopped_;

  // State-related.
  TransportProtocol protocol_;
  FunapiTransportState state_;

  std::weak_ptr<FunapiNetwork> network_;

  std::vector<std::function<bool()>> v_send_;
  std::mutex v_send_mutex_;

  // Encoding-serializer-releated member variables.
  FunEncoding encoding_ = FunEncoding::kNone;

  time_t connect_timeout_seconds_ = 1;
  FunapiTimer connect_timeout_timer_;

  FunapiEvent<TransportEventHandler> on_transport_stared_;
  FunapiEvent<TransportEventHandler> on_transport_closed_;
  FunapiEvent<TransportEventHandler> on_transport_failure_;
  FunapiEvent<TransportEventHandler> on_connect_timeout_;

  void OnTransportStarted(const TransportProtocol protocol);
  void OnTransportClosed(const TransportProtocol protocol);
  void OnTransportFailure(const TransportProtocol protocol);
  void OnConnectTimeout(const TransportProtocol protocol);

 private:
  // Message-related.
  bool first_sending_ = true;
};


FunapiTransportBase::FunapiTransportBase(TransportProtocol protocol, FunEncoding encoding)
  : protocol_(protocol), state_(kDisconnected), encoding_(encoding) {
}


FunapiTransportBase::~FunapiTransportBase() {
}


void FunapiTransportBase::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  on_received_ = on_received;
  on_stopped_ = on_stopped;
}


bool FunapiTransportBase::EncodeMessage(std::vector<uint8_t> &body) {
  std::string header = MakeHeaderString(body);

  std::string body_string(body.cbegin(), body.cend());
  FUNAPI_LOG("Header to send: %s", header.c_str());
  FUNAPI_LOG("send message: %s", body_string.c_str());

  body.insert(body.cbegin(), header.cbegin(), header.cend());

  return true;
}


bool FunapiTransportBase::DecodeMessage(int nRead, std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  if (nRead < 0) {
    FUNAPI_LOG("receive failed: %s", strerror(errno));

    return false;
  }

  // Tries to decode as many messags as possible.
  while (true) {
    if (header_decoded == false) {
      if (TryToDecodeHeader(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
    if (header_decoded) {
      if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
      else {
        int new_length = static_cast<int>(receiving.size() - next_decoding_offset);
        if (new_length > 0) {
          receiving.assign(receiving.begin() + next_decoding_offset, receiving.end());
        }
        else {
          new_length = 0;
        }
        next_decoding_offset = 0;
        receiving.resize(new_length);
      }
    }
  }

  return true;
}


bool FunapiTransportBase::DecodeMessage(int nRead, std::vector<uint8_t> &receiving) {
  if (nRead < 0) {
    FUNAPI_LOG("receive failed: %s", strerror(errno));

    return false;
  }

  int next_decoding_offset = 0;
  bool header_decoded = false;
  HeaderFields header_fields;

  FUNAPI_LOG("Received %d bytes.", nRead);

  // Tries to decode as many messags as possible.
  while (true) {
    if (header_decoded == false) {
      if (TryToDecodeHeader(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
    if (header_decoded) {
      if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
        break;
      }
    }
  }

  return true;
}


void FunapiTransportBase::SendMessage(rapidjson::Document &message) {
  rapidjson::StringBuffer string_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
  message.Accept(writer);

  SendMessage(string_buffer.GetString());
}


void FunapiTransportBase::SendMessage(FunMessage &message) {
  std::string body = message.SerializeAsString();
  SendMessage(body.c_str());
}


void FunapiTransportBase::SendMessage(const char *body) {
  std::vector<uint8_t> v_body(strlen(body));
  memcpy(v_body.data(), body, strlen(body));

  PushSendQueue([this,v_body]()->bool{ return EncodeThenSendMessage(v_body); });
}


bool FunapiTransportBase::TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  FUNAPI_LOG("Trying to decode header fields.");
  int received_size = static_cast<int>(receiving.size());
  for (; next_decoding_offset < received_size;) {
    char *base = reinterpret_cast<char *>(receiving.data());
    char *ptr =
      std::search(base + next_decoding_offset,
      base + received_size,
      kHeaderDelimeter,
      kHeaderDelimeter + strlen(kHeaderDelimeter));

    ssize_t eol_offset = ptr - base;
    if (eol_offset >= received_size) {
      // Not enough bytes. Wait for more bytes to come.
      FUNAPI_LOG("We need more bytes for a header field. Waiting.");
      return false;
    }

    // Generates a null-termianted string by replacing the delimeter with \0.
    *ptr = '\0';
    char *line = base + next_decoding_offset;
    FUNAPI_LOG("Header line: %s", line);

    ssize_t line_length = eol_offset - next_decoding_offset;
    next_decoding_offset = static_cast<int>(eol_offset + 1);

    if (line_length == 0) {
      // End of header.
      header_decoded = true;
      FUNAPI_LOG("End of header reached. Will decode body from now.");
      return true;
    }

    ptr = std::search(
      line, line + line_length, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
    assert((ptr - base) < eol_offset);

    // Generates null-terminated string by replacing the delimeter with \0.
    *ptr = '\0';
    char *e1 = line, *e2 = ptr + 1;
    while (*e2 == ' ' || *e2 == '\t') ++e2;
    FUNAPI_LOG("Decoded header field '%s' => '%s'", e1, e2);
    header_fields[e1] = e2;
  }
  return false;
}


bool FunapiTransportBase::TryToDecodeBody(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  int received_size = static_cast<int>(receiving.size());
  // version header
  HeaderFields::const_iterator it = header_fields.find(kVersionHeaderField);
  assert(it != header_fields.end());
  int version = atoi(it->second.c_str());
  assert(version == static_cast<int>(FunapiVersion::kProtocolVersion));

  // length header
  it = header_fields.find(kLengthHeaderField);
  int body_length = atoi(it->second.c_str());
  FUNAPI_LOG("We need %d bytes for a message body.", body_length);

  if (received_size - next_decoding_offset < body_length) {
    // Need more bytes.
    FUNAPI_LOG("We need more bytes for a message body. Waiting.");
    return false;
  }

  if (body_length > 0) {
    assert(state_ == kConnected);

    if (state_ != kConnected) {
      FUNAPI_LOG("unexpected message");
      return false;
    }

    std::vector<uint8_t> v(receiving.begin() + next_decoding_offset, receiving.begin() + next_decoding_offset + body_length);
    v.push_back('\0');

    // Moves the read offset.
    next_decoding_offset += body_length;

    // The network module eats the fields and invokes registered handler
    PushTaskQueue([this, header_fields, v](){ on_received_(protocol_, encoding_, header_fields, v); });
  }

  // Prepares for a next message.
  header_decoded = false;
  header_fields.clear();

  return true;
}


std::string FunapiTransportBase::MakeHeaderString(const std::vector<uint8_t> &body) {
  std::string header;
  char buffer[1024];

  snprintf(buffer, sizeof(buffer), "%s%s%d%s",
    kVersionHeaderField, kHeaderFieldDelimeter,
    static_cast<int>(FunapiVersion::kProtocolVersion), kHeaderDelimeter);
  header += buffer;

  if (first_sending_) {
    snprintf(buffer, sizeof(buffer), "%s%s%d%s",
             kPluginVersionHeaderField, kHeaderFieldDelimeter,
             static_cast<int>(FunapiVersion::kPluginVersion), kHeaderDelimeter);
    header += buffer;

    first_sending_ = false;
  }

  snprintf(buffer, sizeof(buffer), "%s%s%lu%s",
    kLengthHeaderField, kHeaderFieldDelimeter,
    static_cast<unsigned long>(body.size()), kHeaderDelimeter);
  header += buffer;

  header += kHeaderDelimeter;

  return header;
}


void FunapiTransportBase::SetNetwork(std::weak_ptr<FunapiNetwork> network)
{
  network_ = network;
}


bool FunapiTransportBase::Started() {
  return (state_ == kConnected);
}


void FunapiTransportBase::PushSendQueue(std::function<bool()> task) {
  std::unique_lock<std::mutex> lock(v_send_mutex_);
  v_send_.push_back(task);
}


void FunapiTransportBase::Send() {
  std::function<bool()> task;
  while (true) {
    task = nullptr;
    {
      std::unique_lock<std::mutex> lock(v_send_mutex_);
      if (v_send_.empty()) {
        break;
      }
      else {
        task = std::move(v_send_.front());
        v_send_.erase(v_send_.cbegin());
      }
    }

    if (task) {
      if (false == task()) {
        std::unique_lock<std::mutex> lock(v_send_mutex_);
        v_send_.insert(v_send_.cbegin(), task);
        break;
      }
    }
  }
}


void FunapiTransportBase::PushTaskQueue(std::function<void()> task) {
  auto network = network_.lock();
  if (network) {
    auto self(shared_from_this());
    network->PushTaskQueue([self, task]{ task(); });
  }
}


void FunapiTransportBase::PushStopTask() {
  PushTaskQueue([this](){
    Stop();
    on_stopped_();
  });
}


void FunapiTransportBase::SetConnectTimeout(time_t timeout) {
  connect_timeout_seconds_ = timeout;
}


void FunapiTransportBase::AddStartedCallback(const TransportEventHandler &handler) {
  on_transport_stared_ += handler;
}


void FunapiTransportBase::AddStoppedCallback(const TransportEventHandler &handler) {
  on_transport_closed_ += handler;
}


void FunapiTransportBase::AddFailureCallback(const TransportEventHandler &handler) {
  on_transport_failure_ += handler;
}


void FunapiTransportBase::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  on_connect_timeout_ += handler;
}


void FunapiTransportBase::OnTransportStarted(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_stared_(protocol); });
}


void FunapiTransportBase::OnTransportClosed(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_closed_(protocol); });
}


void FunapiTransportBase::OnTransportFailure(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_transport_failure_(protocol); });
}


void FunapiTransportBase::OnConnectTimeout(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_connect_timeout_(protocol); });
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTransportImpl implementation.

class FunapiTransportImpl : public FunapiTransportBase {
 public:
  FunapiTransportImpl(TransportProtocol protocol,
                      const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTransportImpl();
  void Stop();
  virtual void ResetPingClientTimeout() {};
  virtual int GetSocket();

  virtual void AddInitSocketCallback(const TransportEventHandler &handler);
  virtual void AddCloseSocketCallback(const TransportEventHandler &handler);

  virtual void OnSocketRead();
  virtual void OnSocketWrite();
  virtual void Update();

 protected:
  void CloseSocket();
  virtual void Recv() = 0;

  // State-related.
  Endpoint endpoint_;
  int sock_;

  std::vector<struct in_addr> in_addrs_;
  uint16_t port_;

  void OnInitSocket(const TransportProtocol protocol);
  void OnCloseSocket(const TransportProtocol protocol);

 private:
  FunapiEvent<TransportEventHandler> on_init_socket_;
  FunapiEvent<TransportEventHandler> on_close_socket_;
};


FunapiTransportImpl::FunapiTransportImpl(TransportProtocol protocol,
                                         const std::string &hostname_or_ip,
                                         uint16_t port, FunEncoding encoding)
    : FunapiTransportBase(protocol, encoding), sock_(-1), port_(port) {
  struct hostent *entry = gethostbyname(hostname_or_ip.c_str());
  assert(entry);

  struct in_addr addr;

  if (entry) {
    int index = 0;
    while (entry->h_addr_list[index]) {
      memcpy(&addr, entry->h_addr_list[index], entry->h_length);
      in_addrs_.push_back(addr);
      ++index;
    }

    addr = in_addrs_[0];

    // log
    /*
    for (auto t : in_addrs_)
    {
      printf ("%s\n", inet_ntoa(t));
    }
    */
  }
  else {
    addr.s_addr = INADDR_NONE;
  }

  endpoint_.sin_family = AF_INET;
  endpoint_.sin_addr = addr;
  endpoint_.sin_port = htons(port);
}


FunapiTransportImpl::~FunapiTransportImpl() {
}


void FunapiTransportImpl::Stop() {
  state_ = kDisconnected;

  CloseSocket();

  OnTransportClosed(GetProtocol());
}


void FunapiTransportImpl::CloseSocket() {
  if (sock_ >= 0) {
#if FUNAPI_PLATFORM_WINDOWS
    closesocket(sock_);
#else
    close(sock_);
#endif
    sock_ = -1;

    OnCloseSocket(GetProtocol());
  }
}


int FunapiTransportImpl::GetSocket() {
  return sock_;
}


void FunapiTransportImpl::AddInitSocketCallback(const TransportEventHandler &handler) {
  on_init_socket_ += handler;
}


void FunapiTransportImpl::AddCloseSocketCallback(const TransportEventHandler &handler) {
  on_close_socket_ += handler;
}


void FunapiTransportImpl::OnInitSocket(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_init_socket_(protocol); });
}


void FunapiTransportImpl::OnCloseSocket(const TransportProtocol protocol) {
  PushTaskQueue([this, protocol] { on_close_socket_(protocol); });
}


void FunapiTransportImpl::OnSocketRead() {
}


void FunapiTransportImpl::OnSocketWrite() {
}


void FunapiTransportImpl::Update() {
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportImpl implementation.

class FunapiTcpTransportImpl : public FunapiTransportImpl {
 public:
  FunapiTcpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransportImpl();

  virtual TransportProtocol GetProtocol();

  void Start();
  void SetDisableNagle(const bool disable_nagle);
  void SetAutoReconnect(const bool auto_reconnect);
  void SetEnablePing(const bool enable_ping);
  void ResetPingClientTimeout();

  void OnSocketRead();
  void OnSocketWrite();
  void Update();

 protected:
  virtual bool EncodeThenSendMessage(std::vector<uint8_t> body);
  void Connect();
  void Recv();

  bool disable_nagle_ = false;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;
  FunapiTimer ping_client_timeout_timer_;

 private:
  void InitSocket();
  bool Connect(Endpoint endpoint);

  // Ping message-related constants.
  static const time_t kPingIntervalSecond = 3;
  static const time_t kPingTimeoutSeconds = 20;

  static const int kMaxReconnectCount = 3;
  static const time_t kMaxReconnectWaitSeconds = 10;
  int reconnect_count_ = 0;
  int connect_addr_index_ = 0;
  FunapiTimer reconnect_wait_timer_;
  time_t reconnect_wait_seconds_;

  std::function<void()> on_socket_read_;
  std::function<void()> on_socket_write_;
  std::function<void()> on_update_;

  void Ping();
  void CheckConnectTimeout();
  void WaitForAutoReconnect();
};


FunapiTcpTransportImpl::FunapiTcpTransportImpl(TransportProtocol protocol,
  const std::string &hostname_or_ip,
  uint16_t port,
  FunEncoding encoding)
  : FunapiTransportImpl(protocol, hostname_or_ip, port, encoding) {
}


FunapiTcpTransportImpl::~FunapiTcpTransportImpl() {
  CloseSocket();
}


TransportProtocol FunapiTcpTransportImpl::GetProtocol() {
  return TransportProtocol::kTcp;
}


void FunapiTcpTransportImpl::Start() {
  state_ = kConnecting;

  on_update_ = [](){};
  on_socket_read_ = [](){};
  on_socket_write_ = [this](){
    int e;
    socklen_t e_size = sizeof(e);
    int r = getsockopt(sock_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&e), &e_size);
    if (r == 0 && e == 0) {
      // Makes a state transition.
      state_ = kConnected;

      on_socket_read_ = [this](){ Recv(); };
      on_socket_write_ = [this](){ Send(); };

      ping_client_timeout_timer_.SetTimer(kPingIntervalSecond + kPingTimeoutSeconds);
      on_update_ = [this](){
        Ping();
      };

      OnTransportStarted(TransportProtocol::kTcp);
    }
    else {
      FUNAPI_LOG("failed - tcp connect");
      OnTransportFailure(TransportProtocol::kTcp);

      ++connect_addr_index_;
      Connect();
    }
  };

  reconnect_count_ = 0;
  connect_addr_index_ = 0;
  reconnect_wait_seconds_ = 1;
  Connect();
}


void FunapiTcpTransportImpl::Ping() {
  if (enable_ping_) {
    static FunapiTimer ping_send_timer;

    if (ping_send_timer.IsExpired()){
      auto network = network_.lock();
      if (network) {
        if (network->SendClientPingMessage(GetProtocol())) {
          ping_send_timer.SetTimer(kPingIntervalSecond);
        }
        else {
          ping_client_timeout_timer_.SetTimer(kPingIntervalSecond + kPingTimeoutSeconds);
        }
      }
    }

    if (ping_client_timeout_timer_.IsExpired()) {
      FUNAPI_LOG("Network seems disabled. Stopping the transport.");
      PushStopTask();
      return;
    }
  }
}


void FunapiTcpTransportImpl::CheckConnectTimeout() {
  if (connect_timeout_timer_.IsExpired()) {
    FUNAPI_LOG("failed - tcp connect - timeout");
    PushTaskQueue([this](){ on_connect_timeout_(TransportProtocol::kTcp); });

    if (auto_reconnect_) {
      ++reconnect_count_;

      if (kMaxReconnectCount < reconnect_count_) {
        ++connect_addr_index_;
        reconnect_count_ = 0;
        reconnect_wait_seconds_ = 1;

        Connect();
      }
      else {
        reconnect_wait_timer_.SetTimer(reconnect_wait_seconds_);

        FUNAPI_LOG("Wait %d seconds for connect to TCP transport.", static_cast<int>(reconnect_wait_seconds_));

        on_update_ = [this](){
          WaitForAutoReconnect();
        };
      }
    }
    else {
      ++connect_addr_index_;
      Connect();
    }
  }
}


void FunapiTcpTransportImpl::WaitForAutoReconnect() {
  if (reconnect_wait_timer_.IsExpired()) {
    reconnect_wait_seconds_ *= 2;
    if (kMaxReconnectWaitSeconds < reconnect_wait_seconds_) {
      reconnect_wait_seconds_ = kMaxReconnectWaitSeconds;
    }

    Connect();
  }
}


void FunapiTcpTransportImpl::SetDisableNagle(const bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


void FunapiTcpTransportImpl::SetAutoReconnect(const bool auto_reconnect) {
  auto_reconnect_ = auto_reconnect;
}


void FunapiTcpTransportImpl::SetEnablePing(const bool enable_ping) {
  enable_ping_ = enable_ping;
}


void FunapiTcpTransportImpl::ResetPingClientTimeout() {
  ping_client_timeout_timer_.SetTimer(kPingTimeoutSeconds);
}


bool FunapiTcpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return false;
  }

  static int offset = 0;

  if (sock_ < 0)
    return false;

  int nSent = static_cast<int>(send(sock_, reinterpret_cast<char*>(body.data()) + offset, body.size() - offset, 0));

  if (nSent < 0) {
    PushStopTask();
    return false;
  }
  else {
    offset += nSent;
  }

  FUNAPI_LOG("Sent %d bytes", nSent);

  if (offset == body.size()) {
    offset = 0;
    return true;
  }

  return false;
}


void FunapiTcpTransportImpl::InitSocket() {
  // Initiates a new socket.
  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock_ >= 0);

  // Makes the fd non-blocking.
#if FUNAPI_PLATFORM_WINDOWS
  u_long argp = 0;
  int flag = ioctlsocket(sock_, FIONBIO, &argp);
  assert(flag >= 0);
#else
  int flag = fcntl(sock_, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(sock_, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);
#endif

  if (disable_nagle_) {
    int flag = 1;
    int result = setsockopt(sock_,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            reinterpret_cast<char*>(&flag),
                            sizeof(int));
    if (result < 0) {
      FUNAPI_LOG("error - TCP_NODELAY");
    }
  }

  OnInitSocket(TransportProtocol::kTcp);
}


void FunapiTcpTransportImpl::Connect() {
  CloseSocket();

  if (connect_addr_index_ >= in_addrs_.size()) {
    on_update_ = [](){};
    on_socket_read_ = [](){};
    on_socket_write_ = [](){};
    return;
  }

  Endpoint endpoint;
  endpoint.sin_family = AF_INET;
  endpoint.sin_addr = in_addrs_[connect_addr_index_];
  endpoint.sin_port = htons(port_);

  InitSocket();

  connect_timeout_timer_.SetTimer(connect_timeout_seconds_);

  // Tries to connect.
  int rc = connect(sock_,
                   reinterpret_cast<const struct sockaddr *>(&endpoint_),
                   sizeof(endpoint_));
  assert(rc == 0 || (rc < 0 && errno == EINPROGRESS));

  FUNAPI_LOG("Try to connect to server - %s", inet_ntoa(endpoint.sin_addr));

  on_update_ = [this](){
    CheckConnectTimeout();
  };
}


void FunapiTcpTransportImpl::Recv() {
  static std::vector<uint8_t> receiving_vector;
  static int next_decoding_offset = 0;
  static bool header_decoded = false;
  static HeaderFields header_fields;

  if (sock_ < 0)
    return;

  std::vector<uint8_t> buffer(kUnitBufferSize);

  int nRead = static_cast<int>(recv(sock_, buffer.data(), kUnitBufferSize, 0));

  if (nRead <= 0) {
    if (nRead < 0) {
      FUNAPI_LOG("receive failed: %s", strerror(errno));
    }
    PushStopTask();
    return;
  }

  receiving_vector.insert(receiving_vector.end(), buffer.cbegin(), buffer.cbegin() + nRead);

  if (!DecodeMessage(nRead, receiving_vector, next_decoding_offset, header_decoded, header_fields)) {
    if (nRead == 0)
      FUNAPI_LOG("Socket [%d] closed.", sock_);

    PushStopTask();
  }
}


void FunapiTcpTransportImpl::OnSocketRead() {
  on_socket_read_();
}


void FunapiTcpTransportImpl::OnSocketWrite() {
  on_socket_write_();
}


void FunapiTcpTransportImpl::Update() {
  on_update_();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportImpl implementation.

class FunapiUdpTransportImpl : public FunapiTransportImpl {
public:
  FunapiUdpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiUdpTransportImpl();

  virtual TransportProtocol GetProtocol();

  void Start();

  void OnSocketRead();
  void OnSocketWrite();

protected:
  virtual bool EncodeThenSendMessage(std::vector<uint8_t> body);
  void Recv();

private:
  Endpoint recv_endpoint_;
};


FunapiUdpTransportImpl::FunapiUdpTransportImpl(TransportProtocol protocol,
  const std::string &hostname_or_ip,
  uint16_t port,
  FunEncoding encoding)
  : FunapiTransportImpl(protocol, hostname_or_ip, port, encoding) {
  recv_endpoint_.sin_family = AF_INET;
  recv_endpoint_.sin_addr.s_addr = htonl(INADDR_ANY);
  recv_endpoint_.sin_port = htons(port);
}


FunapiUdpTransportImpl::~FunapiUdpTransportImpl() {
  CloseSocket();
}


TransportProtocol FunapiUdpTransportImpl::GetProtocol() {
  return TransportProtocol::kUdp;
}


void FunapiUdpTransportImpl::Start() {
  CloseSocket();

  sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(sock_ >= 0);
  state_ = kConnected;

  OnInitSocket(TransportProtocol::kUdp);
  OnTransportStarted(TransportProtocol::kUdp);
}


bool FunapiUdpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return false;
  }

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("Send = %s", *FString(temp_string.c_str()));

  socklen_t len = sizeof(endpoint_);

  if (sock_ < 0)
    return false;

  int nSent = static_cast<int>(sendto(sock_, reinterpret_cast<char*>(body.data()), body.size(), 0, reinterpret_cast<struct sockaddr*>(&endpoint_), len));

  if (nSent < 0) {
    PushStopTask();
    return false;
  }

  // FUNAPI_LOG("Sent %d bytes", nSent);

  return true;
}


void FunapiUdpTransportImpl::Recv() {
  std::vector<uint8_t> receiving_vector(kUnitBufferSize);
  socklen_t len = sizeof(recv_endpoint_);

  if (sock_<0)
    return;

  int nRead = static_cast<int>(recvfrom(sock_, reinterpret_cast<char*>(receiving_vector.data()), receiving_vector.size(), 0, reinterpret_cast<struct sockaddr*>(&recv_endpoint_), &len));

  // FUNAPI_LOG("nRead = %d", nRead);

  if (nRead < 0) {
    FUNAPI_LOG("receive failed: %s", strerror(errno));
    PushStopTask();
    return;
  }

  if (!DecodeMessage(nRead, receiving_vector)) {
    if (nRead == 0)
      FUNAPI_LOG("Socket [%d] closed.", sock_);

    PushStopTask();
  }
}


void FunapiUdpTransportImpl::OnSocketRead() {
  Recv();
}


void FunapiUdpTransportImpl::OnSocketWrite() {
  Send();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportImpl implementation.

class FunapiHttpTransportImpl : public FunapiTransportBase {
 public:
  FunapiHttpTransportImpl(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransportImpl();

  virtual TransportProtocol GetProtocol();

  void Start();
  void Stop();

  void Update();

 protected:
  virtual bool EncodeThenSendMessage(std::vector<uint8_t> body);

 private:
  static size_t HttpResponseCb(void *data, size_t size, size_t count, void *cb);
  void WebResponseHeaderCb(void *data, int len, HeaderFields &header_fields);
  void WebResponseBodyCb(void *data, int len, std::vector<uint8_t> &receiving);

  std::string host_url_;
};


FunapiHttpTransportImpl::FunapiHttpTransportImpl(const std::string &hostname_or_ip,
                                                 uint16_t port, bool https, FunEncoding encoding)
  : FunapiTransportBase(TransportProtocol::kHttp, encoding) {
  char url[1024];
  sprintf(url, "%s://%s:%d/v%d/",
      https ? "https" : "http", hostname_or_ip.c_str(), port,
      static_cast<int>(FunapiVersion::kProtocolVersion));
  host_url_ = url;

  FUNAPI_LOG("Host url : %s", host_url_.c_str());
}


FunapiHttpTransportImpl::~FunapiHttpTransportImpl() {
}


TransportProtocol FunapiHttpTransportImpl::GetProtocol() {
  return TransportProtocol::kHttp;
}


void FunapiHttpTransportImpl::Start() {
  state_ = kConnected;
  FUNAPI_LOG("Started.");

  OnTransportStarted(TransportProtocol::kHttp);
}


void FunapiHttpTransportImpl::Stop() {
  if (state_ == kDisconnected)
    return;

  state_ = kDisconnected;
  FUNAPI_LOG("Stopped.");

  OnTransportClosed(TransportProtocol::kHttp);
}


size_t FunapiHttpTransportImpl::HttpResponseCb(void *data, size_t size, size_t count, void *cb) {
  AsyncWebResponseCallback *callback = (AsyncWebResponseCallback*)(cb);
  if (callback != NULL)
    (*callback)(data, static_cast<int>(size * count));
  return size * count;
}


bool FunapiHttpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  std::string header = MakeHeaderString(body);

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("HTTP Send header = %s \n body = %s", *FString(header.c_str()), *FString(temp_string.c_str()));

  CURL *ctx = curl_easy_init();
  if (ctx == NULL) {
    FUNAPI_LOG("Unable to initialize cURL interface.");
    return false;
  }

  HeaderFields header_fields;
  std::vector<uint8_t> receiving;

  AsyncWebResponseCallback receive_header = [this, &header_fields](void* data, int len){ WebResponseHeaderCb(data, len, header_fields); };
  AsyncWebResponseCallback receive_body = [this, &receiving](void* data, int len){ WebResponseBodyCb(data, len, receiving); };

  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, header.c_str());
  curl_easy_setopt(ctx, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(ctx, CURLOPT_URL, host_url_.c_str());
  curl_easy_setopt(ctx, CURLOPT_POST, 1L);
  curl_easy_setopt(ctx, CURLOPT_POSTFIELDS, body.data());
  curl_easy_setopt(ctx, CURLOPT_POSTFIELDSIZE, body.size());
  curl_easy_setopt(ctx, CURLOPT_HEADERDATA, &receive_header);
  curl_easy_setopt(ctx, CURLOPT_WRITEDATA, &receive_body);
  curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, &FunapiHttpTransportImpl::HttpResponseCb);

  CURLcode res = curl_easy_perform(ctx);
  if (res != CURLE_OK) {
    FUNAPI_LOG("Error from cURL: %s", curl_easy_strerror(res));
    return false;
  }
  else {
    // std::to_string is not supported on android, using std::stringstream instead.
    std::stringstream ss_protocol_version;
    ss_protocol_version << static_cast<int>(FunapiVersion::kProtocolVersion);
    header_fields[kVersionHeaderField] = ss_protocol_version.str();

    std::stringstream ss_version_header_field;
    ss_version_header_field << receiving.size();
    header_fields[kLengthHeaderField] = ss_version_header_field.str();

    bool header_decoded = true;
    int next_decoding_offset = 0;
    if (TryToDecodeBody(receiving, next_decoding_offset, header_decoded, header_fields) == false) {
      PushStopTask();
    }
  }

  curl_easy_cleanup(ctx);
  curl_slist_free_all(chunk);

  return true;
}


void FunapiHttpTransportImpl::WebResponseHeaderCb(void *data, int len, HeaderFields &header_fields) {
  char buf[1024];
  memcpy(buf, data, len);
  buf[len-2] = '\0';

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter,
      kHeaderFieldDelimeter + strlen(kHeaderFieldDelimeter));
  ssize_t eol_offset = ptr - buf;
  if (eol_offset >= len)
    return;

  // Generates null-terminated string by replacing the delimeter with \0.
  *ptr = '\0';
  const char *e1 = buf, *e2 = ptr + 1;
  while (*e2 == ' ' || *e2 == '\t') ++e2;
  FUNAPI_LOG("Decoded header field '%s' => '%s'", e1, e2);
  header_fields[e1] = e2;
}


void FunapiHttpTransportImpl::WebResponseBodyCb(void *data, int len, std::vector<uint8_t> &receiving) {
  receiving.insert(receiving.cend(), (uint8_t*)data, (uint8_t*)data + len);
}


void FunapiHttpTransportImpl::Update() {
  // FUNAPI_LOG("%s", __FUNCTION__);
  Send();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

FunapiTcpTransport::FunapiTcpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiTcpTransportImpl>(TransportProtocol::kTcp, hostname_or_ip, port, encoding)) {
}


std::shared_ptr<FunapiTcpTransport> FunapiTcpTransport::create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding) {
  return std::make_shared<FunapiTcpTransport>(hostname_or_ip, port, encoding);
}


TransportProtocol FunapiTcpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiTcpTransport::RegisterEventHandlers(
  const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiTcpTransport::Start() {
  impl_->Start();
}


void FunapiTcpTransport::Stop() {
  impl_->Stop();
}


void FunapiTcpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiTcpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiTcpTransport::Started() const {
  return impl_->Started();
}


void FunapiTcpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiTcpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiTcpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiTcpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiTcpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiTcpTransport::AddFailureCallback(const TransportEventHandler &handler) {
  return impl_->AddFailureCallback(handler);
}


void FunapiTcpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiTcpTransport::SetDisableNagle(const bool disable_nagle) {
  return impl_->SetDisableNagle(disable_nagle);
}


void FunapiTcpTransport::SetAutoReconnect(const bool auto_reconnect) {
  return impl_->SetAutoReconnect(auto_reconnect);
}


void FunapiTcpTransport::SetEnablePing(const bool enable_ping) {
  return impl_->SetEnablePing(enable_ping);
}


void FunapiTcpTransport::ResetPingClientTimeout() {
  return impl_->ResetPingClientTimeout();
}


int FunapiTcpTransport::GetSocket() {
  return impl_->GetSocket();
}


void FunapiTcpTransport::AddInitSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddInitSocketCallback(handler);
}


void FunapiTcpTransport::AddCloseSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddCloseSocketCallback(handler);
}


void FunapiTcpTransport::OnSocketRead() {
  return impl_->OnSocketRead();
}


void FunapiTcpTransport::OnSocketWrite() {
  return impl_->OnSocketWrite();
}


void FunapiTcpTransport::Update() {
  return impl_->Update();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

FunapiUdpTransport::FunapiUdpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiUdpTransportImpl>(TransportProtocol::kUdp, hostname_or_ip, port, encoding)) {
}


std::shared_ptr<FunapiUdpTransport> FunapiUdpTransport::create(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding) {
  return std::make_shared<FunapiUdpTransport>(hostname_or_ip, port, encoding);
}


TransportProtocol FunapiUdpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiUdpTransport::RegisterEventHandlers(
  const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiUdpTransport::Start() {
  impl_->Start();
}


void FunapiUdpTransport::Stop() {
  impl_->Stop();
}


void FunapiUdpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiUdpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiUdpTransport::Started() const {
  return impl_->Started();
}


void FunapiUdpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiUdpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiUdpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiUdpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiUdpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiUdpTransport::AddFailureCallback(const TransportEventHandler &handler) {
  return impl_->AddFailureCallback(handler);
}


void FunapiUdpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiUdpTransport::ResetPingClientTimeout() {
  return impl_->ResetPingClientTimeout();
}


int FunapiUdpTransport::GetSocket() {
  return impl_->GetSocket();
}


void FunapiUdpTransport::AddInitSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddInitSocketCallback(handler);
}


void FunapiUdpTransport::AddCloseSocketCallback(const TransportEventHandler &handler) {
  return impl_->AddCloseSocketCallback(handler);
}


void FunapiUdpTransport::OnSocketRead() {
  return impl_->OnSocketRead();
}


void FunapiUdpTransport::OnSocketWrite() {
  return impl_->OnSocketWrite();
}


void FunapiUdpTransport::Update() {
  return impl_->Update();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

FunapiHttpTransport::FunapiHttpTransport (const std::string &hostname_or_ip,
  uint16_t port, bool https, FunEncoding encoding)
  : impl_(std::make_shared<FunapiHttpTransportImpl>(hostname_or_ip, port, https, encoding)) {
}


std::shared_ptr<FunapiHttpTransport> FunapiHttpTransport::create(const std::string &hostname_or_ip,
  uint16_t port, bool https, FunEncoding encoding) {
  return std::make_shared<FunapiHttpTransport>(hostname_or_ip, port, https, encoding);
}


TransportProtocol FunapiHttpTransport::GetProtocol() const {
  return impl_->GetProtocol();
}


void FunapiHttpTransport::RegisterEventHandlers(
  const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiHttpTransport::Start() {
  impl_->Start();
}


void FunapiHttpTransport::Stop() {
  impl_->Stop();
}


void FunapiHttpTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiHttpTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiHttpTransport::Started() const {
  return impl_->Started();
}


void FunapiHttpTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiHttpTransport::GetEncoding() const {
  return impl_->GetEncoding();
}


void FunapiHttpTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiHttpTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiHttpTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiHttpTransport::AddFailureCallback(const TransportEventHandler &handler) {
  return impl_->AddFailureCallback(handler);
}


void FunapiHttpTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiHttpTransport::ResetPingClientTimeout() {
  return impl_->ResetPingClientTimeout();
}


void FunapiHttpTransport::Update() {
  return impl_->Update();
}

}  // namespace fun
