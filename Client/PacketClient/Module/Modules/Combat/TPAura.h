#pragma once
#include "../../../../Utils/TargetUtil.h"
#include "../../ModuleManager.h"
#include "../Module.h"

class TPAura : public IModule {
private:
	int tick = 0;
	int realdelay = 0;
	int delay = 10;
	int height = 0;
	int multi = 1;
	bool spoof = true;
	bool backsettings = true;
	bool once = false;
	bool behind = false;
	bool voidCheck = false;
	bool back = false;
	int check = 0;
	int bdelay = 0;
	vec3_t savepos;
public:
	int range = 250;

	std::string name = "TPAura";

	virtual const char* getModuleName();
	virtual void onTick(C_GameMode* gm);
	virtual void onDisable();
	virtual void onEnable();
	TPAura();
};
