APP_STL := gnustl_static
APP_CPPFLAGS := -fexceptions -std=c++11
APP_CPPFLAGS += -O3 -DNDEBUG -Wno-inconsistent-missing-override
#APP_CPPFLAGS += -D__DO_PROF__ -g -pg
APP_ABI := armeabi-v7a
APP_ABI += arm64-v8a
APP_PLATFORM := android-10
#NDK_TOOLCHAIN_VERSION := 4.9
NDK_TOOLCHAIN_VERSION := clang