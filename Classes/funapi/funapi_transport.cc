﻿// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_network.h"


namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiTransportBase implementation.

class FunapiTransportBase : public std::enable_shared_from_this<FunapiTransportBase> {
 public:
  // Buffer-related constants.
  const int kUnitBufferSize = 65536;

  // Funapi header-related constants.
  std::string kHeaderDelimeter = "\n";
  std::string kHeaderFieldDelimeter = ":";
  std::string kVersionHeaderField = "VER";
  std::string kLengthHeaderField = "LEN";

  typedef FunapiTransport::OnReceived OnReceived;
  typedef FunapiTransport::OnStopped OnStopped;

  FunapiTransportBase(FunapiTransportType type);
  virtual ~FunapiTransportBase();

  bool Started();
  virtual void Start() = 0;
  virtual void Stop() = 0;

  void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);

  void SendMessage(Json &message);
  // void SendMessage(FJsonObject &message);
  void SendMessage(FunMessage &message);

  void SetNetwork(std::weak_ptr<FunapiNetwork> network);

 protected:
  typedef std::map<std::string, std::string> HeaderFields;

  void SendMessage(const char *body);
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body) = 0;

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
  FunapiTransportType type_;
  FunapiTransportState state_;

  std::weak_ptr<FunapiNetwork> network_;

  std::queue<std::function<void()>> send_queue_;
  std::mutex send_queue_mutex_;
  std::condition_variable_any send_queue_condition_;

  bool send_thread_run_ = false;
  std::thread send_thread_;
};


FunapiTransportBase::FunapiTransportBase(FunapiTransportType type)
  : type_(type), state_(kDisconnected) {
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


void FunapiTransportBase::SendMessage(Json &message) {
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


bool FunapiTransportBase::TryToDecodeHeader(std::vector<uint8_t> &receiving, int &next_decoding_offset, bool &header_decoded, HeaderFields &header_fields) {
  FUNAPI_LOG("Trying to decode header fields.");
  int received_size = (int)receiving.size();
  for (; next_decoding_offset < received_size;) {
    char *base = reinterpret_cast<char *>(receiving.data());
    char *ptr =
      std::search(base + next_decoding_offset,
      base + received_size,
      kHeaderDelimeter.c_str(),
      kHeaderDelimeter.c_str() + kHeaderDelimeter.length());

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
      line, line + line_length, kHeaderFieldDelimeter.c_str(),
      kHeaderFieldDelimeter.c_str() + kHeaderFieldDelimeter.length());
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
    PushTaskQueue([this, header_fields, v](){ on_received_(header_fields, v); });
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
    kVersionHeaderField.c_str(), kHeaderFieldDelimeter.c_str(),
    (int)FunapiVersion::kProtocolVersion, kHeaderDelimeter.c_str());
  header += buffer;

  snprintf(buffer, sizeof(buffer), "%s%s%lu%s",
    kLengthHeaderField.c_str(), kHeaderFieldDelimeter.c_str(),
    (unsigned long)body.size(), kHeaderDelimeter.c_str());
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


////////////////////////////////////////////////////////////////////////////////
// FunapiTransportImpl implementation.

class FunapiTransportImpl : public FunapiTransportBase {
 public:
   FunapiTransportImpl(FunapiTransportType type,
                      const std::string &hostname_or_ip, uint16_t port);
   virtual ~FunapiTransportImpl();
   void Stop();

protected:
  virtual void JoinThread();
  void CloseSocket();
  virtual void Recv() = 0;

  // State-related.
  Endpoint endpoint_;
  int sock_;

  bool recv_thread_run_ = false;
  std::thread recv_thread_;
};


FunapiTransportImpl::FunapiTransportImpl(FunapiTransportType type,
                                         const std::string &hostname_or_ip,
                                         uint16_t port)
    : FunapiTransportBase(type), sock_(-1) {

  struct hostent *entry = gethostbyname(hostname_or_ip.c_str());
  assert(entry);

  struct in_addr addr;

  if (entry) {
    struct in_addr **l = reinterpret_cast<struct in_addr **>(entry->h_addr_list);
    assert(l);
    assert(l[0]);

    addr = *l[0];
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
  FunapiTcpTransportImpl(FunapiTransportType type,
    const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTcpTransportImpl();

  void Start();

protected:
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body);
  virtual void JoinThread();
  void Connect();
  void Recv();

  bool connect_thread_run_ = false;
  std::thread connect_thread_;
};


FunapiTcpTransportImpl::FunapiTcpTransportImpl(FunapiTransportType type,
  const std::string &hostname_or_ip,
  uint16_t port)
  : FunapiTransportImpl(type, hostname_or_ip, port) {
}


FunapiTcpTransportImpl::~FunapiTcpTransportImpl() {
  CloseSocket();
  JoinThread();
}


void FunapiTcpTransportImpl::Start() {
  CloseSocket();
  JoinThread();

  // Initiates a new socket.
  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock_ >= 0);

  state_ = kConnecting;

  // connecting
  connect_thread_run_ = true;
  send_thread_run_ = true;
  recv_thread_run_ = true;
  connect_thread_ = std::thread([this] {
    Connect();
  });
}


void FunapiTcpTransportImpl::EncodeThenSendMessage(std::vector<uint8_t> body) {
  if (!EncodeMessage(body)) {
    PushStopTask();
    return;
  }

  fd_set wset;
  int offset = 0;

  if (sock_ < 0)
    return;

  do {
    FD_ZERO(&wset);
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


void FunapiTcpTransportImpl::Connect() {
  // Makes the fd non-blocking.
#if PLATFORM_WINDOWS || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
  u_long argp = 0;
  int flag = ioctlsocket(sock_, FIONBIO, &argp);
  assert(flag >= 0);
  int rc;
#else
  int flag = fcntl(sock_, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(sock_, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);
#endif

  time_t connect_timeout = time(NULL) + 10;

  // Tries to connect.
  rc = connect(sock_,
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
        if (getsockopt(sock_, SOL_SOCKET, SO_ERROR, (char *)&e, &e_size) == 0) {
          is_connected = true;

          // Makes a state transition.
          state_ = kConnected;

          recv_thread_ = std::thread([this] {
            Recv();
          });

          send_thread_ = std::thread([this] {
            Send();
          });
        }
        else {
          // assert(r < 0 && errno == EBADF);
          FUNAPI_LOG("failed - tcp connect");
        }
      }
    }
    else {
      time_t now = time(NULL);
      if (now > connect_timeout) {
        FUNAPI_LOG("failed - tcp connect - timeout");
        is_timeout = true;
      }
    }
  }
}


void FunapiTcpTransportImpl::Recv() {
  std::vector<uint8_t> receiving_vector;
  fd_set rset;

  int next_decoding_offset = 0;
  bool header_decoded = false;
  HeaderFields header_fields;

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

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}


////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransportImpl implementation.

class FunapiUdpTransportImpl : public FunapiTransportImpl {
public:
  FunapiUdpTransportImpl(FunapiTransportType type,
    const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiUdpTransportImpl();

  void Start();

protected:
  virtual void EncodeThenSendMessage(std::vector<uint8_t> body);
  void Recv();

private:
  Endpoint recv_endpoint_;
};


FunapiUdpTransportImpl::FunapiUdpTransportImpl(FunapiTransportType type,
  const std::string &hostname_or_ip,
  uint16_t port)
  : FunapiTransportImpl(type, hostname_or_ip, port) {
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

  sock_ = socket(AF_INET, SOCK_DGRAM, 0);
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
  FunapiHttpTransportImpl(const std::string &hostname_or_ip, uint16_t port, bool https);
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
                                                 uint16_t port, bool https)
                                                 : FunapiTransportBase(kHttp) {
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

  char *ptr = std::search(buf, buf + len, kHeaderFieldDelimeter.c_str(),
      kHeaderFieldDelimeter.c_str() + kHeaderFieldDelimeter.length());
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

FunapiTcpTransport::FunapiTcpTransport (const std::string &hostname_or_ip, uint16_t port) {
  impl_ = std::make_shared<FunapiTcpTransportImpl>(kTcp, hostname_or_ip, port);
}

TransportProtocol FunapiTcpTransport::Protocol() const {
  return TransportProtocol::kTcp;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiUdpTransport implementation.

FunapiUdpTransport::FunapiUdpTransport (const std::string &hostname_or_ip, uint16_t port) {
  impl_ = std::make_shared<FunapiUdpTransportImpl>(kUdp, hostname_or_ip, port);
}

TransportProtocol FunapiUdpTransport::Protocol() const {
  return TransportProtocol::kUdp;
}

////////////////////////////////////////////////////////////////////////////////
// FunapiHttpTransport implementation.

FunapiHttpTransport::FunapiHttpTransport (const std::string &hostname_or_ip,
  uint16_t port,
  bool https /*= false*/) {
  impl_ = std::make_shared<FunapiHttpTransportImpl>(hostname_or_ip, port, https);
}

TransportProtocol FunapiHttpTransport::Protocol() const {
  return TransportProtocol::kHttp;
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

void FunapiTransport::SendMessage(Json &message) {
  impl_->SendMessage(message);
}

void FunapiTransport::SendMessage(FunMessage &message) {
  impl_->SendMessage(message);
}

bool FunapiTransport::Started() const {
  return impl_->Started();
}

void FunapiTransport::SetNetwork(std::weak_ptr<FunapiNetwork> network)
{
  impl_->SetNetwork(network);
}

}  // namespace fun
