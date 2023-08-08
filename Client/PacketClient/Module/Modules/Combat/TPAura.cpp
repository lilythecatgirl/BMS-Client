#include "TPAura.h"
#include "../pch.h"

using namespace std;
TPAura::TPAura() : IModule(0, Category::COMBAT, "Attacks entities from far ranges") {
	registerIntSetting("Range", &range, range, 5, 250);
	registerIntSetting("Delay", &delay, delay, 0, 40);
	registerIntSetting("BackDelay", &bdelay, bdelay, 0, 40);
	registerIntSetting("Height", &height, height, 0, 5);
	registerIntSetting("MultiAttack", &multi, multi, 1, 20);
	registerBoolSetting("Once", &once, once);
	registerBoolSetting("Behind", &behind, behind);
	registerBoolSetting("voidCheck", &voidCheck, voidCheck);
	registerBoolSetting("Spoof", &spoof, spoof);
	registerBoolSetting("Back", &backsettings, backsettings);
}

const char* TPAura::getModuleName() {
	return ("TPAura");
}

static vector<C_Entity*> targetList;
void findEntTPA(C_Entity* currentEntity, bool isRegularEntity) {
	static auto tpaura = moduleMgr->getModule<TPAura>();

	if (currentEntity == nullptr)
		return;

	if (currentEntity == g_Data.getLocalPlayer())
		return;

	if (!g_Data.getLocalPlayer()->canAttack(currentEntity, false))
		return;

	if (!g_Data.getLocalPlayer()->isAlive())
		return;

	if (!currentEntity->isAlive())
		return;

	if (currentEntity->getEntityTypeId() == 80 || currentEntity->getEntityTypeId() == 69)  // XP and Arrows
		return;

	if (!TargetUtil::isValidTarget(currentEntity))
		return;

	if (currentEntity->getEntityTypeId() == 51 || currentEntity->getEntityTypeId() == 1677999)  // Villagers and NPCS
		return;

	float dist = (*currentEntity->getPos()).dist(*g_Data.getLocalPlayer()->getPos());
	if (dist < tpaura->range) targetList.push_back(currentEntity);
}

void TPAura::onEnable() {
	targetList.clear();
	back = false;
	check = 0;
}

struct CompareTargetEnArray {
	bool operator()(C_Entity* lhs, C_Entity* rhs) {
		C_LocalPlayer* localPlayer = g_Data.getLocalPlayer();
		return (*lhs->getPos()).dist(*localPlayer->getPos()) < (*rhs->getPos()).dist(*localPlayer->getPos());
	}
};

void TPAura::onTick(C_GameMode* gm) {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
	if (!once) {
		tick++;
		if (tick < realdelay)
			return;
		tick = 0;
	}
	targetList.clear();
	g_Data.forEachEntity(findEntTPA);
	sort(targetList.begin(), targetList.end(), CompareTargetEnArray());
	if (g_Data.canUseMoveKeys() && !targetList.empty()) {
		vec3_t pos = player->currentPos;
		if (!back) savepos = player->currentPos;
		vec3_t pos2 = targetList[0]->currentPos;
		float yawRad = (targetList[0]->yaw + 90) * (PI / 180);
		float pitchRad = (targetList[0]->pitch) * -(PI / 180);
		if (behind)
			pos2 = pos2.add(
				(cos(yawRad) * cos(pitchRad)) * -1.5f,
				0,
				(sin(yawRad) * cos(pitchRad)) * -1.5f
			);
		pos2 = pos2.add(0, height, 0);
		float dist = pos.dist(pos2);
		float backdist = pos.dist(savepos);
		if (!back)
		{
			check = 0;
			for (auto& target : targetList)
			{
				check++;
				pos = player->currentPos;
				vec3_t pos2 = target->currentPos;
				float yawRad = (target->yaw + 90) * (PI / 180);
				float pitchRad = (target->pitch) * -(PI / 180);
				if (behind)
					pos2 = pos2.add(
						(cos(yawRad) * cos(pitchRad)) * -1.5f,
						0,
						(sin(yawRad) * cos(pitchRad)) * -1.5f
					);
				pos2 = pos2.add(0, height, 0);
				float dist = pos.dist(pos2);
				for (int i = 0; i < dist * 1.f; i++)
				{
					vec3_t pos3 = pos.lerp(pos2, i / (float)(dist * 1.f));
					PlayerAuthInputPacket p = PlayerAuthInputPacket(pos3, player->pitch, player->yaw, player->yawUnused1);
					g_Data.getClientInstance()->loopbackPacketSender->sendToServer(&p);
					if (!spoof) player->setPos(pos3);
				}
				if (!moduleMgr->getModule<NoSwing>()->isEnabled())
					player->swing();
				gm->attack(target);
				back = true;
				realdelay = bdelay;
				if (check >= multi) break;
			}
		}
		else if (backsettings)
		{
			for (int i = 0; i < backdist * 1.f; i++)
			{
				vec3_t pos3 = pos.lerp(savepos, i / (float)(backdist * 1.f));
				PlayerAuthInputPacket p = PlayerAuthInputPacket(pos3, player->pitch, player->yaw, player->yawUnused1);
				g_Data.getClientInstance()->loopbackPacketSender->sendToServer(&p);
				if (!spoof) player->setPos(pos3);
			}
			back = false;
			realdelay = delay;
		}
	}
	if (once)
		setEnabled(false);
}

void TPAura::onDisable() {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;
}
