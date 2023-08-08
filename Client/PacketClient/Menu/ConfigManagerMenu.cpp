#include "ConfigManagerMenu.h"
#include <Windows.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include "../Config/ConfigManager.h"

static bool rightClickDown = false;

static bool hasInitializedCM = false;
static int timesRenderedCM = 0;
int scrollingDirectionA = 0;

int spacing = 26;

using namespace std;
void ConfigManagerMenu::render() {

	auto player = g_Data.getLocalPlayer();
	auto configManager = moduleMgr->getModule<ConfigManagerMod>();
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	if (GameData::isKeyDown(VK_ESCAPE)) moduleMgr->getModule<ConfigManagerMod>()->setEnabled(false);

	auto interfaceMod = moduleMgr->getModule<Interface>();

	vector<std::string> configs = configManager->configs;

	std::string dir_path = (getenv("AppData") + (std::string)"\\..\\Local\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe\\RoamingState\\PacketSkid\\Configs\\");

	for (const auto& entry : std::filesystem::directory_iterator(dir_path))
	{
		if (!entry.is_directory())
		{
			std::string filename = entry.path().filename().string();
			configs.push_back(filename);
		}
	}

	configs.clear();

	for (const auto& entry : std::filesystem::directory_iterator(dir_path))
	{
		if (!entry.is_directory())
		{
			std::string filename = entry.path().filename().string();
			configs.push_back(filename);
		}
	}

	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;

		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	int indexI = 0;
	indexI++; int curIndex = -indexI * interfaceMod->spacing;
	auto interfaceColor = ColorUtil::interfaceColor(curIndex);

	vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
	vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	rightClickDown = g_Data.isRightClickDown();
	string sub_title = string(GRAY) + "Press ESC to exit";
	float width = 161;
	float height = 95;
	width *= 2.45;
	height *= 2.3;
	vec4_t rectPos = vec4_t(60, 40, 160 + width, 100 + height);

	vec2_t textPos = vec2_t(rectPos.x + 6, rectPos.y + 9);

	vec2_t exitpos = vec2_t(rectPos.x + rectPos.z / 2.16, rectPos.y - 15);

	vec4_t exitRect = vec4_t(exitpos.x - 47, rectPos.y - 22, exitpos.x + 45, exitpos.y + 5);

	const bool exitFocused = exitRect.contains(&mousePos);

	if (exitFocused) {
		if (g_Data.isLeftClickDown()) {
			moduleMgr->getModule<ConfigManagerMod>()->setEnabled(false);
		}
	}

	DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, 100));
	DrawUtils::drawCenteredString(vec2_t(exitpos), &sub_title, 1.f, MC_Color(255, 255, 255), true);
	DrawUtils::drawBottomLine(vec4_t(rectPos.x, rectPos.y + 100 + height, rectPos.z, rectPos.w), MC_Color(interfaceColor), 1.f, 1.f);
	DrawUtils::flush();

	//vec4_t inputRect = vec4_t(rectPos.z - 180, rectPos.w - 30, rectPos.z - 100, rectPos.w - 20);

	//DrawUtils::drawRectangle(inputRect, MC_Color(interfaceColor), 1.f);

	//string inputText = "";

	vec4_t textPosRect;

	string a = "mouse pos here";

	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();

	if (scrollingDirectionA < 0)
		scrollingDirectionA = 0;
	if (scrollingDirectionA > configs.size() - 3)
		scrollingDirectionA = configs.size() - 3;
	int index = -1;
	int realIndex = -1;

	for (int i = 0; i < configs.size(); i++)
	{

		realIndex++;
		if (realIndex < scrollingDirectionA)
			continue;
		index++;
		if (index >= 10)
			break;

		textPosRect = vec4_t(textPos.x, textPos.y, textPos.x + 100, textPos.y + 18);

		string load = "load";
		string save = "save";

		vec4_t btnRect1 = vec4_t(textPos.x + 420, textPos.y, textPos.x + 440, textPos.y + 15);
		vec4_t btnRect2 = vec4_t(textPos.x + 445, textPos.y, textPos.x + 465, textPos.y + 15);
		DrawUtils::drawText(vec2_t(btnRect1.x, btnRect1.y), &load, btnRect1.contains(&mousePos) ? MC_Color(interfaceColor) : MC_Color(255, 255, 255), 1.f, 1.f, true);
		DrawUtils::drawText(vec2_t(btnRect2.x, btnRect2.y), &save, btnRect2.contains(&mousePos) ? MC_Color(interfaceColor) : MC_Color(255, 255, 255), 1.f, 1.f, true);
		//DrawUtils::drawRoundRectangle(btnRect1, btnRect1.contains(&mousePos) ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);
		//DrawUtils::drawRoundRectangle(btnRect2, btnRect2.contains(&mousePos) ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);
		//DrawUtils::drawImage("textures/ui/book_shiftright_default.png", vec2_t(btnRect2.x, btnRect2.y), vec2_t(20, 20), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f), MC_Color(230, 230, 230));
		//DrawUtils::drawImage("textures/gui/newgui/check2.png", vec2_t(btnRect1.x, btnRect1.y), vec2_t(20, 20), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f), MC_Color(230, 230, 230));
		if (btnRect1.contains(&mousePos) && g_Data.isLeftClickDown()) {
			configManager->setEnabled(false);
			std::string filename = configs[i];

			std::regex re("\\.txt$");

			std::string replaced = std::regex_replace(filename, re, "");
			configMgr->loadConfig(replaced, false);
			SettingMgr->loadSettings("Settings", false);
		}
		if (btnRect2.contains(&mousePos) && g_Data.isLeftClickDown()) {
			configManager->setEnabled(false);
			std::string filename = configs[i];

			std::regex re("\\.txt$");

			std::string replaced = std::regex_replace(filename, re, "");
			configMgr->saveConfigWithCustomName(replaced);
			SettingMgr->loadSettings("Settings", true);
		}

		//	DrawUtils::fillRectangleA(textPosRect, MC_Color(100, 100, 110, 100));

		auto last_write_time = std::filesystem::last_write_time(dir_path + "\\" + configs[i]);

		auto last_write_time_duration = last_write_time.time_since_epoch();

		auto last_write_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(last_write_time_duration);

		std::time_t last_write_time_t = last_write_time_seconds.count();

		std::tm* last_write_time_tm = std::localtime(&last_write_time_t);

		last_write_time_tm->tm_year -= 369;

		last_write_time_tm->tm_mday += 1;

		std::ostringstream oss;

		oss << std::put_time(last_write_time_tm, "%Y/%m/%d/%H:%M");

		std::string last_write_time_str = string(GRAY) + oss.str();

		DrawUtils::drawText(textPos, &configs[i], btnRect1.contains(&mousePos) ? MC_Color(interfaceColor) : MC_Color(255, 255, 255), 1.f, 1.f, true);
		DrawUtils::drawText(vec2_t(textPos.x, textPos.y + 9), &last_write_time_str, MC_Color(255, 255, 255), 1.f, 1.f, true);
		textPos.y += spacing;

		const bool configDragging = g_Data.isLeftClickDown();

		//vec2_t mousePos2 = vec2_t(mousePos.x - 185, mousePos.y - 212);

	//	DrawUtils::drawText(mousePos, &a, MC_Color(interfaceColor), 1.f, 1.f, true);

		const bool configFocused = textPosRect.contains(&mousePos);

		if (configFocused) {
			if (configDragging) {
				std::string filename = configs[i];

				std::regex re("\\.txt$");

				std::string replaced = std::regex_replace(filename, re, "");

				configMgr->loadConfig(replaced, false);

				moduleMgr->getModule<ConfigManagerMod>()->setEnabled(false);
			}
		}
	}
}

void ConfigManagerMenu::onWheelScroll(bool direction) {
	if (!direction) scrollingDirectionA++;
	else scrollingDirectionA--;
}

void ConfigManagerMenu::init() { hasInitializedCM = true; }

void ConfigManagerMenu::onKeyUpdate(int key, bool isDown) {
	if (!hasInitializedCM)
		return;
	static auto configManager = moduleMgr->getModule<ConfigManagerMod>();

	if (!isDown)
		return;

	if (!configManager->isEnabled()) {
		timesRenderedCM = 0;
		return;
	}

	if (timesRenderedCM < 10)
		return;
	timesRenderedCM = 0;

	switch (key) {
	default:
		break;
	case VK_ESCAPE:
		configManager->setEnabled(false);
		break;
	case VK_HOME:
		configManager->setEnabled(false);
		break;
	}
}