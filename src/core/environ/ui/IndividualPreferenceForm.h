#pragma once
#include "PreferenceForm.h"

class IndividualPreferenceForm : public TVPPreferenceForm {
public:
	static IndividualPreferenceForm *create(const tPreferenceScreen *config = nullptr);
};