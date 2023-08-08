#include "ClickGUIMod.h"
#include "pch.h"

ClickGUIMod::ClickGUIMod() : IModule(VK_INSERT, Category::VISUAL, "A GUI that displays every module") {
	registerEnumSetting("Theme", &theme, 9);
	theme.addEntry("PacketV2", 0);
	theme.addEntry("Vape", 1);
	theme.addEntry("Astolfo", 2);
	theme.addEntry("PacketOld", 3);
	theme.addEntry("Tenacity", 4);
	//theme.addEntry("Rise", 5); //LOL I'm KaeruClient :cold: developed
	theme.addEntry("Lunar", 6); //Made by rasky1
	//theme.addEntry("Badlion", 7); //ika turai kanasii
	theme.addEntry("ONECONFIG", 8); //ika ha turakunai kara motto tukurou! made by rasky1
	theme.addEntry("TENACITY NEW", 9); //ika ga dev yametakara orega kawarini iroiro tukurimasu by rasky1 (watasi ha dougu deha arimasen)
	registerEnumSetting("Color", &color, 0);
	color.addEntry("Rainbow", 0);
	color.addEntry("Astolfo", 1);
	color.addEntry("Wave", 2);
	color.addEntry("RGBWave", 3);
	registerFloatSetting("TextHeight", &txtheight, txtheight, 1.f, 17.f);
	registerFloatSetting("TextSize", &txtsize, txtsize, 0.f, 2.f);
	registerBoolSetting("CategoryColors", &categoryColors, categoryColors);
	registerBoolSetting("HudEditor", &showHudEditor, showHudEditor);
	registerBoolSetting("ToolTips", &showTooltips, showTooltips);
	registerBoolSetting("Sounds", &sounds, sounds);
	registerFloatSetting("Red", &r1, r1, 0.f, 255.f);
	registerFloatSetting("Green", &g1, g1, 0.f, 255.f);
	registerFloatSetting("Blue", &b1, b1, 0.f, 255.f);
	registerFloatSetting("Red2", &r2, r2, 0.f, 255.f);
	registerFloatSetting("Green2", &g2, g2, 0.f, 255.f);
	registerFloatSetting("Blue2", &b2, b2, 0.f, 255.f);
	registerIntSetting("Opacity", &opacity, opacity, 0, 255);
	shouldHide = true;
}

const char* ClickGUIMod::getModuleName() {
	return ("ClickGUI");
}

bool ClickGUIMod::allowAutoStart() {
	return false;
}

void ClickGUIMod::onEnable() {
	skipClick = true;
	g_Data.getClientInstance()->releaseMouse();
	openAnim = -500;
	openTimeOffset = 0;
	openTime = 0;
	configs.clear();
	isSettingOpened = false;

	std::string ConfigFolder = (getenv("AppData") + (std::string)"\\..\\Local\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe\\RoamingState\\PacketSkid\\Configs\\");
	for (const auto& file : std::filesystem::directory_iterator(ConfigFolder)) {
		if (!file.is_directory()) {
			configs.push_back(file);
		}
	}

	if (!showHudEditor) {
		hasOpenedGUI = true;
	}
}

void ClickGUIMod::onPlayerTick(C_Player* plr) {
	if (hasOpenedGUI && openTimeOffset == 0)
		openTimeOffset = TimerUtil::getCurrentMs();
	if (openAnim < 27 && hasOpenedGUI) openAnim += (28 - openAnim) * 0.09f;
	if (hasOpenedGUI) openTime = TimerUtil::getCurrentMs() - openTimeOffset;
}

void ClickGUIMod::onTick(C_GameMode* gm) {
	shouldHide = true;
}

void ClickGUIMod::onPostRender(C_MinecraftUIRenderContext* renderCtx) {
	if (!hasOpenedGUI) DrawUtils::drawRectangle(vec4_t(0, 0, g_Data.getClientInstance()->getGuiData()->widthGame, g_Data.getClientInstance()->getGuiData()->heightGame), MC_Color(255, 255, 255), 1.f, 2.f);
	if (GameData::canUseMoveKeys())
		g_Data.getClientInstance()->releaseMouse();
}

void ClickGUIMod::onPreRender(C_MinecraftUIRenderContext* renderCtx) {
	if (!hasOpenedGUI) DrawUtils::fillRectangle(vec4_t(0, 0, g_Data.getClientInstance()->getGuiData()->widthGame, g_Data.getClientInstance()->getGuiData()->heightGame), MC_Color(0, 0, 0), 0.3f);
}

void ClickGUIMod::onDisable() {
	skipClick = true;
	g_Data.getClientInstance()->grabMouse();
	openAnimation = false;
	hasOpenedGUI = false;
	animation = 1;
	openTimeOffset = 0;
	openTime = 0;
	isSettingOpened = false;
	settingOpened = false;

}

void ClickGUIMod::onLoadConfig(void* conf) {
	IModule::onLoadConfig(conf);
	ClickGui::onLoadConfig(conf);
	HudEditor::onLoadSettings(conf);
}
void ClickGUIMod::onSaveConfig(void* conf) {
	IModule::onSaveConfig(conf);
	ClickGui::onSaveConfig(conf);
	HudEditor::onSaveSettings(conf);
}
