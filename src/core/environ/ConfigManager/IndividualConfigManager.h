#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "GlobalConfigManager.h"

class IndividualConfigManager : public iSysConfigManager {
	typedef iSysConfigManager inherit;

	virtual std::string GetFilePath() override;

	void Clear();

	std::string CurrentPath;

public:
	static IndividualConfigManager* GetInstance();
	static bool CheckExistAt(const std::string &folder);
	bool CreatePreferenceAt(const std::string &folder);
	bool UsePreferenceAt(const std::string &folder);

	bool GetValueBool(const std::string &name, bool defVal = false);
	int GetValueInt(const std::string &name, int defVal = 0);
	float GetValueFloat(const std::string &name, float defVal = 0);
	std::string GetValueString(const std::string &name, std::string defVal = "");

	std::vector<std::string> GetCustomArgumentsForPush();
};