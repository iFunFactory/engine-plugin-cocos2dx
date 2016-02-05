LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos)

LOCAL_MODULE := cocos2dcpp_shared

LOCAL_MODULE_FILENAME := libcocos2dcpp

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../Classes/AppDelegate.cpp \
                   ../../Classes/FunapiTestScene.cpp \
                   ../../Classes/funapi/funapi_network.cc \
                   ../../Classes/funapi/funapi_transport.cc \
                   ../../Classes/funapi/funapi_multicasting.cc \
                   ../../Classes/funapi/pb/management/maintenance_message.pb.cc \
                   ../../Classes/funapi/pb/network/fun_message.pb.cc \
                   ../../Classes/funapi/pb/network/ping_message.pb.cc \
                   ../../Classes/funapi/pb/service/multicast_message.pb.cc \
                   ../../Classes/funapi/pb/test_messages.pb.cc \
                   ../../Classes/google/protobuf/descriptor.cc \
                   ../../Classes/google/protobuf/descriptor.pb.cc \
                   ../../Classes/google/protobuf/descriptor_database.cc \
                   ../../Classes/google/protobuf/dynamic_message.cc \
                   ../../Classes/google/protobuf/extension_set.cc \
                   ../../Classes/google/protobuf/extension_set_heavy.cc \
                   ../../Classes/google/protobuf/generated_message_reflection.cc \
                   ../../Classes/google/protobuf/generated_message_util.cc \
                   ../../Classes/google/protobuf/io/coded_stream.cc \
                   ../../Classes/google/protobuf/io/gzip_stream.cc \
                   ../../Classes/google/protobuf/io/printer.cc \
                   ../../Classes/google/protobuf/io/strtod.cc \
                   ../../Classes/google/protobuf/io/tokenizer.cc \
                   ../../Classes/google/protobuf/io/zero_copy_stream.cc \
                   ../../Classes/google/protobuf/io/zero_copy_stream_impl.cc \
                   ../../Classes/google/protobuf/io/zero_copy_stream_impl_lite.cc \
                   ../../Classes/google/protobuf/message.cc \
                   ../../Classes/google/protobuf/message_lite.cc \
                   ../../Classes/google/protobuf/reflection_ops.cc \
                   ../../Classes/google/protobuf/repeated_field.cc \
                   ../../Classes/google/protobuf/service.cc \
                   ../../Classes/google/protobuf/stubs/atomicops_internals_x86_gcc.cc \
                   ../../Classes/google/protobuf/stubs/atomicops_internals_x86_msvc.cc \
                   ../../Classes/google/protobuf/stubs/common.cc \
                   ../../Classes/google/protobuf/stubs/once.cc \
                   ../../Classes/google/protobuf/stubs/stringprintf.cc \
                   ../../Classes/google/protobuf/stubs/structurally_valid.cc \
                   ../../Classes/google/protobuf/stubs/strutil.cc \
                   ../../Classes/google/protobuf/stubs/substitute.cc \
                   ../../Classes/google/protobuf/text_format.cc \
                   ../../Classes/google/protobuf/unknown_field_set.cc \
                   ../../Classes/google/protobuf/wire_format.cc \
                   ../../Classes/google/protobuf/wire_format_lite.cc \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Classes \
                    $(LOCAL_PATH)/../../Classes/funapi \
                    $(LOCAL_PATH)/../../Classes/funapi/pb \
                    $(LOCAL_PATH)/../../Classes/funapi/pb/management \
                    $(LOCAL_PATH)/../../Classes/funapi/pb/network \
                    $(LOCAL_PATH)/../../Classes/funapi/pb/service \
                    $(LOCAL_PATH)/../../Classes/google \
                    $(LOCAL_PATH)/../../Classes/google/protobuf \
                    $(LOCAL_PATH)/../../Classes/google/protobuf/io \
                    $(LOCAL_PATH)/../../Classes/google/protobuf/stubs

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END


LOCAL_STATIC_LIBRARIES := cocos2dx_static

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END

$(call import-module,curl/prebuilt/android)
