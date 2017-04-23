#include "IndividualPreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIButton.h"
#include "cocos2d/MainScene.h"
#include "ui/UIListView.h"
#include "platform/CCFileUtils.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "Platform.h"

using namespace cocos2d;
using namespace cocos2d::ui;
#define INDIVIDUAL_PREFERENCE

const char * const FileName_NaviBar = "ui/NaviBar.csb";
const char * const FileName_Body = "ui/ListView.csb";
#define TVPGlobalPreferenceForm IndividualPreferenceForm

static bool PreferenceGetValueBool(const std::string &name, bool defval) {
	return IndividualConfigManager::GetInstance()->GetValue<bool>(name, defval);
}
static void PreferenceSetValueBool(const std::string &name, bool v) {
	IndividualConfigManager::GetInstance()->SetValueInt(name, v);
}
static std::string PreferenceGetValueString(const std::string &name, const std::string& defval) {
	return IndividualConfigManager::GetInstance()->GetValue<std::string>(name, defval);
}
static void PreferenceSetValueString(const std::string &name, const std::string& v) {
	IndividualConfigManager::GetInstance()->SetValue(name, v);
}
static float PreferenceGetValueFloat(const std::string &name, float defval) {
	return IndividualConfigManager::GetInstance()->GetValue<float>(name, defval);
}
static void PreferenceSetValueFloat(const std::string &name, float v) {
	IndividualConfigManager::GetInstance()->SetValueFloat(name, v);
}
#include "PreferenceConfig.h"

#undef TVPGlobalPreferenceForm

static void initInividualConfig() {
	if (!RootPreference.Preferences.empty()) return;
	initAllConfig();
	RootPreference.Title = "preference_title_individual";
}

IndividualPreferenceForm * IndividualPreferenceForm::create(const tPreferenceScreen *config) {
	initInividualConfig();
	if (!config) config = &RootPreference;
	IndividualPreferenceForm *ret = new IndividualPreferenceForm();
	ret->autorelease();
	ret->initFromFile(FileName_NaviBar, FileName_Body, nullptr);
	PrefListSize = ret->PrefList->getContentSize();
	ret->initPref(config);
	ret->setOnExitCallback(std::bind(&IndividualConfigManager::SaveToFile, IndividualConfigManager::GetInstance()));
	return ret;
}
