#include "LocaleConfigManager.h"
#include "CCFileUtils.h"
#include "GlobalConfigManager.h"
#include "tinyxml2/tinyxml2.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"

LocaleConfigManager::LocaleConfigManager() {

}

std::string LocaleConfigManager::GetFilePath() {
	std::string pathprefix = "locale/"; // constant file in app package
	std::string fullpath = pathprefix + currentLangCode + ".xml"; // exp. "local/en_us.xml"
	if (!cocos2d::FileUtils::getInstance()->isFileExist(fullpath)) {
		currentLangCode = "en_us"; // restore to default language config(must exist)
		return GetFilePath();
	}
	return cocos2d::FileUtils::getInstance()->fullPathForFilename(fullpath);
}

LocaleConfigManager* LocaleConfigManager::GetInstance() {
	static LocaleConfigManager instance;
	return &instance;
}

const std::string &LocaleConfigManager::GetText(const std::string &tid) {
	auto it = AllConfig.find(tid);
	if (it == AllConfig.end()) {
		AllConfig[tid] = tid;
		return AllConfig[tid];
	}
	return it->second;
}

void LocaleConfigManager::Initialize(const std::string &sysLang) {
	// override by global configured lang
	currentLangCode = GlobalConfigManager::GetInstance()->GetValue<std::string>("user_language", "");
	if (currentLangCode.empty()) currentLangCode = sysLang;
	AllConfig.clear();
	tinyxml2::XMLDocument doc;
	std::string xmlData = cocos2d::FileUtils::getInstance()->getStringFromFile(GetFilePath());
	bool _writeBOM = false;
	const char* p = xmlData.c_str();
	p = tinyxml2::XMLUtil::ReadBOM(p, &_writeBOM);
	doc.ParseDeep((char*)p, nullptr);
	tinyxml2::XMLElement *rootElement = doc.RootElement();
	if (rootElement) {
		for (tinyxml2::XMLElement *item = rootElement->FirstChildElement(); item; item = item->NextSiblingElement()) {
			const char *key = item->Attribute("id");
			const char *val = item->Attribute("text");
			if (key && val) {
				AllConfig[key] = val;
			}
		}
	}
}

bool LocaleConfigManager::initText(cocos2d::ui::Text *ctrl) {
	if (!ctrl) return false;
	return initText(ctrl, ctrl->getString());
}

bool LocaleConfigManager::initText(cocos2d::ui::Button *ctrl)
{
	if (!ctrl) return false;
	return initText(ctrl, ctrl->getTitleText());
}

bool LocaleConfigManager::initText(cocos2d::ui::Text *ctrl, const std::string &tid) {
	if (!ctrl) return false;

	std::string txt = GetText(tid);
	if (txt.empty()) {
		ctrl->setColor(cocos2d::Color3B::RED);
		return false;
	}

	ctrl->setString(txt);
	return true;
}

bool LocaleConfigManager::initText(cocos2d::ui::Button *ctrl, const std::string &tid) {
	if (!ctrl) return false;

	std::string txt = GetText(tid);
	if (txt.empty()) {
		ctrl->setTitleColor(cocos2d::Color3B::RED);
		return false;
	}

	ctrl->setTitleText(txt);
	return true;
}

