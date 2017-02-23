#pragma once
#include <unordered_map>
#include <string>
#include <vector>

class iSysConfigManager {
protected:
	std::unordered_map<std::string, std::string> AllConfig;
	std::vector<std::pair<std::string, std::string> > CustomArguments;

	bool ConfigUpdated;

	virtual std::string GetFilePath() = 0;

	void Initialize();

public:
	void SaveToFile();

	bool IsValueExist(const std::string &name);

	template<typename T>
	T GetValue(const std::string &name, const T& defVal);

	void SetValueInt(const std::string &name, int val);
	void SetValueFloat(const std::string &name, float val);
	void SetValue(const std::string &name, const std::string & val);

	std::vector<std::pair<std::string, std::string> > &GetCustomArgumentsForModify() {
		ConfigUpdated = true;
		return CustomArguments;
	}

	const std::vector<std::pair<std::string, std::string> > &GetCustomArguments() const { return CustomArguments; }

	std::vector<std::string> GetCustomArgumentsForPush();
};

template<> bool iSysConfigManager::GetValue<bool>(const std::string &name, const bool& defVal);
template<> int iSysConfigManager::GetValue<int>(const std::string &name, const int& defVal);
template<> float iSysConfigManager::GetValue<float>(const std::string &name, const float& defVal);
template<> std::string iSysConfigManager::GetValue<std::string>(const std::string &name, const std::string& defVal);

class GlobalConfigManager : public iSysConfigManager {

	virtual std::string GetFilePath() override;

	GlobalConfigManager();

public:
	static GlobalConfigManager* GetInstance();
};