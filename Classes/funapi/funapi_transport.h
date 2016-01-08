// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_TRANSPORT_H_
#define SRC_FUNAPI_TRANSPORT_H_

#include "funapi_plugin.h"
#include "pb/network/fun_message.pb.h"


namespace fun {

typedef sockaddr_in Endpoint;
typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum FunapiTransportState {
  kDisconnected = 0,
  kConnecting,
  kConnected,
};

// Funapi transport protocol
enum class TransportProtocol : int
{
  kDefault = 0,
  kTcp,
  kUdp,
  kHttp
};

// Message encoding type
enum class FunEncoding
{
  kNone,
  kJson,
  kProtobuf
};

// Error code
enum class ErrorCode
{
  kNone,
  kConnectFailed,
  kSendFailed,
  kReceiveFailed,
  kEncryptionFailed,
  kInvalidEncryption,
  kUnknownEncryption,
  kRequestTimeout,
  kDisconnected,
  kExceptionError
};

class FunapiNetwork;
class FunapiTransportImpl;
class FunapiTransport {
 public:
  enum class State : int
  {
    kUnknown = 0,
    kConnecting,
    kEncryptionHandshaking,
    kConnected,
    kWaitForSession,
    kWaitForAck,
    kEstablished
  };

  enum class ConnectState : int
  {
    kUnknown = 0,
    kConnecting,
    kReconnecting,
    kRedirecting,
    kConnected
  };

  typedef std::map<std::string, std::string> HeaderType;

  typedef std::function<void(const TransportProtocol, const FunEncoding, const HeaderType &, const std::vector<uint8_t> &)> OnReceived;
  typedef std::function<void(void)> OnStopped;

  // Event handler delegate
  typedef std::function<void(const TransportProtocol protocol)> TransportEventHandler;

  virtual ~FunapiTransport() = default;

  // Start connecting
  virtual void Start();

  // Disconnection
  virtual void Stop();

  // Check connection
  virtual bool Started() const;

  // Send a message
  virtual void SendMessage(rapidjson::Document &message);
  virtual void SendMessage(FunMessage &message);
  virtual void SendMessage(const char *body);

  virtual TransportProtocol Protocol() const = 0;
  virtual FunEncoding Encoding() const;

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  virtual void SetConnectTimeout(time_t timeout);

  virtual void AddStartedCallback(const TransportEventHandler &handler);
  virtual void AddStoppedCallback(const TransportEventHandler &handler);
  virtual void AddFailureCallback(const TransportEventHandler &handler);
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void ResetPingClientTimeout();

 private:
  std::shared_ptr<FunapiTransportImpl> impl_;
};


class FunapiTcpTransportImpl;
class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiTcpTransport() = default;

  // Start connecting
  virtual void Start();

  // Disconnection
  virtual void Stop();

  // Check connection
  virtual bool Started() const;

  // Send a message
  virtual void SendMessage(rapidjson::Document &message);
  virtual void SendMessage(FunMessage &message);
  virtual void SendMessage(const char *body);

  virtual TransportProtocol Protocol() const;
  virtual FunEncoding Encoding() const;

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  virtual void SetConnectTimeout(time_t timeout);

  virtual void AddStartedCallback(const TransportEventHandler &handler);
  virtual void AddStoppedCallback(const TransportEventHandler &handler);
  virtual void AddFailureCallback(const TransportEventHandler &handler);
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void SetDisableNagle(bool disable_nagle);
  virtual void SetAutoReconnect(bool auto_reconnect);
  virtual void SetEnablePing(bool enable_ping);

  virtual void ResetPingClientTimeout();

 private:
  std::shared_ptr<FunapiTcpTransportImpl> impl_;
};

class FunapiUdpTransportImpl;
class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const std::string &hostname_or_ip, uint16_t port, FunEncoding encoding);
  virtual ~FunapiUdpTransport() = default;

  // Start connecting
  virtual void Start();

  // Disconnection
  virtual void Stop();

  // Check connection
  virtual bool Started() const;

  // Send a message
  virtual void SendMessage(rapidjson::Document &message);
  virtual void SendMessage(FunMessage &message);
  virtual void SendMessage(const char *body);

  virtual TransportProtocol Protocol() const;
  virtual FunEncoding Encoding() const;

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  virtual void SetConnectTimeout(time_t timeout);

  virtual void AddStartedCallback(const TransportEventHandler &handler);
  virtual void AddStoppedCallback(const TransportEventHandler &handler);
  virtual void AddFailureCallback(const TransportEventHandler &handler);
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void ResetPingClientTimeout();

 private:
  std::shared_ptr<FunapiUdpTransportImpl> impl_;
};

class FunapiHttpTransportImpl;
class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(const std::string &hostname_or_ip, uint16_t port, bool https, FunEncoding encoding);
  virtual ~FunapiHttpTransport() = default;

  // Start connecting
  virtual void Start();

  // Disconnection
  virtual void Stop();

  // Check connection
  virtual bool Started() const;

  // Send a message
  virtual void SendMessage(rapidjson::Document &message);
  virtual void SendMessage(FunMessage &message);
  virtual void SendMessage(const char *body);

  virtual TransportProtocol Protocol() const;
  virtual FunEncoding Encoding() const;

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network);
  virtual void SetConnectTimeout(time_t timeout);

  virtual void AddStartedCallback(const TransportEventHandler &handler);
  virtual void AddStoppedCallback(const TransportEventHandler &handler);
  virtual void AddFailureCallback(const TransportEventHandler &handler);
  virtual void AddConnectTimeoutCallback(const TransportEventHandler &handler);

  virtual void ResetPingClientTimeout();

 private:
  std::shared_ptr<FunapiHttpTransportImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_TRANSPORT_H_
