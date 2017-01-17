LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := kr2plugin

LOCAL_SRC_FILES := \
$(wildcard $(LOCAL_PATH)/*.cpp) \
$(wildcard $(LOCAL_PATH)/*.c) \
ncbind/ncbind.cpp \

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES += \
$(LOCAL_PATH) \
$(LOCAL_PATH)/ncbind \
$(LOCAL_PATH)/../../vendor/freetype/current/include \
$(LOCAL_PATH)/../core \
$(LOCAL_PATH)/../core/base  \
$(LOCAL_PATH)/../core/base/win32 \
$(LOCAL_PATH)/../core/environ \
$(LOCAL_PATH)/../core/environ/sdl \
$(LOCAL_PATH)/../core/msg \
$(LOCAL_PATH)/../core/msg/win32 \
$(LOCAL_PATH)/../core/sound \
$(LOCAL_PATH)/../core/sound/win32 \
$(LOCAL_PATH)/../core/tjs2 \
$(LOCAL_PATH)/../core/utils \
$(LOCAL_PATH)/../core/utils/win32 \
$(LOCAL_PATH)/../core/visual \
$(LOCAL_PATH)/../core/visual/ARM \
$(LOCAL_PATH)/../core/visual/win32 \
$(LOCAL_PATH)/../plugins \
$(LOCAL_PATH)/../../vendor/boost/current \
$(LOCAL_PATH)/../../vendor/libiconv/current/include \
$(LOCAL_PATH)/../../vendor/libgdiplus/src \
$(LOCAL_PATH)/../../vendor/agg/current/include \
$(LOCAL_PATH)/../../vendor/expat/current/lib \
$(LOCAL_PATH)/../../vendor/openal/current/include \
$(LOCAL_PATH)/../../libs/android/ffmpeg/include \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/external/jpeg/include/android \
				 
#LOCAL_LDLIBS := -lGLESv1_CM -lz -llog
LOCAL_CPPFLAGS := -fexceptions -std=c++11 -D__STDC_CONSTANT_MACROS -DTJS_TEXT_OUT_CRLF -fvisibility=hidden
LOCAL_CFLAGS += -DTJS_TEXT_OUT_CRLF
LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_STATIC_LIBRARY)
$(call import-module,android/cpufeatures)
