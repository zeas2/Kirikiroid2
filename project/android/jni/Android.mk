#ndk-build MODULE_PATH=jni

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := kirikiri2

LOCAL_MODULE_FILENAME := libgame

LOCAL_SRC_FILES := src/SDL_android_main.cpp

LOCAL_LDLIBS := -lGLESv1_CM -llog -ldl -lGLESv2 -landroid -lm
LOCAL_LDLIBS += -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=realloc -Wl,--wrap=calloc

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../vendor/libgdiplus/src \
	$(LOCAL_PATH)/../../../vendor/google_breakpad/current/src \
	$(LOCAL_PATH)/../../../vendor/google_breakpad/current/src/common/android/include \
	$(LOCAL_PATH)/../../../src/core/environ \
	$(LOCAL_PATH)/../../../src/core/environ/android \
	$(LOCAL_PATH)/../../../src/core/tjs2 \
	$(LOCAL_PATH)/../../../src/core/base \
	$(LOCAL_PATH)/../../../src/core/visual \
	$(LOCAL_PATH)/../../../src/core/visual/win32 \
	$(LOCAL_PATH)/../../../src/core/sound \
	$(LOCAL_PATH)/../../../src/core/sound/win32 \
	$(LOCAL_PATH)/../../../src/core/utils \
				 
LOCAL_WHOLE_STATIC_LIBRARIES := kr2plugin krkr2 krkr2_neon_opt cocos2dx_static
#LOCAL_WHOLE_STATIC_LIBRARIES += tcmalloc
LOCAL_STATIC_LIBRARIES := android-ndk-profiler
LOCAL_CPPFLAGS += -Dtypeof=decltype

include $(BUILD_SHARED_LIBRARY)
$(call import-module, core)
$(call import-module, ARM_neon)
$(call import-module, plugins)
#$(call import-module, png) #cocos
#$(call import-module, jpeg) #cocos
$(call import-module, cocos)
#$(call import-module, tcmalloc)
#$(call import-module, ../../../vendor/libgdiplus/jni)