#include "ConfigManagerMod.h"

#include "../../Menu/ConfigManagerMenu.h"
#include "pch.h"

ConfigManagerMod::ConfigManagerMod() : IModule(VK_HOME, Category::VISUAL, "Manage Your Config") {
	shouldHide = true;
}

const char* ConfigManagerMod::getModuleName() {
	return ("ConfigManager");
}

bool ConfigManagerMod::allowAutoStart() {
	return false;
}
bool clickguiEnabled;
void ConfigManagerMod::onEnable() {
	g_Data.getClientInstance()->releaseMouse();
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	clickguiEnabled = clickGUI->enabled;
	clickGUI->setEnabled(false);
}

void ConfigManagerMod::onTick(C_GameMode* gm) {
	shouldHide = true;
}

void ConfigManagerMod::onPostRender(C_MinecraftUIRenderContext* renderCtx) {
	if (GameData::canUseMoveKeys()) g_Data.getClientInstance()->releaseMouse();
}

void ConfigManagerMod::onPreRender(C_MinecraftUIRenderContext* renderCtx) {
}

void ConfigManagerMod::onDisable() {
	g_Data.getClientInstance()->grabMouse();
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	clickGUI->setEnabled(clickguiEnabled);
}

void ConfigManagerMod::onLoadConfig(void* conf) {
	//IModule::onLoadConfig(conf);
	//ConfigManager::onLoadSettings(conf);
}
void ConfigManagerMod::onSaveConfig(void* conf) {
	//ConfigManager::onSaveSettings(conf);
}
