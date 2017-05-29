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

$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos/audio/include)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../../Classes/AppDelegate.cpp \
                   ../../../Classes/FunapiTestScene.cpp \
                   ../../../Classes/funapi/funapi_transport.cpp \
                   ../../../Classes/funapi/funapi_multicasting.cpp \
                   ../../../Classes/funapi/funapi_encryption.cpp \
                   ../../../Classes/funapi/funapi_downloader.cpp \
                   ../../../Classes/funapi/funapi_session.cpp \
                   ../../../Classes/funapi/funapi_tasks.cpp \
                   ../../../Classes/funapi/funapi_announcement.cpp \
                   ../../../Classes/funapi/funapi_http.cpp \
                   ../../../Classes/funapi/funapi_utils.cpp \
                   ../../../Classes/funapi/funapi_socket.cpp \
                   ../../../Classes/funapi/md5/md5.cpp \
                   ../../../Classes/funapi/management/maintenance_message.pb.cc \
                   ../../../Classes/funapi/network/fun_message.pb.cc \
                   ../../../Classes/funapi/network/ping_message.pb.cc \
                   ../../../Classes/funapi/service/multicast_message.pb.cc \
                   ../../../Classes/funapi/service/redirect_message.pb.cc \
                   ../../../Classes/test_messages.pb.cc \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes \
                    $(LOCAL_PATH)/../../../Classes/funapi \
                    $(LOCAL_PATH)/../../../Classes/funapi/md5 \
                    $(LOCAL_PATH)/../../../Classes/funapi/management \
                    $(LOCAL_PATH)/../../../Classes/funapi/network \
                    $(LOCAL_PATH)/../../../Classes/funapi/service \
                    $(LOCAL_PATH)/libsodium/include \
                    $(LOCAL_PATH)/libprotobuf/include/ARMv7

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END

LOCAL_STATIC_LIBRARIES := cocos2dx_static libsodium libprotobuf cocos_curl_static

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END

$(call import-module,curl/prebuilt/android)