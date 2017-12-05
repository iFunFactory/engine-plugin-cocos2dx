~/Library/Android/sdk/platform-tools/adb push app/build/intermediates/ndkBuild/debug/obj/local/armeabi-v7a/libgnustl_shared.so /data/local/tmp/
~/Library/Android/sdk/platform-tools/adb push app/build/intermediates/ndkBuild/debug/obj/local/armeabi-v7a/libMyGame.so /data/local/tmp/
~/Library/Android/sdk/platform-tools/adb push app/build/intermediates/ndkBuild/debug/obj/local/armeabi-v7a/funapi_unittest /data/local/tmp/
~/Library/Android/sdk/platform-tools/adb shell chmod 755 /data/local/tmp/funapi_unittest
~/Library/Android/sdk/platform-tools/adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/funapi_unittest"