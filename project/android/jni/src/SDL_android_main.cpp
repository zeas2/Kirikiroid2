/* Include the SDL main definition header */
#include <jni.h>
#include "platform/android/jni/JniHelper.h"
#include "cocos2d/AppDelegate.h"
#include "cocos2d/MainScene.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "Application.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <string.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"

//std::string Android_GetDumpStoragePath();

static bool __DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
	void* context, bool succeeded)
{
	return succeeded;
}

extern bool TVPSystemUninitCalled;

static bool __DumpFilter(void *data) {
	if(TVPSystemUninitCalled) return false; // if trying exit system, ignore all exception
	return true;
}


//static void __InitAndroidDump() {
//    static google_breakpad::MinidumpDescriptor descriptor(Android_GetDumpStoragePath());
//	static google_breakpad::ExceptionHandler eh(descriptor, __DumpFilter, __DumpCallback,
//		NULL, true, -1);
//}

void cocos_android_app_init (JNIEnv* env) { // for cocos3.10+
//	__InitAndroidDump();
	static TVPAppDelegate *pAppDelegate = new TVPAppDelegate();
}

void cocos_android_app_init(JNIEnv* env, jobject thiz) {
//	__InitAndroidDump();
	static TVPAppDelegate *pAppDelegate = new TVPAppDelegate();
}

namespace kr2android {
	extern std::condition_variable MessageBoxCond;
	extern std::mutex MessageBoxLock;
	extern int MsgBoxRet;
    extern std::string MessageBoxRetText;
}
void Android_PushEvents(const std::function<void()> &func);
using namespace kr2android;
extern "C" {
	void Java_org_tvp_kirikiri2_KR2Activity_initDump(JNIEnv* env, jclass cls, jstring path) {
		const char* pszPath = env->GetStringUTFChars(path, NULL);
		if (pszPath && *pszPath) {
			static google_breakpad::MinidumpDescriptor descriptor(pszPath);
			static google_breakpad::ExceptionHandler eh(descriptor, __DumpFilter, __DumpCallback,
				NULL, true, -1);
		}
		env->ReleaseStringUTFChars(path, pszPath);
	}
	
	void Java_org_tvp_kirikiri2_KR2Activity_onMessageBoxOK(JNIEnv* env, jclass cls, jint nButton) {
		MsgBoxRet = nButton;
		MessageBoxCond.notify_one();
	}
    
	void Java_org_tvp_kirikiri2_KR2Activity_onMessageBoxText(JNIEnv* env, jclass cls, jstring text) {
		const char* pszText = env->GetStringUTFChars(text, NULL);
		if (pszText && *pszText) {
            MessageBoxRetText = pszText;
		}
		env->ReleaseStringUTFChars(text, pszText);
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeTouchesBegin(JNIEnv * env, jobject thiz, jint id, jfloat x, jfloat y) {
		intptr_t idlong = id;
		Android_PushEvents([idlong, x, y](){
			cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesBegin(1, (intptr_t*)&idlong, (float*)&x, (float*)&y);
		});
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeTouchesEnd(JNIEnv * env, jobject thiz, jint id, jfloat x, jfloat y) {
		intptr_t idlong = id;
		Android_PushEvents([idlong, x, y](){
			cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesEnd(1, (intptr_t*)&idlong, (float*)&x, (float*)&y);
		});
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeTouchesMove(JNIEnv * env, jobject thiz, jintArray ids, jfloatArray xs, jfloatArray ys) {
		int size = env->GetArrayLength(ids);
		if (size == 1) {
			intptr_t idlong;
			jint id;
			jfloat x;
			jfloat y;
			env->GetIntArrayRegion(ids, 0, size, &id);
			env->GetFloatArrayRegion(xs, 0, size, &x);
			env->GetFloatArrayRegion(ys, 0, size, &y);
			idlong = id;
			Android_PushEvents([idlong, x, y](){
				cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesMove(1, (intptr_t*)&idlong, (float*)&x, (float*)&y);
			});
			return;
		}
		
		jint id[size];
		std::vector<jfloat> x; x.resize(size);
		std::vector<jfloat> y; y.resize(size);

		env->GetIntArrayRegion(ids, 0, size, id);
		env->GetFloatArrayRegion(xs, 0, size, &x[0]);
		env->GetFloatArrayRegion(ys, 0, size, &y[0]);

		std::vector<intptr_t> idlong; idlong.resize(size);
		for (int i = 0; i < size; i++)
			idlong[i] = id[i];

		Android_PushEvents([idlong, x, y](){
			cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesMove(idlong.size(), (intptr_t*)&idlong[0], (float*)&x[0], (float*)&y[0]);
		});
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeTouchesCancel(JNIEnv * env, jobject thiz, jintArray ids, jfloatArray xs, jfloatArray ys) {
		int size = env->GetArrayLength(ids);
		if (size == 1) {
			intptr_t idlong;
			jint id;
			jfloat x;
			jfloat y;
			env->GetIntArrayRegion(ids, 0, size, &id);
			env->GetFloatArrayRegion(xs, 0, size, &x);
			env->GetFloatArrayRegion(ys, 0, size, &y);
			idlong = id;
			Android_PushEvents([idlong, x, y](){
				cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesCancel(1, (intptr_t*)&idlong, (float*)&x, (float*)&y);
			});
			return;
		}

		jint id[size];
		std::vector<jfloat> x; x.resize(size);
		std::vector<jfloat> y; y.resize(size);

		env->GetIntArrayRegion(ids, 0, size, id);
		env->GetFloatArrayRegion(xs, 0, size, &x[0]);
		env->GetFloatArrayRegion(ys, 0, size, &y[0]);

		std::vector<intptr_t> idlong; idlong.resize(size);
		for (int i = 0; i < size; i++)
			idlong[i] = id[i];

		Android_PushEvents([idlong, x, y](){
			cocos2d::Director::getInstance()->getOpenGLView()->handleTouchesCancel(idlong.size(), (intptr_t*)&idlong[0], (float*)&x[0], (float*)&y[0]);
		});
	}

#define KEYCODE_BACK 0x04
#define KEYCODE_MENU 0x52
#define KEYCODE_DPAD_UP 0x13
#define KEYCODE_DPAD_DOWN 0x14
#define KEYCODE_DPAD_LEFT 0x15
#define KEYCODE_DPAD_RIGHT 0x16
#define KEYCODE_ENTER 0x42
#define KEYCODE_PLAY  0x7e
#define KEYCODE_DPAD_CENTER  0x17
#define KEYCODE_DEL 0x43

	JNIEXPORT jboolean JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeKeyAction(JNIEnv * env, jclass cls, jint keyCode, jboolean isPress) {
		cocos2d::EventKeyboard::KeyCode pKeyCode;
		switch (keyCode) {
		case KEYCODE_BACK		: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE	; break;
		case KEYCODE_MENU		: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_MENU		; break;
		case KEYCODE_DPAD_UP	: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_DPAD_UP	; break;
		case KEYCODE_DPAD_DOWN	: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_DPAD_DOWN	; break;
		case KEYCODE_DPAD_LEFT	: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_DPAD_LEFT	; break;
		case KEYCODE_DPAD_RIGHT	: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_DPAD_RIGHT; break;
		case KEYCODE_ENTER		: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_ENTER		; break;
		case KEYCODE_PLAY		: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_PLAY		; break;
		case KEYCODE_DPAD_CENTER: pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_DPAD_CENTER; break;
        case KEYCODE_DEL          : pKeyCode = cocos2d::EventKeyboard::KeyCode::KEY_BACKSPACE; break;
		default: return JNI_FALSE;
		}

		Android_PushEvents([pKeyCode, isPress](){
			cocos2d::EventKeyboard event(pKeyCode, isPress);
			cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
		});
		return JNI_TRUE;
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeInsertText(JNIEnv* env, jclass cls, jstring text) {
		const char* pszText = env->GetStringUTFChars(text, NULL);
		if (pszText && *pszText) {
			std::string str = pszText;
			Android_PushEvents([str](){
				cocos2d::IMEDispatcher::sharedDispatcher()->dispatchInsertText(str.c_str(), str.length());
			});
		}
		env->ReleaseStringUTFChars(text, pszText);
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeDeleteBackward(JNIEnv* env, jclass cls) {
		Android_PushEvents(std::bind(&cocos2d::IMEDispatcher::dispatchDeleteBackward,
			cocos2d::IMEDispatcher::sharedDispatcher()));
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeCharInput(JNIEnv* env, jclass cls, jint keyCode) {
		TVPMainScene *pScene = TVPMainScene::GetInstance();
		if (!pScene) return;
		pScene->getScheduler()->performFunctionInCocosThread(std::bind(&TVPMainScene::onCharInput, keyCode));
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeCommitText(
		JNIEnv* env, jclass cls,
		jstring text, jint newCursorPosition)
	{
		TVPMainScene *pScene = TVPMainScene::GetInstance();
		if (!pScene) return;
		const char *utftext = env->GetStringUTFChars(text, NULL);
		std::string str(utftext);
		pScene->getScheduler()->performFunctionInCocosThread(std::bind(&TVPMainScene::onTextInput, str));
		env->ReleaseStringUTFChars(text, utftext);
	}

	JNIEXPORT jboolean JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton(JNIEnv* env, jclass cls)
	{
		return GlobalConfigManager::GetInstance()->GetValue<bool>("hide_android_sys_btn", false);
	}

	static float _mouseX, _mouseY;

	JNIEXPORT jboolean JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeHoverMoved(JNIEnv* env, jclass cls, jfloat x, jfloat y)
	{
		Android_PushEvents([x, y]() {
			cocos2d::GLView *glview = cocos2d::Director::getInstance()->getOpenGLView();
			float _scaleX = glview->getScaleX(), _scaleY = glview->getScaleY();
			_mouseX = x; _mouseY = y;
			const cocos2d::Rect _viewPortRect = glview->getViewPortRect();

			float cursorX = (_mouseX - _viewPortRect.origin.x) / _scaleX;
			float cursorY = (_viewPortRect.origin.y + _viewPortRect.size.height - _mouseY) / _scaleY;

			cocos2d::EventMouse event(cocos2d::EventMouse::MouseEventType::MOUSE_MOVE);
			event.setCursorPosition(cursorX, cursorY);
			cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
		});
		return true;
	}

	JNIEXPORT jboolean JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeMouseScrolled(JNIEnv* env, jclass cls, jfloat v)
	{
		Android_PushEvents([v]() {
			cocos2d::GLView *glview = cocos2d::Director::getInstance()->getOpenGLView();
			float _scaleX = glview->getScaleX(), _scaleY = glview->getScaleY();
			const cocos2d::Rect _viewPortRect = glview->getViewPortRect();

			float cursorX = (_mouseX - _viewPortRect.origin.x) / _scaleX;
			float cursorY = (_viewPortRect.origin.y + _viewPortRect.size.height - _mouseY) / _scaleY;

			cocos2d::EventMouse event(cocos2d::EventMouse::MouseEventType::MOUSE_SCROLL);
			event.setScrollData(0, v);
			event.setCursorPosition(cursorX, cursorY);
			cocos2d::Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
		});
		return true;
	}

	JNIEXPORT void JNICALL Java_org_tvp_kirikiri2_KR2Activity_nativeOnLowMemory(JNIEnv* env, jclass cls)
	{
		Android_PushEvents([]() {
			::Application->OnLowMemory();
		});
	}
}