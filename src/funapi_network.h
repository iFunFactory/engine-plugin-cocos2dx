// Copyright (C) 2013 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

/** @file */

#ifndef SRC_FUNAPI_NETWORK_H_
#define SRC_FUNAPI_NETWORK_H_

#include <time.h>

#include <iostream>
#include <map>
#include <string>

#include "rapidjson/document.h"


namespace fun {

typedef std::string string;


namespace helper {

template <typename R, typename Ctxt>
class Binder0 {
 public:
  typedef R (*F)(Ctxt);
  Binder0() : f_(NULL), ctxt_(NULL) { }
  Binder0(const F &f, Ctxt ctxt) : f_(f), ctxt_(ctxt) { }
  R operator()() { if (f_) f_(ctxt_); }
 private:
  F f_;
  Ctxt ctxt_;
};


template <typename R, typename A1, typename Ctxt>
class Binder1 {
 public:
  typedef R (*F)(A1, Ctxt);
  Binder1() : f_(NULL), ctxt_(NULL) { }
  Binder1(const F &f, void *ctxt) : f_(f), ctxt_(ctxt) { }
  R operator()(A1 a1) { if (f_) f_(a1, ctxt_); }
 private:
  F f_;
  Ctxt ctxt_;
};


template <typename R, typename A1, typename A2, typename Ctxt>
class Binder2 {
 public:
  typedef R (*F)(A1, A2, Ctxt);
  Binder2() : f_(NULL), ctxt_(NULL) { }
  Binder2(const F &f, void *ctxt) : f_(f), ctxt_(ctxt) { }
  R operator()(A1 a1, A2 a2) { if (f_) f_(a1, a2, ctxt_); }
 private:
  F f_;
  Ctxt ctxt_;
};

}   // namespace helper


class FunapiTransport {
 public:
  typedef std::map<string, string> HeaderType;

  // NOTE: Type definitions below imply that
  //  1) OnReceived should be of
  //      'void f(const HeaderType &, const rapidjson::Document &, void *)'
  //  2) OnStopped should be of 'void f(void *)'
  //
  // You can create a function object instance simply like this:
  //   e.g., OnReceived(my_on_received_handler, my_context);
  typedef helper::Binder2<
      void, const HeaderType &, rapidjson::Document &, void *> OnReceived;
  typedef helper::Binder0<void, void *> OnStopped;

  virtual ~FunapiTransport() {}
  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2) = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual void SendMessage(rapidjson::Document &message) = 0;
  virtual bool Started() const = 0;

 protected:
  FunapiTransport() {}
};


class FunapiTransportImpl;
class FunapiTcpTransport : public FunapiTransport {
 public:
  FunapiTcpTransport(const string &hostname_or_ip, uint16_t port);
  virtual ~FunapiTcpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(rapidjson::Document &message);
  virtual bool Started() const;

 private:
  FunapiTransportImpl *impl_;
};

class FunapiUdpTransport : public FunapiTransport {
 public:
  FunapiUdpTransport(const string &hostname_or_ip, uint16_t port);
  virtual ~FunapiUdpTransport();

  virtual void RegisterEventHandlers(const OnReceived &cb1, const OnStopped &cb2);
  virtual void Start();
  virtual void Stop();
  virtual void SendMessage(rapidjson::Document &message);
  virtual bool Started() const;

 private:
  FunapiTransportImpl *impl_;
};


class FunapiNetworkImpl;
class FunapiNetwork {
 public:
  // NOTE: Type definitions below imply that
  //  1) MessageHandler should be of
  //      'void f(const string &, const rapidjson::Document &, void *)'
  //  2) OnSessionInitiated should be of 'void f(const string &, void *)'
  //  3) OnSessionClosed should be of 'void f(void *)'
  //
  // You can create a function object instance simply like this:
  //   e.g., MessageHandler(my_message_handler, my_context);
  typedef helper::Binder2<void, const string &, const rapidjson::Document &, void *> MessageHandler;
  typedef helper::Binder1<void, const string &, void *> OnSessionInitiated;
  typedef helper::Binder0<void, void *> OnSessionClosed;

  static void Initialize(time_t session_timeout = 3600, std::ostream &logstream = std::cout);
  static void Finalize();

  FunapiNetwork(FunapiTransport *funapi_transport,
                const OnSessionInitiated &on_session_initiated,
                const OnSessionClosed &on_session_closed);
  ~FunapiNetwork();

  void RegisterHandler(const string &msg_type, const MessageHandler &handler);
  void Start();
  void Stop();
  void SendMessage(const string &msg_type, rapidjson::Document &body);
  bool Started() const;
  bool Connected() const;

 private:
  FunapiNetworkImpl *impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_NETWORK_H_
