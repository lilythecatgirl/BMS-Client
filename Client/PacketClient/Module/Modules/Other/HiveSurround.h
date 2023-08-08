#pragma once
#include "../Module.h"

class HiveSurround : public IModule {
private:
	int placeBlock(vec3_t pos);
public:
	virtual void onTick(C_GameMode* mode);
	void onEnable() override;
	virtual const char* getModuleName();
	HiveSurround();
};