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

	bool GetValueBool(const std::string &name, bool defVal = false);
	int GetValueInt(const std::string &name, int defVal = 0);
	float GetValueFloat(const std::string &name, float defVal = 0);
	std::string GetValueString(const std::string &name, std::string defVal = "");

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

class GlobalConfigManager : public iSysConfigManager {

	virtual std::string GetFilePath() override;

	GlobalConfigManager();

public:
	static GlobalConfigManager* GetInstance();
};