#pragma once
#include "cocos2d.h"

void TVPAddAutoSearchArchive(const std::string &path);
class TVPSkinManager {
public:
	static TVPSkinManager* getInstance();

	void InitSkin();
	static bool Check(const std::string &skin_path);
	static void Reset();
	static bool Use(const std::string &skin_path);
	static bool InstallAndUse(const std::string &skin_path);
};
