#include "PlayerList.h"
#include "../pch.h"

using namespace std;
PlayerList::PlayerList() : IModule(VK_TAB, Category::VISUAL, "Displays all players in your render distance") {
	registerBoolSetting("Hold", &hold, hold);
}

const char* PlayerList::getModuleName() {
	return "PlayerList";
}

static vector<C_Entity*> playerList;
void findPlayers(C_Entity* currentEntity, bool isRegularEntity) {
	if (currentEntity == nullptr) return;
	if (!g_Data.getLocalPlayer()->canAttack(currentEntity, false)) return;
	if (!g_Data.getLocalPlayer()->isAlive()) return;
	if (!currentEntity->isAlive()) return;
	if (currentEntity->getEntityTypeId() != 319) return;
	if (currentEntity->getNameTag()->getTextLength() <= 1) return;
	playerList.push_back(currentEntity);
}

bool PlayerList::isHoldMode() {
	return hold;
}

void PlayerList::onEnable() {
	playerList.clear();
}

void PlayerList::onTick(C_GameMode* gm) {
	playerList.clear();
	g_Data.forEachEntity(findPlayers);
}

void PlayerList::onPostRender(C_MinecraftUIRenderContext* renderCtx) {
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;

	auto interfaceColor = ColorUtil::interfaceColor(1);
	if (g_Data.canUseMoveKeys() || !clickGUI->hasOpenedGUI) {
		vec4_t rectPos = vec4_t(g_Data.getClientInstance()->getGuiData()->widthGame / 2, 1, 1, 1);
		vec2_t textPos = vec2_t(rectPos.x, rectPos.y);
		int maxWidth = 0;
		for (auto& i : playerList) {
			textPos.y += 8;
			string test = i->getNameTag()->getText(); test = Utils::sanitize(test); test = test.substr(0, test.find('\n'));
			int textWidth = DrawUtils::getTextWidth(&test);
			if (textWidth > maxWidth)
				maxWidth = textWidth;
			DrawUtils::drawCenteredString(textPos, &test, 1.f, MC_Color(255, 255, 255), true);
		}
		DrawUtils::fillRectangle(vec4_t(rectPos.x - maxWidth / 2 - 8, rectPos.y + 1.5, rectPos.x + maxWidth / 2 + 8, textPos.y + 3.5f), MC_Color(0, 0, 0), 0.5f);
	}
}

void PlayerList::onDisable() {
	playerList.clear();
}