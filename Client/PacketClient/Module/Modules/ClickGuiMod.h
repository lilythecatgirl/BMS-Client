#pragma once
#include "../../Menu/HudEditor.h"
#include "../../Menu/ClickGui.h"
#include "../../Config/ConfigManager.h"
#include "../../Config/SettingManager.h"
#include "Module.h"

class ClickGUIMod : public IModule {
private:
public:
	int openTimeOffset = 0;
	bool showHudEditor = true;
	bool hasOpenedGUI = false; // start with hudeditor
	std::vector<std::filesystem::directory_entry> configs;
	bool settingOpened = false;
	bool skipClick = true; // kore nannka insert ositatokino ano modules botan nagaosisuruto sonotokini usironiatta module katteni osareru yatuwo kaihisuruyatu
	float r1 = 255.f;
	float g1 = 255.f;
	float b1 = 255.f;
	float r2 = 0.f;
	float g2 = 0.f;
	float b2 = 0.f;
	bool headerAnimation = false;
	bool categoryColors = true;
	bool openAnimation = false;
	bool showTooltips = false;
	bool isSettingOpened = false;
	bool rainbowColor = false;
	bool animations = true;
	bool enabled = true;
	bool sounds = true;
	bool cFont = false;
	float animation = 1.f;
	float openAnim = 1.f;
	int openTime = 0;
	int opacity = 175;
	int t = 0;
	float txtsize = 0.9f;
	float txtheight = 9;

	SettingEnum theme = this;
	SettingEnum color = this;

	virtual void onPostRender(C_MinecraftUIRenderContext* renderCtx);
	virtual void onPreRender(C_MinecraftUIRenderContext* renderCtx);
	virtual void onPlayerTick(C_Player* plr);
	virtual void onLoadConfig(void* conf);
	virtual void onSaveConfig(void* conf);
	virtual const char* getModuleName();
	virtual void onTick(C_GameMode* gm);
	virtual bool allowAutoStart();
	virtual void onDisable();
	virtual void onEnable();
	ClickGUIMod();
};
