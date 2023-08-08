#pragma once
#include "../../../../Utils/Utils.h"
#include "../../ModuleManager.h"
#include "../Module.h"

class Spammer : public IModule {
private:
	std::string message = u8"@here NUKING ANTICHEAT @here";
	int Odelay = 0;
	int delay = 3;
	int minLength = 100;
	int maxLength = 120;
public:
	inline std::string& getMessage() { return message; };
	SettingEnum mode = this;
	virtual const char* getModuleName() override;
	virtual void onTick(C_GameMode* gm) override;
	Spammer();
};
