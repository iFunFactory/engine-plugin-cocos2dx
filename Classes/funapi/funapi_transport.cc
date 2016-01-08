// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_network.h"

namespace fun {

// Buffer-related constants.
static const int kUnitBufferSize = 65536;

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

  FunEncoding GetEncoding() { return encoding_; };

  void SetConnectTimeout(time_t timeout);

  void AddStartedCallback(const TransportEventHandler &handler);
  void AddStoppedCallback(const TransportEventHandler &handler);
  void AddFailureCallback(const TransportEventHandler &handler);
  void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void ResetPingClientTimeout() {};

 protected:
  typedef std::map<std::string, std::string> HeaderFields;

  virtual void EncodeThenSendMessage(std::vector<uint8_t> body) = 0;
  void SendEmptyMessage(const TransportProtocol &protocol, const FunEncoding &encoding);

  void PushSendQueue(std::function<void()> task);
  virtual void JoinThread();
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

  std::queue<std::function<void()>> send_queue_;
  std::mutex send_queue_mutex_;
  std::condition_variable_any send_queue_condition_;

  bool send_thread_run_ = false;
  std::thread send_thread_;

  // Encoding-serializer-releated member variables.
  FunEncoding encoding_ = FunEncoding::kNone;

  time_t connect_timeout_ = 1;

  FEvent<TransportEventHandler> on_transport_stared_;
  FEvent<TransportEventHandler> on_transport_closed_;
  FEvent<TransportEventHandler> on_transport_failure_;
  FEvent<TransportEventHandler> on_connect_timeout_;

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
        int new_length = (int)(receiving.size() - next_decoding_offset);
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

  // int received_size = (int)receiving.size();
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

  PushSendQueue([this,v_body]{ EncodeThenSendMessage(v_body); });
}


void FunapiTransportBase::SendEmptyMessage(const TransportProtocol &protocol, const FunEncoding &encoding) {
  auto network = network_.lock();
  if (network) {
    network->SendEmptyMessage(protocol, encoding);
  }
}


bool FunapiTransportBase::TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  FUNAPI_LOG("Trying to decode header fields.");
  int received_size = (int)receiving.size();
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
    next_decoding_offset = (int)(eol_offset + 1);

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
  int received_size = (int)receiving.size();
  // version header 읽기
  HeaderFields::const_iterator it = header_fields.find(kVersionHeaderField);
  assert(it != header_fields.end());
  int version = atoi(it->second.c_str());
  assert(version == (int)FunapiVersion::kProtocolVersion);

  // length header 읽기
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
    (int)FunapiVersion::kProtocolVersion, kHeaderDelimeter);
  header += buffer;

  if (first_sending_) {
    snprintf(buffer, sizeof(buffer), "%s%s%d%s",
             kPluginVersionHeaderField, kHeaderFieldDelimeter,
             (int)FunapiVersion::kPluginVersion, kHeaderDelimeter);
    header += buffer;

    first_sending_ = false;
  }

  snprintf(buffer, sizeof(buffer), "%s%s%lu%s",
    kLengthHeaderField, kHeaderFieldDelimeter,
    (unsigned long)body.size(), kHeaderDelimeter);
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


void FunapiTransportBase::PushSendQueue(std::function<void()> task) {
  std::unique_lock<std::mutex> lock(send_queue_mutex_);
  send_queue_.push(task);
  send_queue_condition_.notify_one();
}


void FunapiTransportBase::JoinThread() {
  send_thread_run_ = false;
  send_queue_condition_.notify_all();
  if (send_thread_.joinable())
    send_thread_.join();
}


void FunapiTransportBase::Send() {
  while (send_thread_run_) {
    std::function<void()> task = nullptr;
    {
      std::unique_lock<std::mutex> lock(send_queue_mutex_);
      if (send_queue_.empty()) {
        send_queue_condition_.wait(send_queue_mutex_);
        continue;
      }
      else {
        task = std::move(send_queue_.front());
        send_queue_.pop();
      }
    }

    task();
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
  PushTaskQueue([this]{
    Stop();
    on_stopped_();
  });
}


void FunapiTransportBase::SetConnectTimeout(time_t timeout) {
  connect_timeout_ = timeout;
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
  on_transport_stared_(protocol);
}


void FunapiTransportBase::OnTransportClosed(const TransportProtocol protocol) {
  on_transport_closed_(protocol);
}


void FunapiTransportBase::OnTransportFailure(const TransportProtocol protocol) {
  on_transport_failure_(protocol);
}


void FunapiTransportBase::OnConnectTimeout(const TransportProtocol protocol) {
  on_connect_timeout_(protocol);
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

 protected:
  virtual void JoinThread();
  void CloseSocket();
  virtual void Recv() = 0;

  // State-related.
  Endpoint endpoint_;
  int sock_;

  bool recv_thread_run_ = false;
  std::thread recv_thread_;

  std::vector<struct in_addr> in_addrs_;
  uint16_t port_;
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
  JoinThread();
}


void FunapiTransportImpl::CloseSocket() {
  if (sock_ >= 0) {
#if PLATFORM_WINDOWS || (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    closesocket(sock_);
#else
    close(sock_);
#endif
    sock_ = -1;
  }
}


void FunapiTransportImpl::JoinThread() {
  FunapiTransportBase::JoinThread();

  recv_thread_run_ = false;
  if (recv_thread_.joinable()) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)      
    recv_thread_.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
#else
    recv_thread_.join();
#endif
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportImpl implementation.

class FunapiTcpTransportImpl : public FunapiTransportImpl {
 public:
  FunapiTcpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransportImpl();

  void Start();
  void SetDisableNagle(bool disable_nagle);
  void SetAutoReconnect(bool auto_reconnect);
  void SetEnablePing(bool enable_ping);
  void ResetPingClientTimeout();

 protected:
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body);
  virtual void JoinThread();
  void Connect();
  void Recv();

  bool connect_thread_run_ = false;
  std::thread connect_thread_;

  bool disable_nagle_ = false;
  bool auto_reconnect_ = false;
  bool enable_ping_ = false;
  time_t ping_client_timeout_;

 private:
  void InitSocket();
  bool Connect(Endpoint endpoint);

  // Ping message-related constants.
  const time_t kPingIntervalSecond = 3;
  const time_t kPingTimeoutSeconds = 20;
};


FunapiTcpTransportImpl::FunapiTcpTransportImpl(TransportProtocol protocol,
  const std::string &hostname_or_ip,
  uint16_t port,
  FunEncoding encoding)
  : FunapiTransportImpl(protocol, hostname_or_ip, port, encoding) {
}


FunapiTcpTransportImpl::~FunapiTcpTransportImpl() {
  CloseSocket();
  JoinThread();
}


void FunapiTcpTransportImpl::Start() {
  CloseSocket();
  JoinThread();

  state_ = kConnecting;

  // connecting
  connect_thread_run_ = true;
  send_thread_run_ = true;
  recv_thread_run_ = true;
  connect_thread_ = std::thread([this] {
    Connect();
  });
}


void FunapiTcpTransportImpl::SetDisableNagle(bool disable_nagle) {
  disable_nagle_ = disable_nagle;
}


void FunapiTcpTransportImpl::SetAutoReconnect(bool auto_reconnect) {
  auto_reconnect_ = auto_reconnect;
}


void FunapiTcpTransportImpl::SetEnablePing(bool enable_ping) {
  enable_ping_ = enable_ping;
}


void FunapiTcpTransportImpl::ResetPingClientTimeout() {
  ping_client_timeout_ = time(NULL) + kPingTimeoutSeconds;
}


void FunapiTcpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return;
  }

  fd_set wset;
  int offset = 0;

  do {
    FD_ZERO(&wset);

    if (sock_ < 0)
      return;

    FD_SET(sock_, &wset);
    struct timeval timeout = { 0, 1 };

    if (select(sock_ + 1, NULL, &wset, NULL, &timeout) > 0) {
      if (FD_ISSET(sock_, &wset)) {
        int nSent = (int)send(sock_, (char*)body.data() + offset, body.size() - offset, 0);

        if (nSent < 0) {
          PushStopTask();
          break;
        }
        else {
          offset += nSent;
        }

        FUNAPI_LOG("Sent %d bytes", nSent);
      }
    }
  } while (offset < body.size());
}


void FunapiTcpTransportImpl::JoinThread() {
  FunapiTransportImpl::JoinThread();

  connect_thread_run_ = false;
  if (connect_thread_.joinable())
    connect_thread_.join();
}


void FunapiTcpTransportImpl::InitSocket() {
  // Initiates a new socket.
  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(sock_ >= 0);

  // Makes the fd non-blocking.
#if PLATFORM_WINDOWS || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
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
                            (char *) &flag,
                            sizeof(int));
    if (result < 0) {
      FUNAPI_LOG("error - TCP_NODELAY");
    }
  }
}


bool FunapiTcpTransportImpl::Connect(Endpoint endpoint) {
  time_t connect_timeout = time(NULL) + connect_timeout_;

  // Tries to connect.
  int rc = connect(sock_,
                   reinterpret_cast<const struct sockaddr *>(&endpoint_),
                   sizeof(endpoint_));
  assert(rc == 0 || (rc < 0 && errno == EINPROGRESS));

  bool is_connected = false;
  bool is_timeout = false;
  while (connect_thread_run_ && !is_connected && !is_timeout) {
    if (sock_ < 0)
      break;

    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(sock_, &wset);
    struct timeval timeout = { 0, 1 };

    if (select(sock_ + 1, NULL, &wset, NULL, &timeout) > 0) {
      if (FD_ISSET(sock_, &wset)) {
        int e;
        socklen_t e_size = sizeof(e);
        int r = getsockopt(sock_, SOL_SOCKET, SO_ERROR, (char *)&e, &e_size);
        if (r == 0) {
          is_connected = true;

          // Makes a state transition.
          state_ = kConnected;

          recv_thread_ = std::thread([this] {
            Recv();
          });

          send_thread_ = std::thread([this] {
            Send();
          });

          // To get a session id
          PushTaskQueue([this]{ SendEmptyMessage(protocol_, encoding_); });
        }
        else {
          assert(r < 0 && errno == EBADF);
          FUNAPI_LOG("failed - tcp connect");

          PushTaskQueue([this]{ on_transport_failure_(TransportProtocol::kTcp); });
        }
      }
    }
    else {
      time_t now = time(NULL);
      if (now > connect_timeout) {
        FUNAPI_LOG("failed - tcp connect - timeout");
        is_timeout = true;

        PushTaskQueue([this]{ on_connect_timeout_(TransportProtocol::kTcp); });
      }
    }
  }

  return is_connected;
}


void FunapiTcpTransportImpl::Connect() {
  const int kMaxReconnectCount = 3;

  for (auto addr : in_addrs_) {
    time_t wait_time = 1;
    bool is_connected = false;
    Endpoint endpoint;
    endpoint.sin_family = AF_INET;
    endpoint.sin_addr = addr;
    endpoint.sin_port = htons(port_);

    for (int i=0;i<=kMaxReconnectCount;++i) {
      CloseSocket();
      InitSocket();

      is_connected = Connect(endpoint);
      if (!connect_thread_run_ || is_connected || !auto_reconnect_) break;

      // wait time
      time_t delay_time = time(NULL) + wait_time;
      FUNAPI_LOG("Wait %d seconds for connect to TCP transport.", (int)wait_time);
      while (connect_thread_run_ && time(NULL) < delay_time) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      if (!connect_thread_run_ || is_connected) break;

      wait_time *= 2;
    }

    if (!connect_thread_run_ || is_connected) break;
  }

  connect_thread_run_ = false;
}


void FunapiTcpTransportImpl::Recv() {
  std::vector<uint8_t> receiving_vector;
  fd_set rset;

  int next_decoding_offset = 0;
  bool header_decoded = false;
  HeaderFields header_fields;

  // ping
  time_t now = time(NULL);
  time_t ping_send_time = now;
  ping_client_timeout_ = now + kPingIntervalSecond + kPingTimeoutSeconds;

  while (recv_thread_run_)
  {
    if (sock_ < 0)
      break;

    FD_ZERO(&rset);
    FD_SET(sock_, &rset);
    struct timeval timeout = { 0, 1 };

    if (select(sock_ + 1, &rset, NULL, NULL, &timeout) > 0) {
      if (FD_ISSET(sock_, &rset)) {
        std::vector<char> buffer(kUnitBufferSize);

        int nRead =
          (int)recv(sock_, buffer.data(), kUnitBufferSize, 0);

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
          return;
        }
      }
    }

    if (enable_ping_) {
      time_t now = time(NULL);
      if (now >= ping_send_time) {
        auto network = network_.lock();
        if (network) {
          if (network->SendClientPingMessage(protocol_)) {
            ping_send_time = now + kPingIntervalSecond;
          }
          else {
            ping_client_timeout_ = now + kPingIntervalSecond + kPingTimeoutSeconds;
          }
        }
      }
      if (now >= ping_client_timeout_) {
        FUNAPI_LOG("Network seems disabled. Stopping the transport.");
        PushStopTask();
        return;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportImpl implementation.

class FunapiUdpTransportImpl : public FunapiTransportImpl {
public:
  FunapiUdpTransportImpl(TransportProtocol protocol,
    const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiUdpTransportImpl();

  void Start();

protected:
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body);
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
  JoinThread();
}


void FunapiUdpTransportImpl::Start() {
  CloseSocket();
  JoinThread();

  sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(sock_ >= 0);
  state_ = kConnected;

  send_thread_run_ = true;
  recv_thread_run_ = true;

  recv_thread_ = std::thread([this] {
    Recv();
  });

  send_thread_ = std::thread([this] {
    Send();
  });

  // To get a session id
  PushTaskQueue([this]{ SendEmptyMessage(protocol_, encoding_); });
}


void FunapiUdpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return;
  }

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("Send = %s", *FString(temp_string.c_str()));

  fd_set wset;
  struct timeval timeout = { 0, 1 };
  socklen_t len = sizeof(endpoint_);

  if (sock_ < 0)
    return;

  FD_ZERO(&wset);
  FD_SET(sock_, &wset);

  if (select(sock_ + 1, NULL, &wset, NULL, &timeout) > 0) {
    if (FD_ISSET(sock_, &wset)) {
      int nSent = (int)sendto(sock_, (char*)body.data(), body.size(), 0, (struct sockaddr*)&endpoint_, len);

      if (nSent < 0) {
        PushStopTask();
        return;
      }

      // FUNAPI_LOG("Sent %d bytes", nSent);
    }
  }
}


void FunapiUdpTransportImpl::Recv() {
  std::vector<uint8_t> receiving_vector(kUnitBufferSize);
  fd_set rset;
  socklen_t len = sizeof(recv_endpoint_);

  while (recv_thread_run_)
  {
    if (sock_<0)
      break;

    FD_ZERO(&rset);
    FD_SET(sock_, &rset);
    struct timeval timeout = { 0, 1 };

    if (select(sock_ + 1, &rset, NULL, NULL, &timeout) > 0) {
      if (FD_ISSET(sock_, &rset)) {
        int nRead =
          (int)recvfrom(sock_, (char*)receiving_vector.data(), receiving_vector.size(), 0, (struct sockaddr*)&recv_endpoint_, &len);

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
          return;
        }
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransportImpl implementation.

class FunapiHttpTransportImpl : public FunapiTransportBase {
 public:
  FunapiHttpTransportImpl(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransportImpl();

  void Start();
  void Stop();

 protected:
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body);

 private:
  static size_t HttpResponseCb(void *data, size_t size, size_t count, void *cb);
  void WebResponseHeaderCb(void *data, int len, HeaderFields &header_fields);
  void WebResponseBodyCb(void *data, int len, std::vector<uint8_t> &receiving);

  std::string host_url_;
};


FunapiHttpTransportImpl::FunapiHttpTransportImpl(const std::string &hostname_or_ip,
                                                 uint16_t port, bool https, FunEncoding encoding)
  : FunapiTransportBase(TransportProtocol::kHttp, encoding) {
  encoding_ = encoding;

  char url[1024];
  sprintf(url, "%s://%s:%d/v%d/",
      https ? "https" : "http", hostname_or_ip.c_str(), port,
      (int)FunapiVersion::kProtocolVersion);
  host_url_ = url;

  FUNAPI_LOG("Host url : %s", host_url_.c_str());
}


FunapiHttpTransportImpl::~FunapiHttpTransportImpl() {
  JoinThread();
}


void FunapiHttpTransportImpl::Start() {
  state_ = kConnected;
  FUNAPI_LOG("Started.");

  JoinThread();

  send_thread_run_ = true;
  send_thread_ = std::thread([this] {
    Send();
  });

  // To get a session id
  PushTaskQueue([this]{ SendEmptyMessage(protocol_, encoding_); });
}


void FunapiHttpTransportImpl::Stop() {
  if (state_ == kDisconnected)
    return;

  state_ = kDisconnected;
  FUNAPI_LOG("Stopped.");

  JoinThread();
}


size_t FunapiHttpTransportImpl::HttpResponseCb(void *data, size_t size, size_t count, void *cb) {
  AsyncWebResponseCallback *callback = (AsyncWebResponseCallback*)(cb);
  if (callback != NULL)
    (*callback)(data, (int)(size * count));
  return size * count;
}


void FunapiHttpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  std::string header = MakeHeaderString(body);

  // log
  // std::string temp_string(body.cbegin(), body.cend());
  // FUNAPI_LOG("HTTP Send header = %s \n body = %s", *FString(header.c_str()), *FString(temp_string.c_str()));

  CURL *ctx = curl_easy_init();
  if (ctx == NULL) {
    FUNAPI_LOG("Unable to initialize cURL interface.");
    return;
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
    assert(false);
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


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

FunapiTcpTransport::FunapiTcpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiTcpTransportImpl>(TransportProtocol::kTcp, hostname_or_ip, port, encoding)) {
}


TransportProtocol FunapiTcpTransport::Protocol() const {
  return TransportProtocol::kTcp;
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


FunEncoding FunapiTcpTransport::Encoding() const {
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


void FunapiTcpTransport::SetDisableNagle(bool disable_nagle) {
  return impl_->SetDisableNagle(disable_nagle);
}


void FunapiTcpTransport::SetAutoReconnect(bool auto_reconnect) {
  return impl_->SetAutoReconnect(auto_reconnect);
}


void FunapiTcpTransport::SetEnablePing(bool enable_ping) {
  return impl_->SetEnablePing(enable_ping);
}


void FunapiTcpTransport::ResetPingClientTimeout() {
  return impl_->ResetPingClientTimeout();
}

////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

FunapiUdpTransport::FunapiUdpTransport (const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding)
  : impl_(std::make_shared<FunapiUdpTransportImpl>(TransportProtocol::kUdp, hostname_or_ip, port, encoding)) {
}


TransportProtocol FunapiUdpTransport::Protocol() const {
  return TransportProtocol::kUdp;
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


FunEncoding FunapiUdpTransport::Encoding() const {
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

////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

FunapiHttpTransport::FunapiHttpTransport (const std::string &hostname_or_ip,
  uint16_t port, bool https, FunEncoding encoding)
  : impl_(std::make_shared<FunapiHttpTransportImpl>(hostname_or_ip, port, https, encoding)) {
}


TransportProtocol FunapiHttpTransport::Protocol() const {
  return TransportProtocol::kHttp;
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


FunEncoding FunapiHttpTransport::Encoding() const {
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


////////////////////////////////////////////////////////////////////////////////
// FunapiTransport implementation.

void FunapiTransport::RegisterEventHandlers(
  const OnReceived &on_received, const OnStopped &on_stopped) {
  impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiTransport::Start() {
  impl_->Start();
}


void FunapiTransport::Stop() {
  impl_->Stop();
}


void FunapiTransport::SendMessage(rapidjson::Document &message) {
  impl_->SendMessage(message);
}


void FunapiTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}


void FunapiTransport::SendMessage(const char *body) {
  impl_->SendMessage(body);
}


bool FunapiTransport::Started() const {
  return impl_->Started();
}


void FunapiTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network) {
  impl_->SetNetwork(network);
}


FunEncoding FunapiTransport::Encoding() const {
  return impl_->GetEncoding();
}


void FunapiTransport::SetConnectTimeout(time_t timeout) {
  return impl_->SetConnectTimeout(timeout);
}


void FunapiTransport::AddStartedCallback(const TransportEventHandler &handler) {
  return impl_->AddStartedCallback(handler);
}


void FunapiTransport::AddStoppedCallback(const TransportEventHandler &handler) {
  return impl_->AddStoppedCallback(handler);
}


void FunapiTransport::AddFailureCallback(const TransportEventHandler &handler) {
  return impl_->AddFailureCallback(handler);
}


void FunapiTransport::AddConnectTimeoutCallback(const TransportEventHandler &handler) {
  return impl_->AddConnectTimeoutCallback(handler);
}


void FunapiTransport::ResetPingClientTimeout() {
  return impl_->ResetPingClientTimeout();
}

}  // namespace fun
