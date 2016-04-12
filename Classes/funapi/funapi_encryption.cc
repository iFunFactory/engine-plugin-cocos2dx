// Copyright (C) 2013-2016 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_plugin.h"
#include "funapi_utils.h"
#include "funapi_transport.h"
#include "funapi_encryption.h"
#include <stdlib.h>

namespace fun {

enum class EncryptionType : int {
  kNoneEncryption = 0,
  kDefaultEncryption,
};


////////////////////////////////////////////////////////////////////////////////
// FunapiEncrytionImpl implementation.

class FunapiEncrytionImpl : public std::enable_shared_from_this<FunapiEncrytionImpl> {
};


////////////////////////////////////////////////////////////////////////////////
// FunapiEncrytion implementation.

FunapiEncrytion::FunapiEncrytion() {
}


FunapiEncrytion::~FunapiEncrytion() {
}


void FunapiEncrytion::SetEncryptionType(EncryptionType type) {
}


bool FunapiEncrytion::Encrypt(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return true;
}


bool FunapiEncrytion::Decrypt(HeaderFields &header_fields, std::vector<uint8_t> &body) {
  return true;
}


void FunapiEncrytion::SetHeaderFieldsForHttpSend (HeaderFields &header_fields) {
}


void FunapiEncrytion::SetHeaderFieldsForHttpRecv (HeaderFields &header_fields) {
}

}  // namespace fun
