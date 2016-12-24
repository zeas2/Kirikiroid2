#include "GlobalPreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIButton.h"
#include "cocos2d/MainScene.h"
#include "ui/UIListView.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "platform/CCFileUtils.h"

using namespace cocos2d;
using namespace cocos2d::ui;
#define GLOBAL_PREFERENCE

const char * const FileName_NaviBar = "ui/NaviBar.csb";
const char * const FileName_Body = "ui/ListView.csb";

namespace {
#include "PreferenceConfig.h"
};

TVPGlobalPreferenceForm * TVPGlobalPreferenceForm::create(const tPreferenceScreen *config) {
	initAllConfig();
	if (!config) config = &RootPreference;
	TVPGlobalPreferenceForm *ret = new TVPGlobalPreferenceForm();
	ret->autorelease();
	ret->initFromFile(FileName_NaviBar, FileName_Body, nullptr);
	PrefListSize = ret->PrefList->getContentSize();
	ret->initPref(config);
	ret->setOnExitCallback(std::bind(&GlobalConfigManager::SaveToFile, GlobalConfigManager::GetInstance()));
	return ret;
}
