// Copyright (C) 2013-2015 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

/** @file */

#pragma once

#include "funapi_plugin.h"
#include "pb/network/fun_message.pb.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// Types.

typedef sockaddr_in Endpoint;
typedef std::function<void(const int)> AsyncWebRequestCallback;
typedef std::function<void(void*, const int)> AsyncWebResponseCallback;

enum FunapiTransportType {
  kTcp = 1,
  kUdp,
  kHttp,
};

enum FunapiTransportState {
  kDisconnected = 0,
  kConnecting,
  kConnected,
};

enum EncodingScheme {
  kUnknownEncoding = 0,
  kJsonEncoding,
  kProtobufEncoding,
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

class FunapiNetwork;
class FunapiTransportBase;
class FunapiTransport {
 public:
  typedef std::map<std::string, std::string> HeaderType;

  typedef std::function<void(const HeaderType&, const std::vector<uint8_t>&)> OnReceived;
  typedef std::function<void(void)> OnStopped;

  virtual ~FunapiTransport() = default;
  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(Json &message);
  // virtual void SendMessage(FJsonObject &message);
  virtual void SendMessage(FunMessage &message);
  virtual bool Started() const;
  virtual TransportProtocol Protocol() const = 0;
  virtual void SetNetwork(std::weak_ptr<FunapiNetwork> network);

 protected:
  FunapiTransport() {}
  std::shared_ptr<FunapiTransportBase> impl_;
};

class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTcpTransport() = default;
  virtual TransportProtocol Protocol() const;
};

class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const std::string &hostname_or_ip, uint16_t port);
  virtual ~FunapiUdpTransport() = default;
  virtual TransportProtocol Protocol() const;
};

class FunapiHttpTransport : public FunapiTransport {
 public:
  FunapiHttpTransport(const std::string &hostname_or_ip, uint16_t port, bool https = false);
  virtual ~FunapiHttpTransport() = default;
  virtual TransportProtocol Protocol() const;
};

}  // namespace fun
