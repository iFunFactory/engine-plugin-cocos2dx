Funapi plugin cocos2d-x
========================

이 플러그인은 아이펀 엔진 서버를 사용하는 cocos2d-x 사용자를 위한 클라이언트 플러그인 입니다.

# 기능

* TCP, UDP, HTTP 프로토콜 사용 가능
* JSON, Protobuf-net 형식의 메시지 타입 지원
* ChaCha20, AES-128을 포함한 4종류의 암호화 타입 지원
* 멀티캐스트, 채팅, 게임내 리소스 다운로드 등 다양한 기능 지원

# 프로젝트 구성

이 프로젝트는 ``아이펀 엔진 플러그인``, ``개발 플랫폼별 테스트 프로젝트``로 구성되어있습니다.

* **Classes/funapi** : 플러그인 소스코드 폴더입니다.
* **외부 라이브러리** : 아이펀 엔진 클라이언트 플러그인이 동작하기 위해서는 필요한 외부 라이브러리들입니다.
  외부 라이브러리들은 개발 플랫폼별로 제공하기 때문에 각 개발 플랫폼별 폴더 안에서 찾으실 수 있습니다.
* **proj.android** : ``Android`` 테스트 프로젝트.
* **proj.ios_mac** : ``iOS``, ``macOS`` 테스트 프로젝트.
* **proj.win32** : ``Win32`` 테스트 프로젝트.

# 테스트 프로젝트

**플러그인 테스트 프로젝트**들은 **[아이펀 엔진](https://www.ifunfactory.com/engine/documents/reference/ko/index.html)**
로 생성하는 기본 서버 프로젝트와 접속해서 아이펀 엔진이 제공하는 여러가지 기능들을 테스트 할 수 있습니다.

아이펀 엔진으로 서버를 생성하면 기본적으로 TCP, HTTP 프로토콜과 JSON 메시지 포맷으로만 동작하도록 설정되어 있습니다.
다른 프로토콜과 메시지 포맷을 사용하려면 서버와 클라이언트의 포트 번호를 맞춰서 변경하고 테스트하면 됩니다.

테스트 소스코드는 ``Classes`` 폴더에 위치하고 아이펀 엔진 플러그인 예제는 ``FunapiTestScene.cpp``, ``FunapiTestScene.h`` 에서 확인할 수 있습니다.

테스트 프로젝트는 연결 대상 서버가 로컬 호스트에 있는 것을 가정하고 있습니다.
만약 서버가 로컬에 있지 않다면 ``FunapiTestScene.h`` 파일의 ``Server Address`` 값을 수정해 주세요.

# 프로젝트에 적용

cocos2d-x 프로젝트에 아이펀 엔진 플러그인을 적용하는 방법을 설명합니다.
아이펀 엔진 플러그인을 적용하려는 cocos2d-x 프로젝트는 **새 프로젝트**로 부르겠습니다.

새 프로젝트에 아이펀 엔진 플러그인을 적용하기 위해서는 **플러그인 소스코드**와 플러그인이 사용하는 **외부 라이브러리**들이 필요합니다.

우선 개발 플랫폼에 공통으로 사용되는 **플러그인 소스코드 폴더**인 ``Classes/funapi`` 를 **새 프로젝트의** ``Classes`` 폴더에 복사합니다.

**플러그인 소스코드**와 **외부 라이브러리**는 플랫폼별 사용법이 다르기 떄문에 아래 **개발 플랫폼별 프로젝트 설정**에서 이어 설명하겠습니다.

### Win32 프로젝트 설정

**Win32 프로젝트**는 ``{ProjectHome}\proj.win32\{ProjectName}.sln`` 파일을 ``Visual Studio`` 프로그램으로 열어서 설정을 진행합니다.

**1. 외부 라이브러리 복사.**

아이펀 엔진 플러그인을 Win32 환경에서 빌드하기 위해 필요한 외부 라이브러리인 ``proj.win32/libsodium-1.0.10``,
``proj.win32/protobuf-2.6.1``, ``proj.win32/zstd-1.3.3`` 들을 **새 프로젝트의** ``proj.win32`` 폴더에 복사합니다.

**2. 외부 라이브러리 설정**

외부 라이브러리 들은 프로젝트과 함께 제공되므로 새 프로젝트 빌드 시에 **함께 빌드되도록** 설정 해 줘야 합니다.
1. 다음 외부 라이브러리의 프로젝트 파일들을 새 프로젝트의 솔루션 파일에 추가합니다.

  * ``proj.win32/libsodium-1.0.10/libsodium.vcxproj``.
  * ``proj.win32/protobuf-2.6.1/vsprojects/libprotobuf.vcxproj``.
  * ``proj.win32/zstd-1.3.3/build/VS2017/libzstd/libzstd.vcxproj``.

2. 추가한 외부 라이브러리 프로젝트에 대해서 의존성 설정하기.

  * 솔루션 속성의 **Project Dependencies** 에서 **새 프로젝트**를 선택 후
    ``libcocos2d``, ``libprotobuf``,``libsodium``, ``libSpine``, ``libzstd`` **체크박스를 활성화** 합니다.

**3. 새 프로젝트에 외부 라이브러리 설정하기.**

1. 프로젝트 속성의 **Include Directories** 에 아래 목록들을 추가합니다.

  - ``$(EngineRoot)external/win32-specific/zlib/include``
  - ``$(EngineRoot)external/uv/include``
  - ``$(EngineRoot)external/websockets/include/win32``
  - ``$(EngineRoot)external/curl/include/win32``
  - ``$(ProjectDir)zstd-1.3.3/lib``
  - ``$(ProjectDir)protobuf-2.6.1/vsprojects/include``
  - ``$(ProjectDir)libsodium-1.0.10/src/libsodium/include``

2. 프로젝트 속성의 **Libarary Directories** 에 아래 목록들을 추가합니다.

  - ``$(ProjectDir)zstd-1.3.3/build/VS2017/bin/Win32_Debug``
  - ``$(ProjectDir)protobuf-2.6.1/vsprojects/Debug``

3. 프로젝트 속성의 **Additional Dependencies** 에 아래 목록들을 추가합니다.

  - ``libzstd_static.lib``
  - ``websockets.lib``
  - ``libcrypto.lib``
  - ``libssl.lib``
  - ``libprotobuf.lib``
  - ``libsodium.lib``

**4. 새 프로젝트에 아이펀 엔진 플러그인 소스코드들과 md5.c 소스코드 추가.**

  - 아이펀 엔진 플러그인 소스코드들을 새 프로젝트 빌드에 포함하기 위해 ``Visual Studio Solution Explorer`` 의 ``{Project_name}/src`` 폴더에 ``Classes/funapi`` 소스코드들을 추가합니다.
  - 다른 플랫폼과 다르게 Win32 플랫폼은 MD5 의존성이 없기 때문에 ``{Project_name}/src`` 에 ``cocos2d/external/md5/md5.c`` 소스코드를 추가합니다.

### iOS 프로젝트 설정

**iOS 프로젝트**는 ``{ProjectHome}\proj.ios_mac\{ProjectName}.xcodeproj`` 파일을 ``Xcode`` 프로그램으로 열어서 설정을 진행합니다.

**iOS**, **macOS** 는 같은 Xcode 프로젝트를 사용하고 **Scheme** 으로 분리 되어 있기 때문에 **Scheme** 을 **mobile** 로 변경 합니다.

**1. 외부 라이브러리 복사.**

아이펀 엔진 플러그인을 iOS 환경에서 빌드하기 위해 필요한 외부 라이브러리 폴더인 ``proj.ios_mac/ios/libsodium``,
``proj.ios_mac/ios/libprotobuf``, ``proj.ios_mac/ios/libzstd`` 들을 **새 프로젝트의** ``proj.ios_mac/ios`` 폴더에 복사합니다.

**2. 새 프로젝트에 외부 라이브러리 설정하기.**

iOS 환경에서 사용되는 **외부 라이브러리**들은 **미리 컴파일된 라이브러리** 파일로 제공되기 때문에 **Xcode** 의
**라이브러리 설정**에 추가하면 컴파일하지 않아도 사용할 수 있습니다.

1. 프로젝트 속성의 **Header Search Path** 에 아래 목록들을 추가합니다.

  - ``$(PROJECT_DIR)/../cocos2d/external/openssl/include/ios``
  - ``$(PROJECT_DIR)/../cocos2d/external/uv/include``
  - ``$(PROJECT_DIR)/../cocos2d/external/websockets/include/ios``
  - ``$(PROJECT_DIR)/../cocos2d/external/curl/include/ios``
  - ``$(PROJECT_DIR)/ios/libzstd/include``
  - ``$(PROJECT_DIR)/ios/libsodium/include``
  - ``$(PROJECT_DIR)/ios/libprotobuf/include``
  - ``$(PROJECT_DIR)/ios/libprotobuf/include``
  - ``$(PROJECT_DIR)/../Classes``

2. 프로젝트 속성의 **Library Search Path** 에 아래 목록들을 추가합니다.

  - ``$(PROJECT_DIR)/../cocos2d/external/curl/prebuilt/ios``
  - ``$(PROJECT_DIR)/ios/libzstd/lib``
  - ``$(PROJECT_DIR)/ios/libsodium/lib``
  - ``$(PROJECT_DIR)/ios/libprotobuf/lib``

3. 프로젝트 속성의 **Framework, Library, and Embedded Content** 에 아래 파일들을 추가합니다.

  - ``proj.ios_mac/ios/libzstd/lib/libzstd.a``
  - ``proj.ios_mac/ios/libsodium/lib/libsodium.a``
  - ``proj.ios_mac/ios/libprotobuf/lib/libprotobuf.a``
  - ``cocos2d/external/curl/prebuilt/ios/libcurl.a``

**4. 새 프로젝트에 아이펀 엔진 플러그인 소스코드 추가.**

복사한 플러그인 소스코드들을 새 프로젝트 빌드에 포함하기 위해 ``Xcode Project Navigator`` 의  ``{Project_name}/Classes`` 폴더에
``Classes/funapi`` 폴더를 복사합니다.

### macOS 프로젝트 설정

**macOS 프로젝트**는 ``{ProjectHome}\proj.ios_mac\{ProjectName}.xcodeproj`` 파일을 ``Xcode`` 프로그램으로 열어서 설정을 진행합니다.

**iOS**, **macOS** 는 같은 Xcode 프로젝트를 사용하고 **Scheme** 으로 분리 되어 있기 때문에 **Scheme** 을 **desktop** 로 변경 합니다.

**1. 외부 라이브러리 복사.**

아이펀 엔진 플러그인을 macOS 환경에서 빌드하기 위해 필요한 외부 라이브러리 폴더인 ``proj.ios_mac/mac/libsodium``,
``proj.ios_mac/mac/libprotobuf``, ``proj.ios_mac/mac/libzstd`` 들을 **새 프로젝트의** ``proj.ios_mac/mac`` 폴더에 복사합니다.

**2. 새 프로젝트에 외부 라이브러리 설정하기.**

macOS 환경에서 사용되는 **플러그인 외부 라이브러리**들은 **미리 컴파일된 라이브러리** 파일로 제공되기 때문에 **Xcode** 의
**라이브러리 설정**에 추가하면 컴파일하지 않아도 사용할 수 있습니다.

1. 프로젝트 속성의 **Header Search Path** 에 아래 목록들을 추가합니다.

  - ``$(PROJECT_DIR)/../cocos2d/external/openssl/include/mac``
  - ``$(PROJECT_DIR)/../cocos2d/external/uv/include``
  - ``$(PROJECT_DIR)/../cocos2d/external/websockets/include/mac``
  - ``$(PROJECT_DIR)/../cocos2d/external/curl/include/mac``
  - ``$(PROJECT_DIR)/mac/libzstd/include``
  - ``$(PROJECT_DIR)/mac/libsodium/include``
  - ``$(PROJECT_DIR)/mac/libprotobuf/include``
  - ``$(PROJECT_DIR)/mac/libprotobuf/include``
  - ``$(PROJECT_DIR)/../Classes``

2. 프로젝트 속성의 **Library Search Path** 에 아래 목록들을 추가합니다.

  - ``$(PROJECT_DIR)/../cocos2d/external/curl/prebuilt/mac``
  - ``$(PROJECT_DIR)/mac/libzstd/lib``
  - ``$(PROJECT_DIR)/mac/libsodium/lib``
  - ``$(PROJECT_DIR)/mac/libprotobuf/lib``

3. 프로젝트 속성의 **Framework, Library, and Embedded Content** 에 아래 파일들을 추가합니다.

  - ``proj.ios_mac/mac/libzstd/lib/libzstd.a``
  - ``proj.ios_mac/mac/libsodium/lib/libsodium.a``
  - ``proj.ios_mac/mac/libprotobuf/lib/libprotobuf.a``
  - ``cocos2d/external/curl/prebuilt/mac/libcurl.a``

**4. 새 프로젝트에 아이펀 엔진 플러그인 소스코드 추가.**

복사한 플러그인 소스코드들을 새 프로젝트 빌드에 포함하기 위해 ``Xcode Project Navigator`` 의  ``{Project_name}/Classes`` 폴더에
``Classes/funapi`` 폴더를 복사합니다.

### Android 프로젝트 설정

**Android 프로젝트**는 ``{ProjectHome}\proj.android`` 폴더를 ``Android studio`` 프로그램으로 열어서 설정을 진행합니다.

**1. 외부 라이브러리 복사.**

아이펀 엔진 플러그인을 Android 환경에서 빌드하기 위해 필요한 외부 라이브러리 폴더인 ``proj.android/app/jni/libsodium``,
``proj.android/app/jni/libprotobuf``, ``proj.android/app/jni/libzstd`` 들을 **새 프로젝트의** ``proj.android/app/jni`` 폴더에 복사합니다.

**2. 새 프로젝트에 외부 라이브러리 설정하기.**

**Android** 환경에서 사용되는 **플러그인 외부 라이브러리**들은 **미리 컴파일된 라이브러리** 파일로 제공이 되며
``Android.mk`` 빌드 스크립트에서 **모듈**로 등록해서 사용됩니다.

* **외부 라이브러리**들을 **로컬 라이브러리 모듈**로 만들기 위해 ``proj.android/app/jni/Android.mk`` 파일에 아래 코드를 추가합니다.

  ```makefile
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
  ```

* 아이펀 엔진 플러그인에서 **cocos2d-x 에 의존하는 모듈들을 import** 하기위해 ``proj.android/app/jni/Android.mk`` 파일에 아래 내용을 추가합니다.

  ```makefile
  $(call import-module, cocos)
  $(call import-module, cocos/external)
  $(call import-module, curl/prebuilt/android)
  ```

* ``proj.android/app/jni/Android.mk`` 파일에 있는 ``LOCAL_STATIC_LIBRARIES`` 변수에 **아이펀 엔진 플러그인에서 사용하는 모듈들을 등록**합니다.
  ```makefile
  LOCAL_STATIC_LIBRARIES := cc_static libsodium libprotobuf libzstd ext_curl ext_crypto
  ```

**3. 아이펀 엔진 플러그인 소스코드들을 프로젝트에 추가.**

아이펀 엔진 플러그인 소스코드들을 새 프로젝트 빌드에 포함하기 위해 ``proj.android/app/jni/Android.mk`` 파일의
``LOCAL_SRC_FILES``, ``LOCAL_C_INCLUDES`` 변수에 아래와 같이 플러그인 소스를 추가해 줍니다.

  ```makefile
  LOCAL_SRC_FILES := $(LOCAL_PATH)/hellocpp/main.cpp \
                     $(LOCAL_PATH)/../../../Classes/AppDelegate.cpp \
                     $(LOCAL_PATH)/../../../Classes/FunapiTestScene.cpp \
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
                     ../../../Classes/funapi/management/maintenance_message.pb.cc \
                     ../../../Classes/funapi/network/fun_message.pb.cc \
                     ../../../Classes/funapi/network/ping_message.pb.cc \
                     ../../../Classes/funapi/service/multicast_message.pb.cc \
                     ../../../Classes/funapi/service/redirect_message.pb.cc \
                     ../../../Classes/funapi/distribution/fun_dedicated_server_rpc_message.pb.cc \
                     ../../../Classes/test_dedicated_server_rpc_messages.pb.cc \
                     ../../../Classes/test_messages.pb.cc

  LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes \
                      $(LOCAL_PATH)/../../../Classes/funapi \
                      $(LOCAL_PATH)/../../../Classes/funapi/management \
                      $(LOCAL_PATH)/../../../Classes/funapi/network \
                      $(LOCAL_PATH)/../../../Classes/funapi/service \
                      $(LOCAL_PATH)/../../../Classes/funapi/distribution \
                      $(LOCAL_PATH)/libsodium/include \
                      $(LOCAL_PATH)/libprotobuf/include/ARMv7 \
                      $(LOCAL_PATH)/libzstd/include/ARMv7
  ```

**4. Gradle 빌드 설정.**

플러그인은 **ndk-build** 를 이용해 빌드 및 테스트가 완료 되었고 **ndk-build** 빌드 방법을 안내합니다.

**cmake** 를 사용해서 빌드하는 경우 cocos2d-x 홈페이지를 참고해 주세요.

* ``proj.android/app/jni/Application.mk`` 파일의 ``APP_STL`` 변수를 **c++_static** 에서 **gnustl_shared** 으로 변경합니다.
* ``proj.android/gradle.properties`` 파일의 ``PROP_BUILD_TYPE`` 변수를 **cmake** 에서 **ndk-build** 로 변경합니다.



# 도움말

클라이언트 플러그인의 도움말은 <https://www.ifunfactory.com/engine/documents/reference/ko/client-plugin.html> 를 참고해 주세요.

플러그인에 대한 궁금한 점은 <https://answers.ifunfactory.com> 에 질문을 올려주세요.
가능한 빠르게 답변해 드립니다.

그 외에 플러그인에 대한 문의 사항이나 버그 신고는 <funapi-support@ifunfactory.com> 으로 메일을
보내주세요.

# cocos2d-x 의 알려진 이슈

 * cocos2d-x 3.17 버전을 Xcode 11 이용해 빌드를 하면 ``Argument value 10880 is outside the valid range [0, 255]`` 에러가 발생합니다.
   위 문제는 cocos2d-x 에서 추후 업데이트로 수정될 예정이며 아래와 같이 문제를 회피할 수 있습니다.

  - ``cocos2d/external/bullet/include/bullet/LinearMath/btVector3.h`` 의 42 번째 라인의``BT_SHUFFLE`` 를 아래 코드로 변경.
  ```
  #define BT_SHUFFLE(x, y, z, w) (((w) << 6 | (z) << 4 | (y) << 2 | (x)) & 0xff)
  ```

# 버전별 주요 이슈

아래 설명의 버전보다 낮은 버전의 플러그인을 사용하고 있다면 아래 내용을 참고해 주세요.
괄호안은 지원하는 서버 버전입니다.

### v98 experimental (2626 experimental)
압축 기능 추가

### v90 experimental (2544 experimental)
Delayed Ack

### v85 experimental (2497 experimental)
TCP TLS

### v93 experimental (2497 experimental)
Websocket Transport

### v73 experimental (2368 experimental)
Dedicated Server Rpc

### v62 (2118 experimental, 2214 stable)
멀티캐스트 채널에 token 을 지정할 수 있는 기능 추가

### v61 (2109 experimental, 2214 stable)
Protobuf 의 Message Type 을 String 대신 Integer 를 쓸 수 있는 기능 추가
