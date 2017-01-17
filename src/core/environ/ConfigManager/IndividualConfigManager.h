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

	template<typename T>
	T GetValue(const std::string &name, const T& defVal);

	std::vector<std::string> GetCustomArgumentsForPush();
};

template<> bool IndividualConfigManager::GetValue<bool>(const std::string &name, const bool& defVal);
template<> int IndividualConfigManager::GetValue<int>(const std::string &name, const int& defVal);
template<> float IndividualConfigManager::GetValue<float>(const std::string &name, const float& defVal);
template<> std::string IndividualConfigManager::GetValue<std::string>(const std::string &name, const std::string& defVal);