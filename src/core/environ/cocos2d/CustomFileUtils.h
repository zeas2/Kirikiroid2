#pragma once
#include "cocos2d.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#include "platform/win32/CCFileUtils-win32.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#import <Foundation/NSBundle.h>
#import "platform/apple/CCFileUtils-apple.h"
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/CCFileUtils-android.h"
#endif
#ifdef MINIZIP_FROM_SYSTEM
#include <minizip/unzip.h>
#else // from our embedded sources
#include "external/unzip/unzip.h"
#endif

NS_CC_BEGIN

typedef
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
FileUtilsWin32
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
FileUtilsApple
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
FileUtilsAndroid
#endif
FileUtilsInherit;

class CustomFileUtils : public FileUtilsInherit
{
public:
	CustomFileUtils();

	void addAutoSearchArchive(const std::string& path);
	virtual std::string fullPathForFilename(const std::string &filename) const override;
	virtual std::string getStringFromFile(const std::string& filename) override;
	virtual Data getDataFromFile(const std::string& filename) override;
	virtual unsigned char* getFileData(const std::string& filename, const char* mode, ssize_t *size) override;
	virtual bool isFileExistInternal(const std::string& strFilePath) const override;
	virtual bool isDirectoryExistInternal(const std::string& dirPath) const override;
	virtual bool init() override {
		return FileUtilsInherit::init();
	}

private:
	unsigned char* getFileDataFromArchive(const std::string& filename, ssize_t *size);

	std::unordered_map<std::string, std::pair<unzFile, unz_file_pos> > _autoSearchArchive;
	std::mutex _lock;
};

NS_CC_END