// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#ifndef SRC_FUNAPI_ENCRYPTION_H_
#define SRC_FUNAPI_ENCRYPTION_H_

namespace fun {

enum class EncryptionType;

class FunapiEncrytionImpl;
class FunapiEncrytion : public std::enable_shared_from_this<FunapiEncrytion> {
 public:
  typedef FunapiTransport::HeaderFields HeaderFields;

  FunapiEncrytion();
  ~FunapiEncrytion();

  void SetEncryptionType(EncryptionType type);

  bool Encrypt(HeaderFields &header_fields, std::vector<uint8_t> &body);
  bool Decrypt(HeaderFields &header_fields, std::vector<uint8_t> &body);

  void SetHeaderFieldsForHttpSend (HeaderFields &header_fields);
  void SetHeaderFieldsForHttpRecv (HeaderFields &header_fields);

 private:
  std::shared_ptr<FunapiEncrytionImpl> impl_;
};

}  // namespace fun

#endif  // SRC_FUNAPI_ENCRYPTION_H_
