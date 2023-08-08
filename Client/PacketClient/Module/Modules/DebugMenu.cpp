#include "DebugMenu.h"
#include "../ModuleManager.h"

using namespace std;
DebugMenu::DebugMenu() : IModule(VK_F3, Category::UNUSED, "Displays debug information") {
	shouldHide = true;
}

const char* DebugMenu::getRawModuleName() {
	return "DebugMenu";
}

const char* DebugMenu::getModuleName() {
	return "DebugMenu";
}

void DebugMenu::onTick(C_GameMode* gm) {
	shouldHide = true;
}
string float2str(float input, int digits) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(digits) << input;
	return oss.str();
}
void DebugMenu::onPostRender(C_MinecraftUIRenderContext* renderCtx) {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;

	auto interfaceMod = moduleMgr->getModule<Interface>();
	vec3_t* pos = player->getPos();
	string yaw = "Yaw: " + to_string(player->yaw);
	string bodyyaw = "BodyYaw: " + to_string(player->bodyYaw);
	vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
	vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	string width = to_string((int)windowSize.x);
	string height = to_string((int)windowSize.y);
	string widthReal = to_string((int)windowSizeReal.x);
	string heightReal = to_string((int)windowSizeReal.y);
	string size = "Display: " + width + "x" + height;
	string realSize = "RealDisplay: " + widthReal + "x" + heightReal;
	string multiplayer;
	if (g_Data.getClientInstance()->isInMultiplayerGame()) multiplayer = "Multiplayer";
	else multiplayer = "Singleplayer";
	string pitch = "Pitch: " + to_string(player->pitch);
	string position = "XYZ: " + to_string((int)floorf(pos->x)) + " / " + to_string((int)floorf(pos->y)) + " / " + to_string((int)floorf(pos->z));
	string fps = to_string(g_Data.getFPS()) + " fps";
	string timer = "Timer: " + to_string(*g_Data.getClientInstance()->minecraft->timer);
	string speedStr = "Speed: " + float2str(g_Data.maxSpeed, 3) + " / " + float2str(g_Data.minSpeed, 3) + " avg " + float2str(g_Data.avgSpeed, 3);
	string textStr[] = {
		"Minecraft 1.18.12 (" + (interfaceMod->versionStr + " / Actinium)"),
		multiplayer,
		fps,
		"",
		position,
		yaw,
		pitch,
		bodyyaw,
		size,
		realSize,
		speedStr,
		timer
	};
	int yOffset = 0;
	for (string str : textStr) {
		if (str.size() != 0) {
			int width = DrawUtils::getTextWidth(&str);
			DrawUtils::drawText(vec2_t(1, yOffset + 2), &str, MC_Color(255, 255, 255), 1, 1, true);
			DrawUtils::fillRectangle(vec4_t(0, yOffset, width + 3, yOffset + 12), MC_Color(50, 50, 50), 0.7f);
		}
		yOffset += 12;
	}
}