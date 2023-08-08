#pragma once

#include "../../../../Utils/DrawUtils.h"
#include "../Module.h"

class SlowSwing : public IModule {
public:
	SlowSwing();
	~SlowSwing();
	bool useSwong = false;
	int SwingSpeedIndex = 0;
	float swingspeed = 50;
	int resetspeed = 50;
	// Inherited via IModule
	virtual const char* getModuleName() override;
	virtual void onPlayerTick(C_Player* plr) override;
	virtual void onTick(C_GameMode* gm) override;
	virtual void onSendPacket(C_Packet* packet) override;
};
