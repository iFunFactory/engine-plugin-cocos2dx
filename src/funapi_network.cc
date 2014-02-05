// Copyright (C) 2013 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include ".//funapi_network.h"


namespace fun {

using std::map;


////////////////////////////////////////////////////////////////////////////////
// Types.

typedef sockaddr_in Endpoint;
typedef helper::Binder1<void, int, void *> AsyncConnectCallback;
typedef helper::Binder1<void, ssize_t, void *> AsyncSendCallback;
typedef helper::Binder1<void, ssize_t, void *> AsyncReceiveCallback;


struct AsyncRequest {
  enum RequestType {
    kConnect = 0,
    kSend,
    kReceive,
  };

  RequestType type_;
  int sock_;

  struct {
    AsyncConnectCallback callback_;
    Endpoint endpoint_;
  } connect_;

  struct {
    AsyncSendCallback callback_;
    uint8_t *buf_;
    size_t buf_len_;
    size_t buf_offset_;
  } send_;

  struct {
    AsyncReceiveCallback callback_;
    uint8_t *buf_;
    size_t capacity_;
  } receive_;
};



////////////////////////////////////////////////////////////////////////////////
// Constants.

// Buffer-related constants.
const int kUnitBufferSize = 65536;

// Funapi header-related constants.
const char kHeaderDelimeter[] = "\n";
const char kHeaderFieldDelimeter[] = ":";
const char kVersionHeaderField[] = "VER";
const char kLengthHeaderField[] = "LEN";
const int kCurrentFunapiProtocolVersion = 1;

// Funapi message-related constants.
const char kMsgTypeBodyField[] = "msgtype";
const char kSessionIdBodyField[] = "sid";
const char kNewSessionMessageType[] = "_session_opened";
const char kSessionClosedMessageType[] = "_session_closed";



////////////////////////////////////////////////////////////////////////////////
// Global variables.

bool initialized = false;
time_t epoch = 0;
time_t funapi_session_timeout = 3600;
std::ostream *logstream = &(std::cout);

typedef std::list<AsyncRequest> AsyncQueue;
AsyncQueue async_queue;

pthread_t async_queue_thread;
bool async_thread_run = false;
pthread_mutex_t async_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t async_queue_cond = PTHREAD_COND_INITIALIZER;



////////////////////////////////////////////////////////////////////////////////
// Helper functions.

struct timespec operator+(
    const struct timespec &t1, const struct timespec &t2) {
  struct timespec r = {t1.tv_sec + t2.tv_sec, t1.tv_nsec + t2.tv_nsec};
  if (r.tv_nsec > 1000 * 1000 * 1000) {
    r.tv_sec += 1;
    r.tv_nsec -= 1000 * 1000 * 1000;
  }
  return r;
}


inline bool operator<(const struct timespec &t1, const struct timespec &t2) {
  if (t1.tv_sec < t2.tv_sec) {
    return true;
  } else if (t1.tv_sec == t2.tv_sec) {
    return t1.tv_nsec < t2.tv_nsec;
  } else {
    return false;
  }
}


inline bool operator==(const struct timespec &t1, const struct timespec &t2) {
  return (t1.tv_sec == t2.tv_sec && t1.tv_nsec == t2.tv_nsec);
}


inline bool operator!=(const struct timespec &t1, const struct timespec &t2) {
  return not operator==(t1, t2);
}


#define LOG(x) (*logstream) << "[" << __FILE__ << ":" << __LINE__ << "] " \
    << x << std::endl


////////////////////////////////////////////////////////////////////////////////
// Asynchronous operation related..

void *AsyncQueueThreadProc(void * /*arg*/) {
  LOG("Thread for async operation has been created.");

  while (async_thread_run) {
    // Waits until we have something to process.
    pthread_mutex_lock(&async_queue_mutex);
    while (async_thread_run && async_queue.empty()) {
      pthread_cond_wait(&async_queue_cond, &async_queue_mutex);
    }

    // Moves element to a worker queue and releaes the mutex
    // for contention prevention.
    AsyncQueue work_queue;
    work_queue.swap(async_queue);
    pthread_mutex_unlock(&async_queue_mutex);

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    // Makes fd_sets for select().
    int max_fd = -1;
    for (AsyncQueue::const_iterator i = work_queue.begin();
         i != work_queue.end(); ++i) {
      if (i->type_ == AsyncRequest::kConnect) {
        // LOG("Checking connect with fd [" << i->sock_ << "]");
        FD_SET(i->sock_, &wset);
      } else if (i->type_ == AsyncRequest::kSend) {
        // LOG("Checking send with fd [" << i->sock_ << "]");
        FD_SET(i->sock_, &wset);
      } else if (i->type_ == AsyncRequest::kReceive) {
        // LOG("Checking receive with fd [" << i->sock_ << "]");
        FD_SET(i->sock_, &rset);
      } else {
        assert(false);
      }
      max_fd = std::max(max_fd, i->sock_);
    }

    // Waits until some events happen to fd in the sets.
    struct timeval timeout = {0, 1};
    int r = select(max_fd + 1, &rset, &wset, NULL, &timeout);

    // Some fd can be invalidated if other thread closed.
    assert(r >= 0 || (r < 0 && errno == EBADF));
    // LOG("woke up: " << r);

    // Checks if the fd is ready.
    for (AsyncQueue::iterator i = work_queue.begin(); i != work_queue.end(); ) {
      bool remove_request = true;
      // Ready. Handles accordingly.
      if (i->type_ == AsyncRequest::kConnect) {
        if (not FD_ISSET(i->sock_, &wset)) {
          remove_request = false;
        } else {
          int e;
          socklen_t e_size = sizeof(e);
          int r = getsockopt(i->sock_, SOL_SOCKET, SO_ERROR, &e, &e_size);
          if (r == 0) {
            i->connect_.callback_(e);
          } else {
            assert(r < 0 && errno == EBADF);
          }
        }
      } else if (i->type_ == AsyncRequest::kSend) {
        if (not FD_ISSET(i->sock_, &wset)) {
          remove_request = false;
        } else {
          ssize_t nSent =
              send(i->sock_, i->send_.buf_ + i->send_.buf_offset_,
                  i->send_.buf_len_ - i->send_.buf_offset_, 0);
          if (nSent < 0) {
            i->send_.callback_(nSent);
          } else if (nSent + i->send_.buf_offset_ == i->send_.buf_len_) {
            i->send_.callback_(i->send_.buf_len_);
          } else {
            i->send_.buf_offset_ += nSent;
            remove_request = false;
          }
        }
      } else if (i->type_ == AsyncRequest::kReceive) {
        if (not FD_ISSET(i->sock_, &rset)) {
          remove_request = false;
        } else {
          ssize_t nRead =
              recv(i->sock_, i->receive_.buf_, i->receive_.capacity_, 0);
          i->receive_.callback_(nRead);
        }
      }

      // If we should keep the fd, puts it back to the queue.
      if (remove_request) {
        AsyncQueue::iterator to_delete = i++;
        work_queue.erase(to_delete);
      } else {
        ++i;
      }
    }

    // Puts back requests that requires more work.
    // We should respect the order.
    pthread_mutex_lock(&async_queue_mutex);
    async_queue.splice(async_queue.begin(), work_queue);
    pthread_mutex_unlock(&async_queue_mutex);
  }

  LOG("Thread for async operation is terminating.");
  return NULL;
}


void AsyncConnect(
    int sock, const Endpoint &endpoint, const AsyncConnectCallback &callback) {
  // Makes the fd non-blocking.
  int flag = fcntl(sock, F_GETFL);
  assert(flag >= 0);
  int rc = fcntl(sock, F_SETFL, O_NONBLOCK | flag);
  assert(rc >= 0);

  // Tries to connect.
  rc = connect(sock, reinterpret_cast<const struct sockaddr *>(&endpoint),
               sizeof(endpoint));
  assert(rc == 0 || (rc < 0 && errno == EINPROGRESS));

  LOG("Queueing 1 async connect.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kConnect;
  r.sock_ = sock;
  r.connect_.callback_ = callback;
  r.connect_.endpoint_ = endpoint;

  pthread_mutex_lock(&async_queue_mutex);
  async_queue.push_back(r);
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}


void AsyncSend(int sock, const std::vector<struct iovec> sending,
               AsyncSendCallback callback) {
  assert(not sending.empty());
  LOG("Queueing " << sending.size() << " async sends.");

  pthread_mutex_lock(&async_queue_mutex);
  for (size_t i = 0; i < sending.size(); ++i) {
    const struct iovec &e = sending[i];
    AsyncRequest r;
    r.type_ = AsyncRequest::kSend;
    r.sock_ = sock;
    r.send_.callback_ = callback;
    r.send_.buf_ = reinterpret_cast<uint8_t *>(e.iov_base);
    r.send_.buf_len_ = e.iov_len;
    r.send_.buf_offset_ = 0;
    async_queue.push_back(r);
  }
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}


void AsyncReceive(
    int sock, const struct iovec &receiving, AsyncReceiveCallback callback) {
  assert(receiving.iov_len != 0);
  LOG("Queueing 1 async receive.");

  AsyncRequest r;
  r.type_ = AsyncRequest::kReceive;
  r.sock_ = sock;
  r.receive_.callback_ = callback;
  r.receive_.buf_ = reinterpret_cast<uint8_t *>(receiving.iov_base);
  r.receive_.capacity_ = receiving.iov_len;

  pthread_mutex_lock(&async_queue_mutex);
  async_queue.push_back(r);
  pthread_cond_signal(&async_queue_cond);
  pthread_mutex_unlock(&async_queue_mutex);
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransportImpl implementation.

class FunapiTcpTransportImpl {
 public:
  typedef FunapiTcpTransport::HeaderType HeaderType;
  typedef FunapiTcpTransport::OnReceived OnReceived;
  typedef FunapiTcpTransport::OnStopped OnStopped;

  FunapiTcpTransportImpl(const string &hostname_or_ip, uint16_t port);
  ~FunapiTcpTransportImpl();

  void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  void Start();
  void Stop();
  void SendMessage(rapidjson::Document &message);
  bool Started() const;

 private:
  static void StartCbWrapper(int rc, void *arg);
  static void SendMessageCbWrapper(ssize_t rc, void *arg);
  static void ReceiveBytesCbWrapper(ssize_t nRead, void *arg);

  void StartCb(int rc);
  void SendMessageCb(ssize_t rc);
  void ReceiveBytesCb(ssize_t nRead);

  bool TryToDecodeHeader();
  bool TryToDecodeBody();

  enum FunapiTcpTransportState {
      kDisconnected = 0,
      kConnecting,
      kConnected,
  };

  // Registered event handlers.
  OnReceived on_received_;
  OnStopped on_stopped_;

  // State-related.
  mutable pthread_mutex_t mutex_;
  FunapiTcpTransportState state_;
  Endpoint endpoint_;
  int sock_;
  std::vector<struct iovec> sending_;
  std::vector<struct iovec> pending_;
  struct iovec receiving_;
  bool header_decoded_;
  int received_size_;
  int next_decoding_offset_;
  typedef map<string, string> HeaderFields;
  HeaderFields header_fields_;
};


FunapiTcpTransportImpl::FunapiTcpTransportImpl(const string &hostname_or_ip,
                                               uint16_t port)
    : state_(kDisconnected), sock_(-1),
      header_decoded_(false), received_size_(0), next_decoding_offset_(0) {
  pthread_mutex_init(&mutex_, NULL);

  struct hostent *entry = gethostbyname(hostname_or_ip.c_str());
  assert(entry);

  struct in_addr **l = reinterpret_cast<struct in_addr **>(entry->h_addr_list);
  assert(l);
  assert(l[0]);

  endpoint_.sin_family = AF_INET;
  endpoint_.sin_addr = *l[0];
  endpoint_.sin_port = htons(port);

  receiving_.iov_len = kUnitBufferSize;
  receiving_.iov_base = new uint8_t[receiving_.iov_len];
  assert(receiving_.iov_base);
}


FunapiTcpTransportImpl::~FunapiTcpTransportImpl() {
  header_fields_.clear();

  for (size_t i = 0; i < sending_.size(); ++i) {
    struct iovec &e = sending_[i];
    delete [] reinterpret_cast<uint8_t *>(e.iov_base);
  }
  sending_.clear();

  for (size_t i = 0; i < pending_.size(); ++i) {
    struct iovec &e = pending_[i];
    delete [] reinterpret_cast<uint8_t *>(e.iov_base);
  }
  pending_.clear();

  if (receiving_.iov_base) {
    delete [] reinterpret_cast<uint8_t *>(receiving_.iov_base);
    receiving_.iov_base = NULL;
  }
}


void FunapiTcpTransportImpl::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  pthread_mutex_lock(&mutex_);
  on_received_ = on_received;
  on_stopped_ = on_stopped;
  pthread_mutex_unlock(&mutex_);
}


void FunapiTcpTransportImpl::Start() {
  pthread_mutex_lock(&mutex_);

  // Resets states.
  header_decoded_ = false;
  received_size_ = 0;
  next_decoding_offset_ = 0;
  header_fields_.clear();
  for (int i = 0; i < sending_.size(); ++i) {
    delete [] reinterpret_cast<uint8_t *>(sending_[i].iov_base);
  }
  sending_.clear();

  // Initiates a new socket.
  state_ = kConnecting;
  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock_ >= 0);
  AsyncConnect(sock_, endpoint_,
               AsyncConnectCallback(&FunapiTcpTransportImpl::StartCbWrapper,
                                    (void *) this));
  pthread_mutex_unlock(&mutex_);
}


void FunapiTcpTransportImpl::Stop() {
  pthread_mutex_lock(&mutex_);
  state_ = kDisconnected;
  if (sock_ >= 0) {
    close(sock_);
    sock_ = -1;
  }
  pthread_mutex_unlock(&mutex_);
}


void FunapiTcpTransportImpl::SendMessage(rapidjson::Document &message) {
  rapidjson::StringBuffer string_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
  message.Accept(writer);
  const char *body = string_buffer.GetString();

  struct iovec body_as_bytes;
  body_as_bytes.iov_len = strlen(body);
  body_as_bytes.iov_base = new uint8_t[body_as_bytes.iov_len];
  memcpy(body_as_bytes.iov_base, body, body_as_bytes.iov_len);

  char header[65536];
  size_t offset = 0;
  offset += snprintf(header + offset, sizeof(header) - offset, "%s%s%d%s",
                     kVersionHeaderField, kHeaderFieldDelimeter,
                     kCurrentFunapiProtocolVersion, kHeaderDelimeter);
  offset += snprintf(header + offset, sizeof(header)- offset, "%s%s%lu%s",
                     kLengthHeaderField, kHeaderFieldDelimeter,
                     body_as_bytes.iov_len, kHeaderDelimeter);
  offset += snprintf(header + offset, sizeof(header)- offset, "%s",
                     kHeaderDelimeter);

  struct iovec header_as_bytes;
  header_as_bytes.iov_len = offset;
  header_as_bytes.iov_base = new uint8_t[header_as_bytes.iov_len];
  memcpy(header_as_bytes.iov_base, header, header_as_bytes.iov_len);

  LOG("Header to send: " << header);
  LOG("JSON to send: " << body);

  pthread_mutex_lock(&mutex_);
  bool failed = false;
  pending_.push_back(header_as_bytes);
  pending_.push_back(body_as_bytes);
  if (state_ == kConnected && sending_.size() == 0) {
    sending_.swap(pending_);
    AsyncSend(sock_, sending_,
              AsyncSendCallback(&FunapiTcpTransportImpl::SendMessageCbWrapper,
                                (void *) this));
  }
  pthread_mutex_unlock(&mutex_);
}


bool FunapiTcpTransportImpl::Started() const {
  pthread_mutex_lock(&mutex_);
  bool r = (state_ == kConnected);
  pthread_mutex_unlock(&mutex_);
  return r;
}


void FunapiTcpTransportImpl::StartCbWrapper(int rc, void *arg) {
  FunapiTcpTransportImpl *obj = reinterpret_cast<FunapiTcpTransportImpl *>(arg);
  obj->StartCb(rc);
}


void FunapiTcpTransportImpl::SendMessageCbWrapper(ssize_t nSent, void *arg) {
  FunapiTcpTransportImpl *obj = reinterpret_cast<FunapiTcpTransportImpl *>(arg);
  obj->SendMessageCb(nSent);
}


void FunapiTcpTransportImpl::ReceiveBytesCbWrapper(ssize_t nRead, void *arg) {
  FunapiTcpTransportImpl *obj = reinterpret_cast<FunapiTcpTransportImpl *>(arg);
  obj->ReceiveBytesCb(nRead);
}


void FunapiTcpTransportImpl::StartCb(int rc) {
  LOG("StartCb called");

  if (rc != 0) {
    LOG("Connect failed: " << strerror(rc));
    Stop();
    on_stopped_();
    return;
  }

  LOG("Connected.");

  pthread_mutex_lock(&mutex_);

  // Makes a state transition.
  state_ = kConnected;

  // Start to handle incoming messages.
  AsyncReceive(
      sock_, receiving_,
      AsyncReceiveCallback(
          &FunapiTcpTransportImpl::ReceiveBytesCbWrapper, (void *) this));
  LOG("Ready to receive");

  // Starts to process if there any data already queue.
  if (pending_.size() > 0) {
    LOG("Flushing pending messages.");
    sending_.swap(pending_);
    AsyncSend(
        sock_, sending_,
        AsyncSendCallback(
            &FunapiTcpTransportImpl::SendMessageCbWrapper, (void *) this));
  }
  pthread_mutex_unlock(&mutex_);
}


void FunapiTcpTransportImpl::SendMessageCb(ssize_t nSent) {
  if (nSent < 0) {
    LOG("send failed: " << strerror(errno));
    Stop();
    on_stopped_();
    return;
  }

  LOG("Sent " << nSent << "bytes");

  pthread_mutex_lock(&mutex_);

  // Now releases memory that we have been holding for transmission.
  assert(not sending_.empty());
  assert(sending_[0].iov_len == nSent);
  sending_.erase(sending_.begin());

  if (sending_.size() == 0 && pending_.size() > 0) {
    sending_.swap(pending_);
    AsyncSend(
        sock_, sending_,
        AsyncSendCallback(
            &FunapiTcpTransportImpl::SendMessageCbWrapper, (void *) this));
  }

  pthread_mutex_unlock(&mutex_);
}


void FunapiTcpTransportImpl::ReceiveBytesCb(ssize_t nRead) {
  if (nRead <= 0) {
    if (nRead < 0) {
      LOG("receive failed: " << strerror(errno));
    } else {
      LOG("Socket [" << sock_ << "] closed.");
      if (received_size_ > next_decoding_offset_) {
        LOG("Buffer has " << (received_size_ - next_decoding_offset_) \
            << "bytes. But they failed to decode. Discarding.");
      }
    }
    Stop();
    on_stopped_();
    return;
  }

  LOG("Received " << nRead << "bytes. " \
      << "Buffer has " << (received_size_ - next_decoding_offset_) << "bytes.");
  pthread_mutex_lock(&mutex_);
  received_size_ += nRead;

  // Tries to decode as many messags as possible.
  while (true) {
    if (header_decoded_ == false) {
      if (TryToDecodeHeader() == false) {
        break;
      }
    }
    if (header_decoded_) {
      if (TryToDecodeBody() == false) {
        break;
      }
    }
  }

  // Checks buvffer space before starting another async receive.
  if (receiving_.iov_len - received_size_ == 0) {
    // If there are space can be collected, compact it first.
    // Otherwise, increase the receiving buffer size.
    if (next_decoding_offset_ > 0) {
      LOG("Compacting a receive buffer to save " \
          << next_decoding_offset_ << "bytes.");
      uint8_t *base = reinterpret_cast<uint8_t *>(receiving_.iov_base);
      memmove(base, base + next_decoding_offset_,
              received_size_ - next_decoding_offset_);
      received_size_ -= next_decoding_offset_;
      next_decoding_offset_ = 0;
    } else {
      size_t new_size = receiving_.iov_len + kUnitBufferSize;
      LOG("Resizing a buffer to " << new_size << "bytes.");
      uint8_t *new_buffer = new uint8_t[new_size];
      memmove(new_buffer, receiving_.iov_base, received_size_);
      receiving_.iov_base = new_buffer;
      receiving_.iov_len = new_size;
    }
  }

  // Starts another async receive.
  struct iovec residual;
  residual.iov_len = receiving_.iov_len - received_size_;
  residual.iov_base =
      reinterpret_cast<uint8_t *>(receiving_.iov_base) + received_size_;
  AsyncReceive(
      sock_, residual,
      AsyncReceiveCallback(&FunapiTcpTransportImpl::ReceiveBytesCbWrapper,
          (void *) this));
  LOG("Ready to receive more. We can receive upto " \
      << (receiving_.iov_len - received_size_) << " more bytes.");

  pthread_mutex_unlock(&mutex_);
}


// The caller must lock mutex_ before call this function.
bool FunapiTcpTransportImpl::TryToDecodeHeader() {
  LOG("Trying to decode header fields.");
  for (; next_decoding_offset_ < received_size_; ) {
    char *base = reinterpret_cast<char *>(receiving_.iov_base);
    char *ptr =
        std::search(base + next_decoding_offset_,
                    base + received_size_,
                    kHeaderDelimeter,
                    kHeaderDelimeter + sizeof(kHeaderDelimeter) - 1);

    ssize_t eol_offset = ptr - base;
    if (eol_offset >= received_size_) {
      // Not enough bytes. Wait for more bytes to come.
      LOG("We need more bytes for a header field. Waiting.");
      return false;
    }

    // Generates a null-termianted string by replacing the delimeter with \0.
    *ptr = '\0';
    char *line = base + next_decoding_offset_;
    LOG("Header line: " << line);

    ssize_t line_length = eol_offset - next_decoding_offset_;
    next_decoding_offset_ = eol_offset + 1;

    if (line_length == 0) {
      // End of header.
      header_decoded_ = true;
      LOG("End of header reached. Will decode body from now.");
      return true;
    }

    ptr = std::search(
        line, line + line_length, kHeaderFieldDelimeter,
        kHeaderFieldDelimeter + sizeof(kHeaderFieldDelimeter) - 1);
    assert((ptr - base) < eol_offset);

    // Generates null-terminated string by replacing the delimeter with \0.
    *ptr = '\0';
    char *e1 = line, *e2 = ptr + 1;
    while (*e2 == ' ' || *e2 == '\t') ++e2;
    LOG("Decoded header field '" << e1 << "' => '" << e2 << "'");
    header_fields_[e1] = e2;
  }
  return false;
}


// The caller must lock mutex_ before call this function.
bool FunapiTcpTransportImpl::TryToDecodeBody() {
  HeaderFields::const_iterator it = header_fields_.find(kVersionHeaderField);
  assert(it != header_fields_.end());

  int version = atoi(it->second.c_str());
  assert(version == kCurrentFunapiProtocolVersion);

  it = header_fields_.find(kLengthHeaderField);
  int body_length = atoi(it->second.c_str());
  LOG("We need " << body_length << "bytes for a message body. " \
      << "Buffer has " << (received_size_ - next_decoding_offset_) << "bytes.");

  if (received_size_ - next_decoding_offset_ < body_length) {
    // Need more bytes.
    LOG("We need more bytes for a message body. Waiting.");
    return false;
  }

  // Generates a null-termianted string for convenience.
  char *base = reinterpret_cast<char *>(receiving_.iov_base);
  base[next_decoding_offset_ + body_length] = '\0';

  // Parses the given json string.
  rapidjson::Document json;
  json.Parse<0>(base + next_decoding_offset_);
  assert(json.IsObject());

  // Moves the read offset.
  next_decoding_offset_ += body_length;

  // Parsed JSON message should have reserved fields.
  // The network module eats the fields and invokes registered handler
  // with a remaining JSON body.
  LOG("Invoking a receive handler.");
  on_received_(header_fields_, json);

  // Prepares for a next message.
  header_decoded_ = false;
  header_fields_.clear();
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiNetworkImpl implementation.

class FunapiNetworkImpl {
 public:
  typedef FunapiTransport::HeaderType HeaderType;
  typedef FunapiNetwork::MessageHandler MessageHandler;
  typedef FunapiNetwork::OnSessionInitiated OnSessionInitiated;
  typedef FunapiNetwork::OnSessionClosed OnSessionClosed;

  FunapiNetworkImpl(FunapiTransport *transport,
                    OnSessionInitiated on_session_initiated,
                    OnSessionClosed on_session_closed);

  ~FunapiNetworkImpl();

  void RegisterHandler(const string &msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const string &msg_type, rapidjson::Document &body);
  bool Started() const;
  bool Connected() const;

 private:
  static void OnTransportReceivedWrapper(const HeaderType &header, rapidjson::Document &body, void *arg);
  static void OnTransportStoppedWrapper(void *arg);
  static void OnNewSessionWrapper(const string &msg_type, const rapidjson::Document &body, void *arg);
  static void OnSessionTimedoutWrapper(const string &msg_type, const rapidjson::Document &body, void *arg);

  void OnTransportReceived(const HeaderType &header, rapidjson::Document &body);
  void OnTransportStopped();
  void OnNewSession(const string &msg_type, const rapidjson::Document &body);
  void OnSessionTimedout(const string &msg_type, const rapidjson::Document &body);

  bool started_;
  FunapiTransport *transport_;
  OnSessionInitiated on_session_initiated_;
  OnSessionClosed on_session_closed_;
  string session_id_;
  typedef map<string, MessageHandler> MessageHandlerMap;
  MessageHandlerMap message_handlers_;
  time_t last_received_;
};



FunapiNetworkImpl::FunapiNetworkImpl(FunapiTransport *transport,
                                     OnSessionInitiated on_session_initiated,
                                     OnSessionClosed on_session_closed)
    : started_(false), transport_(transport),
      on_session_initiated_(on_session_initiated),
      on_session_closed_(on_session_closed),
      session_id_(""), last_received_(0) {
  transport_->RegisterEventHandlers(
      FunapiTcpTransport::OnReceived(
          &FunapiNetworkImpl::OnTransportReceivedWrapper, (void *) this),
      FunapiTcpTransport::OnStopped(
          &FunapiNetworkImpl::OnTransportStoppedWrapper, (void *) this));
}


FunapiNetworkImpl::~FunapiNetworkImpl() {
  if (transport_) {
    delete transport_;
    transport_ = NULL;
  }
  message_handlers_.clear();
}


void FunapiNetworkImpl::RegisterHandler(
    const string &msg_type, const MessageHandler &handler) {
  LOG("New handler for message type '" << msg_type << "'");
  message_handlers_[msg_type] = handler;
}


void FunapiNetworkImpl::Start() {
  // Installs event handlers.
  message_handlers_[kNewSessionMessageType] =
      MessageHandler(&FunapiNetworkImpl::OnNewSessionWrapper,
      (void *) this);
  message_handlers_[kSessionClosedMessageType] =
      MessageHandler(&FunapiNetworkImpl::OnSessionTimedoutWrapper,
      (void *) this);

  // Then, asks the transport to work.
  LOG("Starting a network module.");
  transport_->Start();

  // Ok. We are ready.
  started_ = true;
  last_received_ = epoch = time(NULL);
}


void FunapiNetworkImpl::Stop() {
  LOG("Stopping a network module.");

  // Turns off the flag.
  started_ = false;

  // Then, requests the transport to stop.
  transport_->Stop();
}


void FunapiNetworkImpl::SendMessage(const string &msg_type,
                                    rapidjson::Document &body) {
  // Invalidates session id if it is too stale.
  time_t now = time(NULL);
  time_t delta = funapi_session_timeout;

  if (last_received_ != epoch && last_received_ + delta < now) {
    LOG("Session is too stale. "
        "The server might have invalidated my session. Resetting.");
    session_id_ = "";
  }

  // Encodes a messsage type.
  rapidjson::Value msg_type_node;
  msg_type_node = msg_type.c_str();
  body.AddMember(kMsgTypeBodyField, msg_type_node, body.GetAllocator());

  // Encodes a session id, if any.
  if (not session_id_.empty()) {
      rapidjson::Value session_id_node;
      session_id_node = session_id_.c_str();
      body.AddMember(kSessionIdBodyField, session_id_node, body.GetAllocator());
  }

  // Sends the manipulated JSON object through the transport.
  transport_->SendMessage(body);
}


bool FunapiNetworkImpl::Started() const {
  return started_;
}


bool FunapiNetworkImpl::Connected() const {
  return transport_->Started();
}


void FunapiNetworkImpl::OnTransportReceivedWrapper(
    const HeaderType &header, rapidjson::Document &body, void *arg) {
  FunapiNetworkImpl *obj = reinterpret_cast<FunapiNetworkImpl *>(arg);
  return obj->OnTransportReceived(header, body);
}


void FunapiNetworkImpl::OnTransportStoppedWrapper(void *arg) {
  FunapiNetworkImpl *obj = reinterpret_cast<FunapiNetworkImpl *>(arg);
  return obj->OnTransportStopped();
}


void FunapiNetworkImpl::OnNewSessionWrapper(
    const string &msg_type, const rapidjson::Document &body, void *arg) {
  FunapiNetworkImpl *obj = reinterpret_cast<FunapiNetworkImpl *>(arg);
  return obj->OnNewSession(msg_type, body);
}


void FunapiNetworkImpl::OnSessionTimedoutWrapper(
    const string &msg_type, const rapidjson::Document &body, void *arg) {
  FunapiNetworkImpl *obj = reinterpret_cast<FunapiNetworkImpl *>(arg);
  return obj->OnSessionTimedout(msg_type, body);
}


void FunapiNetworkImpl::OnTransportReceived(
    const HeaderType &header, rapidjson::Document &body) {
  LOG("OnReceived invoked");

  last_received_ = time(NULL);

  const rapidjson::Value &msg_type_node = body[kMsgTypeBodyField];
  assert(msg_type_node.IsString());
  const string &msg_type = msg_type_node.GetString();
  body.RemoveMember(kMsgTypeBodyField);

  const rapidjson::Value &session_id_node = body[kSessionIdBodyField];
  assert(session_id_node.IsString());
  const string &session_id = session_id_node.GetString();
  body.RemoveMember(kSessionIdBodyField);

  if (session_id_.empty()) {
    session_id_ = session_id;
    LOG("New session id: " << session_id);
    on_session_initiated_(session_id_);
  }

  if (session_id_ != session_id) {
    LOG("Session id changed: " << session_id_ << " => " << session_id);
    session_id_ = session_id;
    on_session_closed_();
    on_session_initiated_(session_id_);
  }

  MessageHandlerMap::iterator it = message_handlers_.find(msg_type);
  if (it == message_handlers_.end()) {
    LOG("No handler for message '" << msg_type << "'. Ignoring.");
  } else {
    it->second(msg_type, body);
  }
}


void FunapiNetworkImpl::OnTransportStopped() {
  LOG("Transport terminated. Stopping. You may restart again.");
  Stop();
}


void FunapiNetworkImpl::OnNewSession(
    const string &msg_type, const rapidjson::Document &body) {
  // ignore
}


void FunapiNetworkImpl::OnSessionTimedout(
    const string &msg_type, const rapidjson::Document &body) {
  LOG("Session timed out. Resetting my session id. " \
      << "The server will send me another one next time.");

  session_id_ = "";
  on_session_closed_();
}



////////////////////////////////////////////////////////////////////////////////
// FunapiTcpTransport implementation.

FunapiTcpTransport::FunapiTcpTransport(
    const string &hostname_or_ip, uint16_t port)
    : impl_(new FunapiTcpTransportImpl(hostname_or_ip, port)) {
  if (not initialized) {
    LOG("You should call FunapiNetwork::Initialize() first.");
    assert(initialized);
  }
}


FunapiTcpTransport::~FunapiTcpTransport() {
  if (impl_) {
    delete impl_;
    impl_ = NULL;
  }
}


void FunapiTcpTransport::RegisterEventHandlers(
    const OnReceived &on_received, const OnStopped &on_stopped) {
  return impl_->RegisterEventHandlers(on_received, on_stopped);
}


void FunapiTcpTransport::Start() {
  return impl_->Start();
}


void FunapiTcpTransport::Stop() {
  return impl_->Stop();
}


void FunapiTcpTransport::SendMessage(rapidjson::Document &message) {
  return impl_->SendMessage(message);
}


bool FunapiTcpTransport::Started() const {
  return impl_->Started();
}



////////////////////////////////////////////////////////////////////////////////
// FunapiNetwork implementation.

void FunapiNetwork::Initialize(time_t session_timeout, std::ostream &stream) {
  assert(not initialized);

  // Remembers the session timeout setting.
  funapi_session_timeout = session_timeout;

  // Makes the global logstream use the given log stream.
  logstream = &stream;

  // Creates a thread to handle async operations.
  async_thread_run = true;
  int r = pthread_create(&async_queue_thread, NULL, AsyncQueueThreadProc, NULL);
  assert(r == 0);

  // Now ready.
  initialized = true;
}


void FunapiNetwork::Finalize() {
  assert(initialized);

  // Terminates the thread for async operations.
  async_thread_run = false;
  pthread_cond_broadcast(&async_queue_cond);
  void *dummy;
  pthread_join(async_queue_thread, &dummy);
  (void) dummy;

  // Resets the logstream.
  logstream = NULL;

  // All set.
  initialized = false;
}


FunapiNetwork::FunapiNetwork(FunapiTransport *transport,
                             const OnSessionInitiated &on_session_initiated,
                             const OnSessionClosed &on_session_closed)
    : impl_(new FunapiNetworkImpl(transport,
                                  on_session_initiated, on_session_closed)) {
  // Makes sure we initialized the module.
  if (not initialized) {
    LOG("You should call FunapiNetwork::Initialize() first.");
    assert(initialized);
  }
}


FunapiNetwork::~FunapiNetwork() {
  if (impl_) {
    delete impl_;
    impl_ = NULL;
  }
}


void FunapiNetwork::RegisterHandler(const string &msg_type,
                                    const MessageHandler &handler) {
  return impl_->RegisterHandler(msg_type, handler);
}


void FunapiNetwork::Start() {
  return impl_->Start();
}


void FunapiNetwork::Stop() {
  return impl_->Stop();
}


void FunapiNetwork::SendMessage(
    const string &msg_type, rapidjson::Document &body) {
  return impl_->SendMessage(msg_type, body);
}


bool FunapiNetwork::Started() const {
  return impl_->Started();
}


bool FunapiNetwork::Connected() const {
  return impl_->Connected();
}

}  // namespace fun
