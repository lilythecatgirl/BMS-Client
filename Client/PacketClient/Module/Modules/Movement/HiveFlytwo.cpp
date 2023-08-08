#include "HiveFlytwo.h"
HiveFlytwo::HiveFlytwo() : IModule(0, Category::MOVEMENT, "How the fuck does this bypass ?!?!?") {
	registerEnumSetting("Mode", &mode, 0);
	mode.addEntry("Jump", 0);
	mode.addEntry("Clip", 1);
	registerFloatSetting("Speed", &speed, speed, .1f, 2.f);
	registerFloatSetting("Height", &height, height, 0.f, 1.f);
	registerFloatSetting("Duration", &duration, duration, 0.5f, 1.f);
	registerFloatSetting("ClipUp", &clipUp, clipUp, 0.f, 5.f);
	registerIntSetting("Timer", &timer, timer, 1, 30);
	registerIntSetting("DashTime", &dashTime, dashTime, 0, 40);
	registerBoolSetting("ClipLimit", &cliplimit, cliplimit);
	registerIntSetting("ClipTimes", &cliptimes, cliptimes, 1, 20);
	registerFloatSetting("ClipValue", &clipvalue, clipvalue, 0.f, 2.f);
}

const char* HiveFlytwo::getRawModuleName() {
	return "HiveFly2";
}

const char* HiveFlytwo::getModuleName() {
	//name = string("HiveFlyTwo ") + string(GRAY) + mode.GetEntry(mode.getSelectedValue()).GetName();
	return name.c_str();
}


void HiveFlytwo::onEnable() {
	dashed = false;
	dspeed = speed;
	nowtimes = 0;
	auautesttick = 0;
	hivetestdelay = 0;
	auto player = g_Data.getLocalPlayer();
	savePos = *player->getPos();
	vec3_t myPos = *player->getPos();
	myPos.y += clipUp;
	player->setPos(myPos);
	auto sped = moduleMgr->getModule<Speed>();
	if (sped->isEnabled())
	{
		speedis = true;
		sped->setEnabled(false);
	}
	else speedis = false;
	g_Data.getClientInstance()->minecraft->setTimerSpeed(timer);
}

void HiveFlytwo::onTick(C_GameMode* gm) {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
}

void HiveFlytwo::onMove(C_MoveInputHandler* input) {
	auto player = g_Data.getLocalPlayer();
	auautesttick++;
	auto sped = moduleMgr->getModule<Speed>();
	if (sped->isEnabled())
	{
		speedis = true;
		sped->setEnabled(false);
	}
	else speedis = false;
	if (player == nullptr) return;
	vec3_t moveVec;
	bool blink = false;
	if (g_Data.canUseMoveKeys()) {
		if (auautesttick > dashTime) {
			auautesttick = 0;
			nowtimes++;
			dashed = true;
			dspeed = dspeed * duration;
			if(mode.getSelectedValue() == 0) // jump
			{
				float calcYaw = (player->yaw + 90) * (PI / 180);
				vec2_t moveVec2d = { input->forwardMovement, -input->sideMovement };
				bool pressed = moveVec2d.magnitude() > 0.01f;
				float c = cos(calcYaw);
				float s = sin(calcYaw);
				moveVec2d = { moveVec2d.x * c - moveVec2d.y * s, moveVec2d.x * s + moveVec2d.y * c };

				if (player->onGround && pressed)
					player->jumpFromGround();
				moveVec.x = moveVec2d.x * dspeed;
				moveVec.y = height;
				player->velocity.y;
				moveVec.z = moveVec2d.y * dspeed;
				if (pressed) player->lerpMotion(moveVec);
			}
			else
			{
				auto hivefly2 = moduleMgr->getModule<HiveFlytwo>();
				if (cliplimit && nowtimes > cliptimes) hivefly2->setEnabled(false);
				vec3_t nowPos = *player->getPos();
				savePos.y -= clipvalue;
				MoveUtil::setSpeed(dspeed);
				nowPos.y = savePos.y;
				player->setPos(nowPos); //normal
				player->velocity.y = 0;
			}
		}
		else {
			dashed = false;
		}
	}
}

void HiveFlytwo::onDisable() {
	uintptr_t ViewBobbing = FindSignature("0F B6 80 DB 01 00 00");
	Utils::patchBytes((unsigned char*)ViewBobbing, (unsigned char*)"\x0F\xB6\x80\xDB\x01\x00\x00", 7);
	g_Data.getClientInstance()->minecraft->setTimerSpeed(20.f);
	auto speedMod = moduleMgr->getModule<Speed>();
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	MoveUtil::fullStop(false);
	if (lock)
	{
		auto lock = moduleMgr->getModule<Freelook>();
		lock->setEnabled(false);
	}
	auto aura = moduleMgr->getModule<Killaura>();
	auto sped = moduleMgr->getModule<Speed>();
	if (aurais == true)
	{
		aura->setEnabled(true);
		aurais = false;
	}
	if (speedis == true)
	{
		sped->setEnabled(true);
		speedis = false;
	}
}
