#pragma once
#include "PreferenceForm.h"

class TVPGlobalPreferenceForm : public TVPPreferenceForm {
public:
	static TVPGlobalPreferenceForm *create(const tPreferenceScreen *config = nullptr);
	static void Initialize();
};