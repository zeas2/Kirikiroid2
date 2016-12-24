#include "IndividualConfigManager.h"
#include "platform/CCFileUtils.h"
#include "LocaleConfigManager.h"
#include "Platform.h"

#define FILENAME "Kirikiroid2Preference.xml"

IndividualConfigManager* IndividualConfigManager::GetInstance() {
	static IndividualConfigManager instance;
	return &instance;
}

std::string IndividualConfigManager::GetFilePath() {
	return CurrentPath;
}

void IndividualConfigManager::Clear()
{
	AllConfig.clear();
	CustomArguments.clear();
	ConfigUpdated = false;
	CurrentPath.clear();
}

bool IndividualConfigManager::CheckExistAt(const std::string &folder) {
	std::string fullpath = folder + "/" FILENAME;
	return cocos2d::FileUtils::getInstance()->isFileExist(fullpath);
}

bool IndividualConfigManager::CreatePreferenceAt(const std::string &folder) {
	std::string fullpath = folder + "/" FILENAME;
	FILE *fp =
#ifdef _MSC_VER
		_wfopen(ttstr(fullpath).c_str(), TJS_W("w"));
#else
		fopen(fullpath.c_str(), "w");
#endif
	Clear();
	if (!fp) {
		TVPShowSimpleMessageBox(
			LocaleConfigManager::GetInstance()->GetText("cannot_create_preference"),
			LocaleConfigManager::GetInstance()->GetText("readonly_storage"));
		return false;
	}
	CurrentPath = fullpath;
	return true;
}

bool IndividualConfigManager::UsePreferenceAt(const std::string &folder)
{
	std::string fullpath = folder + "/" FILENAME;
	if (CurrentPath == fullpath) return true;
	Clear();
	if (!cocos2d::FileUtils::getInstance()->isFileExist(fullpath)) return false;
	CurrentPath = fullpath;
	Initialize();
	return true;
}

bool IndividualConfigManager::GetValueBool(const std::string &name, bool defVal /*= false*/)
{
	return inherit::GetValueBool(name, GlobalConfigManager::GetInstance()->GetValueBool(name, defVal));
}

int IndividualConfigManager::GetValueInt(const std::string &name, int defVal /*= 0*/)
{
	return inherit::GetValueInt(name, GlobalConfigManager::GetInstance()->GetValueInt(name, defVal));
}

float IndividualConfigManager::GetValueFloat(const std::string &name, float defVal /*= 0*/)
{
	return inherit::GetValueFloat(name, GlobalConfigManager::GetInstance()->GetValueFloat(name, defVal));
}

std::string IndividualConfigManager::GetValueString(const std::string &name, std::string defVal /*= ""*/)
{
	return inherit::GetValueString(name, GlobalConfigManager::GetInstance()->GetValueString(name, defVal));
}

std::vector<std::string> IndividualConfigManager::GetCustomArgumentsForPush()
{
	if (CustomArguments.empty()) {
		return GlobalConfigManager::GetInstance()->GetCustomArgumentsForPush();
	}
	return inherit::GetCustomArgumentsForPush();
}
