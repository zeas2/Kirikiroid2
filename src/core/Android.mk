LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := krkr2

LOCAL_SRC_FILES := \
$(filter-out $(LOCAL_PATH)/visual/Resampler.cpp, $(wildcard $(LOCAL_PATH)/visual/*.cpp)) \
$(wildcard $(LOCAL_PATH)/base/7zip/*.c) \
$(wildcard $(LOCAL_PATH)/base/7zip/C/*.c) \
$(wildcard $(LOCAL_PATH)/base/7zip/CPP/*/*.cpp) \
$(wildcard $(LOCAL_PATH)/base/7zip/CPP/*/*/*.cpp) \
$(wildcard $(LOCAL_PATH)/base/7zip/CPP/*/*/*/*.cpp) \
$(wildcard $(LOCAL_PATH)/base/*.cpp) \
$(filter-out $(LOCAL_PATH)/base/win32/FuncStubs.cpp $(LOCAL_PATH)/base/win32/SusieArchive.cpp, $(wildcard $(LOCAL_PATH)/base/win32/*.cpp)) \
$(filter-out $(LOCAL_PATH)/environ/MainFormUnit.cpp, $(wildcard $(LOCAL_PATH)/environ/*.cpp)) \
$(wildcard $(LOCAL_PATH)/environ/cocos2d/*.cpp) \
$(wildcard $(LOCAL_PATH)/environ/android/*.cpp) \
$(wildcard $(LOCAL_PATH)/environ/linux/*.cpp) \
$(wildcard $(LOCAL_PATH)/environ/ui/*.cpp) \
$(wildcard $(LOCAL_PATH)/environ/ui/extension/*.cpp) \
environ/win32/SystemControl.cpp         \
$(wildcard $(LOCAL_PATH)/extension/*.cpp) \
$(wildcard $(LOCAL_PATH)/movie/*.cpp) \
$(wildcard $(LOCAL_PATH)/movie/*/*.cpp) \
$(wildcard $(LOCAL_PATH)/msg/*.cpp) \
msg/win32/MsgImpl.cpp               \
msg/win32/OptionsDesc.cpp               \
$(filter-out $(LOCAL_PATH)/sound/xmmlib.cpp $(LOCAL_PATH)/sound/WaveFormatConverter_SSE.cpp, $(wildcard $(LOCAL_PATH)/sound/*.cpp)) \
$(wildcard $(LOCAL_PATH)/sound/win32/*.cpp) \
$(wildcard $(LOCAL_PATH)/tjs2/*.cpp) \
$(wildcard $(LOCAL_PATH)/utils/*.c) \
$(wildcard $(LOCAL_PATH)/utils/*.cpp) \
$(wildcard $(LOCAL_PATH)/utils/encoding/*.c) \
$(wildcard $(LOCAL_PATH)/utils/minizip/*.c) \
$(wildcard $(LOCAL_PATH)/utils/minizip/*.cpp) \
$(wildcard $(LOCAL_PATH)/utils/win32/*.cpp) \
$(wildcard $(LOCAL_PATH)/visual/gl/*.cpp) \
$(wildcard $(LOCAL_PATH)/visual/ogl/*.cpp) \
$(filter-out $(LOCAL_PATH)/visual/win32/GDIFontRasterizer.cpp $(LOCAL_PATH)/visual/win32/NativeFreeTypeFace.cpp \
	$(LOCAL_PATH)/visual/win32/TVPSysFont.cpp $(LOCAL_PATH)/visual/win32/VSyncTimingThread.cpp \
	, $(wildcard $(LOCAL_PATH)/visual/win32/*.cpp)) \

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/base  \
                 $(LOCAL_PATH)/base/win32 \
                 $(LOCAL_PATH)/environ \
                 $(LOCAL_PATH)/environ/win32 \
                 $(LOCAL_PATH)/environ/android \
                 $(LOCAL_PATH)/environ/sdl \
                 $(LOCAL_PATH)/msg \
                 $(LOCAL_PATH)/msg/win32 \
                 $(LOCAL_PATH)/extension \
                 $(LOCAL_PATH)/sound \
                 $(LOCAL_PATH)/sound/win32 \
                 $(LOCAL_PATH)/tjs2 \
                 $(LOCAL_PATH)/utils \
                 $(LOCAL_PATH)/utils/win32 \
				 $(LOCAL_PATH)/../../vendor/freetype/current/include \
                 $(LOCAL_PATH)/visual \
                 $(LOCAL_PATH)/visual/ARM \
                 $(LOCAL_PATH)/visual/win32 \
                 $(LOCAL_PATH)/../plugins \
				 $(LOCAL_PATH)/../../vendor/jxrlib/current/common/include \
				 $(LOCAL_PATH)/../../vendor/jxrlib/current/image/sys \
				 $(LOCAL_PATH)/../../vendor/jxrlib/current/jxrgluelib \
				 $(LOCAL_PATH)/../../vendor/libjpeg-turbo/current \
				 $(LOCAL_PATH)/../../vendor/onig/current \
				 $(LOCAL_PATH)/../../vendor/libiconv/current/include \
				 $(LOCAL_PATH)/../../vendor/fmod/include \
                 $(LOCAL_PATH)/../../vendor/libgdiplus/src \
                 $(LOCAL_PATH)/../../vendor/opus/current/include \
                 $(LOCAL_PATH)/../../vendor/opus/opusfile/include \
                 $(LOCAL_PATH)/../../vendor/opencv/current/build/include \
                 $(LOCAL_PATH)/../../vendor/openal/current/include \
                 $(LOCAL_PATH)/../../vendor/lz4 \
				 $(LOCAL_PATH)/../../libs/android/bpg/include \
				 $(LOCAL_PATH)/../../libs/android/ffmpeg/include \
				 $(LOCAL_PATH)/../../libs/android/libarchive/include \
                 $(LOCAL_PATH)/visual/RenderScript/rs \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/cocos \
	$(LOCAL_PATH)/../../vendor/cocos2d-x/current/cocos/platform \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/external/webp/include/android \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/external/jpeg/include/android \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/external/png/include/android \
    $(LOCAL_PATH)/../../vendor/cocos2d-x/current/external \

LOCAL_CPPFLAGS += -DTJS_TEXT_OUT_CRLF -D__STDC_CONSTANT_MACROS -DUSE_UNICODE_FSTRING
LOCAL_CFLAGS += -DTJS_TEXT_OUT_CRLF -D_7ZIP_ST
LOCAL_STATIC_LIBRARIES := ffmpeg libopencv_imgproc libopencv_core libopencv_hal libtbb gdiplus_static cpufeatures \
        opusfile_static opus_static onig_static libbpg_static vorbis_static cairo_static pixman_static expat_static \
		breakpad_client openal_static jxrlib_static libarchive_static
# libRScpp_static

include $(BUILD_STATIC_LIBRARY)
$(call import-module, android/cpufeatures)
$(call import-module, opus)
$(call import-module, opusfile)
$(call import-module, onig)
$(call import-module, bpg)
$(call import-module, opencv)
$(call import-module, openal)
$(call import-module, vorbis)
#$(call import-module, android-ndk-profiler)
$(call import-module, jxrlib)
$(call import-module, ffmpeg)
$(call import-module, cairo)
$(call import-module, pixman)
$(call import-module, gdiplus)
$(call import-module, expat)
$(call import-module, google_breakpad)
#$(call import-module, libRScpp)
$(call import-module, libarchive)