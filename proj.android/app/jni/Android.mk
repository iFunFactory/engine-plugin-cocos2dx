LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libsodium
LOCAL_SRC_FILES := libsodium/lib/libsodium.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libprotobuf
LOCAL_SRC_FILES := libprotobuf/lib/ARMv7/libprotobuf.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libzstd
LOCAL_SRC_FILES := libzstd/lib/ARMv7/libzstd.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos/audio/include)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../../Classes/AppDelegate.cpp \
                   ../../../Classes/FunapiTestScene.cpp \
                   ../../../Classes/funapi/funapi_option.cpp \
                   ../../../Classes/funapi/funapi_multicasting.cpp \
                   ../../../Classes/funapi/funapi_encryption.cpp \
                   ../../../Classes/funapi/funapi_compression.cpp \
                   ../../../Classes/funapi/funapi_downloader.cpp \
                   ../../../Classes/funapi/funapi_session.cpp \
                   ../../../Classes/funapi/funapi_tasks.cpp \
                   ../../../Classes/funapi/funapi_announcement.cpp \
                   ../../../Classes/funapi/funapi_http.cpp \
                   ../../../Classes/funapi/funapi_utils.cpp \
                   ../../../Classes/funapi/funapi_socket.cpp \
                   ../../../Classes/funapi/funapi_websocket.cpp \
                   ../../../Classes/funapi/funapi_rpc.cpp \
                   ../../../Classes/funapi/funapi_send_flag_manager.cpp \
                   ../../../Classes/funapi/management/maintenance_message.pb.cc \
                   ../../../Classes/funapi/network/fun_message.pb.cc \
                   ../../../Classes/funapi/network/ping_message.pb.cc \
                   ../../../Classes/funapi/service/multicast_message.pb.cc \
                   ../../../Classes/funapi/service/redirect_message.pb.cc \
                   ../../../Classes/funapi/distribution/fun_dedicated_server_rpc_message.pb.cc \
                   ../../../Classes/test_dedicated_server_rpc_messages.pb.cc \
                   ../../../Classes/test_messages.pb.cc \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes \
                    $(LOCAL_PATH)/../../../Classes/funapi \
                    $(LOCAL_PATH)/../../../Classes/funapi/management \
                    $(LOCAL_PATH)/../../../Classes/funapi/network \
                    $(LOCAL_PATH)/../../../Classes/funapi/service \
                    $(LOCAL_PATH)/../../../Classes/funapi/distribution \
                    $(LOCAL_PATH)/libsodium/include \
                    $(LOCAL_PATH)/libprotobuf/include/ARMv7 \
                    $(LOCAL_PATH)/libzstd/include/ARMv7

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END

LOCAL_STATIC_LIBRARIES := libsodium libprotobuf libzstd cc_static ext_curl ext_crypto

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := funapi_unittest
LOCAL_SRC_FILES := hellocpp/unittest.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes \
                    $(LOCAL_PATH)/../../../Classes/funapi \
                    $(LOCAL_PATH)/../../../Classes/funapi/management \
                    $(LOCAL_PATH)/../../../Classes/funapi/network \
                    $(LOCAL_PATH)/../../../Classes/funapi/service \
                    $(LOCAL_PATH)/../../../Classes/funapi/distribution \
                    $(LOCAL_PATH)/libprotobuf/include/ARMv7
LOCAL_SHARED_LIBRARIES := MyGame_shared
LOCAL_STATIC_LIBRARIES := googletest_main
LOCAL_CFLAGS += -fPIE
LOCAL_LDFLAGS += -fPIE -pie
include $(BUILD_EXECUTABLE)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END

$(call import-module,curl/prebuilt/android)
$(call import-module,third_party/googletest)
