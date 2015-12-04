// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: funapi/management/maintenance_message.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
// #include "funapi/management/maintenance_message.pb.h"
#include "maintenance_message.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace {

const ::google::protobuf::Descriptor* MaintenanceMessage_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  MaintenanceMessage_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto() {
  protobuf_AddDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "funapi/management/maintenance_message.proto");
  GOOGLE_CHECK(file != NULL);
  MaintenanceMessage_descriptor_ = file->message_type(0);
  static const int MaintenanceMessage_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MaintenanceMessage, date_start_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MaintenanceMessage, date_end_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MaintenanceMessage, messages_),
  };
  MaintenanceMessage_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      MaintenanceMessage_descriptor_,
      MaintenanceMessage::default_instance_,
      MaintenanceMessage_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MaintenanceMessage, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(MaintenanceMessage, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(MaintenanceMessage));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    MaintenanceMessage_descriptor_, &MaintenanceMessage::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto() {
  delete MaintenanceMessage::default_instance_;
  delete MaintenanceMessage_reflection_;
}

void protobuf_AddDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::protobuf_AddDesc_funapi_2fnetwork_2ffun_5fmessage_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n+funapi/management/maintenance_message."
    "proto\032 funapi/network/fun_message.proto\""
    "L\n\022MaintenanceMessage\022\022\n\ndate_start\030\001 \001("
    "\t\022\020\n\010date_end\030\002 \001(\t\022\020\n\010messages\030\003 \001(\t::\n"
    "\020pbuf_maintenance\022\013.FunMessage\030\017 \001(\0132\023.M"
    "aintenanceMessage", 217);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "funapi/management/maintenance_message.proto", &protobuf_RegisterTypes);
  MaintenanceMessage::default_instance_ = new MaintenanceMessage();
  ::google::protobuf::internal::ExtensionSet::RegisterMessageExtension(
    &::FunMessage::default_instance(),
    15, 11, false, false,
    &::MaintenanceMessage::default_instance());
  MaintenanceMessage::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto {
  StaticDescriptorInitializer_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto() {
    protobuf_AddDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto();
  }
} static_descriptor_initializer_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int MaintenanceMessage::kDateStartFieldNumber;
const int MaintenanceMessage::kDateEndFieldNumber;
const int MaintenanceMessage::kMessagesFieldNumber;
#endif  // !_MSC_VER

MaintenanceMessage::MaintenanceMessage()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:MaintenanceMessage)
}

void MaintenanceMessage::InitAsDefaultInstance() {
}

MaintenanceMessage::MaintenanceMessage(const MaintenanceMessage& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:MaintenanceMessage)
}

void MaintenanceMessage::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  date_start_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  date_end_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  messages_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

MaintenanceMessage::~MaintenanceMessage() {
  // @@protoc_insertion_point(destructor:MaintenanceMessage)
  SharedDtor();
}

void MaintenanceMessage::SharedDtor() {
  if (date_start_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete date_start_;
  }
  if (date_end_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete date_end_;
  }
  if (messages_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete messages_;
  }
  if (this != default_instance_) {
  }
}

void MaintenanceMessage::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* MaintenanceMessage::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return MaintenanceMessage_descriptor_;
}

const MaintenanceMessage& MaintenanceMessage::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_funapi_2fmanagement_2fmaintenance_5fmessage_2eproto();
  return *default_instance_;
}

MaintenanceMessage* MaintenanceMessage::default_instance_ = NULL;

MaintenanceMessage* MaintenanceMessage::New() const {
  return new MaintenanceMessage;
}

void MaintenanceMessage::Clear() {
  if (_has_bits_[0 / 32] & 7) {
    if (has_date_start()) {
      if (date_start_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        date_start_->clear();
      }
    }
    if (has_date_end()) {
      if (date_end_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        date_end_->clear();
      }
    }
    if (has_messages()) {
      if (messages_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        messages_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool MaintenanceMessage::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:MaintenanceMessage)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string date_start = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_date_start()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->date_start().data(), this->date_start().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "date_start");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_date_end;
        break;
      }

      // optional string date_end = 2;
      case 2: {
        if (tag == 18) {
         parse_date_end:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_date_end()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->date_end().data(), this->date_end().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "date_end");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(26)) goto parse_messages;
        break;
      }

      // optional string messages = 3;
      case 3: {
        if (tag == 26) {
         parse_messages:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_messages()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->messages().data(), this->messages().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "messages");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:MaintenanceMessage)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:MaintenanceMessage)
  return false;
#undef DO_
}

void MaintenanceMessage::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:MaintenanceMessage)
  // optional string date_start = 1;
  if (has_date_start()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->date_start().data(), this->date_start().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "date_start");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->date_start(), output);
  }

  // optional string date_end = 2;
  if (has_date_end()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->date_end().data(), this->date_end().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "date_end");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->date_end(), output);
  }

  // optional string messages = 3;
  if (has_messages()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->messages().data(), this->messages().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "messages");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      3, this->messages(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:MaintenanceMessage)
}

::google::protobuf::uint8* MaintenanceMessage::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:MaintenanceMessage)
  // optional string date_start = 1;
  if (has_date_start()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->date_start().data(), this->date_start().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "date_start");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->date_start(), target);
  }

  // optional string date_end = 2;
  if (has_date_end()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->date_end().data(), this->date_end().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "date_end");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->date_end(), target);
  }

  // optional string messages = 3;
  if (has_messages()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->messages().data(), this->messages().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "messages");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        3, this->messages(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:MaintenanceMessage)
  return target;
}

int MaintenanceMessage::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string date_start = 1;
    if (has_date_start()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->date_start());
    }

    // optional string date_end = 2;
    if (has_date_end()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->date_end());
    }

    // optional string messages = 3;
    if (has_messages()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->messages());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void MaintenanceMessage::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const MaintenanceMessage* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const MaintenanceMessage*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void MaintenanceMessage::MergeFrom(const MaintenanceMessage& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_date_start()) {
      set_date_start(from.date_start());
    }
    if (from.has_date_end()) {
      set_date_end(from.date_end());
    }
    if (from.has_messages()) {
      set_messages(from.messages());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void MaintenanceMessage::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void MaintenanceMessage::CopyFrom(const MaintenanceMessage& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool MaintenanceMessage::IsInitialized() const {

  return true;
}

void MaintenanceMessage::Swap(MaintenanceMessage* other) {
  if (other != this) {
    std::swap(date_start_, other->date_start_);
    std::swap(date_end_, other->date_end_);
    std::swap(messages_, other->messages_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata MaintenanceMessage::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = MaintenanceMessage_descriptor_;
  metadata.reflection = MaintenanceMessage_reflection_;
  return metadata;
}

::google::protobuf::internal::ExtensionIdentifier< ::FunMessage,
    ::google::protobuf::internal::MessageTypeTraits< ::MaintenanceMessage >, 11, false >
  pbuf_maintenance(kPbufMaintenanceFieldNumber, ::MaintenanceMessage::default_instance());

// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)
