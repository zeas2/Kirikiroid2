#include "AndroidUtils.h"
#include "minizip/unzip.h"
#include "zlib.h"
#include <map>
#include <string>
#include <vector>
#include "tjs.h"
#include "MsgIntf.h"
#include "md5.h"
#include "DebugIntf.h"
#include <condition_variable>
#include <mutex>
#include "platform/android/jni/JniHelper.h"
#include <set>
#include <sstream>
#include "SysInitIntf.h"
#include "platform/CCFileUtils.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "Platform.h"
#include "platform/CCCommon.h"
#include <EGL/egl.h>
#include <queue>
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include "TickCount.h"
#include "StorageImpl.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "EventIntf.h"
#include "RenderManager.h"
#include <sys/stat.h>

USING_NS_CC;

#define KR2ActJavaPath "org/tvp/kirikiri2/KR2Activity"
//#define KR2EntryJavaPath "org/tvp/kirikiri2/Kirikiroid2"

extern unsigned int __page_size = getpagesize();

void TVPPrintLog(const char *str) {
	__android_log_print(ANDROID_LOG_INFO, "kr2 debug info", "%s", str);
}

static tjs_uint32 _lastMemoryInfoQuery = 0;
static tjs_int _availMemory, _usedMemory;
static void _updateMemoryInfo() {
	if (TVPGetRoughTickCount32() - _lastMemoryInfoQuery > 3000) { // freq in 3s

		JniMethodInfo methodInfo;
		if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "updateMemoryInfo", "()V"))
		{
			methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}

		if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "getAvailMemory", "()J"))
		{
			_availMemory = methodInfo.env->CallStaticLongMethod(methodInfo.classID, methodInfo.methodID) / (1024 * 1024);
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}

		if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "getUsedMemory", "()J"))
		{
			// in kB
			_usedMemory = methodInfo.env->CallStaticLongMethod(methodInfo.classID, methodInfo.methodID) / 1024;
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		}

		_lastMemoryInfoQuery = TVPGetRoughTickCount32();
	}
}

tjs_int TVPGetSystemFreeMemory()
{
	_updateMemoryInfo();
	return _availMemory;
}

tjs_int TVPGetSelfUsedMemory()
{
	_updateMemoryInfo();
	return _usedMemory;
}

std::string TVPGetDeviceID()
{
    std::string ret;

    // use pure jni to avoid java code
// 	jclass classID = pEnv->FindClass(KR2EntryJavaPath);
// 	std::string strtmp("()L"); strtmp += KR2EntryJavaPath; strtmp += ";";
// 	jmethodID methodGetInstance = pEnv->GetStaticMethodID(classID, "GetInstance", strtmp.c_str());
// 	jobject sInstance = pEnv->CallStaticObjectMethod(classID, methodGetInstance);
// 	jmethodID getSystemService = pEnv->GetMethodID(classID, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
// 	jstring jstrPhone = pEnv->NewStringUTF("phone");
// 	jobject telephonyManager = pEnv->CallObjectMethod(sInstance, getSystemService, jstrPhone);
// 	pEnv->DeleteLocalRef(jstrPhone);
// 
// 	jclass clsTelephonyManager = pEnv->FindClass("android/telephony/TelephonyManager");
// 	jmethodID getDeviceId = pEnv->GetMethodID(clsTelephonyManager, "getDeviceId", "()Ljava/lang/String;");
// 	jstring jstrDevID = (jstring)pEnv->CallObjectMethod(telephonyManager, getDeviceId);
// 	if (jstrDevID) {
// 		const char *p = pEnv->GetStringUTFChars(jstrDevID, 0);
// 		if (p && *p) {
// 			ret = "DevID=";
// 			ret += p;
// 			pEnv->ReleaseStringUTFChars(jstrDevID, p);
// 		} else {
// 			if (p) {
// 				pEnv->ReleaseStringUTFChars(jstrDevID, p);
// 			}
// 			jmethodID getContentResolver = pEnv->GetMethodID(classID, "getContentResolver", "()Landroid/content/ContentResolver;");
// 			jobject contentResolver = pEnv->CallObjectMethod(sInstance, getContentResolver);
// 
// 			jclass clsSecure = pEnv->FindClass("android/provider/Settings/Secure");
// 			if (clsSecure) {
// 				jmethodID Secure_getString = pEnv->GetMethodID(clsSecure, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");
// 				jstring jastrAndroid_ID = pEnv->NewStringUTF("android_id");
// 				jstring jstrAndroidID = (jstring)pEnv->CallStaticObjectMethod(clsSecure, Secure_getString, contentResolver, jastrAndroid_ID);
// 				if (jstrAndroidID) {
// 					const char *p = pEnv->GetStringUTFChars(jstrAndroidID, 0);
// 					if (p && strlen(p) > 8 && strcmp(p, "9774d56d682e549c")) {
// 						ret = "AndroidID=";
// 						ret += p;
// 					}
// 				}
// 				pEnv->ReleaseStringUTFChars(jstrAndroidID, p);
// 				pEnv->DeleteLocalRef(jastrAndroid_ID);
// 			}
// 		}
// 	}
// 	if (ret.empty())
	{
		JniMethodInfo methodInfo;
		if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "getDeviceId", "()Ljava/lang/String;"))
		{
			jstring result = (jstring)methodInfo.env->CallStaticObjectMethod(methodInfo.classID, methodInfo.methodID);
			ret = JniHelper::jstring2string(result);
			methodInfo.env->DeleteLocalRef(result);
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
			char *t = (char*)ret.c_str();
			while (*t) {
				if (*t == ':') {
					*t = '=';
					break;
				}
				t++;
			}
		}
	}

    return ret;
}

static jobject GetKR2ActInstance() {
	JniMethodInfo methodInfo;
	std::string strtmp("()L"); strtmp += KR2ActJavaPath; strtmp += ";";
	if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "GetInstance", strtmp.c_str())) {
		jobject ret = methodInfo.env->CallStaticObjectMethod(methodInfo.classID, methodInfo.methodID);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
		return ret;
	}
	return 0;
}

static std::string GetApkStoragePath() {
	JniMethodInfo methodInfo;
	jobject sInstance = GetKR2ActInstance();
	if (!JniHelper::getMethodInfo(methodInfo, "android/content/Context", "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;")) {
		methodInfo.env->DeleteLocalRef(sInstance);
		return "";
	}
	jobject ApplicationInfo = methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID);
	jclass clsApplicationInfo = methodInfo.env->FindClass("android/content/pm/ApplicationInfo");
	jfieldID id_sourceDir = methodInfo.env->GetFieldID(clsApplicationInfo, "sourceDir", "Ljava/lang/String;");
	methodInfo.env->DeleteLocalRef(sInstance);
	return JniHelper::jstring2string((jstring)methodInfo.env->GetObjectField(ApplicationInfo, id_sourceDir));
}

static std::string GetPackageName() {
	JniMethodInfo methodInfo;
	jobject sInstance = GetKR2ActInstance();
	if (!JniHelper::getMethodInfo(methodInfo, "android/content/ContextWrapper", "getPackageName", "()Ljava/lang/String;")) {
		methodInfo.env->DeleteLocalRef(sInstance);
		return "";
	}
	return JniHelper::jstring2string((jstring)methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID));
}

// from unzip.cpp
#define FLAG_UTF8 (1<<11)
extern zlib_filefunc64_def TVPZlibFileFunc;
class ZipFile
{
	unzFile uf;
	bool utf8;

public:
	ZipFile() : uf(0) {
	}
	~ZipFile() {
		if (uf) {
			unzClose(uf);
			uf = NULL;
		}
	}
    bool Open(const char *filename) {
        if ((uf = unzOpen(filename)) == NULL) {
            ttstr msg = filename;
            msg += TJS_W(" can't open.");
            TVPThrowExceptionMessage(msg.c_str());
            return false;
        }
        // UTF8¤Ê¥Õ¥¡¥¤¥ëÃû¤«¤É¤¦¤«¤ÎÅÐ¶¨¡£×î³õ¤Î¥Õ¥¡¥¤¥ë¤Ç›Q¤á¤ë
        unzGoToFirstFile(uf);
        unz_file_info file_info;
        if (unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
            utf8 = (file_info.flag & FLAG_UTF8) != 0;
            return true;
        }
        return false;
    }
	bool GetData(std::vector<unsigned char> &buff, const char *filename) {
		bool ret = false;
		if (unzLocateFile(uf, filename, 1) == UNZ_OK) {
			int result = unzOpenCurrentFile(uf);
			if (result == UNZ_OK) {
				unz_file_info info;
				unzGetCurrentFileInfo(uf, &info, NULL, 0, NULL, 0, NULL, 0);
				buff.resize(info.uncompressed_size);
				unsigned int size = unzReadCurrentFile(uf, &buff[0], info.uncompressed_size);
				if (size == info.uncompressed_size) ret = true;
				unzCloseCurrentFile(uf);
			}
		}
		return ret;
	}
    tjs_int64 GetMD5InZip(const char *filename) {
        std::vector<unsigned char> buff;
        if(!GetData(buff, filename)) return 0;
        md5_state_t state;
        md5_init(&state);
        md5_append(&state, (const md5_byte_t*)&buff[0], buff.size());
        union {
            tjs_int64 _s64[2];
            tjs_uint8 _u8[16];
        } digest;
        md5_finish(&state, digest._u8);
        return digest._s64[0] ^ digest._s64[1];
    }
private:
	unzFile zipFile;
};

std::string TVPGetDeviceLanguage() {
	// use pure jni to avoid java code
	JniMethodInfo methodInfo;
	if (!JniHelper::getStaticMethodInfo(methodInfo, "java/util/Locale", "getDefault", "()Ljava/util/Locale;"))
		return "";
	jobject LocaleObj = methodInfo.env->CallStaticObjectMethod(methodInfo.classID, methodInfo.methodID);
	if (!JniHelper::getMethodInfo(methodInfo, "java/util/Locale", "getLanguage", "()Ljava/lang/String;"))
		return "";
	jstring languageID = (jstring)methodInfo.env->CallObjectMethod(LocaleObj, methodInfo.methodID);
	methodInfo.env->DeleteLocalRef(methodInfo.classID);
	return JniHelper::jstring2string(languageID);
}

std::string TVPGetPackageVersionString() {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, KR2ActJavaPath, "GetVersion", "()Ljava/lang/String;")) {
		return JniHelper::jstring2string((jstring)methodInfo.env->CallStaticObjectMethod(methodInfo.classID, methodInfo.methodID));
	}
	return "";
}

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.emplace_back(item);
	}
	return elems;
}

static std::string File_getAbsolutePath(jobject FileObj) {
	if (!FileObj) return "";
	JniMethodInfo methodInfo;
	if (!JniHelper::getMethodInfo(methodInfo, "java/io/File", "exists", "()Z")) return "";
	if (!methodInfo.env->CallBooleanMethod(FileObj, methodInfo.methodID)) return "";
	if (!JniHelper::getMethodInfo(methodInfo, "java/io/File", "getAbsolutePath", "()Ljava/lang/String;")) return "";
	jstring path = (jstring)methodInfo.env->CallObjectMethod(FileObj, methodInfo.methodID);
	std::string ret = cocos2d::JniHelper::jstring2string(path);
	return ret;
}

static std::string GetInternalStoragePath() {
	jobject sInstance = GetKR2ActInstance();
	JniMethodInfo methodInfo;
	if (!JniHelper::getMethodInfo(methodInfo, "android/content/ContextWrapper", "getFilesDir", "()Ljava/io/File;")) {
		return "";
	}
	jobject FileObj = methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID);
	return File_getAbsolutePath(FileObj);
}

std::string Android_GetDumpStoragePath() {
	return GetInternalStoragePath() + "/dump";
}

static int InsertFilepathInto(JNIEnv *env, std::vector<std::string>& vec, jobjectArray FileObjs) {
	int count = env->GetArrayLength(FileObjs);
	for (int i = 0; i < count; ++i) {
		jobject FileObj = env->GetObjectArrayElement(FileObjs, i);
		std::string path = File_getAbsolutePath(FileObj);
		if (!path.empty()) vec.emplace_back(path);
	}
	return count;
}

static int GetExternalStoragePath(std::vector<std::string> &ret) {
	int count = 0;
	JniMethodInfo methodInfo;
	jobject sInstance = GetKR2ActInstance();
// 	if (JniHelper::getMethodInfo(methodInfo, "android/content/Context", "getExternalMediaDirs", "()[Ljava/io/File;")) {
// 		jobjectArray FileObjs = (jobjectArray)methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID);
// 		if(FileObjs) count += InsertFilepathInto(methodInfo.env, ret, FileObjs);
// 	}
	if (JniHelper::getMethodInfo(methodInfo, "android/content/Context", "getExternalFilesDirs", "(Ljava/lang/String;)[Ljava/io/File;")) {
		jobjectArray FileObjs = (jobjectArray)methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID, nullptr);
		if (FileObjs) count += InsertFilepathInto(methodInfo.env, ret, FileObjs);
	} else if (JniHelper::getMethodInfo(methodInfo, "android/content/Context", "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;")) {
		jobject FileObj = methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID, nullptr);
		if (FileObj) {
			ret.emplace_back(File_getAbsolutePath(FileObj));
			++count;
		}
	}
	return count;
}

std::vector<std::string> TVPGetAppStoragePath() {
	std::vector<std::string> ret;
	ret.emplace_back(GetInternalStoragePath());
	GetExternalStoragePath(ret);
	return ret;
}

std::vector<std::string> TVPGetDriverPath() {
	std::vector<std::string> ret;
	jobject sInstance = GetKR2ActInstance();
	JniMethodInfo methodInfo;
	if (JniHelper::getMethodInfo(methodInfo, KR2ActJavaPath, "getStoragePath", "()[Ljava/lang/String;")) {
		jobjectArray PathObjs = (jobjectArray)methodInfo.env->CallObjectMethod(sInstance, methodInfo.methodID);
		if (PathObjs) {
			int count = methodInfo.env->GetArrayLength(PathObjs);
			for (int i = 0; i < count; ++i) {
				jstring path = (jstring)methodInfo.env->GetObjectArrayElement(PathObjs, i);
				if (path) ret.emplace_back(cocos2d::JniHelper::jstring2string(path));
			}
		}
	}
	
	if (!ret.empty()) return ret;

	char buffer[256] = { 0 };

	// enum all mounted storages
	FILE *fp = fopen("/proc/mounts", "r");
	std::set<std::string> mounted;
	while (fgets(buffer, sizeof(buffer), fp)) {
		std::vector<std::string> tabs;
		split(buffer, ' ', tabs);
		if (tabs.size() < 3) continue;
		if (mounted.find(tabs[0]) != mounted.end()) continue;
		const std::string &path = tabs[1];
		if (tabs[3].find("rw,") != std::string::npos && (tabs[2] == "vfat" || path.find("/mnt") != std::string::npos)) {
			if (path.find("/mnt/secure") != std::string::npos ||
				path.find("/mnt/asec") != std::string::npos ||
				path.find("/mnt/mapper") != std::string::npos ||
				path.find("/mnt/obb") != std::string::npos ||
				tabs[0] == "tmpfs" || tabs[2] == "tmpfs") {
				continue;
			}
			mounted.insert(tabs[0]);
			ret.emplace_back(path);
		}
	}

	return ret;
}

// extern "C" int TVPCheckValidate()
// {
//     JNIEnv *pEnv = 0;
//     bool ret = true;
// 
//     if (! getEnv(&pEnv))
//     {
//         return false;
//     }
// 	{
// 		jclass classID = pEnv->FindClass(KR2EntryJavaPath);
// 		std::string strtmp("()L"); strtmp += KR2EntryJavaPath; strtmp += ";";
// 		jmethodID methodGetInstance = pEnv->GetStaticMethodID(classID, "GetInstance", strtmp.c_str());
// 		jobject sInstance = pEnv->CallStaticObjectMethod(classID, methodGetInstance);
// 		
// 		jclass clsPreferenceManager = pEnv->FindClass("android.preference.PreferenceManager");
// 		jmethodID getDefaultSharedPreferences = pEnv->GetMethodID(clsPreferenceManager, "getDefaultSharedPreferences", "(Landroid/content/Context;)Landroid.preference.PreferenceManager;");
// 		jobject PreferenceManager = pEnv->CallStaticObjectMethod(clsPreferenceManager, getDefaultSharedPreferences, sInstance);
// 		jmethodID getString = pEnv->GetMethodID(clsPreferenceManager, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
// 		jstring jstrConstAPPID = pEnv->NewStringUTF("APP_ID");
// 		jstring jstrNull = pEnv->NewStringUTF("");
// 		jstring jstrAPPID = (jstring)pEnv->CallObjectMethod(PreferenceManager, getString, jstrConstAPPID, jstrNull);
// 		pEnv->DeleteLocalRef(jstrConstAPPID);
// 		pEnv->DeleteLocalRef(jstrNull);
// 		const char *p = pEnv->GetStringUTFChars(jstrAPPID, 0);
// 		if(0x929e08af != adler32(0, (const Bytef*)p, strlen(p))) ret = false;
// 		pEnv->ReleaseStringUTFChars(jstrAPPID, p);
// 	}
// 
//     return ret;
// }
namespace kr2android {
	std::condition_variable MessageBoxCond;
	std::mutex MessageBoxLock;
	int MsgBoxRet = 0;
	std::string MessageBoxRetText;
}
using namespace kr2android;

int TVPShowSimpleMessageBox(const char *pszText, const char *pszTitle, unsigned int nButton, const char **btnText) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "ShowMessageBox", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V"))
	{
		jstring jstrTitle = methodInfo.env->NewStringUTF(pszTitle);
		jstring jstrText = methodInfo.env->NewStringUTF(pszText);
		jclass strcls = methodInfo.env->FindClass("java/lang/String");
		jobjectArray btns = methodInfo.env->NewObjectArray(nButton, strcls, nullptr);
		for (unsigned int i = 0; i < nButton; ++i) {
			jstring jstrBtn = methodInfo.env->NewStringUTF(btnText[i]);
			methodInfo.env->SetObjectArrayElement(btns, i, jstrBtn);
			methodInfo.env->DeleteLocalRef(jstrBtn);
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, jstrTitle, jstrText, btns);

		methodInfo.env->DeleteLocalRef(jstrTitle);
		methodInfo.env->DeleteLocalRef(jstrText);
		methodInfo.env->DeleteLocalRef(btns);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);

		std::unique_lock<std::mutex> lk(MessageBoxLock);
		MessageBoxCond.wait(lk);
		return MsgBoxRet;
	}
	return -1;
}

int TVPShowSimpleMessageBox(const ttstr & text, const ttstr & caption, const std::vector<ttstr> &vecButtons) {
	tTJSNarrowStringHolder pszText(text.c_str());
	tTJSNarrowStringHolder pszTitle(caption.c_str());
	std::vector<const char *> btnText; btnText.reserve(vecButtons.size());
	std::vector<std::string> btnTextHold; btnTextHold.reserve(vecButtons.size());
	for (const ttstr &btn : vecButtons) {
		btnTextHold.emplace_back(btn.AsStdString());
		btnText.emplace_back(btnTextHold.back().c_str());
	}
	return TVPShowSimpleMessageBox(pszText, pszTitle, btnText.size(), &btnText[0]);
}

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption, const ttstr &prompt, const std::vector<ttstr> &vecButtons) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "ShowInputBox", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V"))
	{
		jstring jstrTitle = methodInfo.env->NewStringUTF(caption.AsStdString().c_str());
		jstring jstrText = methodInfo.env->NewStringUTF(text.AsStdString().c_str());
		jstring jstrPrompt = methodInfo.env->NewStringUTF(prompt.AsStdString().c_str());
		jclass strcls = methodInfo.env->FindClass("java/lang/String");
		jobjectArray btns = methodInfo.env->NewObjectArray(vecButtons.size(), strcls, nullptr);
		for (unsigned int i = 0; i < vecButtons.size(); ++i) {
			jstring jstrBtn = methodInfo.env->NewStringUTF(vecButtons[i].AsStdString().c_str());
			methodInfo.env->SetObjectArrayElement(btns, i, jstrBtn);
			methodInfo.env->DeleteLocalRef(jstrBtn);
		}

		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, jstrTitle, jstrPrompt, jstrText, btns);

		methodInfo.env->DeleteLocalRef(jstrTitle);
		methodInfo.env->DeleteLocalRef(jstrText);
		methodInfo.env->DeleteLocalRef(jstrPrompt);
		methodInfo.env->DeleteLocalRef(btns);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);

		std::unique_lock<std::mutex> lk(MessageBoxLock);
		MessageBoxCond.wait(lk);
		text = MessageBoxRetText;
		return MsgBoxRet;
	}
	return -1;
}

extern std::string Android_ShowInputDialog(const char* pszTitle, const char *pszInitText);
extern std::string Android_ShowFileBrowser(const char* pszTitle, const char *pszInitDir, bool showEditor);
extern ttstr TVPGetAppPath();
extern ttstr TVPGetLocallyAccessibleName(const ttstr &name);

std::vector<ttstr> Android_GetExternalStoragePath() {
	static std::vector<ttstr> ret;
	if (ret.empty()) {
		std::vector<std::string> pathlist;
		GetExternalStoragePath(pathlist);
		for (const std::string &path : pathlist) {
			std::string strPath = "file://.";
			strPath += path;
			ret.emplace_back(strPath);
		}
	}
	return ret;
}

ttstr Android_GetInternalStoragePath() {
	static ttstr strPath;
	if (strPath.IsEmpty()) {
		strPath = "file://.";
		strPath += GetInternalStoragePath();
	}
	return strPath;
}

ttstr Android_GetApkStoragePath() {
	static ttstr strPath;
	if (strPath.IsEmpty()) {
		strPath = "file://.";
		strPath += GetApkStoragePath();
	}
	return strPath;
}

struct _eventQueueNode {
	std::function<void()> func;
	_eventQueueNode *prev;
	_eventQueueNode *next;
};

static std::atomic<_eventQueueNode*> _lastQueuedEvents(nullptr);
static void _processEvents(float) {
	_eventQueueNode* q = _lastQueuedEvents.exchange(nullptr);
	if (q) {
		q->next = nullptr;
		while (q->prev) {
			q->prev->next = q;
			q = q->prev;
		}
	}
	while (q) {
		q->func();
		_eventQueueNode *nq = q->next;
		delete q;
		q = nq;
	}
}

void Android_PushEvents(const std::function<void()> &func) {
	_eventQueueNode *node = new _eventQueueNode;
	node->func = func;
	node->next = nullptr;
	node->prev = nullptr;
	while (!_lastQueuedEvents.compare_exchange_weak(node->prev, node)) {}
}

void TVPCheckAndSendDumps(const std::string &dumpdir, const std::string &packageName, const std::string &versionStr);
bool TVPCheckStartupArg() {
	// check dump
	TVPCheckAndSendDumps(Android_GetDumpStoragePath(), GetPackageName(), TVPGetPackageVersionString());

	// register event dispatcher
	cocos2d::Director *director = cocos2d::Director::getInstance();
	class HackForScheduler : public cocos2d::Scheduler {
	public:
		void regProcessEvents() {
			schedulePerFrame(_processEvents, &_lastQueuedEvents, -1, false);
		}
	};
	static_cast<HackForScheduler*>(director->getScheduler())->regProcessEvents();

	return false;
}

void TVPControlAdDialog(int adType, int arg1, int arg2) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "MessageController", "(III)V")) {
		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, adType, arg1, arg2);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
	}
}

void TVPForceSwapBuffer() {
	eglSwapBuffers(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW));
}

static int _GetAndroidSDKVersion() {
	JNIEnv *pEnv = JniHelper::getEnv();
	jclass classID = pEnv->FindClass("android/os/Build$VERSION");
	jfieldID idSDK_INT = pEnv->GetStaticFieldID(classID, "SDK_INT", "I");
	return pEnv->GetStaticIntField(classID, idSDK_INT);
}
static int GetAndroidSDKVersion() {
	static int result = _GetAndroidSDKVersion();
	return result;
}

static bool IsLollipop() {
	return GetAndroidSDKVersion() >= 21;
}

static bool IsOreo() {
	return GetAndroidSDKVersion() >= 26;
}

void TVPFetchSDCardPermission() {
	if (!IsLollipop())
		return;
	std::vector<std::string> paths;
	GetExternalStoragePath(paths);
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "requireLEXA", "(Ljava/lang/String;)V")) {
		jstring jstrPath = methodInfo.env->NewStringUTF(paths.back().c_str());
		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, jstrPath);
		methodInfo.env->DeleteLocalRef(jstrPath);
		return;
	}
	return;
}

bool TVPCheckStartupPath(const std::string &path) {
	// check writing permission first
	int pos = path.find_last_of('/');
	if (pos == path.npos) return false;
	std::string parent = path.substr(0, pos);
	JniMethodInfo methodInfo;
	bool success = false;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "isWritableNormalOrSaf", "(Ljava/lang/String;)Z")) {
		jstring jstrPath = methodInfo.env->NewStringUTF(parent.c_str());
		success = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstrPath);
		methodInfo.env->DeleteLocalRef(jstrPath);
		if (success) {
			parent += "/savedata";
			if (!TVPCheckExistentLocalFolder(parent)) {
				TVPCreateFolders(parent);
			}
			jstrPath = methodInfo.env->NewStringUTF(parent.c_str());
			success = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstrPath);
			methodInfo.env->DeleteLocalRef(jstrPath);
		}
	}

	if (!success) {
		std::vector<std::string> paths;
		paths.emplace_back(GetInternalStoragePath());
		GetExternalStoragePath(paths);
		std::string msg = LocaleConfigManager::GetInstance()->GetText("use_internal_path");
		if (paths.size() > 0) {
			size_t pos = msg.find("%1");
			if (pos != msg.npos) {
				msg = msg.replace(msg.begin() + pos, msg.begin() + pos + 2, paths.back());
			}
		}
		std::vector<ttstr> btns;
		btns.push_back(LocaleConfigManager::GetInstance()->GetText("continue_run"));
		bool isLOLLIPOP = IsLollipop();
		if (isLOLLIPOP)
			btns.push_back(LocaleConfigManager::GetInstance()->GetText("get_sdcard_permission"));
		else
			btns.push_back(LocaleConfigManager::GetInstance()->GetText("cancel"));
		int result = TVPShowSimpleMessageBox(msg, LocaleConfigManager::GetInstance()->GetText("readonly_storage"), btns);
		if (isLOLLIPOP && result == 1) {
			TVPFetchSDCardPermission();
		}
		if (result != 0)
			return false;
	}

	// check adreno GPU issue
// 	if (IndividualConfigManager::GetInstance()->GetValue<std::string>("renderer", "software") == "opengl") {
// 		TVPOnOpenGLRendererSelected(false);
// 	}
	return true;
}

bool TVPCreateFolders(const ttstr &folder)
{
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "CreateFolders", "(Ljava/lang/String;)Z")) {
		jstring jstr = methodInfo.env->NewStringUTF(folder.AsStdString().c_str());
		bool ret = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstr);
		methodInfo.env->DeleteLocalRef(jstr);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
		return ret;
	}
	return false;
}

static bool TVPWriteDataToFileJava(const std::string &filename, const void* data, unsigned int size) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "WriteFile", "(Ljava/lang/String;[B)Z")) {
		cocos2d::FileUtils *fileutil = cocos2d::FileUtils::getInstance();
		bool ret = false;
		int retry = 3;
		do {
			jstring jstr = methodInfo.env->NewStringUTF(filename.c_str());
			jbyteArray arr = methodInfo.env->NewByteArray(size);
			methodInfo.env->SetByteArrayRegion(arr, 0, size, (jbyte*)data);
			ret = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstr, arr);
			methodInfo.env->DeleteLocalRef(arr);
			methodInfo.env->DeleteLocalRef(jstr);
			methodInfo.env->DeleteLocalRef(methodInfo.classID);
		} while (!fileutil->isFileExist(filename) && --retry);
		return ret;
	}
	return false;
}

bool TVPWriteDataToFile(const ttstr &filepath, const void *data, unsigned int size) {
	std::string filename = filepath.AsStdString();
	cocos2d::FileUtils *fileutil = cocos2d::FileUtils::getInstance();
	while (fileutil->isFileExist(filename)) {
		// for number filename suffix issue
		time_t t = time(nullptr);
		std::vector<char> buffer;
		buffer.resize(filename.size() + 32);
		sprintf(&buffer.front(), "%s.%d.bak", filename.c_str(), (int)t);
		std::string tempname = &buffer.front();
		if (rename(filename.c_str(), tempname.c_str()) == 0) {
			// file api is OK
			FILE *fp = fopen(filename.c_str(), "wb");
			if (fp) {
				bool ret = fwrite(data, 1, size, fp) == size;
				fclose(fp);
				remove(tempname.c_str());
				return ret;
			}
		}
		bool ret = TVPWriteDataToFileJava(filename, data, size);
		if (fileutil->isFileExist(tempname.c_str())) {
			TVPDeleteFile(tempname);
		}
		return ret;
	}
	FILE *fp = fopen(filename.c_str(), "wb");
	if (fp) {
		// file api is OK
		int writed = fwrite(data, 1, size, fp);
		fclose(fp);
		return writed == size;
	}
	return TVPWriteDataToFileJava(filename, data, size);
}

std::string TVPGetCurrentLanguage() {
	JniMethodInfo t;
	std::string ret("");
	
	if (JniHelper::getStaticMethodInfo(t, "org/tvp/kirikiri2/KR2Activity", "getLocaleName", "()Ljava/lang/String;")) {
		jstring str = (jstring)t.env->CallStaticObjectMethod(t.classID, t.methodID);
		t.env->DeleteLocalRef(t.classID);
		ret = JniHelper::jstring2string(str);
		t.env->DeleteLocalRef(str);
	}

	return ret;
}

void TVPExitApplication(int code) {
	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
	if (!TVPIsSoftwareRenderManager())
		iTVPTexture2D::RecycleProcess();
	JniMethodInfo t;
	if (JniHelper::getStaticMethodInfo(t, "org/tvp/kirikiri2/KR2Activity", "exit", "()V")) {
		t.env->CallStaticVoidMethod(t.classID, t.methodID);
		t.env->DeleteLocalRef(t.classID);
	}
	exit(code);
}

void TVPHideIME() {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "hideTextInput", "()V")) {
		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID);
	}
}

void TVPShowIME(int x, int y, int w, int h) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "showTextInput", "(IIII)V")) {
		methodInfo.env->CallStaticVoidMethod(methodInfo.classID, methodInfo.methodID, x, y, w, h);
	}
}

void TVPProcessInputEvents() {}

bool TVPDeleteFile(const std::string &filename) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "DeleteFile", "(Ljava/lang/String;)Z")) {
		jstring jstr = methodInfo.env->NewStringUTF(filename.c_str());
		bool ret = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstr);
		methodInfo.env->DeleteLocalRef(jstr);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
		return ret;
	}
	return false;
}

bool TVPRenameFile(const std::string &from, const std::string &to) {
	JniMethodInfo methodInfo;
	if (JniHelper::getStaticMethodInfo(methodInfo, "org/tvp/kirikiri2/KR2Activity", "RenameFile", "(Ljava/lang/String;Ljava/lang/String;)Z")) {
		jstring jstr = methodInfo.env->NewStringUTF(from.c_str());
		jstring jstr2 = methodInfo.env->NewStringUTF(to.c_str());
		bool ret = methodInfo.env->CallStaticBooleanMethod(methodInfo.classID, methodInfo.methodID, jstr, jstr2);
		methodInfo.env->DeleteLocalRef(jstr);
		methodInfo.env->DeleteLocalRef(jstr2);
		methodInfo.env->DeleteLocalRef(methodInfo.classID);
		return ret;
	}
	return false;
}

tjs_uint32 TVPGetRoughTickCount32()
{
	tjs_uint32 uptime = 0;
	struct timespec on;
	if (clock_gettime(CLOCK_MONOTONIC, &on) == 0)
		uptime = on.tv_sec * 1000 + on.tv_nsec / 1000000;
	return uptime;
}

bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
	tTJSNarrowStringHolder holder(name);
	return TVP_stat(holder, s);
}

#undef st_atime
#undef st_ctime
#undef st_mtime
//int stat64(const char* __path, struct stat64* __buf) __INTRODUCED_IN(21); // force link it !
bool TVP_stat(const char *name, tTVP_stat &s) {
	struct stat t;
	// static_assert(sizeof(t.st_size) == 4, "");
	static_assert(sizeof(t.st_size) == 8, "");
	bool ret = !stat(name, &t);
	s.st_mode = t.st_mode;
	s.st_size = t.st_size;
	s.st_atime = t.st_atim.tv_sec;
	s.st_mtime = t.st_mtim.tv_sec;
	s.st_ctime = t.st_ctim.tv_sec;
	return ret;
}

void TVPSendToOtherApp(const std::string &filename) {

}
