#include "GlobalPreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIButton.h"
#include "cocos2d/MainScene.h"
#include "ui/UIListView.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "platform/CCFileUtils.h"
#include "Platform.h"

using namespace cocos2d;
using namespace cocos2d::ui;
#define GLOBAL_PREFERENCE

const char * const FileName_NaviBar = "ui/NaviBar.csb";
const char * const FileName_Body = "ui/ListView.csb";
static bool PreferenceGetValueBool(const std::string &name, bool defval) {
	return GlobalConfigManager::GetInstance()->GetValue<bool>(name, defval);
}
static void PreferenceSetValueBool(const std::string &name, bool v) {
	GlobalConfigManager::GetInstance()->SetValueInt(name, v);
}
static std::string PreferenceGetValueString(const std::string &name, const std::string& defval) {
	return GlobalConfigManager::GetInstance()->GetValue<std::string>(name, defval);
}
static void PreferenceSetValueString(const std::string &name, const std::string& v) {
	GlobalConfigManager::GetInstance()->SetValue(name, v);
}
static float PreferenceGetValueFloat(const std::string &name, float defval) {
	return GlobalConfigManager::GetInstance()->GetValue<float>(name, defval);
}
static void PreferenceSetValueFloat(const std::string &name, float v) {
	GlobalConfigManager::GetInstance()->SetValueFloat(name, v);
}
#include "PreferenceConfig.h"

TVPGlobalPreferenceForm * TVPGlobalPreferenceForm::create(const tPreferenceScreen *config) {
	Initialize();
	if (!config) config = &RootPreference;
	TVPGlobalPreferenceForm *ret = new TVPGlobalPreferenceForm();
	ret->autorelease();
	ret->initFromFile(FileName_NaviBar, FileName_Body, nullptr);
	PrefListSize = ret->PrefList->getContentSize();
	ret->initPref(config);
	ret->setOnExitCallback(std::bind(&GlobalConfigManager::SaveToFile, GlobalConfigManager::GetInstance()));
	return ret;
}

static void WalkConfig(tPreferenceScreen* pref) {
	for (iTVPPreferenceInfo* info : pref->Preferences) {
		info->InitDefaultConfig();
		tPreferenceScreen* subpref = info->GetSubScreenInfo();
		if (subpref) {
			WalkConfig(subpref);
		}
	}
}

void TVPGlobalPreferenceForm::Initialize()
{
	static bool Inited = false;
	if (!Inited) {
		Inited = true;
		if (!GlobalConfigManager::GetInstance()->IsValueExist("GL_EXT_shader_framebuffer_fetch")) {
			// disable GL_EXT_shader_framebuffer_fetch normally for adreno GPU
			if (strstr((const char*)glGetString(GL_RENDERER), "Adreno")) {
				GlobalConfigManager::GetInstance()->SetValueInt("GL_EXT_shader_framebuffer_fetch", 0);
			}
		}

		initAllConfig();
		WalkConfig(&RootPreference);
		WalkConfig(&SoftRendererOptPreference);
		WalkConfig(&OpenglOptPreference);
	}
}
