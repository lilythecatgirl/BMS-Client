#include "Scaffold.h"
#include "../pch.h"

uintptr_t HiveRotations1 = Utils::getBase() + 0x8F3895;
uintptr_t HiveRotations2 = Utils::getBase() + 0x8F87C7;
uintptr_t HiveRotations3 = Utils::getBase() + 0x8F53B1;
uintptr_t HiveRotations4 = Utils::getBase() + 0x98AF833C1;
uintptr_t HiveRotations5 = Utils::getBase() + 0x173ACFA01D; //From Skidders
int testextend = 0;
int radiumdelay = 0;
//a
using namespace std;
Scaffold::Scaffold() : IModule(0, Category::PLAYER, "Places blocks under you") {
	registerEnumSetting("Rotations", &rotations, 0);
	rotations.addEntry("Normal", 0);
	rotations.addEntry("Hive", 1);
	rotations.addEntry("Down", 2);
	rotations.addEntry("Back", 3);
	rotations.addEntry("Silent", 4);
	rotations.addEntry("None", 5);
	rotations.addEntry("Smooth", 6);
	rotations.addEntry("Flareon", 7);
	rotations.addEntry("Smart", 8);
	//rotations.addEntry("Test", 9);
	registerEnumSetting("Tower", &tower, 0);
	tower.addEntry("Vanilla", 0);
	tower.addEntry("Timer", 1);
	tower.addEntry("Clip", 2);
	tower.addEntry("Timer2", 3);
	tower.addEntry("HighJump", 4);
	tower.addEntry("Test", 5);
	tower.addEntry("HiveFast", 6);
	tower.addEntry("None", 7);
	registerEnumSetting("BlockCount", &blocleft, 0);/////
	blocleft.addEntry("Off", 0);
	blocleft.addEntry("Zephyr", 1);
	blocleft.addEntry("Packet", 2);
	blocleft.addEntry("Packet+", 3);
	blocleft.addEntry("Old", 4);
	blocleft.addEntry("Actinium", 5);
	registerEnumSetting("Select", &holdType, 0);
	holdType.addEntry("Switch", 0);
	holdType.addEntry("Spoof", 1);
	holdType.addEntry("Fake", 2);
	registerEnumSetting("Mode", &extendType, 0);
	extendType.addEntry("Celsius", 0);
	extendType.addEntry("Radium", 1);
	extendType.addEntry("Packet", 2);
	extendType.addEntry("Actinium", 3);
	registerEnumSetting("DiagBypass", &diagType, 0);
	diagType.addEntry("None", 0);
	diagType.addEntry("UnderBlock", 1);
	diagType.addEntry("DelayPlace", 2);
	diagType.addEntry("IDK", 3);
	diagType.addEntry("NoSpeed", 4);
	diagType.addEntry("UpBlock", 5);
	registerBoolSetting("LockY", &lockY, lockY);
	registerBoolSetting("TowerNoMove", &towerOnlyNoMove, towerOnlyNoMove);
	registerBoolSetting("NoSpeed", &preventkicks, preventkicks);
	registerBoolSetting("Downwards", &downmode, downmode);
	registerBoolSetting("AlwaysUnder", &alwaysUnderPlace, alwaysUnderPlace);
	registerBoolSetting("Spoof", &spoof, spoof);
	registerIntSetting("Extend", &extend, extend, 0, 15);
	registerIntSetting("ExtendDelay", &delay, delay, 0, 5);
	registerIntSetting("UnderDelay", &underplacedelay, underplacedelay, 0, 5);
	registerIntSetting("Timer", &timer, timer, 15, 60);
	registerIntSetting("RotSpeed", &rotspeed, rotspeed, 1, 50);
	registerIntSetting("PlaceDelay", &tellydalay, tellydalay, 0, 20);
	//registerEnumSetting("Down", &downwards, 0);
	//downwards.addEntry("Vanilla", 0);
	//downwards.addEntry("None", 1);
	//registerBoolSetting("Sprint", &sprint, sprint);
	//registerBoolSetting("BlockCount", &blockCount, blockCount);
	//registerBoolSetting("Swing", &swing, swing);
	//registerBoolSetting("DigBypass", &digbypass, digbypass);
	//registerBoolSetting("Visual", &this->showExposed, this->showExposed);
	//registerIntSetting("R", &this->expR, this->expR, 0, 255);
	//registerIntSetting("G", &this->expG, this->expG, 0, 255);
	//registerIntSetting("B", &this->expB, this->expB, 0, 255);
	//registerFloatSetting("T", &this->expT, this->expT, 0.f, 1.f);
	//registerIntSetting("TowerTimer", &towerTimer, towerTimer, 20, 60);
	//registerFloatSetting("TowerMultiply", &towerMultiply, towerMultiply, 0.1f, 2.f);
	//registerBoolSetting("ZipLine", &zipline, zipline); patched
}

const char* Scaffold::getRawModuleName() {
	return "Scaffold";
}

const char* Scaffold::getModuleName() {
	if (rotations.getSelectedValue() <= 9) name = string("Scaffold ") + string(GRAY) + rotations.GetEntry(rotations.getSelectedValue()).GetName();
	else name = string("Scaffold");
	return name.c_str();
}

static std::vector<C_Entity*> guyzz;
bool weLookForAGuy(C_Entity* curEnt, bool isRegularEntity) {
	if (curEnt == nullptr) return false;
	if (curEnt == g_Data.getLocalPlayer()) return false;  // Skip Local player
	if (!curEnt->isAlive()) return false;
	if (!g_Data.getLocalPlayer()->isAlive()) return false;
	if (curEnt->getEntityTypeId() == 71) return false;                    // endcrystal
	if (curEnt->getEntityTypeId() == 66) return false;                    // falling block
	if (curEnt->getEntityTypeId() == 64) return false;                    // item
	if (curEnt->getEntityTypeId() == 69) return false;                    // xp orb
	if (curEnt->width <= 0.01f || curEnt->height <= 0.01f) return false;  // Don't hit this pesky antibot on 2b2e.org
	if (TargetUtil::isValidTarget(curEnt)) return false;

	//float dist = (curEnt->getHumanPos()).dist(g_Data.getLocalPlayer()->getHumanPos());
	/*if (dist <= moduleMgr->getModule<Scaffold>()->Range) {
		guyzz.push_back(curEnt);
		return true;
	}*/
}

bool Scaffold::isBlockReplacable(vec3_t blockPos) {
	return g_Data.getLocalPlayer()->region->getBlock(vec3_ti(blockPos.floor()))->blockLegacy->material->isReplaceable;
}

void Scaffold::onEnable() {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	blockBelowY = g_Data.getLocalPlayer()->eyePos0;  // Block below the player
	blockBelowY.y -= g_Data.getLocalPlayer()->height;
	blockBelowY.y -= 0.5f;
	blockBelowY = blockBelowY.floor();
	if (zipline) blockBelowY.y += 3.f;
	blockBelowY = blockBelowY.floor();
	if (diagType.getSelectedValue() == 2) tellydalay = 1;
	fakespoofticks = 0;
	testextend = 0;
	canspoof = false;
	rundown = 0;
	radiumdelay = 0;
	countopa = 0;
	county = -128;
	auto speedMod = moduleMgr->getModule<Speed>();
	if (speedMod->isEnabled()) {
		speedwasenabled = true;
	}
	else {
		speedwasenabled = false;
	}
	auto av = moduleMgr->getModule<AntiVoid>();
	if (av->lockis == true) lockY = true;
	animFlareon = player->yawUnused1;
	animFlareonPitch = player->pitch;
	C_PlayerInventoryProxy* supplies = player->getSupplies();
	slot = supplies->selectedHotbarSlot;
	towerTick = 0;
	if (lockY) firstlock = true;
	else firstlock = false;

	if (holdType.getSelectedValue() == 2) {
		selectBlock();
	}
}

void Scaffold::onTick(C_GameMode* gm) {
	C_GameSettingsInput* input = g_Data.getClientInstance()->getGameSettingsInput();
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	auto sprintMod = moduleMgr->getModule<Sprint>();
	auto speed = moduleMgr->getModule<Speed>();
	C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
	jumping = GameData::isKeyDown(*input->spaceBarKey);
	sneaking = GameData::isKeyDown(*input->sneakKey);
	if (holdType.getSelectedValue() == 2) {
		fakespoofticks++;
		if (1 < fakespoofticks) canspoof = true;
	}
        Odelay++;
	if (!jumping || !foundBlock)
		g_Data.getClientInstance()->minecraft->setTimerSpeed(timer);

	if (player->onGround) offGroundTicks = 0;
	else offGroundTicks++;

	if (preventkicks && speed->isEnabled()) {
		speed->setEnabled(false);
		auto notification = g_Data.addNotification("Scaffold:", "Disabled Speed"); notification->duration = 5;
	}

	// Select
	static bool restored = false;
	if (!selectBlock()) {
		if (!restored) restored = true;
		else {
			auto warning = g_Data.addNotification("Scaffold", "No Blocks Found"); warning->duration = 3;
			setEnabled(false);
		}
		return;
	}
	restored = false;

	//CustomTelly
	if (telly2) telly2 = false;
	else
	{
		if (tellytick >= tellydalay)
		{
			telly2 = true;
			tellytick = 0;
		}
		else tellytick++;
	}

	//GroundTime
	float velocityxz = g_Data.getLocalPlayer()->velocity.magnitudexz();
	if (diagType.getSelectedValue() == 2 && !jumping && velocityxz >= 0.01 && player->velocity.y <= 0.01) groundtime2++;
	else groundtime2 = 0;

	//Extend Bypass
	if (testextend < extend && !jumping && velocityxz >= 0.01) testextend++;
	if (velocityxz < 0.01) testextend = 0;

	// Build Block
	vec3_t vel = g_Data.getLocalPlayer()->velocity; vel = vel.normalize();
	float cal = (player->yaw + 90) * (PI / 180);
	vec3_t blockBelow;
	if (lockY) {
		blockBelow = g_Data.getLocalPlayer()->eyePos0;  // Block below the player
		if (blockBelowY.y > blockBelow.y) {
			blockBelow = player->eyePos0;  // Block below the player
			blockBelow.y -= player->height;
			blockBelow.y -= 0.5f;
			if (zipline) blockBelowY.y += 2.8f;
		}
		else {
			blockBelow.y = blockBelowY.y;
		}
	}
	else {
		blockBelow = player->eyePos0;  // Block below the player
		blockBelow.y -= player->height;
		blockBelow.y -= 0.5f;
		if (zipline) blockBelowY.y += 2.8f;
	}
	vec3_t blockBelowPredict = player->eyePos0;  // Block below the player
	blockBelowPredict.y -= player->height;
	blockBelowPredict.y -= 0.5f;

	//Under Up Place 
	if (diagType.getSelectedValue() == 1) {
		if (rundown >= underplacedelay)
		{
			blockBelow.y -= 1;
			if (alwaysUnderPlace || ((player->velocity.x > 0.06 || player->velocity.x < -0.06) && (player->velocity.z > 0.06 || player->velocity.z < -0.06)))
				buildBlock(blockBelow);
			blockBelow.y += 1;
			rundown = 0;
		}
		else rundown++;
	}
	if (diagType.getSelectedValue() == 5) {
		if (rundown >= underplacedelay)
		{
			blockBelow.y += 1;
			buildBlock(blockBelow);
			blockBelow.y -= 1;
			rundown = 0;
		}
		else rundown++;
	}

	//UpPlace
	if (diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4)
	{
		auto sped = moduleMgr->getModule<Speed>();
		if ((player->velocity.x > 0.06 || player->velocity.x < -0.06) && (player->velocity.z > 0.06 || player->velocity.z < -0.06) && diagType.getSelectedValue() == 3)
		{
			lockY = false;
			blockBelowY = g_Data.getLocalPlayer()->eyePos0;  // Block below the player
			blockBelowY.y -= g_Data.getLocalPlayer()->height;
			blockBelowY.y -= 0.5f;
			blockBelowY = blockBelowY.floor();
		}
		else if (diagType.getSelectedValue() == 3) lockY = true;
		if ((player->velocity.x > 0.06 || player->velocity.x < -0.06) && (player->velocity.z > 0.06 || player->velocity.z < -0.06) && diagType.getSelectedValue() == 4) sped->enabled = false;
		else if (diagType.getSelectedValue() == 4 && speedwasenabled)
		{
			sped->enabled = true;
		}
	}

	//Build Down Block
	bool downwardPlaced = false;
	if (sneaking && downmode) {
		downwardPlaced = true;
		vec3_t blockBelow15 = player->eyePos0;  // Block 1 block below the player
		blockBelow15.y -= player->height;
		blockBelow15.y -= 1.5f;
		blockBelowY = blockBelow15;
		vec3_t blockBelow2 = g_Data.getLocalPlayer()->eyePos0;  // Block 2 blocks below the player
		blockBelow2.y -= g_Data.getLocalPlayer()->height;
		blockBelow2.y -= 2.0f;
		g_Data.getClientInstance()->getMoveTurnInput()->isSneakDown = false;
		blockBelow.x = blockBelow.x += cos(cal) * 0.5f; blockBelow.z = blockBelow.z += sin(cal) * 0.5f;
		if (!buildBlock(blockBelow15) && !buildBlock(blockBelow2)) {
			if (velocityxz > 0.f) {
				blockBelow15.z -= vel.z * 0.4f;
				blockBelow15.z -= vel.z * 0.4f;
				if (!buildBlock(blockBelow15) && !buildBlock(blockBelow2)) {
					blockBelow15.x -= vel.x * 0.4f;
					blockBelow2.x -= vel.x * 0.4f;
					if (!buildBlock(blockBelow15) && !buildBlock(blockBelow2) && g_Data.getLocalPlayer()->isSprinting()) {
						Odelay++;
						if (Odelay > delay)
						{
							blockBelow15.z += vel.z;
							blockBelow15.x += vel.x;
							blockBelow2.z += vel.z;
							blockBelow2.x += vel.x;
							buildBlock(blockBelow15);
							buildBlock(blockBelow2);
						}
					}
				}
			}
		}
	}

	//Celsius
	if (extendType.getSelectedValue() == 0)
	{
		if (((diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && !jumping && velocityxz >= 0.01) || groundtime >= 10 || groundtime2 >= 10 || (diagType.getSelectedValue() == 2 && !jumping && velocityxz >= 0.01 && telly2))
		{
			vec3_t defaultblockBelow = blockBelow;
			for (int i = 0; i < testextend; i++) {
				if (Odelay > delay)
				{
					if (!jumping && velocityxz >= 0.01) { blockBelow.x += vel.x * i; blockBelow.z += vel.z * i; }
					if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
					else if (!buildBlock(blockBelow)) {
						if (velocityxz > 0.f) {  // Are we actually walking?
							blockBelow.x -= vel.x;
							blockBelow.z -= vel.z;
							if (!buildBlock(blockBelow) && g_Data.getLocalPlayer()->isSprinting()) {
								blockBelow.x += vel.x;
								blockBelow.z += vel.z;
								buildBlock(blockBelow);
							}
						}
					}
				}
			}
			blockBelow = defaultblockBelow;
			if (!jumping && velocityxz >= 0.01) { blockBelow.x += vel.x * testextend; blockBelow.z += vel.z * testextend; }
			if (delay > 0)
			{
				if (Odelay < 6 - delay) { if (isBlockReplacable(blockBelow)) predictBlock(blockBelow); }
				else Odelay = 0;
			}
			else if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
		}
		else
		{
			if (groundtime2 >= 5 || groundtime >= 5 || velocityxz <= 0.01 || (diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && player->onGround || telly2)
			{
				if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
				else buildBlock(blockBelow);
			}
		}
		oldpos = blockBelow.floor();
	}

	//Radium
	if (extendType.getSelectedValue() == 1)
	{
		if (((diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && !jumping && velocityxz >= 0.01) || groundtime >= 10 || groundtime2 >= 10 || (diagType.getSelectedValue() == 2 && !jumping && velocityxz >= 0.01 && telly2))
		{
			for (int i = 0; i <= testextend; i++)
			{
				if (delay > 0)
				{
					radiumdelay++;
					if (radiumdelay < 6 - delay)
					{
						int tempx = vel.x * i;
						int tempz = vel.z * i;
						vec3_t temp = blockBelow;
						temp.x += tempx;
						temp.z += tempz;
						if (!placed.empty())
						{
							bool skip = false;
							for (auto& i : placed)
							{
								if (i == temp)
								{
									skip = true;
								}
							}
							if (skip)
							{
								clientMessageF("Skipped due same block");
								continue;
							}
						}
						if (isBlockReplacable(temp)) predictBlock(temp);
						else if (buildBlock(temp))
						{
							placed.push_back(temp);
							clientMessageF("Breaked due placed");
							break;
						}
					}
					else radiumdelay = 0;
				}
				else
				{
					int tempx = vel.x * i;
					int tempz = vel.z * i;
					vec3_t temp = blockBelow;
					temp.x += tempx;
					temp.z += tempz;
					if (!placed.empty())
					{
						bool skip = false;
						for (auto& i : placed)
						{
							if (i == temp)
							{
								skip = true;
							}
						}
						if (skip)
						{
							clientMessageF("Skipped due same block");
							continue;
						}
					}
					if (isBlockReplacable(temp)) predictBlock(temp);
					else if (buildBlock(temp))
					{
						placed.push_back(temp);
						clientMessageF("Breaked due placed");
						break;
					}
				}
			}
			placed.clear();
			oldpos = blockBelow.floor();
		}
		else
		{
			if (groundtime2 >= 5 || groundtime >= 5 || velocityxz <= 0.01 || (diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && player->onGround || telly2)
			{
				if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
				else buildBlock(blockBelow);
			}
		}

		/*
		if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
		else if (!buildBlock(blockBelow)) {
			if (velocityxz > 0.f) {  // Are we actually walking?
				blockBelow.x -= vel.x;
				blockBelow.z -= vel.z;
				if (!buildBlock(blockBelow) && g_Data.getLocalPlayer()->isSprinting()) {
					blockBelow.x += vel.x;
					blockBelow.z += vel.z;
					buildBlock(blockBelow);
				}
			}
		}
		*/

		oldpos = blockBelow.floor();
	}

	//Packet
	if (extendType.getSelectedValue() == 2)
	{
		currExtend = extend;
		if (((diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && !jumping && velocityxz >= 0.01) || groundtime >= 10 || groundtime2 >= 10 || (diagType.getSelectedValue() == 2 && !jumping && velocityxz >= 0.01 && telly2))
		{
			if (!jumping && velocityxz >= 0.01) { blockBelow.x += vel.x * currExtend; blockBelow.z += vel.z * currExtend; }
			if (delay > 0)
			{
				if (Odelay < 6 - delay) { if (isBlockReplacable(blockBelow)) predictBlock(blockBelow); }
				else Odelay = 0;
			}
			else if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
		}
		else
		{
			if (groundtime2 >= 5 || groundtime >= 5 || velocityxz <= 0.01 || (diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && player->onGround || telly2)
			{
				if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
				else buildBlock(blockBelow);
			}
		}
		oldpos = blockBelow.floor();
	}

	//Actinium
	if (extendType.getSelectedValue() == 3)
	{
		vec3_t defaultblockBelow = blockBelow;
		for (int i = 0; i < testextend; i++) {
			if (delay > 0)
			{
				if (Odelay < 6 - delay)
				{
					if (!jumping && velocityxz >= 0.01) { blockBelow.x += vel.x * i; blockBelow.z += vel.z * i; }
					if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
				}
				else Odelay = 0;
			}
			else
			{
				if (!jumping && velocityxz >= 0.01) { blockBelow.x += vel.x * i; blockBelow.z += vel.z * i; }
				if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
			}
			blockBelow = defaultblockBelow;
		}
		if (groundtime2 >= 5 || groundtime >= 5 || velocityxz <= 0.01 || (diagType.getSelectedValue() == 0 || diagType.getSelectedValue() == 1 || diagType.getSelectedValue() == 3 || diagType.getSelectedValue() == 4 || diagType.getSelectedValue() == 5) && player->onGround || telly2)
		{
			if (isBlockReplacable(blockBelow)) predictBlock(blockBelow);
			else buildBlock(blockBelow);
		}
		oldpos = blockBelow.floor();
	}


	if (!sprint) { gm->player->setSprinting(false); sprintMod->useSprint = false; }

	//Insane Hive Rotation
	if (rotations.getSelectedValue() == 9) {
		//player->pointingStruct->rayHitType = 0;
		//Utils::nopBytes((unsigned char*)HiveRotations1, 3);
		//Utils::patchBytes((unsigned char*)HiveRotations2, (unsigned char*)"\xC7\x40\x18\x00\x00\x00\x00", 7);
		//Utils::patchBytes((unsigned char*)HiveRotations3, (unsigned char*)"\xA4\x60\x38\x02\x13\x86\x01\x13\x8", 11);
	}

	//digbypass (only asia)
	/*if (digbypass)
	{
		auto sped = moduleMgr->getModule<Speed>();
		if (sped->isEnabled())
		{
			if ((player->velocity.x > 0.06 || player->velocity.x < -0.06) && (player->velocity.z > 0.06 || player->velocity.z < -0.06))
			{
				lockY = false;
				blockBelowY = g_Data.getLocalPlayer()->eyePos0;  // Block below the player
				blockBelowY.y -= g_Data.getLocalPlayer()->height;
				blockBelowY.y -= 0.5f;
				blockBelowY = blockBelowY.floor();
			}
			else lockY = true;
		}
		else lockY = true;
	}*/
	if (holdType.getSelectedValue() > 0) {
		if (holdType.getSelectedValue() == 1) {
			supplies->selectedHotbarSlot = slot;
		}
		else if (canspoof)
		{
			supplies->selectedHotbarSlot = slot;
		}
	}

	// Hive Bypass
	if (rotations.getSelectedValue() == 1 || rotations.getSelectedValue() == 7 || rotations.getSelectedValue() == 8) {
		//player->pointingStruct->rayHitType = 0;
		//Utils::nopBytes((unsigned char*)HiveRotations1, 3);
		//Utils::patchBytes((unsigned char*)HiveRotations2, (unsigned char*)"\xC7\x40\x18\x00\x00\x00\x00", 7);
		//Utils::patchBytes((unsigned char*)HiveRotations3, (unsigned char*)"\xA4\x60\x38\x02\x13\x86\x01\x13\x8", 11);
	}
	vec3_t eyePos = g_Data.getLocalPlayer()->eyePos0;
	backPos = vec3_t(player->getPos()->x, player->getPos()->y - 1, player->getPos()->z);
	flareonpos = vec3_t(player->getPos()->x, player->getPos()->y - 1, player->getPos()->z);
	flareonpos2 = vec3_t(eyePos.x, eyePos.y, eyePos.z);
}

bool Scaffold::isBlockAGoodCity(vec3_ti* blk, vec3_ti* personPos) {
	C_Block* block = g_Data.getLocalPlayer()->region->getBlock(*blk);
	C_BlockLegacy* blockLegacy = (block->blockLegacy);

	if (blockLegacy->blockId == 7) {  // block is bedrock
		return false;
	}

	if (oneDot12) {
		if (!g_Data.getLocalPlayer()->region->getBlock(blk->toVector3().add(0, 1, 0))->toLegacy()->material->isReplaceable) {  // top half is blocked

			vec3_ti delta = blk->sub(*personPos);
			vec3_ti checkMe = blk->add(delta);

			if (!g_Data.getLocalPlayer()->region->getBlock(checkMe.toVector3())->toLegacy()->material->isReplaceable)  // next block after is also blocked
				return false;
		}
	}

	return true;
}

void Scaffold::onMove(C_MoveInputHandler* input) {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	if (towerOnlyNoMove && g_Data.getLocalPlayer()->velocity.magnitudexz() >= 0.05) return;

	// Math
	vec3_t pos = *player->getPos();
	float velocityxz = g_Data.getLocalPlayer()->velocity.magnitudexz();
	vec2_t movement = { input->forwardMovement, -input->sideMovement };
	float mathGround = round(pos.y / 0.015625) * 0.015625;
	float calcYaw = (player->yaw + 90) * (PI / 180);
	float c = cos(calcYaw);
	float s = sin(calcYaw);
	vec3_t moveVec;
	moveVec.x = g_Data.getLocalPlayer()->velocity.x;
	moveVec.z = g_Data.getLocalPlayer()->velocity.z;

	if (jumping && foundBlock) {
		uwu = true;
		g_Data.getClientInstance()->minecraft->setTimerSpeed(towerTimer);
		switch (tower.getSelectedValue()) {
		case 0: // Vanilla
			moveVec.y = 0.4f;
			g_Data.getLocalPlayer()->lerpMotion(moveVec);
			break;
		case 1: // Timer
			g_Data.getClientInstance()->minecraft->setTimerSpeed(90.f); // LOL
			/*old*/
			/*movement = {movement.x * c - movement.y * s, movement.x * s + movement.y * c};
			player->setPos(vec3_t(pos.x, mathGround, pos.z));
			moveVec.y = player->velocity.y;
			player->jumpFromGround();*/
			break;
		case 2: // Clip
			if (player->onGround) {
				player->setPos(vec3_t(pos.x, pos.y + 0.8f, pos.z));
			}
			break;
		case 3: // Timer2
			g_Data.getClientInstance()->minecraft->setTimerSpeed(30.f); // LOL
			break;
		case 4: // Fast
			if (player->onGround) {
				player->jumpFromGround();
				moveVec.y = 0.7f;
				g_Data.getLocalPlayer()->lerpMotion(moveVec);
			}
			break;
		case 5: //Test
			if (player) {
				vec3_t myPos = *player->getPos();
				myPos.y += 0.37;
				player->setPos(myPos);
				moveVec.y = 0.01;
				g_Data.getLocalPlayer()->lerpMotion(moveVec);
			}
			break;
		case 6: //FlareonFast
			g_Data.getClientInstance()->minecraft->setTimerSpeed(40.f);
			if (offGroundTicks < 3) {
				moveVec.y = 0.37;
				g_Data.getLocalPlayer()->lerpMotion(moveVec);
			}
			else if (offGroundTicks == 3) {
				player->velocity.y = -1;
			}
			break;
		}
		if (tower.getSelectedValue() != 5) {
			vec3_t blockBelow = player->eyePos0;  // Block 1 block below the player
			blockBelow.y -= player->height;
			blockBelow.y -= 0.5f;
			blockBelowY = blockBelow;
		}
	}
	else if (uwu) {
		uwu = false;
		player->velocity.y = -0.1;
	}
}

void Scaffold::onPlayerTick(C_Player* plr) {
	C_GameSettingsInput* input = g_Data.getClientInstance()->getGameSettingsInput();
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;

	if (g_Data.canUseMoveKeys() && moduleMgr->getModule<Killaura>()->targetListEmpty) {
		vec3_t pPos = vec3_t(player->getPos()->x, player->getPos()->y - 1, player->getPos()->z);
		vec2_t blockPosition = player->getPos()->CalcAngle(vec3_t(calcPos));
		vec2_t back = player->getPos()->CalcAngle(backPos).normAngles();
		vec2_t flareon = player->getPos()->CalcAngle(flareonpos).normAngles();
		vec2_t flareon2 = player->getPos()->CalcAngle(flareonpos2).normAngles();

		float speed = g_Data.getLocalPlayer()->getBlocksPerSecond();

		// Normal
		if (animYaw > blockPosition.y) animYaw -= ((animYaw - blockPosition.y) / rotspeed);
		else if (animYaw < blockPosition.y) animYaw += ((blockPosition.y - animYaw) / rotspeed);
		animPitch = blockPosition.x;

		//Flareon
		if (rotations.getSelectedValue() == 6 || rotations.getSelectedValue() == 7) {
			if (animFlareon > flareon2.y) animFlareon -= ((animFlareon - flareon2.y) / 1);
			else if (animFlareon < flareon2.y) animFlareon += ((flareon2.y - animFlareon) / 1);

			if (animFlareonPitch > flareon2.x) animFlareonPitch -= ((animFlareonPitch - flareon2.x) / 1);
			else if (animFlareonPitch < flareon2.x) animFlareonPitch += ((flareon2.x - animFlareonPitch) / 1);
		}

		//Smart
		if (rotations.getSelectedValue() == 8) {
			if (animFlareon > flareon.y) animFlareon -= ((animFlareon - flareon.y) / 15);
			else if (animFlareon < flareon.y) animFlareon += ((flareon.y - animFlareon) / 15);

			if (animFlareonPitch > flareon.x) animFlareonPitch -= ((animFlareonPitch - flareon.x) / 5);
			else if (animFlareonPitch < flareon.x) animFlareonPitch += ((flareon.x - animFlareonPitch) / 5);
		}


		// Back
		if (rotations.getSelectedValue() == 3) {
			if (animBack > back.y) animBack -= ((animBack - back.y) / rotspeed);
			else if (animBack < back.y) animBack += ((back.y - animBack) / rotspeed);
		}

		if ((speed > 0.05f || GameData::isKeyDown(*input->spaceBarKey)) && !moduleMgr->getModule<Killaura>()->atk) {
			switch (rotations.getSelectedValue()) {
			case 0: // Normal
				player->yawUnused1 = animYaw;
				player->bodyYaw = animYaw;
				player->pitch = animPitch;
				break;
			case 1: // Hive
				player->pitch = animPitch;
				break;
			case 2: // Down
				player->pitch = 83;
				break;
			case 3: // Back
				player->yawUnused1 = animBack;
				player->bodyYaw = animBack;
				player->pitch = 80;
				break;
			case 6: // Smooth
				player->yawUnused1 = animFlareon;
				player->bodyYaw = animFlareon;
				player->pitch = 80;
				break;
			case 7: // Flareon
				player->yawUnused1 = animFlareon;
				player->bodyYaw = animFlareon;
				player->pitch = animFlareonPitch;
				break;
			case 8: // Smart
				player->yawUnused1 = animFlareon;
				player->bodyYaw = animFlareon;
				player->pitch = animFlareonPitch;
				break;
			case 9: // Extra
				break;
			}
		}
	}
}

void Scaffold::onSendPacket(C_Packet* packet) {
	C_GameSettingsInput* input = g_Data.getClientInstance()->getGameSettingsInput();
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;

	auto* EquipPacket = reinterpret_cast<C_MobEquipmentPacket*>(packet);
	__int64 id = *g_Data.getLocalPlayer()->getUniqueId();
	C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
	C_Inventory* inv = supplies->inventory;

	if (packet->isInstanceOf<C_MovePlayerPacket>() || packet->isInstanceOf<PlayerAuthInputPacket>()) {
		if (g_Data.canUseMoveKeys()) {
			vec2_t blockPosition = player->getPos()->CalcAngle(vec3_t(calcPos));
			float speed = g_Data.getLocalPlayer()->getBlocksPerSecond();
			auto* movePacket = reinterpret_cast<PlayerAuthInputPacket*>(packet);

			if (speed > 0.05f || GameData::isKeyDown(*input->spaceBarKey)) {
				switch (rotations.getSelectedValue()) {
				case 0: // Normal
					movePacket->yawUnused = animYaw;
					movePacket->yaw = animYaw;
					movePacket->pitch = animPitch;
					break;
				case 1: // Hive
					movePacket->pitch = animPitch;
				case 4: // Silent
					movePacket->yawUnused = blockPosition.y;
					movePacket->yaw = blockPosition.y;
					movePacket->pitch = blockPosition.x;
					break;
				case 3: // Back
					movePacket->yawUnused = animBack;
					movePacket->yaw = animBack;
					movePacket->pitch = 75;
					break;
				case 6: // Smooth
					movePacket->yawUnused = animFlareon;
					movePacket->yaw = animFlareon;
					movePacket->pitch = 80;
					break;
				case 8: // Smart
					movePacket->yawUnused = animFlareon;
					movePacket->yaw = animFlareon;
					movePacket->pitch = animFlareonPitch;
					break;
				}
			}
		}
	}
}

void Scaffold::onPreRender(C_MinecraftUIRenderContext* renderCtx) {
	if (!(countopa > 149)) countopa += 15;
	if (!(county > -0.6)) county = county / 1.25;
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	static auto inter = moduleMgr->getModule<Interface>();
	float renderx = g_Data.getClientInstance()->getGuiData()->windowSize.x / 2;
	float rendery = roundf(g_Data.getGuiData()->heightGame * 0.75f) + 0.5f;
	vec2_t textPosZephyr = vec2_t(renderx, rendery - 31.f);
	if (inter->Fonts.selected == 1)
	{
		vec4_t testRect = vec4_t(scX, scY - county, 56 + scX, scY + 16 - county);
		vec2_t textPos(testRect.x + 20, testRect.y + 5);
		vec2_t blockPos(testRect.x + 3, testRect.y + 7);

		//Zephyr
		if (blocleft.getSelectedValue() == 1 && !clickGUI->isEnabled()) {
			vec4_t plusRect = vec4_t(renderx, rendery + 6, 55 + renderx, rendery + 16);

			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					totalCount += stack->count;
				}
			}
			//std::string text = "Blocks left: " + stack->count;
			string text = "   Blocks Left: " + to_string(totalCount);
			float textLen = DrawUtils::getTextWidth(&text, 1) / 2;
			textPosZephyr.x -= textLen;
			plusRect.x -= textLen;
			vec4_t rectangleRect = vec4_t(plusRect.x - 15, plusRect.y - 39, plusRect.z + 15, plusRect.w - 37);
			DrawUtils::drawText(textPosZephyr, &text, MC_Color(255, 255, 255), 1.f, 1.f, true);
			DrawUtils::fillRectangleA(rectangleRect, MC_Color(4, 3, 5, 170));
			DrawUtils::drawBottomLine(vec4_t(rectangleRect.x + 1, rectangleRect.y, rectangleRect.z, rectangleRect.w), ColorUtil::interfaceColor(1), 3);
			//DrawUtils::drawBottomLine(vec4_t{ rect.x + 1, rect.y - 2.0f, rect.x + 135 - (notification->duration / 5) * 165, rect.w }, MC_Color(interfaceColor), 3);
			//DrawUtils::drawRoundRectangle(vec4_t(plusRect.x, plusRect.y + 1, plusRect.z + 27, plusRect.w), MC_Color(0, 255, 0), false);
		}

		//Packet
		if (blocleft.getSelectedValue() == 2 && !clickGUI->isEnabled()) {
			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;

			DrawUtils::fillRoundRectangle(testRect, MC_Color(0, 0, 0, 150), false);
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					if (stack->isValid()) DrawUtils::drawItem(stack, vec2_t(blockPos.x - 1, blockPos.y - 7), 1, 1, false);
					totalCount += stack->count;
				}
			}

			string count = to_string(totalCount);
			MC_Color color = MC_Color();
			if (totalCount > 64) color = MC_Color(255, 255, 255);
			if (totalCount < 64) color = MC_Color(255, 255, 20);
			if (totalCount < 32) color = MC_Color(255, 196, 0);
			if (totalCount < 16) color = MC_Color(252, 62, 62);
			if (totalCount < 1) color = MC_Color(255, 0, 0);
			DrawUtils::drawText(vec2_t(textPos), &count, color, 1.f, true);
		}

		//Packet+
		if (blocleft.getSelectedValue() == 3 && !clickGUI->isEnabled()) {
			vec4_t plusRect = vec4_t(scX, scY + 6, 55 + scX, scY + 16);

			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					totalCount += stack->count;
				}
			}
			string count = "Blocks Left:" + to_string(totalCount);
			DrawUtils::drawText(vec2_t(textPos.x - 4, textPos.y), &count, MC_Color(255, 255, 255), 1.f, 1.f, true);
			DrawUtils::fillRoundRectangle(vec4_t(plusRect.x, plusRect.y, plusRect.z + 27, plusRect.w), MC_Color(0, 0, 0, 150), false);
			DrawUtils::drawRoundRectangle(vec4_t(plusRect.x, plusRect.y + 1, plusRect.z + 27, plusRect.w), MC_Color(0, 255, 0), false);
		}

		//OldExistent
		if (blocleft.getSelectedValue() == 4 && !clickGUI->isEnabled()) {
			vec4_t plusRect = vec4_t(scX, scY + 6, 55 + scX, scY + 16);

			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					totalCount += stack->count;
				}
			}
			string count = to_string(totalCount) + std::string(WHITE) + " blocks";

			MC_Color color = MC_Color();
			if (totalCount > 64) color = MC_Color(0, 255, 50);
			if (totalCount < 65) color = MC_Color(225, 210, 0);
			if (totalCount < 32) color = MC_Color(255, 175, 0);
			if (totalCount < 16) color = MC_Color(252, 125, 0);
			if (totalCount < 10) color = MC_Color(255, 0, 0);

			DrawUtils::drawText(vec2_t(textPos.x - 4, textPos.y), &count, color, 1.f, 1.f, true);


		}
		// Actinium (a hell load of code) what the hell
		if (blocleft.getSelectedValue() == 5 && !clickGUI->isEnabled()) {
			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					totalCount += stack->count;
				}
			}
			if (totalCount > 99)
			{
				vec4_t testRect2 = vec4_t(scX, scY - county, 61 + scX, scY + 16 - county);
				vec4_t testRect3 = vec4_t((scX)+4, (scY - county) + 4, (61 + scX) - 4, (scY + 16 - county) - 4);
				if (moduleMgr->getModule<Interface>()->glowlayers > 0) DrawUtils::drawRoundGlow(testRect3, MC_Color(0, 0, 0, 100 / moduleMgr->getModule<Interface>()->glowlayers), moduleMgr->getModule<Interface>()->glowlayers, 4);
				DrawUtils::fillRoundRectangle(testRect2, MC_Color(0, 0, 0, countopa), true);
			}
			else
			{
				vec4_t testRect2 = vec4_t(scX, scY - county, 56 + scX, scY + 16 - county);
				vec4_t testRect3 = vec4_t((scX)+4, (scY - county) + 4, (56 + scX) - 4, (scY + 16 - county) - 4);
				if (moduleMgr->getModule<Interface>()->glowlayers > 0) DrawUtils::drawRoundGlow(testRect3, MC_Color(0, 0, 0, 100 / moduleMgr->getModule<Interface>()->glowlayers), moduleMgr->getModule<Interface>()->glowlayers, 4);
				DrawUtils::fillRoundRectangle(testRect2, MC_Color(0, 0, 0, countopa), true);
			}
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					if (stack->isValid()) DrawUtils::drawItem(stack, vec2_t(blockPos.x - 1, blockPos.y - 7), 1, 1, false);
				}
			}
			string count = to_string(totalCount);
			string text = count + " blocks";
			MC_Color color = MC_Color();
			if (totalCount > 99)
			{
				string text = count + "  blocks";
			}
			if (totalCount > 64) color = MC_Color(255, 255, 255);
			if (totalCount < 64) color = MC_Color(255, 255, 20);
			if (totalCount < 32) color = MC_Color(255, 196, 0);
			if (totalCount < 16) color = MC_Color(252, 62, 62);
			if (totalCount < 1) color = MC_Color(255, 0, 0);
			if (totalCount == 0)
			{
				auto sca = moduleMgr->getModule<Scaffold>();
				sca->setEnabled(false);
			}
			DrawUtils::drawText(vec2_t(textPos), &text, color, 1.f, true);
		}
	}
	// Actinium Else Call
	else
	{
		vec4_t testRect = vec4_t(scX, scY - county, 70 + scX, scY + 16 - county);
		vec2_t textPos(testRect.x + 20, testRect.y + 5);
		vec2_t blockPos(testRect.x + 3, testRect.y + 7);

		if (blocleft.getSelectedValue() == 5 && !clickGUI->isEnabled()) {
			C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
			C_Inventory* inv = supplies->inventory;
			int totalCount = 0;
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					totalCount += stack->count;
				}
			}
			if (totalCount > 99)
			{
				vec4_t testRect2 = vec4_t(scX, scY - county, 76 + scX, scY + 16 - county);
				vec4_t testRect3 = vec4_t((scX)+4, (scY - county) + 4, (76 + scX) - 4, (scY + 16 - county) - 4);
				if (moduleMgr->getModule<Interface>()->glowlayers > 0) DrawUtils::drawRoundGlow(testRect3, MC_Color(0, 0, 0, 100 / moduleMgr->getModule<Interface>()->glowlayers), moduleMgr->getModule<Interface>()->glowlayers, 4);
				DrawUtils::fillRoundRectangle(testRect2, MC_Color(0, 0, 0, countopa), false);
			}
			else
			{
				vec4_t testRect2 = vec4_t(scX, scY - county, 70 + scX, scY + 16 - county);
				vec4_t testRect3 = vec4_t((scX)+4, (scY - county) + 4, (70 + scX) - 4, (scY + 16 - county) - 4);
				if (moduleMgr->getModule<Interface>()->glowlayers > 0) DrawUtils::drawRoundGlow(testRect3, MC_Color(0, 0, 0, 100 / moduleMgr->getModule<Interface>()->glowlayers), moduleMgr->getModule<Interface>()->glowlayers, 4);
				DrawUtils::fillRoundRectangle(testRect2, MC_Color(0, 0, 0, countopa), false);
			}
			for (int s = 0; s < 9; s++) {
				C_ItemStack* stack = inv->getItemStack(s);
				if (stack->item != nullptr && stack->getItem()->isBlock() && findBlocks(stack)) {
					if (stack->isValid()) DrawUtils::drawItem(stack, vec2_t(blockPos.x - 1, blockPos.y - 7), 1, 1, false);
				}
			}
			string count = to_string(totalCount);
			string text = count + " blocks";
			MC_Color color = MC_Color();
			if (totalCount > 99)
			{
				string text = count + "  blocks";
			}
			if (totalCount > 64) color = MC_Color(255, 255, 255);
			if (totalCount < 64) color = MC_Color(255, 255, 20);
			if (totalCount < 32) color = MC_Color(255, 196, 0);
			if (totalCount < 16) color = MC_Color(252, 62, 62);
			if (totalCount < 1) color = MC_Color(255, 0, 0);
			if (totalCount == 0)
			{
				auto sca = moduleMgr->getModule<Scaffold>();
				sca->setEnabled(false);
			}
			DrawUtils::drawText(vec2_t(textPos), &text, color, 1.f, true);
		}
	}

	if (showExposed) {
		vec3_t outline = oldpos;
		outline.x += 1.f;
		outline.y += 1.f;
		outline.z += 1.f;
		int index = 0;
		auto interfaceMod = moduleMgr->getModule<Interface>();
		index++; int curIndex = -index * interfaceMod->spacing;
		auto interfaceColor = ColorUtil::interfaceColor(curIndex);
		DrawUtils::setColor(interfaceColor.r, interfaceColor.g, interfaceColor.b, 1.f);
		DrawUtils::drawBox(oldpos, outline, expT, 1.f, false);
	}
}

bool Scaffold::buildBlock(vec3_t blockBelow) {
	C_GameSettingsInput* input = g_Data.getClientInstance()->getGameSettingsInput();
	float velocityxz = g_Data.getLocalPlayer()->velocity.magnitudexz();
	static vector<vec3_ti*> checklist;
	auto player = g_Data.getLocalPlayer();
	blockBelow = blockBelow.floor();
	int i = 0;

	C_Block* block = g_Data.getLocalPlayer()->region->getBlock(vec3_ti(blockBelow));
	C_BlockLegacy* blockLegacy = (block->blockLegacy);
	if (blockLegacy->material->isReplaceable) {
		vec3_ti blok(blockBelow);
		if (checklist.empty()) {
			checklist.push_back(new vec3_ti(0, -1, 0));
			checklist.push_back(new vec3_ti(0, 1, 0));

			checklist.push_back(new vec3_ti(0, 0, -1));
			checklist.push_back(new vec3_ti(0, 0, 1));

			checklist.push_back(new vec3_ti(-1, 0, 0));
			checklist.push_back(new vec3_ti(1, 0, 0));
		}
		foundBlock = false;
		for (auto current : checklist) {
			vec3_ti calc = blok.sub(*current);
			if (!g_Data.getLocalPlayer()->region->getBlock(calc)->blockLegacy->material->isReplaceable) {
				foundBlock = true;
				blok = calc;
				break;
			}
			i++;
		}
		if (foundBlock) {
			g_Data.getCGameMode()->buildBlock(&blok, i);
			calcPos = blok.toVector3().add(0.78f, 0.78f, 0.78f);
			return true;
		}
	}
	return false;
}

bool Scaffold::predictBlock(vec3_t blockBelow) {
	static vector<vec3_ti> blocks;
	if (blocks.empty()) {
		for (int y = -range; y <= 0; y++) {
			for (int x = -range; x <= range; x++) {
				for (int z = -range; z <= range; z++) {
					blocks.push_back(vec3_ti(x, y, z));
				}
			}
		}
		//https://www.mathsisfun.com/geometry/pythagoras-3d.html c2 = x2 + y2 + z2
		sort(blocks.begin(), blocks.end(), [](vec3_ti start, vec3_ti end) {
			return sqrtf((start.x * start.x) + (start.y * start.y) + (start.z * start.z)) < sqrtf((end.x * end.x) + (end.y * end.y) + (end.z * end.z));
			});
	}

	blockBelow = blockBelow.floor();

	for (const vec3_ti& offset : blocks) {
		vec3_ti currentBlock = vec3_ti(blockBelow).add(offset);
		C_Block* block = g_Data.getLocalPlayer()->region->getBlock(currentBlock);
		C_GameSettingsInput* input = g_Data.getClientInstance()->getGameSettingsInput();
		C_BlockLegacy* blockLegacy = (block->blockLegacy);
		auto player = g_Data.getLocalPlayer();
		float velocityxz = player->velocity.magnitudexz();
		if (blockLegacy->material->isReplaceable) {
			vec3_ti blok(currentBlock);
			int i = 0;
			static vector<vec3_ti*> checklist;
			if (checklist.empty()) {
				checklist.push_back(new vec3_ti(0, -1, 0));
				checklist.push_back(new vec3_ti(0, 1, 0));

				checklist.push_back(new vec3_ti(0, 0, -1));
				checklist.push_back(new vec3_ti(0, 0, 1));

				checklist.push_back(new vec3_ti(-1, 0, 0));
				checklist.push_back(new vec3_ti(1, 0, 0));
			}
			foundBlock = false;
			for (auto current : checklist) {
				vec3_ti calc = blok.sub(*current);
				if (!g_Data.getLocalPlayer()->region->getBlock(calc)->blockLegacy->material->isReplaceable) {
					foundBlock = true;
					blok = calc;
					break;
				}
				i++;
			}
			if (foundBlock) {
				g_Data.getCGameMode()->buildBlock(&blok, i);
				calcPos = blok.toVector3().add(0.78f, 0.78f, 0.78f);
				return true;
			}
		}
	}
	return false;
}

bool Scaffold::selectBlock() {
	C_PlayerInventoryProxy* supplies = g_Data.getLocalPlayer()->getSupplies();
	C_Inventory* inv = supplies->inventory;

	auto prevSlot = supplies->selectedHotbarSlot;
	if (holdType.getSelectedValue() == 1 || holdType.getSelectedValue() == 2) {
		for (int n = 0; n < 9; n++) {
			C_ItemStack* stack = inv->getItemStack(n);
			if (stack->item != nullptr) {
				if (stack->getItem()->isBlock() && isUsefulBlock(stack)) {
					if (prevSlot != n)
						supplies->selectedHotbarSlot = n;
					return true;
				}
			}
		}
	}
	else {
		int currentSlot = 0;
		int CurrentBlockCount = 0;
		for (int n = 0; n < 9; n++) {
			C_ItemStack* stack = inv->getItemStack(n);
			if (stack->item != nullptr) {
				int currentstack = stack->count;
				if (currentstack > CurrentBlockCount && stack->getItem()->isBlock() && isUsefulBlock(stack)) {
					CurrentBlockCount = currentstack;
					currentSlot = n;
				}
			}
		}
		supplies->selectedHotbarSlot = currentSlot;
		return true;
	}
	return false;
}

bool Scaffold::isUsefulBlock(C_ItemStack* itemStack) {
	vector<string> uselessSubstrings = { "_coral", "_button", "chest", "boom", "vine", "pressure_plate", "fence", "_wall", "_stairs", "_table", "furnace", "trapdoor", "command_block", "torch", "carpet", "flower" };
	vector<string> uselessNames = { "cake", "ladder", "tnt", "lever", "loom", "scaffolding", "web", "sand", "gravel", "dragon_egg", "anvil", "bamboo" };
	string itemName = itemStack->getItem()->name.getText();

	for (auto substring : uselessSubstrings) {
		if (itemName.find(substring) != string::npos) {
			return 0;
		}
	}

	for (auto name : uselessNames) {
		if (itemName == name) {
			return 0;
		}
	}
	return 1;
}

bool Scaffold::findBlocks(C_ItemStack* itemStack) {
	if (itemStack->item == nullptr) return true;
	if ((*itemStack->item)->isBlock() && isUsefulBlock(itemStack)) return true; // Block
	return false;
}

void Scaffold::onDisable() {
	g_Data.getClientInstance()->minecraft->setTimerSpeed(20.f);
	auto sprint = moduleMgr->getModule<Sprint>();
	auto speedMod = moduleMgr->getModule<Speed>();
	sprint->useSprint = true;

	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	if (firstlock) lockY = true;
	else lockY = false;
	if (speedwasenabled) { speedMod->setEnabled(true); speedwasenabled = false; }
	if (tower.getSelectedValue() == 1 && foundBlock && jumping) player->velocity.y = 0.f;
	C_PlayerInventoryProxy* supplies = player->getSupplies();
	selectBlock();
	supplies->selectedHotbarSlot = slot;
	Utils::patchBytes((unsigned char*)HiveRotations1, (unsigned char*)"\x89\x41\x18", 3);
	Utils::patchBytes((unsigned char*)HiveRotations2, (unsigned char*)"\xC7\x40\x18\x03\x00\x00\x00", 7);

	Utils::patchBytes((unsigned char*)HiveRotations3, (unsigned char*)"\xA4\x60\x38\x02\x13\x86\x01\x13\x8", 11);

	Utils::patchBytes((unsigned char*)HiveRotations1, (unsigned char*)"\x89\x41\x18", 3);
	Utils::patchBytes((unsigned char*)HiveRotations2, (unsigned char*)"\xC7\x40\x18\x03\x00\x00\x00", 7);
	Utils::patchBytes((unsigned char*)HiveRotations3, (unsigned char*)"\xA4\x60\x38\x02\x13\x86\x017\x13\x8", 11);
}
