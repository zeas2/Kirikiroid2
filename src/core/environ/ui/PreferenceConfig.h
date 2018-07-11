extern void TVPInitTextureFormatList();
extern bool TVPIsSupportTextureFormat(GLenum fmt);

static bool PreferenceGetValueBool(const std::string &name, bool defval) {
	return GetConfigManager()->GetValue<bool>(name, defval);
}
static void PreferenceSetValueBool(const std::string &name, bool v) {
	GetConfigManager()->SetValueInt(name, v);
}
static std::string PreferenceGetValueString(const std::string &name, const std::string& defval) {
	return GetConfigManager()->GetValue<std::string>(name, defval);
}
static void PreferenceSetValueString(const std::string &name, const std::string& v) {
	GetConfigManager()->SetValue(name, v);
}
static float PreferenceGetValueFloat(const std::string &name, float defval) {
	return GetConfigManager()->GetValue<float>(name, defval);
}
static void PreferenceSetValueFloat(const std::string &name, float v) {
	GetConfigManager()->SetValueFloat(name, v);
}

namespace {
static tPreferenceScreen RootPreference;
static tPreferenceScreen OpenglOptPreference, SoftRendererOptPreference;
static Size PrefListSize;

class tTVPPreferenceInfoConstant : public iTVPPreferenceInfo {
public:
	tTVPPreferenceInfoConstant(const std::string &cap) : iTVPPreferenceInfo(cap, "") {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		return CreatePreferenceItem<tPreferenceItemConstant>(idx, PrefListSize, locmgr->GetText(Caption));
	}
};

class tTVPPreferenceInfoCheckBox : public tTVPPreferenceInfo<bool> {
public:
	tTVPPreferenceInfoCheckBox(const std::string &cap, const std::string &key, bool defval)
		: tTVPPreferenceInfo<bool>(cap, key, defval) {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		return CreatePreferenceItem<tPreferenceItemCheckBox>(idx, PrefListSize, locmgr->GetText(Caption),
			[this](tPreferenceItemCheckBox* item) {
			item->_getter = std::bind(&PreferenceGetValueBool, Key, DefaultValue);
			item->_setter = std::bind(&PreferenceSetValueBool, Key, std::placeholders::_1);
		});
	}
};

class tTVPPreferenceInfoSelectList : public tTVPPreferenceInfo<std::string>, tPreferenceItemSelectListInfo {
public:
	tTVPPreferenceInfoSelectList(const std::string &cap, const std::string &key, const std::string &defval,
		const std::initializer_list<std::pair<std::string, std::string> > &listinfo)
		: tTVPPreferenceInfo<std::string>(cap, key, defval), ListInfo(listinfo){}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		return CreatePreferenceItem<tPreferenceItemSelectList>(idx, PrefListSize, locmgr->GetText(Caption),
			[this](tPreferenceItemSelectList* item) {
			item->initInfo(this);
			item->_getter = std::bind(&PreferenceGetValueString, Key, DefaultValue);
			item->_setter = [this](std::string v){ onSetValue(v); };
		});
	}
	virtual const std::vector<std::pair<std::string, std::string> >& getListInfo() override {
		return ListInfo;
	}
	virtual void onSetValue(const std::string &v) {
		PreferenceSetValueString(Key, v);
	}
	std::vector<std::pair<std::string, std::string> > ListInfo;
};

class tTVPPreferenceInfoTextureCompressionSelectList : public tTVPPreferenceInfoSelectList {
public:
	tTVPPreferenceInfoTextureCompressionSelectList(const std::string &cap, const std::string &key, const std::string &defval,
		const std::initializer_list<std::tuple<std::string, std::string, unsigned int> > &listinfo)
	: tTVPPreferenceInfoSelectList(cap, key, defval, std::initializer_list<std::pair<std::string, std::string> >())
	, TextFmtList(listinfo)
	{}

	virtual const std::vector<std::pair<std::string, std::string> >& getListInfo() override {
		if (!ListInited) {
			ListInited = true;
			TVPInitTextureFormatList();
			for (const auto &it : TextFmtList) {
				unsigned int fmt = std::get<2>(it);
				if (!fmt || TVPIsSupportTextureFormat(fmt)) {
					ListInfo.emplace_back(std::get<0>(it), std::get<1>(it));
				}
			}
		}
		return ListInfo;
	}

	bool ListInited = false;
	std::vector<std::tuple<std::string, std::string, unsigned int> > TextFmtList;
};

// class tTVPPreferenceInfoSelectRenderer : public tTVPPreferenceInfoSelectList {
// 	typedef tTVPPreferenceInfoSelectList inherit;
// public:
// 	tTVPPreferenceInfoSelectRenderer(const std::string &cap, const std::string &key, const std::string &defval,
// 		const std::initializer_list<std::pair<std::string, std::string> > &listinfo) : inherit(cap, key, defval, listinfo) {}
// 	virtual void onSetValue(const std::string &v) {
// 		inherit::onSetValue(v);
// 		if (v == "opengl") {
// 			TVPOnOpenGLRendererSelected(true);
// 		}
// 	}
// };

class tTVPPreferenceInfoSelectFile : public tTVPPreferenceInfo<std::string> {
public:
	tTVPPreferenceInfoSelectFile(const std::string &cap, const std::string &key, const std::string &defval)
		: tTVPPreferenceInfo<std::string>(cap, key, defval) {}

	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		return CreatePreferenceItem<tPreferenceItemFileSelect>(idx, PrefListSize, locmgr->GetText(Caption),
			[this](tPreferenceItemFileSelect* item) {
			item->_getter = std::bind(&PreferenceGetValueString, Key, DefaultValue);
			item->_setter = std::bind(&PreferenceSetValueString, Key, std::placeholders::_1);
		});
	}
};

class tTVPPreferenceInfoRendererSubPref : public iTVPPreferenceInfo {
public:
	tTVPPreferenceInfoRendererSubPref(const std::string &cap) { Caption = cap; } // Key is useless
	static tPreferenceScreen* GetSubPreferenceInfo() {
		std::string renderer = PreferenceGetValueString("renderer", "software");
		if (renderer == "opengl")
			return &OpenglOptPreference;
		else if (renderer == "software")
			return &SoftRendererOptPreference;
		return nullptr;
	}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		iPreferenceItem *ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, PrefListSize, locmgr->GetText(Caption));
		ret->addClickEventListener([](Ref*) {
			TVPMainScene::GetInstance()->pushUIForm(TVPGlobalPreferenceForm::create(GetSubPreferenceInfo()));
		});
		return ret;
	}
	virtual tPreferenceScreen* GetSubScreenInfo() { return GetSubPreferenceInfo(); }
};

class tTVPPreferenceInfoSubPref : public iTVPPreferenceInfo {
	tPreferenceScreen Preference;
public:
	tTVPPreferenceInfoSubPref(const std::string &title, const std::initializer_list<iTVPPreferenceInfo*> &elem)
		: Preference(title, elem)
	{
		Caption = title;
	}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		iPreferenceItem *ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, PrefListSize, locmgr->GetText(Caption));
		ret->addClickEventListener([this](Ref*){
			TVPMainScene::GetInstance()->pushUIForm(TVPGlobalPreferenceForm::create(&Preference));
		});
		return ret;
	}
	virtual tPreferenceScreen* GetSubScreenInfo() { return &Preference; }
};

class tTVPPreferenceInfoSliderIcon : public tTVPPreferenceInfo<float> {
public:
	tTVPPreferenceInfoSliderIcon(const std::string &cap, const std::string &key, float defval)
		: tTVPPreferenceInfo<float>(cap, key, defval) {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();

		tPreferenceItemCursorSlider * ret = new tPreferenceItemCursorSlider(DefaultValue, TVPMainScene::convertCursorScale);
		ret->autorelease();
		ret->_getter = std::bind(&PreferenceGetValueFloat, Key, DefaultValue);
		ret->_setter = std::bind(&PreferenceSetValueFloat, Key, std::placeholders::_1);
		ret->initFromInfo(idx, PrefListSize, locmgr->GetText(Caption));
		return ret;
	}
};

class tTVPPreferenceInfoSliderText : public tTVPPreferenceInfo<float> {
public:
	tTVPPreferenceInfoSliderText(const std::string &cap, const std::string &key, float defval)
		: tTVPPreferenceInfo<float>(cap, key, defval) {}

	static std::string convertPercentScale(float scale) {
		char buf[16];
		sprintf(buf, "%d%%", (int)(scale * 100));
		return buf;
	}

	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		tPreferenceItemTextSlider * ret = new tPreferenceItemTextSlider(DefaultValue, convertPercentScale);
		ret->autorelease();
		ret->_getter = std::bind(&PreferenceGetValueFloat, Key, DefaultValue);
		ret->_setter = std::bind(&PreferenceSetValueFloat, Key, std::placeholders::_1);
		ret->initFromInfo(idx, PrefListSize, locmgr->GetText(Caption));
		return ret;
	}
};

class tTVPPreferenceInfoFetchSDCardPermission : public iTVPPreferenceInfo {
public:
	tTVPPreferenceInfoFetchSDCardPermission(const std::string &cap) : iTVPPreferenceInfo(cap, "") {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		tPreferenceItemConstant* ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, PrefListSize, locmgr->GetText(Caption));
		ret->setTouchEnabled(true);
		ret->addClickEventListener([](Ref*) {
			TVPFetchSDCardPermission();
		});
		return ret;
	}
};

class tTVPPreferenceInfoResetDefaultSkin : public iTVPPreferenceInfo {
public:
	tTVPPreferenceInfoResetDefaultSkin(const std::string &cap) : iTVPPreferenceInfo(cap, "") {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		tPreferenceItemConstant* ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, PrefListSize, locmgr->GetText(Caption));
		ret->setTouchEnabled(true);
		ret->addClickEventListener([](Ref*) {
			LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
			if (TVPShowSimpleMessageBoxYesNo(locmgr->GetText("preference_ensure_reset_skin"), locmgr->GetText("notice")) == 0) {
				PreferenceSetValueString("skin_path", "");
			}
		});
		return ret;
	}
};

class tTVPPreferenceInfoKeyMap : public iTVPPreferenceInfo {
public:
	tTVPPreferenceInfoKeyMap(const std::string &cap) : iTVPPreferenceInfo(cap, "") {}
	virtual iPreferenceItem *createItem(int idx) override {
		LocaleConfigManager *locmgr = LocaleConfigManager::GetInstance();
		iPreferenceItem *ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, PrefListSize, locmgr->GetText(Caption));
		ret->addClickEventListener([this](Ref*) {
			TVPMainScene::GetInstance()->pushUIForm(KeyMapPreferenceForm::create(GetConfigManager()));
		});
		return ret;
	}
};

static void initAllConfig() {
	if (!RootPreference.Preferences.empty()) return;
	RootPreference.Title = "preference_title";
	RootPreference.Preferences = {
		new tTVPPreferenceInfoCheckBox("preference_output_log", "outputlog", true),
		new tTVPPreferenceInfoCheckBox("preference_show_fps", "showfps", false),
		new tTVPPreferenceInfoSelectList("preference_fps_limit", "fps_limit", "60",{
			{ "60", "60" },
			{ "45", "45" },
			{ "30", "30" },
			{ "15", "15" }
		}),
		new tTVPPreferenceInfoSelectList("preference_select_renderer", "renderer", "software", {
			{ "preference_opengl", "opengl" },
			{ "preference_software", "software" }
		}),
		new tTVPPreferenceInfoRendererSubPref("preference_renderer_opt"),
		new tTVPPreferenceInfoSelectFile("preference_default_font", "default_font", ""),
		new tTVPPreferenceInfoCheckBox("preference_force_def_font", "force_default_font", false),
#ifdef CC_TARGET_OS_IPHONE
		new tTVPPreferenceInfoSelectList("preference_mem_limit", "memusage", "high", {
#else
		new tTVPPreferenceInfoSelectList("preference_mem_limit", "memusage", "unlimited", {
#endif
			{ "preference_mem_unlimited", "unlimited" },
			{ "preference_mem_high", "high" },
			{ "preference_mem_medium", "medium" },
			{ "preference_mem_low", "low" }
			}),
			new tTVPPreferenceInfoCheckBox("preference_keep_screen_alive", "keep_screen_alive", true),
			new tTVPPreferenceInfoSliderIcon("preference_virtual_cursor_scale", "vcursor_scale", 0.5f),
			new tTVPPreferenceInfoSliderText("preference_menu_handler_opacity", "menu_handler_opa", 0.15f),
			new tTVPPreferenceInfoKeyMap("preference_keymap"),
#ifdef GLOBAL_PREFERENCE
			new tTVPPreferenceInfoCheckBox("preference_remember_last_path", "remember_last_path", true),
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
			new tTVPPreferenceInfoCheckBox("preference_hide_android_sys_btn", "hide_android_sys_btn", false),
			// new tTVPPreferenceInfoFetchSDCardPermission("preference_android_fetch_sdcard_permission"),
#endif
			new tTVPPreferenceInfoResetDefaultSkin("preference_reset_def_skin"),

#endif
#if 0
			new tTVPPreferenceInfo(tTVPPreferenceInfo::eTypeSubPref, "preference_custom_option"),
#endif
//			nullptr
	};

	SoftRendererOptPreference.Title = "preference_soft_renderer_opt";
	SoftRendererOptPreference.Preferences = {
		new tTVPPreferenceInfoSelectList("preference_multi_draw_thread", "software_draw_thread", "0", {
			{ "preference_draw_thread_auto", "0" },
			{ "preference_draw_thread_1", "1" },
			{ "preference_draw_thread_2", "2" },
			{ "preference_draw_thread_3", "3" },
			{ "preference_draw_thread_4", "4" },
			{ "preference_draw_thread_5", "5" },
			{ "preference_draw_thread_6", "6" },
			{ "preference_draw_thread_7", "7" },
			{ "preference_draw_thread_8", "8" }
		}),
		new tTVPPreferenceInfoSelectList("preference_software_compress_tex", "software_compress_tex", "none", {
			{ "preference_soft_compress_tex_none", "none" },
			{ "preference_soft_compress_tex_halfline", "halfline" },
 			{ "lz4", "lz4" },
 			{ "lz4+TLG5", "lz4+tlg5" }
		}),
	};

	OpenglOptPreference.Title = "preference_opengl_renderer_opt";
	OpenglOptPreference.Preferences = {
		new tTVPPreferenceInfoSubPref("preference_opengl_extension_opt", {
			new tTVPPreferenceInfoConstant("preference_opengl_extension_desc"),
#ifdef CC_TARGET_OS_IPHONE
			new tTVPPreferenceInfoCheckBox("GL_EXT_shader_framebuffer_fetch", "GL_EXT_shader_framebuffer_fetch", true),
#else
			new tTVPPreferenceInfoCheckBox("GL_EXT_shader_framebuffer_fetch", "GL_EXT_shader_framebuffer_fetch", false),
#endif
			new tTVPPreferenceInfoCheckBox("GL_ARM_shader_framebuffer_fetch", "GL_ARM_shader_framebuffer_fetch", true),
			new tTVPPreferenceInfoCheckBox("GL_NV_shader_framebuffer_fetch", "GL_NV_shader_framebuffer_fetch", true),
			new tTVPPreferenceInfoCheckBox("GL_EXT_copy_image", "GL_EXT_copy_image", false),
			new tTVPPreferenceInfoCheckBox("GL_OES_copy_image", "GL_OES_copy_image", false),
			new tTVPPreferenceInfoCheckBox("GL_ARB_copy_image", "GL_ARB_copy_image", false),
			new tTVPPreferenceInfoCheckBox("GL_NV_copy_image", "GL_NV_copy_image", false),
			new tTVPPreferenceInfoCheckBox("GL_EXT_clear_texture", "GL_EXT_clear_texture", true),
			new tTVPPreferenceInfoCheckBox("GL_ARB_clear_texture", "GL_ARB_clear_texture", true),
			new tTVPPreferenceInfoCheckBox("GL_QCOM_alpha_test", "GL_QCOM_alpha_test", true),
		}),
		new tTVPPreferenceInfoCheckBox("preference_ogl_accurate_render", "ogl_accurate_render", false),
//		new tTVPPreferenceInfoCheckBox("preference_opengl_dup_target", "ogl_dup_target", true),
		new tTVPPreferenceInfoSelectList("preference_ogl_max_texsize", "ogl_max_texsize", "0", {
			{ "preference_ogl_texsize_auto", "0" },
			{ "preference_ogl_texsize_1024", "1024" },
			{ "preference_ogl_texsize_2048", "2048" },
			{ "preference_ogl_texsize_4096", "4096" },
			{ "preference_ogl_texsize_8192", "8192" },
			{ "preference_ogl_texsize_16384", "16384" }
		}),
		new tTVPPreferenceInfoTextureCompressionSelectList("preference_ogl_compress_tex", "ogl_compress_tex", "none", {
			std::make_tuple("preference_ogl_compress_tex_none", "none", 0),
			std::make_tuple("preference_ogl_compress_tex_half", "half", 0),
			std::make_tuple("ETC2", "etc2", 0x9278), // GL_COMPRESSED_RGBA8_ETC2_EAC
			std::make_tuple("PVRTC", "pvrtc", 0x8C02), // GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
		}),
// 		new tTVPPreferenceInfoSelectList("preference_ogl_render_tex_quality", "ogl_render_tex_quality", "1", {
// 			{ "preference_ogl_render_tex_quality_100", "1" },
// 			{ "preference_ogl_render_tex_quality_75", "0.75" },
// 			{ "preference_ogl_render_tex_quality_50", "0.5" }
// 		}),
	};
}
};
