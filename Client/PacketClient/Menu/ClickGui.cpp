#include "ClickGui.h"

#include <Windows.h>
#include "../../Utils/Json.hpp"
#include "../../Utils/Logger.h"
#include <regex>
#include "../../Utils/AnimationUtils.h"
using namespace std;
bool isLeftClickDown = false;
bool isRightClickDown = false;
bool shouldToggleLeftClick = false;  // If true, toggle the focused module
bool shouldToggleRightClick = false;
int isKeyDown = false;
int shouldToggleKeyDown = false;
int pressedKey = 0;
bool resetStartPos = true;
bool initialised = false;
int scrollingDirection = 0;
int configScrollingDirection = 0;

struct SavedWindowSettings {
	vec2_t pos = { -1, -1 };
	bool isExtended = true;
	const char* name = "";
};

SettingEntry* keybindMenuCurrent = nullptr;  // What setting is currently capturing the user's input?
int newKeybind = 0;
bool isCapturingKey = false;
bool shouldStopCapturing = false;

bool isConfirmingKey = false;  // Show a cancel and save button

map<unsigned int, shared_ptr<ClickWindow>> windowMap;
map<unsigned int, shared_ptr<ClickWindow2>> windowMap2;
map<unsigned int, SavedWindowSettings> savedWindowSettings;

bool isDragging = false;
unsigned int draggedWindow = -1;
vec2_t dragStart = vec2_t();

unsigned int focusedElement = -1;
bool isFocused = false;

//static constexpr float textPadding = 2.0f;
static constexpr float textPadding = 1.0f;
static constexpr float textSize = 1.0f;
static constexpr float textHeight = textSize * 9.5f;
static constexpr float categoryMargin = 0.5f;
static constexpr float paddingRight = 13.5f;
static constexpr float crossSize = textHeight / 2.f;
static constexpr float crossWidth = 0.2f;

static float rcolors[4];  // Rainbow color array RGBA

float currentYOffset = 0;
float currentXOffset = 0;

int timesRendered = 0;

void ClickGui::getModuleListByCategory(Category category, vector<shared_ptr<IModule>>* modList) {
	auto lock = moduleMgr->lockModuleList();
	vector<shared_ptr<IModule>>* moduleList = moduleMgr->getModuleList();

	for (auto& it : *moduleList) {
		if (it->getCategory() == category || category == Category::ALL) modList->push_back(it);
	}
}

std::shared_ptr<ClickWindow> ClickGui::getWindow(const char* name) {
	unsigned int id = Utils::getCrcHash(name);

	auto search = windowMap.find(id);
	if (search != windowMap.end()) {  // Window exists already
		return search->second;
	}
	else {  // Create window
		// TODO: restore settings for position etc
		std::shared_ptr<ClickWindow> newWindow = std::make_shared<ClickWindow>();
		newWindow->name = name;

		auto savedSearch = savedWindowSettings.find(id);
		if (savedSearch != savedWindowSettings.end()) {  // Use values from config
			newWindow->isExtended = savedSearch->second.isExtended;
			if (savedSearch->second.pos.x > 0)
				newWindow->pos = savedSearch->second.pos;
		}

		windowMap.insert(std::make_pair(id, newWindow));
		return newWindow;
	}
}

std::shared_ptr<ClickModule> ClickGui::getClickModule(std::shared_ptr<ClickWindow> window, const char* name) {
	unsigned int id = Utils::getCrcHash(name);

	auto search = window->moduleMap.find(id);
	if (search != window->moduleMap.end()) {  // Window exists already
		return search->second;
	}
	else {  // Create window
		// TODO: restore settings for position etc
		std::shared_ptr<ClickModule> newModule = std::make_shared<ClickModule>();

		window->moduleMap.insert(std::make_pair(id, newModule));
		return newModule;
	}
}

shared_ptr<ClickModule2> ClickGui::getClickModule2(shared_ptr<ClickWindow2> window, const char* name) {
	unsigned int id = Utils::getCrcHash(name);

	auto search = window->moduleMap2.find(id);
	if (search != window->moduleMap2.end()) {  // Window exists already
		return search->second;
	}
	else {  // Create window
		// TODO: restore settings for position etc
		shared_ptr<ClickModule2> newModule = make_shared<ClickModule2>();

		window->moduleMap2.insert(make_pair(id, newModule));
		return newModule;
	}
}

std::map<unsigned int, std::shared_ptr<ClickModule3>> moduleMap2;
shared_ptr<ClickModule3> getClickModule3(const char* name) {
	unsigned int id = Utils::getCrcHash(name);

	auto search = moduleMap2.find(id);
	if (search != moduleMap2.end()) {
		return search->second;
	}
	else {
		shared_ptr<ClickModule3> newModule = make_shared<ClickModule3>();
		moduleMap2.insert(make_pair(id, newModule));
		return newModule;
	}
}

shared_ptr<ClickWindow2> ClickGui::getWindow2(const char* name) {
	unsigned int id = Utils::getCrcHash(name);

	auto search = windowMap2.find(id);
	if (search != windowMap2.end()) {  // Window exists already
		return search->second;
	}
	else {  // Create window
		// TODO: restore settings for position etc
		shared_ptr<ClickWindow2> newWindow = make_shared<ClickWindow2>();
		newWindow->name = name;

		auto savedSearch = savedWindowSettings.find(id);
		if (savedSearch != savedWindowSettings.end()) {  // Use values from config
			newWindow->isExtended = savedSearch->second.isExtended;
			if (savedSearch->second.pos.x > 0) newWindow->pos = savedSearch->second.pos;
		}

		windowMap2.insert(make_pair(id, newWindow));
		return newWindow;
	}
}

void ClickGui::renderLabel(const char* text) { }

void ClickGui::renderTooltip(string* text) {
	vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	float lFPS = DrawUtils::getTextWidth(text, 1) + 6.5;
	float posY = windowSize.y - 24;
	vec4_t rectPos = vec4_t(2, posY + 4, lFPS + 1, posY + 20);
	vec2_t textPos = vec2_t(rectPos.x + textPadding + 2, rectPos.y + 4);
	DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, 20));
	DrawUtils::drawRectangle(rectPos, MC_Color(0, 0, 0), 0.6f, 0.5f);
	DrawUtils::drawText(textPos, text, MC_Color(255, 255, 255), 1);
}





Category selectedCategory = Category::ALL;
int selectedTab = 0;
shared_ptr<IModule> selectedModule;
bool focusConfigRect = false;
int selectedList = -1;
AnimationValue<float> selectedCategoryAnimY(0.f, easeOutExpo);
AnimationValue<float> scrollAnim(0.f, easeOutExpo);
std::map<Category, std::unique_ptr<AnimationValue<MC_Color>>> categoryTextColorAnim;
void ClickGui::renderTenacityNew() {
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	auto player = g_Data.getLocalPlayer();
	PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
	int guiWidth = 284;
	int guiHeight = 190;
	vec2_t screenSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	float animTime = min(clickGUI->openTime, 1000) / 1000.f;
	animTime = easeOutElastic(animTime);
	float centerX = (screenSize.x / 2.f) * animTime;
	float centerY = (screenSize.y / 2);
	float leftOffset = centerX - guiWidth / 2.f;
	float topOffset = centerY - guiHeight / 2;
	float rightOffset = centerX + guiWidth / 2;
	float bottomOffset = centerY + guiHeight / 2;
	auto interfaceMod = moduleMgr->getModule<Interface>();
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}
	vec4_t guiRect = vec4_t(leftOffset, topOffset, rightOffset, bottomOffset);
	DrawUtils::fillRectangle(guiRect, MC_Color(20, 20, 20), true);
	vec4_t categoryRect = vec4_t(leftOffset, topOffset, leftOffset + 60, bottomOffset);
	DrawUtils::fillRectangle(categoryRect, MC_Color(39, 39, 39), true);
	int categoryOffsetY = topOffset + 19;
	DrawUtils::fillRectangle(vec4_t(categoryRect.x + 4, categoryOffsetY, categoryRect.z - 4, categoryOffsetY + 1), MC_Color(62, 62, 62), 1.f);
	DrawUtils::drawCenteredString(vec2_t((categoryRect.z + categoryRect.x) / 2, categoryRect.y + 8), &(string)"Actinium", 1.f, MC_Color(255, 255, 255), true);
	DrawUtils::drawCenteredString(vec2_t((categoryRect.z + categoryRect.x) / 2, categoryRect.y + 17), &interfaceMod->versionStr, 0.6f, MC_Color(255, 255, 255), true);
	categoryOffsetY += 2;
	vector<Category> categories = { Category::COMBAT,Category::MOVEMENT,Category::VISUAL,Category::PLAYER,Category::EXPLOIT,Category::OTHER };
	float categorycurrentY = selectedCategoryAnimY.get();
	DrawUtils::fillRectangle(vec4_t(categoryRect.x, categorycurrentY, categoryRect.z, categorycurrentY + 20), MC_Color(27, 27, 27), 1.f);
	if (selectedCategory == Category::ALL)
		selectedCategory = Category::COMBAT;
	bool skipMouse = selectedList != -1;
	for (auto category : categories) {
		const char* categoryName = ClickGui::catToName(category);
		if (category == Category::VISUAL)
			categoryName = "Render";
		else if (category == Category::OTHER)
			categoryName = "Misc";
		vec4_t rect1 = vec4_t(categoryRect.x, categoryOffsetY, categoryRect.z, categoryOffsetY + 20);
		bool selected = category == selectedCategory;
		if (selected && selectedCategoryAnimY.get() == 0) {
			selectedCategoryAnimY.forceSet(categoryOffsetY);
		}
		if (!skipMouse && rect1.contains(&mousePos) && shouldToggleLeftClick && !selected) {
			selectedCategory = category;
			scrollingDirection = 0;
			scrollAnim.forceSet(0);
			selectedCategoryAnimY.set(categoryOffsetY, 1.f);
			if (clickGUI->sounds) {
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
		}
		if (categoryTextColorAnim.find(category) == categoryTextColorAnim.end())
			categoryTextColorAnim[category] = std::make_unique<AnimationValue<MC_Color>>(MC_Color(0, 0, 0), easeOutExpo);
		auto& anim = categoryTextColorAnim[category];
		if (selected)
			anim->set(MC_Color(255, 255, 255), 1.f);
		else
			if (!skipMouse && rect1.contains(&mousePos))
				anim->set(MC_Color(157, 157, 157), 0.5f);
			else
				anim->set(MC_Color(107, 107, 107), 0.5f);

		DrawUtils::drawText(vec2_t(rect1.x + 5, rect1.y + 6.5f), &(string)categoryName, anim->get(), 0.9f, 1.0f, true);
		categoryOffsetY += 21;
	}
	DrawUtils::fillRectangle(vec4_t(categoryRect.x + 4, categoryOffsetY, categoryRect.z - 4, categoryOffsetY + 1), MC_Color(62, 62, 62), 1.f);

	int modulesOffsetX = leftOffset + 66;

	{
		std::vector<std::shared_ptr<IModule>> ModuleList;
		getModuleListByCategory(selectedCategory, &ModuleList);
		scrollAnim.set(scrollingDirection, 0.75f);
		int index = -1;
		float scroll = scrollAnim.get() * 20;
		float yOffset1 = topOffset - scroll;
		float yOffset2 = topOffset - scroll;
		std::function drawLists = []() {};
		vec4_t listRect = vec4_t(0, 0, 0, 0);
		for (auto module : ModuleList) {
			float moduleHeight = 0;
			index++;
			int left = modulesOffsetX + ((index % 2) * (100 + 6));
			float top = index % 2 == 0 ? yOffset1 : yOffset2;
			int bottom = top + 20;
			vec4_t rect1 = vec4_t(left, top, left + 100, bottom);
			vec4_t rect2 = vec4_t(left, top, left + 100, bottom - 10);
			vec4_t rect3 = vec4_t(left, bottom - 9, left + 100, bottom);

			std::function fillRect = [=](vec4_t vec, MC_Color color) {
				if (vec.y < guiRect.y && vec.w < guiRect.y)
					return;
				if (vec.y > guiRect.w && vec.w > guiRect.w)
					return;
				DrawUtils::fillRectangle(vec4_t(vec.x, max(vec.y, guiRect.y), vec.z, min(vec.w, guiRect.w)), color, color.a);
			};
			std::function drawRect = [=](vec4_t vec, MC_Color color, float lineWidth = 1.f) {
				if (vec.y < guiRect.y && vec.w < guiRect.y)
					return;
				if (vec.y > guiRect.w && vec.w > guiRect.w)
					return;
				DrawUtils::drawRectangle(vec4_t(vec.x, max(vec.y, guiRect.y), vec.z, min(vec.w, guiRect.w)), color, color.a, lineWidth);
			};
			std::function drawRoundRect = [=](vec4_t vec, MC_Color color, bool rounder) {
				if (vec.y < guiRect.y && vec.w < guiRect.y)
					return;
				if (vec.y > guiRect.w && vec.w > guiRect.w)
					return;
				DrawUtils::fillRoundRectangle(vec4_t(vec.x, max(vec.y, guiRect.y) + 2, vec.z, min(vec.w, guiRect.w) - 2), color, rounder);
			};
			std::function drawTriangle = [=](vec2_t p1, vec2_t p2, vec2_t p3, MC_Color color) {
				float maxY = max(max(p1.y, p2.y), p3.y);
				float minY = min(min(p1.y, p2.y), p3.y);
				if (maxY < guiRect.y)
					return;
				if (minY > guiRect.w)
					return;
				float distance1 = min(maxY - guiRect.y, 3.f);
				float distance2 = min(guiRect.w - minY, 3.f);
				DrawUtils::setColor(color.r * 255, color.g * 255, color.b * 255, (color.a * 255) * (distance1 / 3.f) * (distance2 / 3.f));
				DrawUtils::drawTriangle(p1, p2, p3);
			};
			C_Font* font = DrawUtils::getFont(Fonts::SMOOTH);
			auto drawText = [=, &listRect](vec2_t pos, std::string textStr, MC_Color color = MC_Color(255, 255, 255), float textSize = 1.0f, bool hasShadow = true, bool center = false, bool ignoreSkip = false) {
				if (pos.y < guiRect.y - 2 && pos.y + font->getLineHeight() < guiRect.y - 2)
					return 0.f;
				if (pos.y > guiRect.w + 2 && pos.y + font->getLineHeight() > guiRect.w + 2)
					return 0.f;
				if (listRect.contains(&pos) && !ignoreSkip)
					return 0.f;
				float distance1 = min(pos.y - guiRect.y, 3.f);
				float distance2 = min(guiRect.w - (pos.y + font->getLineHeight() - 4), 3.f);
				if (!center)
					DrawUtils::drawText(pos, &textStr, color, textSize, color.a * (distance1 / 3.f) * (distance2 / 3.f), hasShadow);
				else
					DrawUtils::drawCenteredString(pos, &textStr, textSize, MC_Color(color.r * 255, color.g * 255, color.b * 255, (color.a * 255) * (distance1 / 3.f) * (distance2 / 3.f)), hasShadow);
				return DrawUtils::getTextWidth(&textStr, textSize);
			};
			auto drawImage = [=](std::string filepath, vec4_t rect, MC_Color flushColor = MC_Color(255, 255, 255)) {
				if (rect.y < guiRect.y && rect.w < guiRect.y)
					return;
				if (rect.y > guiRect.w && rect.w > guiRect.w)
					return;
				DrawUtils::drawImage(filepath, vec2_t(rect.x, rect.y), vec2_t(rect.z - rect.x, rect.w - rect.y), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f), flushColor);
			};

			fillRect(vec4_t(left, top + 3, left + 100, bottom - 3), MC_Color(30, 30, 30));
			float keybindX = left + 6;
			keybindX += drawText(vec2_t(left + 2.f, top + 7.f), (string)module->getRawModuleName(), MC_Color(255, 255, 255), 0.75f);
			string str;
			if (module->getKeybind() != 0) {
				str = regex_replace(Utils::getKeybindName(module->getKeybind()), regex("VK_"), "");
			}
			else {
				str = "NONE";
			}
			float textWidth = max(DrawUtils::getTextWidth(&str, 0.5f) + 5, 12.f);
			vec4_t keybindRect = vec4_t(keybindX, top + 6, keybindX + textWidth, bottom - 6);
			fillRect(keybindRect, MC_Color(65, 69, 77));

			{ //logic from risegui
				auto settings = *module->getSettings();
				auto setting = *std::find_if(settings.begin(), settings.end(), [](SettingEntry* entry) {
					return entry->valueType == ValueType::KEYBIND_T;
					});
				if (isCapturingKey && keybindMenuCurrent == setting)
					str = "?";
				bool isFocused = keybindRect.contains(&mousePos);

				if (!skipMouse && isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
					keybindMenuCurrent = setting;
					isCapturingKey = true;
				}

				if (!skipMouse && isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);

					setting->value->_int = 0x0;  // Clear
					isCapturingKey = false;
				}

				if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
					shouldStopCapturing = false;
					isCapturingKey = false;
					setting->value->_int = newKeybind;
				}
			}

			drawText(vec2_t(keybindX + (textWidth / 2), top + 13.2), str, MC_Color(255, 255, 255), 0.5f, false, true);
			vec4_t enableButton = vec4_t(rect1.z - 10, top + 7, rect1.z - 4, bottom - 7);
			if (!skipMouse && rect1.contains(&mousePos) && shouldToggleLeftClick && !keybindRect.contains(&mousePos)) {
				if (clickGUI->sounds) {
					level->playSound("random.click", *player->getPos(), 1, 1);
				}
				module->setEnabled(!module->enabled);
			}
			drawRoundRect(enableButton,
				module->isEnabled() ? MC_Color(52, 76, 141) : MC_Color(64, 68, 75), true);
			if (module->isEnabled())
				drawImage("textures/gui/newgui/check2.png", vec4_t(rect1.z - 10, top + 7, rect1.z - 3, top + 13));

			auto clickMod = getClickModule3(module->getRawModuleName());
			if (rect1.contains(&mousePos) && shouldToggleRightClick) {
				clickMod->isExtended = !clickMod->isExtended;
				if (clickGUI->sounds)
					level->playSound("random.click", *player->getPos(), 1, 1);
			}
			moduleHeight += 20;
			if (clickMod->isExtended) {
				moduleHeight -= 3.5f;
				auto settings = *module->getSettings();
				int height = 12;
				int index = 0;
				for (auto setting : settings) {
					index++;
					if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "Keybind") == 0) continue;
					float top2 = top + moduleHeight;
					fillRect(vec4_t(rect1.x, top2, rect1.z, top2 + height), MC_Color(35, 35, 35));
					drawText(vec2_t(left + 2.5f, top2 + 3.f), regex_replace((std::string)setting->name, regex("Hide in list"), "Drown"), MC_Color(255, 255, 255), 0.6f);
					if (setting->valueType == ValueType::BOOL_T) {
						vec4_t rect = vec4_t(rect1.z - 9, top2 + 3, rect1.z - 4, top2 + 8);
						fillRect(rect, MC_Color(65, 68, 76));
						drawRect(rect,
							setting->value->_bool ? MC_Color(65, 89, 161) : MC_Color(73, 78, 86), 1.0f);
						if (setting->value->_bool)
							drawImage("textures/gui/newgui/check2.png", vec4_t(rect1.z - 8.5f, top2 + 3.5f, rect1.z - 4.5f, top2 + 7.5f));
						if (!skipMouse && vec4_t(rect.x - 6, rect.y - 2, rect.z + 2, rect.w + 2).contains(&mousePos) && shouldToggleLeftClick) {
							if (clickGUI->sounds)
								level->playSound("random.click", *player->getPos(), 1, 1);
							setting->value->_bool = !setting->value->_bool;
						}
					}
					else if (setting->valueType == ValueType::FLOAT_T || setting->valueType == ValueType::INT_T) {
						float step = setting->step;
						float currentValue;
						float maxValue;
						float minValue;
						switch (setting->valueType)
						{
						case ValueType::FLOAT_T:
							currentValue = setting->value->_float;
							maxValue = setting->maxValue->_float;
							minValue = setting->minValue->_float;
							break;
						case ValueType::INT_T:
							currentValue = setting->value->_int;
							maxValue = setting->maxValue->_int;
							minValue = setting->minValue->_int;
							break;
						}
						std::stringstream ss;
						ss << std::fixed << std::setprecision(3) << currentValue;
						std::string s = ss.str();
						s.erase(s.find_last_not_of('0') + 1, std::string::npos);
						s.erase(s.find_last_not_of('.') + 1, std::string::npos);
						float textWidth = 11/*DrawUtils::getTextWidth(&s, 0.45f)*/;
						vec4_t rect = vec4_t(rect1.z - textWidth - 5, top2 + 3, rect1.z - 2, top2 + 8);
						fillRect(rect, MC_Color(64, 68, 75));
						drawText(vec2_t(rect1.z - textWidth / 2 - 3.65f, top2 + 8.75f), s, MC_Color(255, 255, 255), 0.45f, false, true);
						int sliderWidth = 30;
						float sliderOffsetX = rect.x - sliderWidth - 4;
						fillRect(vec4_t(sliderOffsetX, top2 + 4.5f, sliderOffsetX + sliderWidth, top2 + 6.f), MC_Color(66, 70, 79));
						float knobOffsetX = sliderOffsetX + (sliderWidth * ((currentValue - minValue) / (maxValue - minValue)));;
						fillRect(vec4_t(sliderOffsetX, top2 + 4.5f, knobOffsetX, top2 + 6.f), MC_Color(65, 89, 161));
						fillRect(vec4_t(knobOffsetX - 0.5f, top2 + 3.f, knobOffsetX + 0.5f, top2 + 7.5f), MC_Color(255, 255, 255));


						vec4_t clickRect = vec4_t(sliderOffsetX - 5, top2 + 3, sliderOffsetX + sliderWidth + 5, top2 + 8);
						if (clickRect.contains(&mousePos) && isLeftClickDown) {
							float val = minValue + ((mousePos.x - sliderOffsetX) / sliderWidth * (maxValue - minValue));
							float newValue = floor(val / step) * step;
							if (newValue < minValue)
								newValue = minValue;
							if (newValue > maxValue)
								newValue = maxValue;
							switch (setting->valueType)
							{
							case ValueType::FLOAT_T:
								setting->value->_float = newValue;
								break;
							case ValueType::INT_T:
								setting->value->_int = (int)newValue;
								break;
							}
						}

					}
					else if (setting->valueType == ValueType::ENUM_T) {
						auto entrys = ((SettingEnum*)setting->extraData)->Entrys;
						float width = 0;
						for (EnumEntry entry : entrys) {
							float textWidth = DrawUtils::getTextWidth(&entry.GetName(), 0.45f);
							if (width < textWidth)
								width = textWidth;
						}
						width += 8;
						float left2 = rect1.z - width - 4;
						vec4_t rect = vec4_t(left2, top2 + 2.f, left2 + width, top2 + 8.f);
						if (selectedModule == module && selectedList == index) {
							listRect = vec4_t(rect.x, rect.y, rect.z, rect.y + entrys.size() * 6);
							drawLists = [=, &listRect]() {
								float top3 = top2;
								vec2_t mousePos2 = vec2_t(mousePos.x, mousePos.y);
								EnumEntry selectedEntry = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								std::vector<EnumEntry> entrys2 = entrys;
								int index2 = selectedEntry.GetValue();
								if (index2 < entrys2.size()) {
									EnumEntry element = entrys2[index2];
									entrys2.erase(entrys2.begin() + index2);
									entrys2.insert(entrys2.begin(), element);
								}

								for (EnumEntry entry : entrys2) {
									drawText(vec2_t(left2 + 1.5f, top3 + 2.9f), entry.GetName(), MC_Color(255, 255, 255), 0.45f, true, false, true);
									vec4_t rect2 = vec4_t(left2, top3 + 2.f, left2 + width, top3 + 8.f);
									fillRect(rect2,
										rect2.contains(&mousePos2) ? MC_Color(59, 83, 146) : MC_Color(42, 42, 42));
									if (rect2.contains(&mousePos2) && shouldToggleLeftClick) {
										((SettingEnum*)setting->extraData)->selected = entry.GetValue();
										if (clickGUI->sounds)
											level->playSound("random.click", *player->getPos(), 1, 1);
										selectedList = -1;
									}
									top3 += 6;
								}
								drawTriangle(vec2_t(rect1.z - 5, top2 + 5.5f), vec2_t(rect1.z - 7, top2 + 3.5f), vec2_t(rect1.z - 9, top2 + 5.5f), MC_Color(255, 255, 255));
								if (!listRect.contains(&mousePos2) && shouldToggleLeftClick)
									selectedList = -1;
							};
						}
						else {
							EnumEntry selectedEntry = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
							fillRect(rect, MC_Color(42, 42, 42));
							drawText(vec2_t(left2 + 1.5f, top2 + 2.9f), selectedEntry.GetName(), MC_Color(255, 255, 255), 0.45f);
							DrawUtils::setColor(255, 255, 255, 255);
							drawTriangle(vec2_t(rect1.z - 7, top2 + 5.5f), vec2_t(rect1.z - 5, top2 + 3.5f), vec2_t(rect1.z - 9, top2 + 3.5f), MC_Color(255, 255, 255));
							if (!skipMouse && rect.contains(&mousePos) && shouldToggleLeftClick) {
								if (clickGUI->sounds)
									level->playSound("random.click", *player->getPos(), 1, 1);
								selectedModule = module;
								selectedList = index;
								shouldToggleLeftClick = false;
							}
						}
					}
					moduleHeight += height;
				}
				moduleHeight += 3;
			}
			if (index % 2 == 0)
				yOffset1 += moduleHeight;
			else
				yOffset2 += moduleHeight;
		}
		drawLists();
		float maxHeight = max(yOffset1 - topOffset + scroll, yOffset2 - topOffset + scroll) - 20;
		if (maxHeight < scroll)
			scrollingDirection = maxHeight / 20;
		else if (0 > scroll)
			scrollingDirection = 0;
	}

}


void ClickGui::renderONECONFIG() {
	MC_Color primaryColor = MC_Color(12, 82, 204);
	MC_Color secondaryColor = MC_Color(49, 51, 55);
	MC_Color hoverColor = MC_Color(55, 57, 64);

	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	auto player = g_Data.getLocalPlayer();
	PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
	int guiWidth = 426;
	int guiHeight = 249;
	vec2_t screenSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	int centerX = screenSize.x / 2;
	int centerY = screenSize.y / 2;
	int leftOffset = screenSize.x / 2 - guiWidth / 2;
	int topOffset = screenSize.y / 2 - guiHeight / 2;
	int rightOffset = screenSize.x / 2 + guiWidth / 2;
	int bottomOffset = screenSize.y / 2 + guiHeight / 2;
	auto interfaceMod = moduleMgr->getModule<Interface>();
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}
	vec4_t guiRect = vec4_t(leftOffset, topOffset, rightOffset, bottomOffset);
	DrawUtils::fillRoundRectangle(guiRect, MC_Color(20, 22, 24, 220), true);
	int categoryOffsetX = leftOffset + 75;
	int categoryOffsetY = topOffset + 25;
	DrawUtils::fillRoundRectangle(vec4_t(categoryOffsetX, topOffset, rightOffset, bottomOffset), MC_Color(20, 22, 24), true);
	DrawUtils::drawText(vec2_t(leftOffset + 14, topOffset + 8), &(std::string)"ACTI", MC_Color(73, 118, 197), 1.1f);
	DrawUtils::drawText(vec2_t(leftOffset + 14.25f, topOffset + 8.f), &(std::string)"ACTI", MC_Color(73, 118, 197), 1.1f);
	DrawUtils::drawText(vec2_t(leftOffset + 38, topOffset + 8), &(std::string)"NIUM", MC_Color(35, 108, 191), 1.1f);
	DrawUtils::drawText(vec2_t(leftOffset + 38.25f, topOffset + 8.f), &(std::string)"NIUM", MC_Color(35, 108, 191), 1.1f);
	categoryOffsetX += 5;

	{
		DrawUtils::drawText(vec2_t(leftOffset + 4, topOffset + 23), &(string)"MOD CONFIG", MC_Color(150, 150, 150), 0.7f);
		int tabOffsetY = topOffset + 30;
		int padding = 3;
		int height = 12;
		for (size_t i = 0; i < 2; i++)
		{
			vec4_t rect = vec4_t(leftOffset + padding, tabOffsetY, leftOffset + 75 - padding, tabOffsetY + height);
			string str;
			switch (i)
			{
			case 0:
				str = "Mods";
				break;
			case 1:
				str = "Profiles";
				break;
			}
			DrawUtils::fillRoundRectangle(vec4_t(rect.x, rect.y + 2, rect.z, rect.w - 2),
				i == selectedTab ? primaryColor :
				(rect.contains(&mousePos) ? hoverColor : MC_Color(0, 0, 0, 0)), true);
			if (rect.contains(&mousePos) && shouldToggleLeftClick) {
				selectedTab = i;
				clickGUI->settingOpened = false;
				scrollingDirection = 0;
			}
			DrawUtils::drawText(vec2_t(rect.x + 10, rect.y + 3), &str, MC_Color(255, 255, 255), 0.7);

			tabOffsetY += height + padding;

			if (i == selectedTab) {
				int textWidth = DrawUtils::getTextWidth(&str, 1.5f);
				vec4_t rect = vec4_t(categoryOffsetX + 2, topOffset + 5, categoryOffsetX + 2 + textWidth, topOffset + 18);
				if (rect.contains(&mousePos) && shouldToggleLeftClick) {
					clickGUI->settingOpened = false;
					scrollingDirection = 0;
				}
				DrawUtils::drawText(vec2_t(categoryOffsetX + 2, topOffset + 5), &str, clickGUI->settingOpened ? (rect.contains(&mousePos) ? MC_Color(255, 255, 255) : MC_Color(160, 162, 164)) : MC_Color(255, 255, 255), 1.5f, true);
				if (clickGUI->settingOpened) {
					DrawUtils::drawText(vec2_t(categoryOffsetX + 45, topOffset + 5), &((std::string)"> " + (std::string)selectedModule->getModuleName()), MC_Color(255, 255, 255), 1.5f, true);
				}
			}
		}
	}



	if (selectedTab == 0)
	{
		if (!clickGUI->settingOpened) {
			selectedList = -1;
			isCapturingKey = false;
			vector<Category> categories = { Category::ALL, Category::COMBAT,Category::VISUAL,Category::MOVEMENT,Category::PLAYER,Category::EXPLOIT,Category::OTHER };
			int textOffset = categoryOffsetX;
			for (auto category : categories) {
				const char* categoryName = ClickGui::catToName(category);
				string str = categoryName;
				float width = DrawUtils::getTextWidth(&str, textSize, Fonts::SMOOTH) + 10;
				vec4_t rect = vec4_t(textOffset, categoryOffsetY - 3, textOffset + width, categoryOffsetY + 13);
				DrawUtils::fillRoundRectangle(vec4_t(rect.x, rect.y + 3, rect.z, rect.w - 3),
					selectedCategory == category ? primaryColor :
					(rect.contains(&mousePos) ? hoverColor : secondaryColor), true);
				if (rect.contains(&mousePos) && shouldToggleLeftClick) {
					selectedCategory = category;
					scrollingDirection = 0;
				}
				DrawUtils::drawCenteredString(vec2_t(textOffset + width / 2, categoryOffsetY + 6.0), &str, 1.0f, MC_Color(255, 255, 255), true);
				textOffset += width + 2;
			}

			int moduleOffsetY = categoryOffsetY + 15;
			// SEX! //
			{
				std::vector<std::shared_ptr<IModule>> ModuleList;
				getModuleListByCategory(selectedCategory, &ModuleList);
				if (scrollingDirection < 0)
					scrollingDirection = 0;
				if (scrollingDirection > (floor((ModuleList.size() - 1) / 4)))
					scrollingDirection = (floor((ModuleList.size() - 1) / 4));
				int index = -1;
				int realIndex = -1;
				int padding = 3;
				int width = 80;
				int height = 38;
				for (auto module : ModuleList) {
					realIndex++;
					if (floor(realIndex / 4) < scrollingDirection)
						continue;
					index++;
					if (floor(index / 4) >= 5)
						break;
					int left = categoryOffsetX + ((index % 4) * (width + padding));
					int top = moduleOffsetY + floor(index / 4) * (height + padding);
					int bottom = top + height;
					vec4_t rect1 = vec4_t(left, top, left + width, bottom);
					vec4_t rect2 = vec4_t(left, top, left + width, bottom - 10);
					vec4_t rect3 = vec4_t(left, bottom - 9, left + width, bottom);


					DrawUtils::fillRoundRectangle(vec4_t(left, top + 3, left + width, bottom - 3),
						rect2.contains(&mousePos) ? hoverColor : secondaryColor, true);

					MC_Color bottomColor = module->isEnabled() ? primaryColor : (rect3.contains(&mousePos) ? hoverColor : secondaryColor);
					DrawUtils::fillRoundRectangle(vec4_t(left, bottom - 8, left + width, bottom - 3), bottomColor, true);
					DrawUtils::fillRectangle(vec4_t(left, bottom - 11, left + width, bottom - 8), bottomColor, true);

					if (rect2.contains(&mousePos) && shouldToggleLeftClick) {
						selectedModule = module;
						clickGUI->skipClick = true;
						isLeftClickDown = false;
						clickGUI->settingOpened = true;
					}
					if (rect3.contains(&mousePos) && shouldToggleLeftClick) {
						module->setEnabled(!module->isEnabled());
					}

					DrawUtils::fillRectangle(vec4_t(left, bottom - 11, left + width, bottom - 10), MC_Color(61, 63, 67), true);
					DrawUtils::drawText(vec2_t(left + 3, bottom - 8), &(string)module->getRawModuleName(), MC_Color(255, 255, 255), 0.7f);

					DrawUtils::drawCenteredString(vec2_t(left + width / 2, top + (height - 10) / 2 + 3), &(string)module->getRawModuleName(), 0.8f, MC_Color(255, 255, 255), true);
				}
			}
		}
		else {
			int padding = 6;
			DrawUtils::fillRoundRectangle(vec4_t(categoryOffsetX + 2, topOffset + 30, rightOffset - padding, bottomOffset - 20), MC_Color(12, 14, 16), true);

			auto settings = *selectedModule->getSettings();

			if (scrollingDirection < 0)
				scrollingDirection = 0;
			if (scrollingDirection > settings.size() - 1)
				scrollingDirection = settings.size() - 1;
			int index = -1;
			int realIndex = -1;
			bool skipMouse = selectedList != -1;
			float topOffset = categoryOffsetY + 16;

			{
				auto tmp = settings[1];
				std::rotate(settings.begin(), settings.begin() + 1, settings.begin() + 1 + 1);
			}
			int settingOffsetX = categoryOffsetX + 10;

			std::function drawMenu = []() {};
			for (auto& setting : settings) {
				realIndex++;
				if (realIndex < scrollingDirection)
					continue;
				index++;
				if (index >= 13)
					break;

				bool skipDrawText = selectedList != -1 && !(selectedList > realIndex - 1) && realIndex < (selectedList + (((SettingEnum*)settings[selectedList]->extraData)->Entrys).size());
				DrawUtils::drawText(vec2_t(settingOffsetX +
					(setting->valueType == ValueType::BOOL_T ? 18.f : 0.f), topOffset - 4.0), &(string)setting->name, MC_Color(255, 255, 255));

				if (setting->valueType == ValueType::BOOL_T) {
					vec4_t rect = vec4_t(settingOffsetX, topOffset - 4, settingOffsetX + 14, topOffset + 4);
					DrawUtils::fillRoundRectangle(vec4_t(rect.x, rect.y + 2, rect.z, rect.w - 2),
						setting->value->_bool ? primaryColor : (!skipMouse && rect.contains(&mousePos) ? hoverColor : MC_Color(48, 50, 54)), true);
					if (setting->value->_bool)
						DrawUtils::fillRoundRectangle(vec4_t(settingOffsetX + 7, topOffset - 1, settingOffsetX + 13, topOffset + 1), MC_Color(255, 255, 255), true);
					else
						DrawUtils::fillRoundRectangle(vec4_t(settingOffsetX + 1, topOffset - 1, settingOffsetX + 7, topOffset + 1), MC_Color(255, 255, 255), true);
					if (!skipMouse && rect.contains(&mousePos) && shouldToggleLeftClick) {
						setting->value->_bool = !setting->value->_bool;
					}
				}
				else if (setting->valueType == ValueType::INT_T || setting->valueType == ValueType::FLOAT_T) {
					int width = 170;
					float step = setting->step;
					float currentValue;
					float maxValue;
					float minValue;
					switch (setting->valueType)
					{
					case ValueType::FLOAT_T:
						currentValue = setting->value->_float;
						maxValue = setting->maxValue->_float;
						minValue = setting->minValue->_float;
						break;
					case ValueType::INT_T:
						currentValue = setting->value->_int;
						maxValue = setting->maxValue->_int;
						minValue = setting->minValue->_int;
						break;
					}
					int sliderOffsetX = rightOffset - padding - 50 - width;

					DrawUtils::fillRectangle(vec4_t(sliderOffsetX, topOffset - 1, sliderOffsetX + width, topOffset + 1), MC_Color(72, 78, 86), 1.0f);
					int knobX = sliderOffsetX + (width * ((currentValue - minValue) / (maxValue - minValue)));
					DrawUtils::fillRectangle(vec4_t(sliderOffsetX, topOffset - 1, knobX, topOffset + 1), primaryColor, 1.0f);
					DrawUtils::fillRoundRectangle(vec4_t(knobX - 3, topOffset - 1, knobX + 3, topOffset + 1), MC_Color(239, 241, 245), true);

					DrawUtils::drawRoundRectangle(vec4_t(sliderOffsetX + width + 10, topOffset - 2, sliderOffsetX + width + 33, topOffset + 2), MC_Color(28, 31, 32), 1.0f);

					std::stringstream ss;
					ss << std::fixed << std::setprecision(3) << currentValue;
					std::string s = ss.str();
					s.erase(s.find_last_not_of('0') + 1, std::string::npos);
					s.erase(s.find_last_not_of('.') + 1, std::string::npos);
					if (!skipDrawText)
						DrawUtils::drawCenteredString(vec2_t(sliderOffsetX + width + 21.5f, topOffset + 2.6f), &s, 0.6f, MC_Color(120, 123, 124), true);
					vec4_t clickRect = vec4_t(sliderOffsetX - 5, topOffset - 5, sliderOffsetX + width + 5, topOffset + 5);
					if (clickRect.contains(&mousePos) && shouldToggleKeyDown) {
						float newValue = currentValue;
						if (pressedKey == VK_LEFT) {
							newValue -= step;
							if (newValue < minValue)
								newValue = minValue;
						}
						else if (pressedKey == VK_RIGHT) {
							newValue += step;
							if (newValue > maxValue)
								newValue = maxValue;
						}
						switch (setting->valueType)
						{
						case ValueType::FLOAT_T:
							setting->value->_float = newValue;
							break;
						case ValueType::INT_T:
							setting->value->_int = (int)newValue;
							break;
						}
					}
					if (!skipMouse && clickRect.contains(&mousePos) && isLeftClickDown) {
						float val = minValue + ((mousePos.x - sliderOffsetX) / width * (maxValue - minValue));
						float newValue = floor(val / step) * step;
						if (newValue < minValue)
							newValue = minValue;
						if (newValue > maxValue)
							newValue = maxValue;
						switch (setting->valueType)
						{
						case ValueType::FLOAT_T:
							setting->value->_float = newValue;
							break;
						case ValueType::INT_T:
							setting->value->_int = (int)newValue;
							break;
						}
					}
				}
				else if (setting->valueType == ValueType::ENUM_T) {
					int width = 80;
					int sliderOffsetX = rightOffset - padding - width - 10;
					vec4_t rect1 = vec4_t(sliderOffsetX, topOffset - 7, sliderOffsetX + width, topOffset + 7);
					DrawUtils::fillRoundRectangle(vec4_t(rect1.x, rect1.y + 3, rect1.z, rect1.w - 3),
						selectedList == realIndex ? primaryColor : (!skipMouse && rect1.contains(&mousePos) ? hoverColor : secondaryColor), true);

					vec4_t rect2 = vec4_t(sliderOffsetX + width - 9.5, topOffset - 6, sliderOffsetX + width - 1, topOffset + 3);

					if (!skipMouse && rect1.contains(&mousePos) && shouldToggleLeftClick) {
						selectedList = realIndex;
						shouldToggleLeftClick = false;
					}

					DrawUtils::fillRoundRectangle(vec4_t(sliderOffsetX + width - 9.5, topOffset - 3.5f, sliderOffsetX + width - 1, topOffset + 3.5f), primaryColor, false);
					DrawUtils::setColor(255, 255, 255, 255);

					DrawUtils::drawLine(vec2_t(rect2.x + 2, topOffset - 1.f), vec2_t(sliderOffsetX + width - 5.f, topOffset - 3.f), 0.5);
					DrawUtils::drawLine(vec2_t(rect2.z - 2, topOffset - 1.f), vec2_t(sliderOffsetX + width - 6.f, topOffset - 3.f), 0.5);

					DrawUtils::drawLine(vec2_t(rect2.x + 2, topOffset + 1.f), vec2_t(sliderOffsetX + width - 5.f, topOffset + 3.f), 0.5);
					DrawUtils::drawLine(vec2_t(rect2.z - 2, topOffset + 1.f), vec2_t(sliderOffsetX + width - 6.f, topOffset + 3.f), 0.5);

					if (!skipDrawText)
						DrawUtils::drawText(vec2_t(sliderOffsetX + 5.f, topOffset - 3), &((SettingEnum*)setting->extraData)->GetSelectedEntry().GetName(), MC_Color(255, 255, 255), 0.8f);


					if (selectedList == realIndex) {
						auto entrys = ((SettingEnum*)setting->extraData)->Entrys;
						vec4_t listRect = vec4_t(sliderOffsetX + 1, topOffset + 10, sliderOffsetX + width - 1, topOffset + 10 +
							entrys.size() * 10);
						drawMenu = [=]() {
							DrawUtils::fillRoundRectangle(listRect, MC_Color(34, 35, 38), true);
							DrawUtils::drawRoundRectangle(listRect, secondaryColor, true);
							float offsetY = topOffset + 12;
							int entryIndex = -1;
							for (EnumEntry entry : entrys) {
								entryIndex++;
								vec4_t rect = vec4_t(sliderOffsetX + 2.5f, offsetY - 3.f, sliderOffsetX + width - 2.5f, offsetY + 7.f);
								vec2_t mousePos2 = vec2_t(mousePos.x, mousePos.y);
								DrawUtils::fillRoundRectangle(vec4_t(rect.x, rect.y + 2, rect.z, rect.w - 2),
									rect.contains(&mousePos2) ? primaryColor : MC_Color(0, 0, 0, 0), true);
								DrawUtils::drawText(vec2_t(sliderOffsetX + 5.f, offsetY - 0.5f), &entry.GetName(),
									rect.contains(&mousePos2) ? MC_Color(255, 255, 255) : MC_Color(190, 190, 191), 0.8f);
								if (rect.contains(&mousePos2) && shouldToggleLeftClick) {
									((SettingEnum*)setting->extraData)->selected = entryIndex;
									isLeftClickDown = false;
									selectedList = -1;
								}
								offsetY += 10;
							}
						};
						if (!listRect.contains(&mousePos) && shouldToggleLeftClick)
							selectedList = -1;
					}
				}
				else if (setting->valueType == ValueType::KEYBIND_T) {
					int width = 80;
					int sliderOffsetX = rightOffset - padding - width - 10;
					vec4_t rect1 = vec4_t(sliderOffsetX, topOffset - 7, sliderOffsetX + width, topOffset + 7);
					DrawUtils::fillRoundRectangle(vec4_t(rect1.x, rect1.y + 3, rect1.z, rect1.w - 3),
						selectedList == realIndex ? primaryColor : (!skipMouse && rect1.contains(&mousePos) ? hoverColor : secondaryColor), true);
					DrawUtils::fillRectangle(vec4_t(rect1.x + 4.2f, rect1.y + 7.2f, rect1.x + 6, rect1.y + 9.2f), MC_Color(255, 255, 255), 1.0f);
					DrawUtils::fillRectangle(vec4_t(rect1.x + 6.75f, rect1.y + 7.2f, rect1.x + 8.5f, rect1.y + 9.2f), MC_Color(255, 255, 255), 1.0f);
					DrawUtils::fillRectangle(vec4_t(rect1.x + 6.75f, rect1.y + 4.5f, rect1.x + 8.5f, rect1.y + 6.45f), MC_Color(255, 255, 255), 1.0f);
					DrawUtils::fillRectangle(vec4_t(rect1.x + 9, rect1.y + 7.2f, rect1.x + 11, rect1.y + 9.2f), MC_Color(255, 255, 255), 1.0f);


					string str = Utils::getKeybindName(setting->value->_int);

					if (!isCapturingKey || (keybindMenuCurrent != setting && isCapturingKey)) {
						char name[0x21];
						sprintf_s(name, 0x21, "%s:", setting->name);
						if (name[0] != 0)
							name[0] = toupper(name[0]);

						if (setting->value->_int > 0 && setting->value->_int < 190)
							str = Utils::getKeybindName(setting->value->_int);
						else if (setting->value->_int == 0x0)
							str = "None";
						else
							str = "???";
					}
					else {
						str = "Recoding...";
					}


					DrawUtils::drawCenteredString(vec2_t(sliderOffsetX + width / 2.f, topOffset + 2.5f), &str, 0.8f, MC_Color(255, 255, 255), true);

					if (!skipMouse) { //logic from risegui
						bool isFocused = rect1.contains(&mousePos);

						if (isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
							if (clickGUI->sounds)
								level->playSound("random.click", *player->getPos(), 1, 1);

							keybindMenuCurrent = setting;
							isCapturingKey = true;
						}

						if (isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
							if (clickGUI->sounds)
								level->playSound("random.click", *player->getPos(), 1, 1);

							setting->value->_int = 0x0;  // Clear
							isCapturingKey = false;
						}

						if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
							if (clickGUI->sounds)
								level->playSound("random.click", *player->getPos(), 1, 1);
							shouldStopCapturing = false;
							isCapturingKey = false;
							setting->value->_int = newKeybind;
						}
					}

				}
				topOffset += 15;
			}

			drawMenu();

		}
	}
}





#pragma region Lunar
void ClickGui::renderLunarCategory() {
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	auto player = g_Data.getLocalPlayer();
	PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
	int width = 500;
	int height = 230;
	vec2_t screenSize = g_Data.getClientInstance()->getGuiData()->windowSize;
	int centerX = screenSize.x / 2;
	int centerY = screenSize.y / 2;
	int leftOffset = screenSize.x / 2 - width / 2;
	int topOffset = screenSize.y / 2 - height / 2;
	int rightOffset = screenSize.x / 2 + width / 2;
	int bottomOffset = screenSize.y / 2 + height / 2;
	auto interfaceMod = moduleMgr->getModule<Interface>();
	vec4_t rect1 = vec4_t(leftOffset, topOffset, rightOffset, bottomOffset);
	DrawUtils::drawRectangle(vec4_t(rect1.x - 1.5, rect1.y - 1.5, rect1.z + 1.5, rect1.w + 1.5), MC_Color(255, 255, 255), 0.3, 1.0f);
	DrawUtils::drawRectangle(vec4_t(rect1.x - 0.5, rect1.y - 0.5, rect1.z + 0.5, rect1.w + 0.5), MC_Color(100, 100, 100), 0.2, 1.0f);
	DrawUtils::fillRectangle(rect1, MC_Color(0, 0, 0), 0.5f);
	DrawUtils::fillRectangle(vec4_t(leftOffset, topOffset, rightOffset, topOffset + 20), MC_Color(0, 0, 0), 0.5f);
	DrawUtils::drawGradientText(vec2_t(leftOffset + 6, topOffset + 6), &(string)"Actinium Client", 1.25f);
	vec4_t configRect = vec4_t(leftOffset, topOffset + 30, leftOffset + 60, bottomOffset);
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}
	DrawUtils::fillRectangle(configRect, MC_Color(0, 0, 0), 0.5f);
	focusConfigRect = configRect.contains(&mousePos);
	int categoryOffsetX = leftOffset + 65;
	int categoryOffsetY = topOffset + 30;
	if (!clickGUI->settingOpened) {
		//Category
		{
			vector<Category> categories = { Category::ALL, Category::COMBAT,Category::VISUAL,Category::MOVEMENT,Category::PLAYER,Category::EXPLOIT,Category::OTHER };

			int textOffset = categoryOffsetX;
			for (auto category : categories) {
				const char* categoryName = ClickGui::catToName(category);
				string str = categoryName;
				float width = DrawUtils::getTextWidth(&str, textSize, Fonts::SMOOTH);
				DrawUtils::drawText(vec2_t(textOffset + 2.5, categoryOffsetY + 1), &str, MC_Color(255, 255, 255), textSize, 1.0f, false, Fonts::SMOOTH);
				vec4_t btnRect = vec4_t(textOffset, categoryOffsetY, textOffset + width + 4.5, categoryOffsetY + 10);
				DrawUtils::drawRoundRectangle(btnRect, MC_Color(89, 114, 153), false);
				DrawUtils::fillRoundRectangle(btnRect,
					selectedCategory == category ? MC_Color(79, 148, 252) :
					(btnRect.contains(&mousePos) ? MC_Color(79, 148, 252) : MC_Color(69, 103, 117))
					, false);
				if (btnRect.contains(&mousePos) && shouldToggleLeftClick) {
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
					if (selectedCategory != category) {
						scrollingDirection = 0;
						selectedCategory = category;
					}
				}
				textOffset += width + 8;
			}
		}
		std::vector<std::shared_ptr<IModule>> ModuleList;
		getModuleListByCategory(selectedCategory, &ModuleList);
		if (scrollingDirection < 0)
			scrollingDirection = 0;
		if (scrollingDirection > (floor((ModuleList.size() - 1) / 4)))
			scrollingDirection = (floor((ModuleList.size() - 1) / 4));
		{
			int index = -1;
			int realIndex = -1;
			for (auto& module : ModuleList) { //Module
				realIndex++;
				if (floor(realIndex / 4) < scrollingDirection)
					continue;
				index++;
				string str = module->getRawModuleName();
				int width = 103;
				int height = 18;
				int margin = 5;
				if (floor(index / 4) >= 7)
					break;
				int offsetX = categoryOffsetX + (index % 4) * (width + margin);
				int offsetY = categoryOffsetY + 20 + floor(index / 4) * (height + margin + 3);
				vec4_t rect = vec4_t(offsetX, offsetY, offsetX + width, offsetY + height);
				DrawUtils::fillRoundRectangle(rect, MC_Color(200, 200, 200,
					rect.contains(&mousePos) ? 50.f : 20.f), false);
				DrawUtils::drawRoundRectangle(rect,
					module->isEnabled() ? MC_Color(47, 133, 66) : MC_Color(126, 54, 49)
					, false);
				DrawUtils::drawText(vec2_t(offsetX + 2, offsetY + height / 2 - 3.25), &str, MC_Color(255, 255, 255), textSize, 1.f, false, Fonts::SMOOTH);
				DrawUtils::fillRectangle(vec4_t(offsetX + width - 18, offsetY + 5, offsetX + width - 17.3, offsetY + height - 5), MC_Color(255, 255, 255), 0.3f);


				vec4_t settingRect = vec4_t(offsetX + width - 15, offsetY + 2, offsetX + width - 15 + 15, offsetY + 2 + 15);
				DrawUtils::drawImage("textures/ui/settings_glyph_color_2x.png", vec2_t(offsetX + width - 15, offsetY + 2), vec2_t(15, 15), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f), settingRect.contains(&mousePos) ? MC_Color(253, 253, 253) : MC_Color(180, 186, 170));
				if ((settingRect.contains(&mousePos) && shouldToggleLeftClick) || (rect.contains(&mousePos) && shouldToggleRightClick)) {
					selectedModule = module;
					clickGUI->skipClick = true;
					isLeftClickDown = false;
					clickGUI->settingOpened = true;
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
					continue;
				}

				if (rect.contains(&mousePos) && shouldToggleLeftClick) {
					module->setEnabled(!module->isEnabled());
					if (clickGUI->sounds) {
						level->playSound("random.orb", *player->getPos(), 1, module->isEnabled() ? 1 : 0.9);
					}
				}
			}
		}
	}
	else {
		//module setting
		string str = selectedModule->getModuleName();
		string str2 = selectedModule->getTooltip();
		if (str2.length() == 0)
			str2 = "No Description.";
		DrawUtils::drawText(vec2_t(categoryOffsetX + 15, categoryOffsetY + 4.5), &str, MC_Color(255, 255, 255), 1.25f, 1.f, false, Fonts::SMOOTH);
		vec4_t rect = vec4_t(categoryOffsetX, categoryOffsetY + 2, rightOffset - 5, bottomOffset - 7);
		DrawUtils::fillRoundRectangle(rect, MC_Color(0, 0, 0, 70.f), false);
		DrawUtils::fillRectangle(vec4_t(rect.x, rect.y + 15, rect.z, rect.y + 16), MC_Color(255, 255, 255), 0.5f);
		vec4_t rect2 = vec4_t(rect.x + 2, rect.y + 2, rect.x + 12, rect.y + 12);
		if (rect2.contains(&mousePos) && shouldToggleLeftClick) {
			clickGUI->settingOpened = false;
			if (clickGUI->sounds)
				level->playSound("random.click", *player->getPos(), 1, 1);
			return;
		}
		DrawUtils::drawImage("textures/ui/arrow_left.png", vec2_t(rect.x + 2, rect.y + 2), vec2_t(10, 10), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f), rect2.contains(&mousePos) ? MC_Color(150, 150, 150) : MC_Color(200, 200, 200));
		DrawUtils::drawText(vec2_t(categoryOffsetX + 5, categoryOffsetY + 22), &str2, MC_Color(255, 255, 255), 0.8f, 0.8f, false, Fonts::SMOOTH);
		DrawUtils::fillRectangle(vec4_t(rect.x, rect.y + 30, rect.z, rect.y + 31), MC_Color(255, 255, 255), 0.3f);
		auto settings = *selectedModule->getSettings();
		float topOffset = categoryOffsetY + 40;

		if (scrollingDirection < 0)
			scrollingDirection = 0;
		if (scrollingDirection > settings.size() - 2)
			scrollingDirection = settings.size() - 2;
		int index = -1;
		int realIndex = -1;
		for (auto& setting : settings) {
			if (!strcmp(setting->name, "enabled"))
				continue;
			realIndex++;
			if (realIndex < scrollingDirection)
				continue;
			index++;
			if (index >= 10)
				break;
			DrawUtils::drawText(vec2_t(categoryOffsetX +
				(setting->valueType == ValueType::BOOL_T ? 35.f : 10.f)
				, topOffset), &(std::string)setting->name, MC_Color(255, 255, 255), 0.8f);


			if (setting->valueType == ValueType::BOOL_T) {
				vec4_t toggleRect = vec4_t(categoryOffsetX + 10, topOffset, categoryOffsetX + 30, topOffset + 6);
				DrawUtils::fillRoundRectangle(toggleRect, MC_Color(50, 50, 50, 150), false);
				DrawUtils::drawRoundRectangle(toggleRect, MC_Color(100, 100, 100, 150), false);

				if (toggleRect.contains(&mousePos) && shouldToggleLeftClick) {
					setting->value->_bool = !setting->value->_bool;
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
				}
				if (setting->value->_bool) {
					vec4_t toggleRect2 = vec4_t(toggleRect.x + 8, toggleRect.y, toggleRect.z, toggleRect.w);
					DrawUtils::fillRoundRectangle(toggleRect2, MC_Color(48, 168, 100), false);
					DrawUtils::drawRoundRectangle(toggleRect2, MC_Color(91, 186, 132), false);
					DrawUtils::drawText(vec2_t(toggleRect.x + 10, toggleRect.y + 0.5), &(std::string)"ON", MC_Color(255, 255, 255), 0.65f);
					DrawUtils::drawText(vec2_t(toggleRect.x + 10.2f, toggleRect.y + 0.5), &(std::string)"ON", MC_Color(255, 255, 255), 0.65f);
				}
				else {
					vec4_t toggleRect2 = vec4_t(toggleRect.x, toggleRect.y, toggleRect.z - 8, toggleRect.w);
					DrawUtils::fillRoundRectangle(toggleRect2, MC_Color(179, 51, 81), false);
					DrawUtils::drawRoundRectangle(toggleRect2, MC_Color(204, 102, 126), false);
					DrawUtils::drawText(vec2_t(toggleRect.x + 0.6f, toggleRect.y + 0.5f), &(std::string)"OFF", MC_Color(255, 255, 255), 0.6f);
					DrawUtils::drawText(vec2_t(toggleRect.x + 0.8f, toggleRect.y + 0.5f), &(std::string)"OFF", MC_Color(255, 255, 255), 0.6f);
				}

			}
			if (setting->valueType == ValueType::FLOAT_T || setting->valueType == ValueType::INT_T) {
				int width = 150;
				int offsetX = rightOffset - width - 10;
				float step = setting->step;
				float currentValue;
				float maxValue;
				float minValue;
				switch (setting->valueType)
				{
				case ValueType::FLOAT_T:
					currentValue = setting->value->_float;
					maxValue = setting->maxValue->_float;
					minValue = setting->minValue->_float;
					break;
				case ValueType::INT_T:
					currentValue = setting->value->_int;
					maxValue = setting->maxValue->_int;
					minValue = setting->minValue->_int;
					break;
				}

				std::stringstream ss;
				ss << std::fixed << std::setprecision(2) << currentValue;
				std::string s = ss.str();
				s.erase(s.find_last_not_of('0') + 1, std::string::npos);
				s.erase(s.find_last_not_of('.') + 1, std::string::npos);
				int textWidth = DrawUtils::getTextWidth(&s, 0.75);
				DrawUtils::drawText(vec2_t(offsetX - textWidth - 5.f, topOffset), &s, MC_Color(255, 255, 255), 0.75);
				vec4_t rect1 = vec4_t(offsetX, topOffset + 0.5f, offsetX + width, topOffset + 4);
				DrawUtils::fillRectangle(rect1, MC_Color(46, 47, 47), 1.f);
				vec4_t clickRect = vec4_t(rect1.x - 5, rect1.y - 3, rect1.z + 5, rect1.w + 3);
				if (clickRect.contains(&mousePos) && shouldToggleKeyDown) {
					float newValue = currentValue;
					if (pressedKey == VK_LEFT) {
						newValue -= step;
						if (newValue < minValue)
							newValue = minValue;
					}
					else if (pressedKey == VK_RIGHT) {
						newValue += step;
						if (newValue > maxValue)
							newValue = maxValue;
					}
					switch (setting->valueType)
					{
					case ValueType::FLOAT_T:
						setting->value->_float = newValue;
						break;
					case ValueType::INT_T:
						setting->value->_int = (int)newValue;
						break;
					}
				}
				if (clickRect.contains(&mousePos) && isLeftClickDown) {
					float val = (mousePos.x - rect1.x) / width * maxValue;
					float newValue = floor(val / step) * step;
					if (newValue < 0)
						newValue = 0;
					if (newValue > maxValue)
						newValue = maxValue;
					switch (setting->valueType)
					{
					case ValueType::FLOAT_T:
						setting->value->_float = newValue;
						break;
					case ValueType::INT_T:
						setting->value->_int = (int)newValue;
						break;
					}
				}
				float circlePosX = offsetX + (currentValue / maxValue) * width;
				DrawUtils::fillRoundRectangle(vec4_t(circlePosX - 3, topOffset, circlePosX + 3, topOffset + 3), MC_Color(79, 148, 252), true);
			}


			if (setting->valueType == ValueType::ENUM_T) {
				int width = 100;
				int offsetX = rightOffset - width - 40;
				EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

				DrawUtils::drawCenteredString(vec2_t(offsetX + width / 2.f, topOffset + 4.f), &i.GetName(), 1.f, MC_Color(255, 255, 255), true);

				vec4_t rect1 = vec4_t(offsetX, topOffset - 2., offsetX + 10, topOffset + 8.);
				vec4_t rect2 = vec4_t(offsetX + width - 10, topOffset - 2, offsetX + width, topOffset + 8);
				DrawUtils::drawImage("textures/ui/arrow_left.png", vec2_t(offsetX, topOffset - 2.), vec2_t(10, 10), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f),
					rect1.contains(&mousePos) ? MC_Color(1, 136, 183) : MC_Color(19, 90, 106));
				DrawUtils::drawImage("textures/ui/arrow_right.png", vec2_t(offsetX + width - 10, topOffset - 2.), vec2_t(10, 10), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f),
					rect2.contains(&mousePos) ? MC_Color(1, 136, 183) : MC_Color(19, 90, 106));
				if (rect1.contains(&mousePos) && shouldToggleLeftClick) {
					((SettingEnum*)setting->extraData)->SelectNextValue(true);
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
				}
				if (rect2.contains(&mousePos) && shouldToggleLeftClick) {
					((SettingEnum*)setting->extraData)->SelectNextValue(false);
					if (clickGUI->sounds)
						level->playSound("random.click", *player->getPos(), 1, 1);
				}

			}

			if (setting->valueType == ValueType::KEYBIND_T) {
				int width = 50;
				int offsetX = rightOffset - width - 16;
				vec4_t rect = vec4_t(offsetX - width, topOffset, offsetX, topOffset + 6);
				DrawUtils::fillRoundRectangle(rect,
					rect.contains(&mousePos) ? MC_Color(120, 120, 120, 200) : MC_Color(120, 120, 120, 100), false);
				DrawUtils::drawRoundRectangle(rect,
					rect.contains(&mousePos) ? MC_Color(200, 200, 200, 255) : MC_Color(200, 200, 200, 200), false);

				string str = Utils::getKeybindName(setting->value->_int);

				if (!isCapturingKey || (keybindMenuCurrent != setting && isCapturingKey)) {
					char name[0x21];
					sprintf_s(name, 0x21, "%s:", setting->name);
					if (name[0] != 0)
						name[0] = toupper(name[0]);

					if (setting->value->_int > 0 && setting->value->_int < 190)
						str = Utils::getKeybindName(setting->value->_int);
					else if (setting->value->_int == 0x0)
						str = "None";
					else
						str = "???";
				}
				else {
					str = "?";
				}
				static auto inter = moduleMgr->getModule<Interface>();
				DrawUtils::drawCenteredString(vec2_t(offsetX - width / 2.f, topOffset +
					(inter->Fonts.getSelectedValue() == 1 ? 3.5 : 5)), &str, 0.8f, MC_Color(255, 255, 255), false);



				{ //logic from risegui
					bool isFocused = rect.contains(&mousePos);

					if (isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
						if (clickGUI->sounds)
							level->playSound("random.click", *player->getPos(), 1, 1);

						keybindMenuCurrent = setting;
						isCapturingKey = true;
					}

					if (isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
						if (clickGUI->sounds)
							level->playSound("random.click", *player->getPos(), 1, 1);

						setting->value->_int = 0x0;  // Clear
						isCapturingKey = false;
					}

					if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
						if (clickGUI->sounds)
							level->playSound("random.click", *player->getPos(), 1, 1);
						shouldStopCapturing = false;
						isCapturingKey = false;
						setting->value->_int = newKeybind;
					}
				}



			}

			topOffset += 15;
		}
	}

	{
		if (configScrollingDirection < 0)
			configScrollingDirection = 0;
		if (configScrollingDirection > clickGUI->configs.size() - 1)
			configScrollingDirection = clickGUI->configs.size() - 1;
		float offsetY = topOffset + 30;
		int index = -1;
		int realIndex = -1;
		for (const auto& config : clickGUI->configs) {
			realIndex++;
			if (realIndex < configScrollingDirection)
				continue;
			index++;
			if (index >= 16)
				break;
			vec4_t rect1 = vec4_t(leftOffset, offsetY, leftOffset + 60, offsetY + 12);
			std::string str = std::string(config.path().filename().string());
			size_t lastindex = str.find_last_of(".");
			string rawname = str.substr(0, lastindex);
			vec4_t rect2 = vec4_t(leftOffset + 52, offsetY, leftOffset + 56, offsetY + 12);
			DrawUtils::drawText(vec2_t(leftOffset + 52, offsetY + 3.2), &(std::string)"x", rect2.contains(&mousePos) ? MC_Color(255, 255, 255) : MC_Color(200, 200, 200), 0.7, 1.0, true);
			if (rect2.contains(&mousePos) && shouldToggleLeftClick) {
				clickGUI->setEnabled(false);
				filesystem::remove(config.path().string());
				return;
			}
			DrawUtils::drawText(vec2_t(leftOffset + 3, offsetY + 3.5), &rawname, MC_Color(255, 255, 255), 0.7, 1.0, true);
			string config = configMgr->currentConfig;
			DrawUtils::drawRectangle(rect1, MC_Color(255, 255, 255), 0.4f);
			if (!rawname.compare(config))
				DrawUtils::fillRectangle(rect1, MC_Color(255, 255, 255), 0.3f);
			if (rect1.contains(&mousePos) && shouldToggleLeftClick) {
				clickGUI->setEnabled(false);
				SettingMgr->loadSettings("Settings", true);
				return configMgr->loadConfig(rawname, false);
			}
			offsetY += 12.51;

		}
	}




}

#pragma endregion




#pragma region PacketOld
void ClickGui::renderPacketOldCategory(Category category, MC_Color categoryColor) {
	static constexpr float textHeight = textSize * 9.f;
	const char* categoryName = ClickGui::catToName(category);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;

		}
	}
	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset;

	// Get All Modules in our category
	std::vector<std::shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);

	// Get max width of all text
	{
		for (auto& it : ModuleList) {
			std::string label = "-------------";
			windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&label, textSize, Fonts::SMOOTH));
		}
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}

	const float xEnd = currentXOffset + windowSize->x + paddingRight;

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f) ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 0.75f) + 0.5f /*fix flickering related to rounding errors*/;
		int moduleIndex = 0;
		for (auto& mod : moduleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;
			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			if (ourWindow->isInAnimation) { // Estimate, we don't know about module settings yet
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			std::string textStr = mod->getRawModuleName();

			vec2_t textPos = vec2_t(currentXOffset + textPadding + 45, currentYOffset + textPadding + 5);
			vec4_t rectPos = vec4_t(
				currentXOffset, currentYOffset, xEnd,
				currentYOffset + textHeight + (textPadding * 2));

			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			// Background
			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					DrawUtils::fillRectangle(rectPos, MC_Color(25, 25, 25), 0.5);
					std::string tooltip = mod->getTooltip();
					static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();
					if (clickGuiMod->showTooltips && !tooltip.empty()) renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
				else {
					DrawUtils::fillRectangleA(rectPos, mod->isEnabled() ? MC_Color(32, 32, 32, clickGUI->opacity) : MC_Color(8, 8, 8, clickGUI->opacity));
				}
			}

			// Text
			if (allowRender) DrawUtils::drawCenteredString(textPos, &textStr, textSize, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);
			//if (allowRender) DrawUtils::drawText(textPos, &textStr, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);

			// Settings
			{
				std::vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
					}
					GuiUtils::drawCrossLine(vec2_t(
						currentXOffset + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
						currentYOffset + textPadding + (textHeight / 2)),
						MC_Color(255, 255, 255), crossWidth, crossSize, !clickMod->isExtended);

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset + textPadding + 5, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							string len = "saturation  ";
							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									vec4_t boxPos = vec4_t(
										rectPos.z + textPadding - 15,
										rectPos.y + textPadding + 2,
										rectPos.z + textHeight - textPadding - 15,
										rectPos.y + textHeight - textPadding + 2);

									DrawUtils::drawRectangle(boxPos, MC_Color(255, 255, 255), isFocused ? 1 : 0.8f, 0.5f);

									if (setting->value->_bool) {
										DrawUtils::setColor(28, 107, 201, 1);
										boxPos.x += textPadding;
										boxPos.y += textPadding;
										boxPos.z -= textPadding;
										boxPos.w -= textPadding;
										DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
										DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
									}
								}

								// Text
								{
									// Convert first letter to uppercase for more friendlieness
									char name[0x21]; sprintf_s(name, 0x21, "%s", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									string elTexto = name;

									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140), true, true, true);
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								// Text and background
								float settingStart = currentYOffset;
								{
									char name[0x22]; sprintf_s(name, +"%s:", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

									char name2[0x21]; sprintf_s(name2, 0x21, " %s", i.GetName().c_str()); if (name2[0] != 0) name2[0] = toupper(name2[0]);
									string elTexto2 = name2;

									string elTexto = string(GRAY) + name + string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight - 0.5 + (textPadding * 2);
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true); //enum aids
									GuiUtils::drawCrossLine(vec2_t(
										currentXOffset + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
										currentYOffset + textPadding + (textHeight / 2)),
										MC_Color(255, 255, 255), crossWidth, crossSize, !setting->minValue->_bool);
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char str[10]; sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5, currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5, currentYOffset - textPadding + textHeight + 3.f);
									// Visuals & Logic
									{
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										rect.z = rect.z - 10;
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);
										if (value > maxValue) value = maxValue; value /= maxValue;
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;

										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									// Convert first letter to uppercase for more friendlieness
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10]; sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 15);
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5,
										currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5,
										currentYOffset - textPadding + textHeight + 3.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background

										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);

										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight + (textPadding * 2);
			}
		}

		vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

		if (winRectPos.contains(&mousePos)) {
			if (scrollingDirection > 0 && overflowing) ourWindow->yOffset += scrollingDirection;
			else if (scrollingDirection < 0) ourWindow->yOffset += scrollingDirection;
			scrollingDirection = 0;
			if (ourWindow->yOffset < 0) ourWindow->yOffset = 0;
		}
	}
	DrawUtils::flush();
	// Draw Category Header
	{
		vec2_t textPos = vec2_t(currentXOffset + textPadding, categoryHeaderYOffset + textPadding);
		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin, categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin,
			categoryHeaderYOffset + textHeight + (textPadding * 2));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : moduleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && ourWindow->animation == 0) ourWindow->animation = 0.2f;
				else if (!ourWindow->isExtended && ourWindow->animation == 1) ourWindow->animation = 0;
				ourWindow->isInAnimation = true;

				for (auto& mod : moduleList) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		// Draw category header
		{
			// Draw Text
			string textStr = categoryName;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255), textSize);
			DrawUtils::fillRectangle(rectPos, MC_Color(0, 0, 0), 0.5f);
			// Draw Dash
			GuiUtils::drawCrossLine(vec2_t(
				currentXOffset + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
				categoryHeaderYOffset + textPadding + (textHeight / 2)),
				MC_Color(255, 255, 255), crossWidth, crossSize, !ourWindow->isExtended);
			//DrawUtils::drawRoundRectangle(rectTest, MC_Color(255, 255, 255), true);
			//DrawUtils::drawRoundRectangle(rectTest2, MC_Color(100, 100, 100), true);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	moduleList.clear();
	DrawUtils::flush();
}
#pragma endregion
#pragma region Packet
void ClickGui::renderPacketCategory(Category category, MC_Color categoryColor) {
	static constexpr float textHeight = textSize * 9.f;
	const char* categoryName = ClickGui::catToName(category);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;

		}
	}
	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset;

	// Get All Modules in our category
	std::vector<std::shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);

	// Get max width of all text
	{
		for (auto& it : ModuleList) {
			std::string label = "-------------";
			windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&label, textSize, Fonts::SMOOTH));
		}
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}

	const float xEnd = currentXOffset + windowSize->x + paddingRight;

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f) ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 0.75f) + 0.5f /*fix flickering related to rounding errors*/;
		int moduleIndex = 0;
		for (auto& mod : moduleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;
			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			if (ourWindow->isInAnimation) { // Estimate, we don't know about module settings yet
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			std::string textStr = mod->getRawModuleName();

			vec2_t textPos = vec2_t(currentXOffset + textPadding + 45, currentYOffset + textPadding + 5);
			vec4_t rectPos = vec4_t(
				currentXOffset, currentYOffset, xEnd,
				currentYOffset + textHeight + (textPadding * 2));

			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			// Background
			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					DrawUtils::fillRectangle(rectPos, MC_Color(25, 25, 25), 0.5);
					std::string tooltip = mod->getTooltip();
					static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();
					if (clickGuiMod->showTooltips && !tooltip.empty()) renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
				else {
					DrawUtils::fillRectangleA(rectPos, mod->isEnabled() ? MC_Color(32, 32, 32, clickGUI->opacity) : MC_Color(8, 8, 8, clickGUI->opacity));
				}
			}

			// Text
			if (allowRender) DrawUtils::drawCenteredString(textPos, &textStr, textSize, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);
			//if (allowRender) DrawUtils::drawText(textPos, &textStr, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);

			// Settings
			{
				std::vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
					}

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset + textPadding + 5, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							string len = "saturation  ";
							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									vec4_t boxPos = vec4_t(
										rectPos.z + textPadding - 15,
										rectPos.y + textPadding + 2,
										rectPos.z + textHeight - textPadding - 15,
										rectPos.y + textHeight - textPadding + 2);

									DrawUtils::drawRectangle(boxPos, MC_Color(255, 255, 255), isFocused ? 1 : 0.8f, 0.5f);

									if (setting->value->_bool) {
										DrawUtils::setColor(28, 107, 201, 1);
										boxPos.x += textPadding;
										boxPos.y += textPadding;
										boxPos.z -= textPadding;
										boxPos.w -= textPadding;
										DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
										DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
									}
								}

								// Text
								{
									// Convert first letter to uppercase for more friendlieness
									char name[0x21]; sprintf_s(name, 0x21, "%s", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									string elTexto = name;

									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140), true, true, true);
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								// Text and background
								float settingStart = currentYOffset;
								{
									char name[0x22]; sprintf_s(name, +"%s:", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

									char name2[0x21]; sprintf_s(name2, 0x21, " %s", i.GetName().c_str()); if (name2[0] != 0) name2[0] = toupper(name2[0]);
									string elTexto2 = name2;

									string elTexto = string(GRAY) + name + string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight - 0.5 + (textPadding * 2);
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true); //enum aids
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char str[10]; sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5, currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5, currentYOffset - textPadding + textHeight + 3.f);
									// Visuals & Logic
									{
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										rect.z = rect.z - 10;
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);
										if (value > maxValue) value = maxValue; value /= maxValue;
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;

										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									// Convert first letter to uppercase for more friendlieness
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10]; sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 15);
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5,
										currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5,
										currentYOffset - textPadding + textHeight + 3.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background

										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);

										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight + (textPadding * 2);
			}
		}

		vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

		if (winRectPos.contains(&mousePos)) {
			if (scrollingDirection > 0 && overflowing) ourWindow->yOffset += scrollingDirection;
			else if (scrollingDirection < 0) ourWindow->yOffset += scrollingDirection;
			scrollingDirection = 0;
			if (ourWindow->yOffset < 0) ourWindow->yOffset = 0;
		}
	}
	DrawUtils::flush();
	// Draw Category Header
	{
		vec2_t textPos = vec2_t(currentXOffset + textPadding, categoryHeaderYOffset + textPadding);
		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin, categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin,
			categoryHeaderYOffset + textHeight + (textPadding * 2));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : moduleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && ourWindow->animation == 0) ourWindow->animation = 0.2f;
				else if (!ourWindow->isExtended && ourWindow->animation == 1) ourWindow->animation = 0;
				ourWindow->isInAnimation = true;

				for (auto& mod : moduleList) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		//Draw a bit more then just the HudEditor button
		/*{
			std::vector<SettingEntry*>* settings = clickGUI->getSettings();
			string textStr = "Packet";
			float textStrLen = DrawUtils::getTextWidth(&string("------------")) - 2.f;
			float textStrLen2 = DrawUtils::getTextWidth(&string("--------------"));
			float stringLen = DrawUtils::getTextWidth(&textStr) + 2;
			vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
			float mid = windowSize2.x / 2 - 20;

			vec4_t rect = vec4_t(mid, 0, mid + textStrLen, 18);
			vec4_t settingsRect = vec4_t(rect.x + stringLen + 3, rect.y + 2, rect.x + stringLen + 17, rect.y + 16);
			vec4_t hudEditor = vec4_t(rect.x + stringLen + 19, rect.y + 2, rect.x + stringLen + 33, rect.y + 16);

			if (hudEditor.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);

			DrawUtils::fillRectangleA(rect, MC_Color(37, 39, 43, 255));
			DrawUtils::fillRectangleA(settingsRect, MC_Color(9, 12, 16, 255));
			DrawUtils::fillRectangleA(hudEditor, MC_Color(15, 20, 26, 255));
			DrawUtils::drawText(vec2_t(rect.x + 3, rect.y + 4), &textStr, MC_Color(255, 255, 255), 1.f, 1.f, true);

			float ourOffset = 17;
			static bool extended = false;

			if (settingsRect.contains(&mousePos) && shouldToggleRightClick) {
				shouldToggleRightClick = false;
				extended = !extended;
			}

			vec4_t idkRect = vec4_t(settingsRect.x, ourOffset, settingsRect.x + textStrLen2, ourOffset + 16);
			for (int t = 0; t < 4; t++)	idkRect.w += ourOffset;

			if (extended) {
				DrawUtils::fillRectangleA(idkRect, MC_Color(45, 45, 45, 255));
				string stringAids;
				string stringAids2;
				if (clickGUI->theme.getSelectedValue() == 0) stringAids = "Theme: Packet";
				if (clickGUI->theme.getSelectedValue() == 1) stringAids = "Theme: Vape";
				if (clickGUI->theme.getSelectedValue() == 2) stringAids = "Theme: Astolfo";

				if (clickGUI->color.getSelectedValue() == 0) stringAids2 = "Color: Rainbow";
				if (clickGUI->color.getSelectedValue() == 1) stringAids2 = "Color: Astolfo";
				if (clickGUI->color.getSelectedValue() == 2) stringAids2 = "Color: Wave";
				if (clickGUI->color.getSelectedValue() == 3) stringAids2 = "Color: RainbowWave";

				vec4_t selectableSurface = vec4_t(settingsRect.x, ourOffset + 2.f, settingsRect.x + textStrLen2, ourOffset + 17.f);
				vec4_t selectableSurface2 = vec4_t(settingsRect.x, ourOffset + 17.f, settingsRect.x + textStrLen2, ourOffset + 37.f);
				DrawUtils::drawText(vec2_t(selectableSurface.x + 2, selectableSurface.y + 3), &stringAids, MC_Color(255, 255, 255), 1.f, 1.f, true);
				DrawUtils::drawText(vec2_t(selectableSurface2.x + 2, selectableSurface2.y + 3), &stringAids2, MC_Color(255, 255, 255), 1.f, 1.f, true);

				if (selectableSurface.contains(&mousePos) && shouldToggleLeftClick) {
					clickGUI->theme.SelectNextValue(false);
					shouldToggleLeftClick = false;
				}
				if (selectableSurface.contains(&mousePos) && shouldToggleRightClick) {
					clickGUI->theme.SelectNextValue(true);
					shouldToggleLeftClick = false;
				}
				if (selectableSurface2.contains(&mousePos) && shouldToggleLeftClick) {
					clickGUI->color.SelectNextValue(false);
					shouldToggleLeftClick = false;
				}
				if (selectableSurface2.contains(&mousePos) && shouldToggleRightClick) {
					clickGUI->color.SelectNextValue(true);
					shouldToggleLeftClick = false;
				}
			}
		}*/

		// Draw category header
		{
			// Draw Text
			string textStr = categoryName;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255), textSize);
			DrawUtils::fillRectangle(rectPos, MC_Color(0, 0, 0), 0.5f);
			DrawUtils::drawRoundRectangle(rectTest, MC_Color(255, 255, 255), true);
			//DrawUtils::drawRoundRectangle(rectTest2, MC_Color(100, 100, 100), true);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	moduleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region TANA
void ClickGui::renderTANACategory(Category category, MC_Color categoryColor) {
	static constexpr float textHeight = textSize * 9.f;
	const char* categoryName = ClickGui::catToName(category);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	float yot = g_Data.getGuiData()->windowSize.x;
	float yot2 = g_Data.getGuiData()->windowSize.y;
	ourWindow->pos.y = 4;
	switch (category) {
	case Category::COMBAT:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 7.5f;
		break;
	case Category::VISUAL:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 4.1f;
		break;
	case Category::MOVEMENT:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 5.6f * 2.f;
		break;
	case Category::PLAYER:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 4.25f * 2.f;
		break;
	case Category::EXPLOIT:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 3.4f * 2.f;
		break;
	case Category::OTHER:
		ourWindow->pos.x = yot / 7.5f;
		ourWindow->pos.y = yot2 / 2.9f * 2.05f;
		break;
	case Category::UNUSED:
		ourWindow->pos.x = yot / 1.6f * 2.2f;
		break;
	case Category::CUSTOM:
		ourWindow->pos.x = yot / 5.6f * 2.f;
		break;

	}
	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset;
	float currentXOffset2 = currentXOffset + 20; //itwas
	currentXOffset2 = yot / 4.25f * 2.f;
	float currentYOffset2 = yot2 / 7.5f; //itwas

	// Get All Modules in our category
	std::vector<std::shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);

	// Get max width of all text
	{
		for (auto& it : ModuleList) {
			std::string label = "-------------";
			windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&label, textSize, Fonts::SMOOTH));
		}
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}

	const float xEnd = currentXOffset2 + windowSize->x + paddingRight;

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f) ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 0.75f) + 0.5f /*fix flickering related to rounding errors*/;
		int moduleIndex = 0;
		for (auto& mod : moduleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;
			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			if (ourWindow->isInAnimation) { // Estimate, we don't know about module settings yet
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			std::string textStr = mod->getRawModuleName();

			vec2_t textPos = vec2_t(currentXOffset2 + textPadding + 45, currentYOffset2 + textPadding + 5);
			vec4_t rectPos = vec4_t(
				currentXOffset2, currentYOffset2, xEnd,
				currentYOffset2 + textHeight + (textPadding * 2));

			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			// Background
			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					DrawUtils::fillRectangle(rectPos, MC_Color(25, 25, 25), 0.5);
					std::string tooltip = mod->getTooltip();
					static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();
					if (clickGuiMod->showTooltips && !tooltip.empty()) renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
				else {
					DrawUtils::fillRectangleA(rectPos, mod->isEnabled() ? MC_Color(32, 32, 32, clickGUI->opacity) : MC_Color(8, 8, 8, clickGUI->opacity));
				}
			}

			// Text
			if (allowRender) DrawUtils::drawCenteredString(textPos, &textStr, textSize, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);
			//if (allowRender) DrawUtils::drawText(textPos, &textStr, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(100, 100, 100), true);

			// Settings
			{
				std::vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
					}
					GuiUtils::drawCrossLine(vec2_t(
						currentXOffset2 + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
						currentYOffset + textPadding + (textHeight / 2)),
						MC_Color(255, 255, 255), crossWidth, crossSize, !clickMod->isExtended);

					currentYOffset2 += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset2 + textPadding + 5, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset2/*itwas*/, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							string len = "saturation  ";
							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									vec4_t boxPos = vec4_t(
										rectPos.z + textPadding - 15,
										rectPos.y + textPadding + 2,
										rectPos.z + textHeight - textPadding - 15,
										rectPos.y + textHeight - textPadding + 2);

									DrawUtils::drawRectangle(boxPos, MC_Color(255, 255, 255), isFocused ? 1 : 0.8f, 0.5f);

									if (setting->value->_bool) {
										DrawUtils::setColor(28, 107, 201, 1);
										boxPos.x += textPadding;
										boxPos.y += textPadding;
										boxPos.z -= textPadding;
										boxPos.w -= textPadding;
										DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
										DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
									}
								}

								// Text
								{
									// Convert first letter to uppercase for more friendlieness
									char name[0x21]; sprintf_s(name, 0x21, "%s", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									string elTexto = name;

									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140), true, true, true);
									currentYOffset2 += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								// Text and background
								float settingStart = currentYOffset;
								{
									char name[0x22]; sprintf_s(name, +"%s:", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

									char name2[0x21]; sprintf_s(name2, 0x21, " %s", i.GetName().c_str()); if (name2[0] != 0) name2[0] = toupper(name2[0]);
									string elTexto2 = name2;

									string elTexto = string(GRAY) + name + string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight - 0.5 + (textPadding * 2);
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true); //enum aids
									GuiUtils::drawCrossLine(vec2_t(
										currentXOffset2 + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
										currentYOffset + textPadding + (textHeight / 2)),
										MC_Color(255, 255, 255), crossWidth, crossSize, !setting->minValue->_bool);
									currentYOffset2 += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset2 += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char str[10]; sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset2 += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset2 + textPadding + 5, currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5, currentYOffset - textPadding + textHeight + 3.f);
									// Visuals & Logic
									{
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										rect.z = rect.z - 10;
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);
										if (value > maxValue) value = maxValue; value /= maxValue;
										const float endXlol = (xEnd - textPadding) - (currentXOffset2 + textPadding + 10);
										value *= endXlol;

										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										/* {
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}*/

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset2 += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									// Convert first letter to uppercase for more friendlieness
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10]; sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									string elTexto = name + text;
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 15);
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset2 += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset2 + textPadding + 5,
										currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5,
										currentYOffset - textPadding + textHeight + 3.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background

										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset2 + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);

										}

										// Drag Logic
										/* {
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}*/

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset2 += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset2 += textHeight + (textPadding * 2);
			}
		}

		vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

		if (winRectPos.contains(&mousePos)) {
			if (scrollingDirection > 0 && overflowing) ourWindow->yOffset += scrollingDirection;
			else if (scrollingDirection < 0) ourWindow->yOffset += scrollingDirection;
			scrollingDirection = 0;
			if (ourWindow->yOffset < 0) ourWindow->yOffset = 0;
		}
	}
	DrawUtils::flush();
	// Draw Category Header
	{
		vec2_t textPos = vec2_t(currentXOffset + textPadding, categoryHeaderYOffset + textPadding);
		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin, categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin,
			categoryHeaderYOffset + textHeight + (textPadding * 2));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : moduleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && ourWindow->animation == 0) ourWindow->animation = 0.2f;
				else if (!ourWindow->isExtended && ourWindow->animation == 1) ourWindow->animation = 0;
				ourWindow->isInAnimation = true;

				for (auto& mod : moduleList) {
					std::shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		/* {
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}*/

		// Draw category header
		{
			// Draw Text
			string textStr = categoryName;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255), textSize);
			DrawUtils::fillRectangle(rectPos, MC_Color(0, 0, 0), 0.5f);
			// Draw Dash
			GuiUtils::drawCrossLine(vec2_t(
				currentXOffset + windowSize->x + paddingRight - (crossSize / 2) - 1.f,
				categoryHeaderYOffset + textPadding + (textHeight / 2)),
				MC_Color(255, 255, 255), crossWidth, crossSize, !ourWindow->isExtended);
			//DrawUtils::drawRoundRectangle(rectTest, MC_Color(255, 255, 255), true);
			//DrawUtils::drawRoundRectangle(rectTest2, MC_Color(100, 100, 100), true);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	moduleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region Vape
void ClickGui::renderVapeCategory(Category category) {
	const char* categoryName = ClickGui::catToName(category);
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();

	const shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		}
	}
	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	float currColor[4];  // ArrayList colors
	currColor[3] = rcolors[3];

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset;

	// Get All Modules in our category
	vector<shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);

	// Get max width of all text

	for (auto& it : ModuleList) {
		string len = "saturation   ";
		windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 20);
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}

	const float xEnd = currentXOffset + windowSize->x + paddingRight;
	const float yEnd = currentYOffset + windowSize->y + paddingRight;

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();

	// Convert mousePos to visual Pos
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;

		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;  // reset scroll
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f)
				ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	// Loop through Modules to display em
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 3.f) + 0.5f /*fix flickering related to rounding errors*/;
		int moduleIndex = 0;
		for (auto& mod : moduleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset)
				continue;
			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			int opacity = 0;
			if (mod->isEnabled()) opacity += 255;
			else opacity -= 0;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave

			if (ourWindow->isInAnimation) {  // Estimate, we don't know about module settings yet
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			string textStr = mod->getRawModuleName();

			vec4_t rectPos = vec4_t(
				currentXOffset,
				currentYOffset,
				xEnd,
				currentYOffset + textHeight + (textPadding * 2));

			string len = "saturatio";
			float lenth = DrawUtils::getTextWidth(&len, textSize) - 3.f;

			vec2_t textPos = vec2_t(
				currentXOffset + lenth,
				currentYOffset + 7);

			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			// Background
			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {  // Is the Mouse hovering above us?
					string tooltip = mod->getTooltip();
					if (clickGUI->showTooltips && !tooltip.empty())
						renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {  // Are we being clicked?
						mod->toggle();
						shouldToggleLeftClick = false;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}
				}
			}

			// Text
			if (allowRender)
			{
				DrawUtils::fillRectangleA(rectPos, mod->isEnabled() ? arrayColor : MC_Color(50, 50, 50, 255));
				DrawUtils::drawCenteredString(textPos, &textStr, textSize, !mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(50, 50, 50, 255), false);
				string len = "saturation    ";
				float lenth = DrawUtils::getTextWidth(&len, textSize) + 19;
				vec4_t rectPos2 = vec4_t(rectPos.x + lenth, rectPos.y, rectPos.x + lenth + 10.5f, rectPos.y + 11.5f);
				DrawUtils::fillRectangleA(rectPos2, MC_Color(35, 35, 35, 255));
			}

			// Settings
			{
				vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}
					string len = "saturation    ";
					float lenth = DrawUtils::getTextWidth(&len, textSize) + 20;
					DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 3.f, rectPos.x + lenth + 6.f, rectPos.y + 4.f), MC_Color(255, 255, 255, 255));
					DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 5.f, rectPos.x + lenth + 6.f, rectPos.y + 6.f), MC_Color(255, 255, 255, 255));
					DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 7.f, rectPos.x + lenth + 6.f, rectPos.y + 8.f), MC_Color(255, 255, 255, 255));

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						string len = "saturation    ";
						float lenth = DrawUtils::getTextWidth(&len, textSize) + 20;
						vec4_t rectPos2 = vec4_t(rectPos.x + lenth, rectPos.y, rectPos.x + lenth + 10.5f, rectPos.y + 11.5f);
						DrawUtils::fillRectangleA(rectPos2, MC_Color(25, 25, 25, 255));
						DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 3.f, rectPos.x + lenth + 6.f, rectPos.y + 4.f), MC_Color(255, 255, 255, 255));
						DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 5.f, rectPos.x + lenth + 6.f, rectPos.y + 6.f), MC_Color(255, 255, 255, 255));
						DrawUtils::fillRectangleA(vec4_t(rectPos.x + lenth + 5, rectPos.y + 7.f, rectPos.x + lenth + 6.f, rectPos.y + 8.f), MC_Color(255, 255, 255, 255));
						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0)
								continue;

							vec2_t textPos = vec2_t(
								currentXOffset + textPadding + 5,
								currentYOffset + textPadding);

							// Incomplete, because we dont know the endY yet
							vec4_t rectPos = vec4_t(
								currentXOffset + 1,
								currentYOffset,
								xEnd - 1,
								0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									vec4_t boxPos = vec4_t(
										rectPos.z + textPadding - 15,
										rectPos.y + textPadding + 2,
										rectPos.z + textHeight - textPadding - 15,
										rectPos.y + textHeight - textPadding + 2);

									DrawUtils::drawRectangle(boxPos, MC_Color(255, 255, 255), isFocused ? 1 : 0.8f, 0.5f);

									if (setting->value->_bool) {
										DrawUtils::setColor(28, 107, 201, 1);
										boxPos.x += textPadding;
										boxPos.y += textPadding;
										boxPos.z -= textPadding;
										boxPos.w -= textPadding;
										DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
										DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
									}
								}

								// Text
								{
									// Convert first letter to uppercase for more friendlieness
									char name[0x21];
									sprintf_s(name, 0x21, "%s", setting->name);
									if (name[0] != 0) name[0] = toupper(name[0]);

									string elTexto = name;
									string len = "saturation   ";
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 17);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140), true, true, true);
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								// Text and background
								float settingStart = currentYOffset;
								{
									char name[0x22];
									sprintf_s(name, +"%s:", setting->name);
									if (name[0] != 0)
										name[0] = toupper(name[0]);

									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

									char name2[0x21];
									sprintf_s(name2, 0x21, " %s", i.GetName().c_str());
									if (name2[0] != 0) name2[0] = toupper(name2[0]);
									string elTexto2 = name2;

									string elTexto = string(GRAY) + name + string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight + (textPadding * 2);
									string len = "saturation   ";
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 17);
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true); //enum aids
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char str[10];
									sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									if (name[0] != 0) name[0] = toupper(name[0]);
									string elTexto = name + text;
									string len = "saturation   ";
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 17);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5,
										currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5,
										currentYOffset - textPadding + textHeight + 3.f);
									// Visuals & Logic
									{
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										rect.z = rect.z - 10;
										//rect.z = rect.z - 10;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background
										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress

										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									// Convert first letter to uppercase for more friendlieness
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									if (name[0] != 0)
										name[0] = toupper(name[0]);
									vec2_t textPos = vec2_t(
										rectPos.x + 10,
										rectPos.y + 10);
									char str[10];
									sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									string elTexto = name + text;
									string len = "saturation   ";
									windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										currentXOffset + textPadding + 5,
										currentYOffset + textPadding + 8.5,
										xEnd - textPadding + 5,
										currentYOffset - textPadding + textHeight + 3.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background

										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue)
											value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);

										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight + (textPadding * 2) + 3.f;
								}
							} break;

							default: {
							} break;//I don't know how to remove this. LOL
							}
						} float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight + (textPadding * 2);
			}


			vec4_t winRectPos = vec4_t(
				xOffset,
				yOffset,
				xEnd,
				currentYOffset);

			if (winRectPos.contains(&mousePos)) {
				if (scrollingDirection > 0 && overflowing) {
					ourWindow->yOffset += scrollingDirection;
				}
				else if (scrollingDirection < 0) {
					ourWindow->yOffset += scrollingDirection;
				}
				scrollingDirection = 0;
				if (ourWindow->yOffset < 0) {
					ourWindow->yOffset = 0;
				}
			}
		}
	}

	DrawUtils::flush();
	// Draw Category Header
	{
		string len = "saturatio";
		float lenth = DrawUtils::getTextWidth(&len, textSize);
		vec2_t textPos = vec2_t(
			currentXOffset + lenth,
			categoryHeaderYOffset + 5);

		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin + 0.5f,
			categoryHeaderYOffset - categoryMargin - 3.f,
			currentXOffset + windowSize->x + paddingRight + categoryMargin - 0.5f,
			categoryHeaderYOffset + textHeight + (textPadding * 2));

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && clickGUI->animation == 0) {
					clickGUI->animation = 0.2f;
				}
				else if (!ourWindow->isExtended && clickGUI->animation == 1)
					clickGUI->animation = 0;
				ourWindow->isInAnimation = true;
				if (clickGUI->sounds) {
					auto player = g_Data.getLocalPlayer();
					PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
					level->playSound("random.click", *player->getPos(), 1, 1);
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {  // WE are being dragged
				if (isLeftClickDown) {                                             // Still dragging
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {  // Stopped dragging
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		// Draw component
		{
			//Draw a bit more then just the HudEditor button
			/* {
				std::vector<SettingEntry*>* settings = clickGUI->getSettings();
				string textStr = "Packet";
				float textStrLen = DrawUtils::getTextWidth(&string("------------")) - 2.f;
				float textStrLen2 = DrawUtils::getTextWidth(&string("--------------"));
				float stringLen = DrawUtils::getTextWidth(&textStr) + 2;
				vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
				float mid = windowSize2.x / 2 - 20;

				vec4_t rect = vec4_t(mid, 0, mid + textStrLen, 18);
				vec4_t settingsRect = vec4_t(rect.x + stringLen + 3, rect.y + 2, rect.x + stringLen + 17, rect.y + 16);
				vec4_t hudEditor = vec4_t(rect.x + stringLen + 19, rect.y + 2, rect.x + stringLen + 33, rect.y + 16);

				if (hudEditor.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);

				DrawUtils::fillRectangleA(rect, MC_Color(37, 39, 43, 255));
				DrawUtils::fillRectangleA(settingsRect, MC_Color(9, 12, 16, 255));
				DrawUtils::fillRectangleA(hudEditor, MC_Color(15, 20, 26, 255));
				DrawUtils::drawText(vec2_t(rect.x + 3, rect.y + 4), &textStr, MC_Color(255, 255, 255), 1.f, 1.f, true);

				float ourOffset = 17;
				static bool extended = false;

				if (settingsRect.contains(&mousePos) && shouldToggleRightClick) {
					shouldToggleRightClick = false;
					extended = !extended;
				}

				vec4_t idkRect = vec4_t(settingsRect.x, ourOffset, settingsRect.x + textStrLen2, ourOffset + 16);
				for (int t = 0; t < 4; t++)	idkRect.w += ourOffset;

				if (extended) {
					DrawUtils::fillRectangleA(idkRect, MC_Color(45, 45, 45, 255));
					string stringAids;
					string stringAids2;
					if (clickGUI->theme.getSelectedValue() == 0) stringAids = "Theme: Packet";
					if (clickGUI->theme.getSelectedValue() == 1) stringAids = "Theme: Vape";
					if (clickGUI->theme.getSelectedValue() == 2) stringAids = "Theme: Astolfo";

					if (clickGUI->color.getSelectedValue() == 0) stringAids2 = "Color: Rainbow";
					if (clickGUI->color.getSelectedValue() == 1) stringAids2 = "Color: Astolfo";
					if (clickGUI->color.getSelectedValue() == 2) stringAids2 = "Color: Wave";
					if (clickGUI->color.getSelectedValue() == 3) stringAids2 = "Color: RainbowWave";

					vec4_t selectableSurface = vec4_t(settingsRect.x, ourOffset + 2.f, settingsRect.x + textStrLen2, ourOffset + 17.f);
					vec4_t selectableSurface2 = vec4_t(settingsRect.x, ourOffset + 17.f, settingsRect.x + textStrLen2, ourOffset + 37.f);
					DrawUtils::drawText(vec2_t(selectableSurface.x + 2, selectableSurface.y + 3), &stringAids, MC_Color(255, 255, 255), 1.f, 1.f, true);
					DrawUtils::drawText(vec2_t(selectableSurface2.x + 2, selectableSurface2.y + 3), &stringAids2, MC_Color(255, 255, 255), 1.f, 1.f, true);

					if (selectableSurface.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->theme.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->theme.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->color.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->color.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
				}
			}*/

			//Draw Config Manager button
			{
				auto configManager = moduleMgr->getModule<ConfigManagerMod>();
				string textStr = "Config Menu";
				float infoTextLength = DrawUtils::getTextWidth(&textStr) + DrawUtils::getTextWidth(&string(""), 0.7) + 2;
				static const float height = (1 + 0.7 * 1) * DrawUtils::getFont(Fonts::SMOOTH)->getLineHeight();
				vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
				constexpr float borderPadding = 1;
				float margin = 2.f;
				vec4_t infoRect = vec4_t(
					windowSize2.x - margin - infoTextLength - 2 - borderPadding * 2,
					windowSize2.y - margin - height - 20,
					windowSize2.x - margin + borderPadding - 2,
					windowSize2.y - margin - 2 - 20);
				vec2_t infoTextPos = vec2_t(infoRect.x + 1.5, infoRect.y + 3);
				if (infoRect.contains(&mousePos) && shouldToggleLeftClick) configManager->setEnabled(true);
				if (infoRect.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);
				if (infoRect.contains(&mousePos)) DrawUtils::fillRoundRectangle(infoRect, MC_Color(64, 64, 64, 10), false);
				else DrawUtils::fillRoundRectangle(infoRect, MC_Color(0, 0, 0, 20), false);
				DrawUtils::fillRoundRectangle(infoRect, MC_Color(0, 0, 0, 20), false);
				DrawUtils::drawText(vec2_t(infoTextPos.x + 2, infoTextPos.y), &textStr, MC_Color(255, 255, 255), 1.f, true);
			}

			string period = ".";
			string textStr = categoryName;
			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * 1); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			DrawUtils::drawCenteredString(textPos, &textStr, textSize, MC_Color(255, 255, 255), false);
			DrawUtils::fillRectangleA(rectPos, MC_Color(35, 35, 35, 255));
			string len = "saturation   ";
			float lenth = DrawUtils::getTextWidth(&len, textSize) + 17;
			vec4_t rectPos2 = vec4_t(rectPos.x + lenth, rectPos.y, rectPos.x + lenth + 10.5f, rectPos.y + 11.5f);
			vec2_t textPosE = vec2_t(rectPos2.x - 5, rectPos2.y + 5);
			string t = "-";
			DrawUtils::drawText(textPosE, &t, MC_Color(255, 255, 255), textSize, true);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) {
			ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		}

		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) {
			ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		}

		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	moduleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region Astolfo
void ClickGui::renderAstolfoCategory(Category category, MC_Color categoryColor, MC_Color categoryColor2) {
	static constexpr float textHeight = textSize * 10.f;
	const char* categoryName = ClickGui::catToName(category);
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;

		}
	}

	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset + 2.f;
	vector<shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);
	string len = "saturation           ";
	if (!clickGUI->cFont) len = "saturation    ";
	windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
	const float xEnd = currentXOffset + windowSize->x + paddingRight;
	const float yEnd = currentYOffset + windowSize->y + paddingRight;
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f)
				ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * ModuleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 3.f) + 0.5f;
		int moduleIndex = 0;
		for (auto& mod : ModuleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave

			auto arrayColor22 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor22 = ColorUtil::rainbowColor(5, 1, 0.7, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor22 = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor22 = ColorUtil::waveColor(clickGUI->r1 - 20, clickGUI->g1 - 20, clickGUI->b1 - 20, clickGUI->r2 - 20, clickGUI->g2 - 20, clickGUI->b2 - 20, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor22 = ColorUtil::RGBWave(clickGUI->r1 - 20, clickGUI->g1 - 20, clickGUI->b1 - 20, clickGUI->r2 - 20, clickGUI->g2 - 20, clickGUI->b2 - 20, ModuleList.size() * moduleIndex * 3); // Wave

			if (clickGUI->categoryColors) arrayColor = categoryColor;
			if (clickGUI->categoryColors) arrayColor22 = categoryColor2;

			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));
			if (ourWindow->isInAnimation) {
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			char name[0x21];
			sprintf_s(name, 0x21, "%s", mod->getRawModuleName());
			string elTexto = name;
			DrawUtils::toLower(elTexto);
			vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec4_t rectPosEnd = vec4_t(xEnd + 10, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec2_t textPos = vec2_t(rectPos.x + 2, currentYOffset + 2);
			vec4_t rectPos2 = vec4_t(currentXOffset + 2.f, yEnd, xEnd - 2.f, currentYOffset + 11.f);
			vec4_t rectPos3 = vec4_t(currentXOffset - 0.5f, yEnd, xEnd + 0.7f, currentYOffset + 12.f);
			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					string tooltip = mod->getTooltip();
					if (clickGUI->showTooltips && !tooltip.empty())
						renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
			}

			if (allowRender)
			{
				shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
				if (!clickMod->isExtended) DrawUtils::fillRectangleA(rectPos, !mod->isEnabled() ? MC_Color(37, 37, 37) : arrayColor);
				DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255));
				string len = "saturation              ";
				float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
				if (!clickGUI->cFont) len = "saturation      ";
				if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
			}

			// Settings
			{
				vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended && !ourWindow->isInAnimation) {

						DrawUtils::fillRectangleA(rectPos, MC_Color(27, 27, 27));

						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset + textPadding + 4, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
								string len = "saturation              ";
								float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
								if (!clickGUI->cFont) len = "saturation      ";
								if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									if (setting->value->_bool) {
										vec4_t boxPos = vec4_t(
											rectPos.x + textPadding + 2.5f, rectPos.y,
											rectPos.z + textHeight - textPadding - 11.5,
											rectPos.y + textHeight + 2.5f);
										DrawUtils::fillRectangle(boxPos, arrayColor, isFocused ? 1 : 0.8f);
									}
								}

								// Text
								{
									char name[0x21];
									sprintf_s(name, 0x21, "%s", setting->name);
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140));
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								float settingStart = currentYOffset;
								{
									char name[0x21];
									sprintf_s(name, +"%s:", setting->name);
									string elTexto = string(GRAY) + name;
									DrawUtils::toLower(elTexto);
									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
									char name2[0x21];
									sprintf_s(name2, 0x21, " %s", i.GetName().c_str());
									string elTexto2 = name2;
									DrawUtils::toUpper(elTexto2);
									float lenth2 = DrawUtils::getTextWidth(&elTexto2, textSize) + 2;
									vec2_t textPos22 = vec2_t(rectPos.z - lenth2, rectPos.y + 2);
									elTexto2 = string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight + 2 + (textPadding * 3);
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f); //enum aids
									DrawUtils::drawRightAlignedString(&elTexto2, rectPos, MC_Color(255, 255, 255), false);
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y + 2);
									char str[10];
									sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									string elTexto = name;
									string texto = str;
									DrawUtils::toLower(elTexto);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f);
									DrawUtils::drawRightAlignedString(&texto, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{

									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 2.5f);
									// Visuals & Logic
									{

										rectPos.w += textHeight;
										rect.z = rect.z - 10;
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));

										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10];
									sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize);
									DrawUtils::drawRightAlignedString(&text, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 4.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);

										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue)
											value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6 - 5;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight;
			}

			vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

			if (winRectPos.contains(&mousePos)) {
				if (scrollingDirection > 0 && overflowing) {
					ourWindow->yOffset += scrollingDirection;
				}
				else if (scrollingDirection < 0) {
					ourWindow->yOffset += scrollingDirection;
				}
				scrollingDirection = 0;
				if (ourWindow->yOffset < 0) {
					ourWindow->yOffset = 0;
				}
			}
		}
	}

	DrawUtils::flush();
	{
		vec2_t textPos = vec2_t(currentXOffset + 2, categoryHeaderYOffset + 2);

		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin + 0.5,
			categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin - 0.5,
			categoryHeaderYOffset + textHeight + (textPadding * 2));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : ModuleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		vec4_t rect = vec4_t(
			currentXOffset + textPadding - 1.5f,
			currentYOffset,
			xEnd - textPadding + 2.5f,
			currentYOffset + 1.5);
		if (!clickGUI->cFont) {
			rect = vec4_t(
				currentXOffset + textPadding - 1.5f,
				currentYOffset,
				xEnd - textPadding + 1.5f,
				currentYOffset + 1.5);
		}
		vec4_t rect2 = vec4_t(
			rect.x,
			rect.y,
			rect.z,
			rect.y + 1.f);

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && clickGUI->animation == 0) {
					clickGUI->animation = 0.2f;
				}
				else if (!ourWindow->isExtended && clickGUI->animation == 1) clickGUI->animation = 0;
				ourWindow->isInAnimation = true;
				if (clickGUI->sounds) {
					auto player = g_Data.getLocalPlayer();
					PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
					level->playSound("random.click", *player->getPos(), 1, 1);
				}

				for (auto& mod : ModuleList) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		{
			//Draw a bit more then just the HudEditor button
			/* {
				std::vector<SettingEntry*>* settings = clickGUI->getSettings();
				string textStr = "Packet";
				float textStrLen = DrawUtils::getTextWidth(&string("------------")) - 2.f;
				float textStrLen2 = DrawUtils::getTextWidth(&string("--------------"));
				float stringLen = DrawUtils::getTextWidth(&textStr) + 2;
				vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
				float mid = windowSize2.x / 2 - 20;

				vec4_t rect = vec4_t(mid, 0, mid + textStrLen, 18);
				vec4_t settingsRect = vec4_t(rect.x + stringLen + 3, rect.y + 2, rect.x + stringLen + 17, rect.y + 16);
				vec4_t hudEditor = vec4_t(rect.x + stringLen + 19, rect.y + 2, rect.x + stringLen + 33, rect.y + 16);

				if (hudEditor.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);

				DrawUtils::fillRectangleA(rect, MC_Color(37, 39, 43, 255));
				DrawUtils::fillRectangleA(settingsRect, MC_Color(9, 12, 16, 255));
				DrawUtils::fillRectangleA(hudEditor, MC_Color(15, 20, 26, 255));
				DrawUtils::drawText(vec2_t(rect.x + 3, rect.y + 4), &textStr, MC_Color(255, 255, 255), 1.f, 1.f, true);

				float ourOffset = 17;
				static bool extended = false;

				if (settingsRect.contains(&mousePos) && shouldToggleRightClick) {
					shouldToggleRightClick = false;
					extended = !extended;
				}

				vec4_t idkRect = vec4_t(settingsRect.x, ourOffset, settingsRect.x + textStrLen2, ourOffset + 16);
				for (int t = 0; t < 4; t++)	idkRect.w += ourOffset;

				if (extended) {
					DrawUtils::fillRectangleA(idkRect, MC_Color(45, 45, 45, 255));
					string stringAids;
					string stringAids2;
					if (clickGUI->theme.getSelectedValue() == 0) stringAids = "Theme: Packet";
					if (clickGUI->theme.getSelectedValue() == 1) stringAids = "Theme: Vape";
					if (clickGUI->theme.getSelectedValue() == 2) stringAids = "Theme: Astolfo";

					if (clickGUI->color.getSelectedValue() == 0) stringAids2 = "Color: Rainbow";
					if (clickGUI->color.getSelectedValue() == 1) stringAids2 = "Color: Astolfo";
					if (clickGUI->color.getSelectedValue() == 2) stringAids2 = "Color: Wave";
					if (clickGUI->color.getSelectedValue() == 3) stringAids2 = "Color: RainbowWave";

					vec4_t selectableSurface = vec4_t(settingsRect.x, ourOffset + 2.f, settingsRect.x + textStrLen2, ourOffset + 17.f);
					vec4_t selectableSurface2 = vec4_t(settingsRect.x, ourOffset + 17.f, settingsRect.x + textStrLen2, ourOffset + 37.f);
					DrawUtils::drawText(vec2_t(selectableSurface.x + 2, selectableSurface.y + 3), &stringAids, MC_Color(255, 255, 255), 1.f, 1.f, true);
					DrawUtils::drawText(vec2_t(selectableSurface2.x + 2, selectableSurface2.y + 3), &stringAids2, MC_Color(255, 255, 255), 1.f, 1.f, true);

					if (selectableSurface.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->theme.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->theme.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->color.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->color.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
				}
			}*/

			int moduleIndex = 0;
			for (auto& mod : ModuleList) moduleIndex++;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->categoryColors) arrayColor = categoryColor;

			auto arrayColor2 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor2 = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor2 = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor2 = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor2 = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->categoryColors) arrayColor2 = categoryColor;

			char name[0x21];
			sprintf_s(name, 0x21, "%s", ClickGui::catToName(category));
			name[0] = tolower(name[0]);
			string textStr = name;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255));
			string len = "saturation             ";
			float lenth = DrawUtils::getTextWidth(&len, textSize) + 22.5f;
			if (!clickGUI->cFont) len = "saturation     ";
			if (!clickGUI->cFont) lenth = DrawUtils::getTextWidth(&len, textSize) + 21.5f;
			DrawUtils::fillRectangleA(rectPos, MC_Color(26, 26, 26, 255));
			DrawUtils::drawRoundRectangle(rectTest2, MC_Color(26, 26, 26), false);
			DrawUtils::drawRoundRectangle(rectTest, arrayColor, false);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) {
			ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		}

		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) {
			ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		}

		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	ModuleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region Tenacity
void ClickGui::renderTenacityCategory(Category category, MC_Color categoryColor, MC_Color categoryColor2) {
	static constexpr float textHeight = textSize * 10.f;
	const char* categoryName = ClickGui::catToName(category);
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;

		}
	}

	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset + 2.f;
	vector<shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);
	string len = "saturation           ";
	if (!clickGUI->cFont) len = "saturation    ";
	windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&len, textSize) + 16);
	const float xEnd = currentXOffset + windowSize->x + paddingRight;
	const float yEnd = currentYOffset + windowSize->y + paddingRight;
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f)
				ourWindow->isInAnimation = false;
		}
	}

	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * ModuleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 3.f) + 0.5f;
		int moduleIndex = 0;
		for (auto& mod : ModuleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave

			auto arrayColor22 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor22 = ColorUtil::rainbowColor(5, 1, 0.7, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor22 = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor22 = ColorUtil::waveColor(clickGUI->r1 - 20, clickGUI->g1 - 20, clickGUI->b1 - 20, clickGUI->r2 - 20, clickGUI->g2 - 20, clickGUI->b2 - 20, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor22 = ColorUtil::RGBWave(clickGUI->r1 - 20, clickGUI->g1 - 20, clickGUI->b1 - 20, clickGUI->r2 - 20, clickGUI->g2 - 20, clickGUI->b2 - 20, ModuleList.size() * moduleIndex * 3); // Wave

			if (clickGUI->categoryColors) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->categoryColors) arrayColor22 = categoryColor2;

			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));
			if (ourWindow->isInAnimation) {
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			char name[0x21];
			sprintf_s(name, 0x21, "%s", mod->getRawModuleName());
			string elTexto = name;
			DrawUtils::toLower(elTexto);
			vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec4_t rectPosEnd = vec4_t(xEnd + 10, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec2_t textPos = vec2_t(rectPos.x + 2, currentYOffset + 2);
			vec4_t rectPos2 = vec4_t(currentXOffset + 2.f, yEnd, xEnd - 2.f, currentYOffset + 11.f);
			vec4_t rectPos3 = vec4_t(currentXOffset - 0.5f, yEnd, xEnd + 0.7f, currentYOffset + 12.f);
			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					string tooltip = mod->getTooltip();
					if (clickGUI->showTooltips && !tooltip.empty())
						renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
			}

			if (allowRender)
			{
				shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
				if (!clickMod->isExtended) DrawUtils::fillRectangleA(rectPos, !mod->isEnabled() ? MC_Color(37, 37, 37) : arrayColor);
				DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255));
				string len = "saturation              ";
				float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
				if (!clickGUI->cFont) len = "saturation      ";
				if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
			}

			// Settings
			{
				vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended && !ourWindow->isInAnimation) {

						DrawUtils::fillRectangleA(rectPos, MC_Color(27, 27, 27));

						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset + textPadding + 4, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
								string len = "saturation              ";
								float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
								if (!clickGUI->cFont) len = "saturation      ";
								if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									if (setting->value->_bool) {
										vec4_t boxPos = vec4_t(
											rectPos.x + textPadding + 2.5f, rectPos.y,
											rectPos.z + textHeight - textPadding - 11.5,
											rectPos.y + textHeight + 2.5f);
										DrawUtils::fillRectangle(boxPos, arrayColor, isFocused ? 1 : 0.8f);
									}
								}

								// Text
								{
									char name[0x21];
									sprintf_s(name, 0x21, "%s", setting->name);
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140));
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								float settingStart = currentYOffset;
								{
									char name[0x21];
									sprintf_s(name, +"%s:", setting->name);
									string elTexto = string(GRAY) + name;
									DrawUtils::toLower(elTexto);
									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
									char name2[0x21];
									sprintf_s(name2, 0x21, " %s", i.GetName().c_str());
									string elTexto2 = name2;
									DrawUtils::toUpper(elTexto2);
									float lenth2 = DrawUtils::getTextWidth(&elTexto2, textSize) + 2;
									vec2_t textPos22 = vec2_t(rectPos.z - lenth2, rectPos.y + 2);
									elTexto2 = string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight + 2 + (textPadding * 3);
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f); //enum aids
									DrawUtils::drawRightAlignedString(&elTexto2, rectPos, MC_Color(255, 255, 255), false);
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y + 2);
									char str[10];
									sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									string elTexto = name;
									string texto = str;
									DrawUtils::toLower(elTexto);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f);
									DrawUtils::drawRightAlignedString(&texto, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{

									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 2.5f);
									// Visuals & Logic
									{

										rectPos.w += textHeight;
										rect.z = rect.z - 10;
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));

										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10];
									sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize);
									DrawUtils::drawRightAlignedString(&text, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, textSize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, textSize) + 17.5f;
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 4.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);

										DrawUtils::fillRectangleA(rectPos, MC_Color(25, 25, 25, 255));
										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue)
											value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6 - 5;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight;
			}

			vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

			if (winRectPos.contains(&mousePos)) {
				if (scrollingDirection > 0 && overflowing) {
					ourWindow->yOffset += scrollingDirection;
				}
				else if (scrollingDirection < 0) {
					ourWindow->yOffset += scrollingDirection;
				}
				scrollingDirection = 0;
				if (ourWindow->yOffset < 0) {
					ourWindow->yOffset = 0;
				}
			}
		}
	}

	DrawUtils::flush();
	{
		vec2_t textPos = vec2_t(currentXOffset + 2, categoryHeaderYOffset + 2);

		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin + 0.5,
			categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin - 0.5,
			categoryHeaderYOffset + textHeight + (textPadding * 3));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : ModuleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		vec4_t rect = vec4_t(
			currentXOffset + textPadding - 1.5f,
			currentYOffset,
			xEnd - textPadding + 2.5f,
			currentYOffset + 1.5);
		if (!clickGUI->cFont) {
			rect = vec4_t(
				currentXOffset + textPadding - 1.5f,
				currentYOffset,
				xEnd - textPadding + 1.5f,
				currentYOffset + 1.5);
		}
		vec4_t rect2 = vec4_t(
			rect.x,
			rect.y,
			rect.z,
			rect.y + 1.f);

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && clickGUI->animation == 0) {
					clickGUI->animation = 0.2f;
				}
				else if (!ourWindow->isExtended && clickGUI->animation == 1) clickGUI->animation = 0;
				ourWindow->isInAnimation = true;
				if (clickGUI->sounds) {
					auto player = g_Data.getLocalPlayer();
					PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
					level->playSound("random.click", *player->getPos(), 1, 1);
				}

				for (auto& mod : ModuleList) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		{
			//Draw a bit more then just the HudEditor button
			/* {
				std::vector<SettingEntry*>* settings = clickGUI->getSettings();
				string textStr = "Packet";
				float textStrLen = DrawUtils::getTextWidth(&string("------------")) - 2.f;
				float textStrLen2 = DrawUtils::getTextWidth(&string("--------------"));
				float stringLen = DrawUtils::getTextWidth(&textStr) + 2;
				vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
				float mid = windowSize2.x / 2 - 20;

				vec4_t rect = vec4_t(mid, 0, mid + textStrLen, 18);
				vec4_t settingsRect = vec4_t(rect.x + stringLen + 3, rect.y + 2, rect.x + stringLen + 17, rect.y + 16);
				vec4_t hudEditor = vec4_t(rect.x + stringLen + 19, rect.y + 2, rect.x + stringLen + 33, rect.y + 16);

				if (hudEditor.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);

				DrawUtils::fillRectangleA(rect, MC_Color(37, 39, 43, 255));
				DrawUtils::fillRectangleA(settingsRect, MC_Color(9, 12, 16, 255));
				DrawUtils::fillRectangleA(hudEditor, MC_Color(15, 20, 26, 255));
				DrawUtils::drawText(vec2_t(rect.x + 3, rect.y + 4), &textStr, MC_Color(255, 255, 255), 1.f, 1.f, true);

				float ourOffset = 17;
				static bool extended = false;

				if (settingsRect.contains(&mousePos) && shouldToggleRightClick) {
					shouldToggleRightClick = false;
					extended = !extended;
				}

				vec4_t idkRect = vec4_t(settingsRect.x, ourOffset, settingsRect.x + textStrLen2, ourOffset + 16);
				for (int t = 0; t < 4; t++)	idkRect.w += ourOffset;

				if (extended) {
					DrawUtils::fillRectangleA(idkRect, MC_Color(45, 45, 45, 255));
					string stringAids;
					string stringAids2;
					if (clickGUI->theme.getSelectedValue() == 0) stringAids = "Theme: Packet";
					if (clickGUI->theme.getSelectedValue() == 1) stringAids = "Theme: Vape";
					if (clickGUI->theme.getSelectedValue() == 2) stringAids = "Theme: Astolfo";

					if (clickGUI->color.getSelectedValue() == 0) stringAids2 = "Color: Rainbow";
					if (clickGUI->color.getSelectedValue() == 1) stringAids2 = "Color: Astolfo";
					if (clickGUI->color.getSelectedValue() == 2) stringAids2 = "Color: Wave";
					if (clickGUI->color.getSelectedValue() == 3) stringAids2 = "Color: RainbowWave";

					vec4_t selectableSurface = vec4_t(settingsRect.x, ourOffset + 2.f, settingsRect.x + textStrLen2, ourOffset + 17.f);
					vec4_t selectableSurface2 = vec4_t(settingsRect.x, ourOffset + 17.f, settingsRect.x + textStrLen2, ourOffset + 37.f);
					DrawUtils::drawText(vec2_t(selectableSurface.x + 2, selectableSurface.y + 3), &stringAids, MC_Color(255, 255, 255), 1.f, 1.f, true);
					DrawUtils::drawText(vec2_t(selectableSurface2.x + 2, selectableSurface2.y + 3), &stringAids2, MC_Color(255, 255, 255), 1.f, 1.f, true);

					if (selectableSurface.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->theme.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->theme.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->color.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->color.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
				}
			}*/

			int moduleIndex = 0;
			for (auto& mod : ModuleList) moduleIndex++;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			//if (clickGUI->categoryColors) arrayColor = categoryColor;
			arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000);

			auto arrayColor2 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor2 = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor2 = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor2 = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor2 = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->categoryColors) arrayColor2 = categoryColor;

			char name[0x21];
			sprintf_s(name, 0x21, "%s", ClickGui::catToName(category));
			name[0] = tolower(name[0]);
			string textStr = name;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255));
			string len = "saturation             ";
			float lenth = DrawUtils::getTextWidth(&len, textSize) + 22.5f;
			if (!clickGUI->cFont) len = "saturation     ";
			if (!clickGUI->cFont) lenth = DrawUtils::getTextWidth(&len, textSize) + 21.5f;
			DrawUtils::fillRectangleA(rectPos, arrayColor); //MC_Color(26, 26, 26, 255)
			DrawUtils::drawRoundRectangle(rectTest2, MC_Color(26, 26, 26), false);
			DrawUtils::drawRoundRectangle(rectTest, arrayColor, false);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) {
			ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		}

		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) {
			ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		}

		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	ModuleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region tenacity
void ClickGui::rendertenaCategory(Category category) {///
	auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	float textHeight = textSize * clickGUI->txtheight;
	const char* categoryName = ClickGui::catToName(category);
	const shared_ptr<ClickWindow> ourWindow = getWindow(categoryName);

	// Reset Windows to pre-set positions to avoid confusion
	if (resetStartPos && ourWindow->pos.x <= 0) {
		float yot = g_Data.getGuiData()->windowSize.x;
		ourWindow->pos.y = 4;
		switch (category) {
		case Category::COMBAT:
			ourWindow->pos.x = yot / 7.5f;
			break;
		case Category::VISUAL:
			ourWindow->pos.x = yot / 4.1f;
			break;
		case Category::MOVEMENT:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;
		case Category::PLAYER:
			ourWindow->pos.x = yot / 4.25f * 2.f;
			break;
		case Category::EXPLOIT:
			ourWindow->pos.x = yot / 3.4f * 2;
			break;
		case Category::OTHER:
			ourWindow->pos.x = yot / 2.9f * 2.05f;
			break;
		case Category::UNUSED:
			ourWindow->pos.x = yot / 1.6f * 2.2f;
			break;
		case Category::CUSTOM:
			ourWindow->pos.x = yot / 5.6f * 2.f;
			break;

		}
	}

	if (clickGUI->openAnim < 27) ourWindow->pos.y = clickGUI->openAnim;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t* windowSize = &ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset + 2.f;
	vector<shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);
	string len = "saturation           ";
	if (!clickGUI->cFont) len = "saturation    ";



	const float xEnd = currentXOffset + windowSize->x + paddingRight;
	const float yEnd = currentYOffset + windowSize->y + paddingRight;
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f)
				ourWindow->isInAnimation = false;
		}
	}
	currentYOffset += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= clickGUI->animation * ModuleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf(g_Data.getGuiData()->heightGame * 3.f) + 0.5f;
		int moduleIndex = 0;
		for (auto& mod : ModuleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * 1); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			//	if (clickGUI->color.getSelectedValue() == 4) arrayColor = ColorUtil::asxRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors
				//if (clickGUI->color.getSelectedValue() == 5) arrayColor = ColorUtil::WeatherRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors

			auto arrayColor22 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * 1); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * 1 * 3); // Wave
			//	if (clickGUI->color.getSelectedValue() == 4) arrayColor = ColorUtil::asxRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors
			//	if (clickGUI->color.getSelectedValue() == 5) arrayColor = ColorUtil::WeatherRainbow(ModuleList.size() * 1 / 2, 1000); // Astolfo Colors


			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));
			if (ourWindow->isInAnimation) {
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > g_Data.getGuiData()->heightGame - 5) {
				overflowing = true;
				break;
			}

			char name[0x21];
			sprintf_s(name, 0x21, "%s", mod->getRawModuleName());
			string elTexto = name;
			DrawUtils::toLower(elTexto);
			vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec4_t rectPosEnd = vec4_t(xEnd + 10, currentYOffset, xEnd, currentYOffset + textHeight + (textPadding * 2));
			vec2_t textPos = vec2_t(currentXOffset + textPadding + 45, currentYOffset + textPadding + 5);
			vec4_t rectPos2 = vec4_t(currentXOffset + 2.f, yEnd, xEnd - 2.f, currentYOffset + 11.f);
			vec4_t rectPos3 = vec4_t(currentXOffset - 0.5f, yEnd, xEnd + 0.7f, currentYOffset + 12.f);
			std::string textStr = mod->getRawModuleName();
			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					string tooltip = mod->getTooltip();
					if (clickGUI->showTooltips && !tooltip.empty())
						renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}
			}

			if (allowRender)
			{
				shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
				if (!clickMod->isExtended) DrawUtils::fillRectangleA(rectPos, !mod->isEnabled() ? MC_Color(57, 57, 57) : arrayColor);
				if (allowRender) DrawUtils::drawCenteredString(textPos, &textStr, clickGUI->txtsize, mod->isEnabled() ? MC_Color(255, 255, 255) : MC_Color(255, 255, 255), false);
				string len = "saturation              ";
				float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
				if (!clickGUI->cFont) len = "saturation      ";
				if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
			}

			// Settings
			{
				vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}

					currentYOffset += textHeight + (textPadding * 2);

					if (clickMod->isExtended && !ourWindow->isInAnimation) {

						DrawUtils::fillRectangleA(rectPos, MC_Color(27, 27, 27));

						float startYOffset = currentYOffset;
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(currentXOffset + textPadding + 4, currentYOffset + textPadding);
							vec4_t rectPos = vec4_t(currentXOffset, currentYOffset, xEnd, 0);

							if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));
								string len = "saturation              ";
								float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
								if (!clickGUI->cFont) len = "saturation      ";
								if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									if (setting->value->_bool) {
										vec4_t boxPos = vec4_t(
											rectPos.x + textPadding + 2.5f, rectPos.y,
											rectPos.z + textHeight - textPadding - 11.5,
											rectPos.y + textHeight + 2.5f);
										DrawUtils::fillRectangle(boxPos, arrayColor, isFocused ? 1 : 0.8f);
									}
								}

								// Text
								{
									char name[0x21];
									sprintf_s(name, 0x21, "%s", setting->name);
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140));
									currentYOffset += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								float settingStart = currentYOffset;
								{
									char name[0x21];
									sprintf_s(name, +"%s:", setting->name);
									string elTexto = string(GRAY) + name;
									DrawUtils::toLower(elTexto);
									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
									char name2[0x21];
									sprintf_s(name2, 0x21, " %s", i.GetName().c_str());
									string elTexto2 = name2;
									DrawUtils::toUpper(elTexto2);
									float lenth2 = DrawUtils::getTextWidth(&elTexto2, clickGUI->txtsize) + 2;
									vec2_t textPos22 = vec2_t(rectPos.z - lenth2, rectPos.y + 2);
									elTexto2 = string(RESET) + elTexto2;
									rectPos.w = currentYOffset + textHeight + 2 + (textPadding * 3);
									DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize, 1.f); //enum aids
									DrawUtils::drawRightAlignedString(&elTexto2, rectPos, MC_Color(255, 255, 255), false);
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
									currentYOffset += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								bool isEven = e % 2 == 0;

								rectPos.w = currentYOffset + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = currentYOffset * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								currentYOffset += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y + 2);
									char str[10];
									sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									string elTexto = name;
									string texto = str;
									DrawUtils::toLower(elTexto);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize, 1.f);
									DrawUtils::drawRightAlignedString(&texto, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 9;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
								}

								if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{

									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 2.5f);
									// Visuals & Logic
									{

										rectPos.w += textHeight;
										rect.z = rect.z - 10;
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));

										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 10);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22];
									sprintf_s(name, "%s: ", setting->name);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10];
									sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									string elTexto = name;
									DrawUtils::toLower(elTexto);
									textPos2.x -= DrawUtils::getTextWidth(&text, clickGUI->txtsize) / 2;
									textPos2.y += 2.5f;
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize);
									DrawUtils::drawRightAlignedString(&text, rectPos, MC_Color(255, 255, 255), false);
									currentYOffset += textPadding + textHeight - 7;
									rectPos.w = currentYOffset;
									DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));
									string len = "saturation              ";
									float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
									if (!clickGUI->cFont) len = "saturation      ";
									if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
								}
								if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										rectPos.x + textPadding + 2.5f,
										rectPos.y,
										rectPos.z + textHeight - textPadding - 1.5f,
										rectPos.y + textHeight + 4.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = currentYOffset;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);

										DrawUtils::fillRectangleA(rectPos, MC_Color(55, 55, 55, 255));
										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue)
											value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value + 6 - 5;
											DrawUtils::fillRectangle(rect, arrayColor22, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, arrayColor, (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									currentYOffset += textHeight;
								}
							} break;
							}
						}
						float endYOffset = currentYOffset;
					}
				}
				else currentYOffset += textHeight;
			}

			vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd, currentYOffset);

			if (winRectPos.contains(&mousePos)) {
				if (scrollingDirection > 0 && overflowing) {
					ourWindow->yOffset += scrollingDirection;
				}
				else if (scrollingDirection < 0) {
					ourWindow->yOffset += scrollingDirection;
				}
				scrollingDirection = 0;
				if (ourWindow->yOffset < 0) {
					ourWindow->yOffset = 0;
				}
			}
		}
	}

	DrawUtils::flush();
	{
		vec2_t textPos = vec2_t(currentXOffset + 2, categoryHeaderYOffset + 2);

		vec4_t rectPos = vec4_t(
			currentXOffset - categoryMargin + 0.5,
			categoryHeaderYOffset - categoryMargin,
			currentXOffset + windowSize->x + paddingRight + categoryMargin - 0.5,
			categoryHeaderYOffset + textHeight + (textPadding * 2));
		vec4_t rectTest = vec4_t(rectPos.x, rectPos.y + 1, rectPos.z, rectPos.w);
		vec4_t rectTest2 = vec4_t(rectPos.x + 1.f, rectPos.y + 2, rectPos.z - 1.f, rectPos.w);

		for (auto& mod : ModuleList) {
			rectTest.w = currentYOffset - 1;
			rectTest2.w = currentYOffset - 2;
		}

		vec4_t rect = vec4_t(
			currentXOffset + textPadding - 1.5f,
			currentYOffset,
			xEnd - textPadding + 2.5f,
			currentYOffset + 1.5);
		if (!clickGUI->cFont) {
			rect = vec4_t(
				currentXOffset + textPadding - 1.5f,
				currentYOffset,
				xEnd - textPadding + 1.5f,
				currentYOffset + 1.5);
		}
		vec4_t rect2 = vec4_t(
			rect.x,
			rect.y,
			rect.z,
			rect.y + 1.f);

		// Extend Logic
		{
			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !isDragging) {
				shouldToggleRightClick = false;
				ourWindow->isExtended = !ourWindow->isExtended;
				if (ourWindow->isExtended && clickGUI->animation == 0) {
					clickGUI->animation = 0.2f;
				}
				else if (!ourWindow->isExtended && clickGUI->animation == 1) clickGUI->animation = 0;
				ourWindow->isInAnimation = true;
				if (clickGUI->sounds) {
					auto player = g_Data.getLocalPlayer();
					PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
					level->playSound("random.click", *player->getPos(), 1, 1);
				}

				for (auto& mod : ModuleList) {
					shared_ptr<ClickModule> clickMod = getClickModule(ourWindow, mod->getRawModuleName());
					clickMod->isExtended = false;
				}
			}
		}

		// Dragging Logic
		{
			if (isDragging && Utils::getCrcHash(categoryName) == draggedWindow) {
				if (isLeftClickDown) {
					vec2_t diff = vec2_t(mousePos).sub(dragStart);
					ourWindow->pos = ourWindow->pos.add(diff);
					dragStart = mousePos;
				}
				else {
					isDragging = false;
				}
			}
			else if (rectPos.contains(&mousePos) && shouldToggleLeftClick) {
				isDragging = true;
				draggedWindow = Utils::getCrcHash(categoryName);
				shouldToggleLeftClick = false;
				dragStart = mousePos;
			}
		}

		{
			//Draw a bit more then just the HudEditor button
			{
				std::vector<SettingEntry*>* settings = clickGUI->getSettings();
				string textStr = "Actinium";
				float textStrLen = DrawUtils::getTextWidth(&string("------------")) - 2.f;
				float textStrLen2 = DrawUtils::getTextWidth(&string("--------------"));
				float stringLen = DrawUtils::getTextWidth(&textStr) + 2;
				vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
				float mid = windowSize2.x / 2 - 20;

				vec4_t rect = vec4_t(mid, 0, mid + textStrLen, 18);
				vec4_t settingsRect = vec4_t(rect.x + stringLen + 3, rect.y + 2, rect.x + stringLen + 17, rect.y + 16);
				vec4_t hudEditor = vec4_t(rect.x + stringLen + 19, rect.y + 2, rect.x + stringLen + 33, rect.y + 16);

				if (hudEditor.contains(&mousePos) && shouldToggleLeftClick) clickGUI->setEnabled(false);

				//DrawUtils::fillRectangleA(rect, MC_Color(37, 39, 43, 255));
				//DrawUtils::fillRectangleA(settingsRect, MC_Color(9, 12, 16, 255));
			//	DrawUtils::fillRectangleA(hudEditor, MC_Color(15, 20, 26, 255));
				DrawUtils::drawText(vec2_t(rect.x + 3, rect.y + 4), &textStr, MC_Color(255, 255, 255), 1.f, 1.f, true);

				float ourOffset = 17;
				static bool extended = false;

				if (settingsRect.contains(&mousePos) && shouldToggleRightClick) {
					shouldToggleRightClick = false;
					extended = !extended;
				}

				vec4_t idkRect = vec4_t(settingsRect.x, ourOffset, settingsRect.x + textStrLen2, ourOffset + 16);
				for (int t = 0; t < 4; t++)	idkRect.w += ourOffset;

				if (extended) {
					DrawUtils::fillRectangleA(idkRect, MC_Color(45, 45, 45, 255));
					string stringAids;
					string stringAids2;
					if (clickGUI->theme.getSelectedValue() == 0) stringAids = "Theme: Packet";
					if (clickGUI->theme.getSelectedValue() == 1) stringAids = "Theme: Vape";
					if (clickGUI->theme.getSelectedValue() == 2) stringAids = "Theme: Astolfo";

					if (clickGUI->color.getSelectedValue() == 0) stringAids2 = "Color: Rainbow";
					if (clickGUI->color.getSelectedValue() == 1) stringAids2 = "Color: Astolfo";
					if (clickGUI->color.getSelectedValue() == 2) stringAids2 = "Color: Wave";
					if (clickGUI->color.getSelectedValue() == 3) stringAids2 = "Color: RainbowWave";

					vec4_t selectableSurface = vec4_t(settingsRect.x, ourOffset + 2.f, settingsRect.x + textStrLen2, ourOffset + 17.f);
					vec4_t selectableSurface2 = vec4_t(settingsRect.x, ourOffset + 17.f, settingsRect.x + textStrLen2, ourOffset + 37.f);
					DrawUtils::drawText(vec2_t(selectableSurface.x + 2, selectableSurface.y + 3), &stringAids, MC_Color(255, 255, 255), 1.f, 1.f, true);
					DrawUtils::drawText(vec2_t(selectableSurface2.x + 2, selectableSurface2.y + 3), &stringAids2, MC_Color(255, 255, 255), 1.f, 1.f, true);

					if (selectableSurface.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->theme.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->theme.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleLeftClick) {
						clickGUI->color.SelectNextValue(false);
						shouldToggleLeftClick = false;
					}
					if (selectableSurface2.contains(&mousePos) && shouldToggleRightClick) {
						clickGUI->color.SelectNextValue(true);
						shouldToggleLeftClick = false;
					}
				}
			}

			int moduleIndex = 0;
			for (auto& mod : ModuleList) moduleIndex++;

			auto arrayColor = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave

			auto arrayColor2 = ColorUtil::rainbowColor(8, 1.F, 1.F, 1);
			if (clickGUI->color.getSelectedValue() == 0) arrayColor2 = ColorUtil::rainbowColor(5, 1, 1, ModuleList.size() * moduleIndex * 2); // Rainbow Colors
			if (clickGUI->color.getSelectedValue() == 1) arrayColor2 = ColorUtil::astolfoRainbow(ModuleList.size() * moduleIndex / 2, 1000); // Astolfo Colors
			if (clickGUI->color.getSelectedValue() == 2) arrayColor2 = ColorUtil::waveColor(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave
			if (clickGUI->color.getSelectedValue() == 3) arrayColor2 = ColorUtil::RGBWave(clickGUI->r1, clickGUI->g1, clickGUI->b1, clickGUI->r2, clickGUI->g2, clickGUI->b2, ModuleList.size() * moduleIndex * 3); // Wave

			char name[0x21];
			sprintf_s(name, 0x21, "%s", ClickGui::catToName(category));
			name[0] = tolower(name[0]);
			string textStr = name;
			DrawUtils::drawText(textPos, &textStr, MC_Color(255, 255, 255));
			string len = "saturation             ";
			float lenth = DrawUtils::getTextWidth(&len, textSize) + 22.5f;
			if (!clickGUI->cFont) len = "saturation     ";
			if (!clickGUI->cFont) lenth = DrawUtils::getTextWidth(&len, textSize) + 21.5f;
			DrawUtils::fillRectangleA(rectPos, MC_Color(arrayColor));
			DrawUtils::drawRoundRectangle(rectTest2, MC_Color(26, 26, 26), false);
			DrawUtils::drawRoundRectangle(rectTest, arrayColor, false);
		}
	}

	// anti idiot
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		if (ourWindow->pos.x + ourWindow->size.x > windowSize.x) {
			ourWindow->pos.x = windowSize.x - ourWindow->size.x;
		}

		if (ourWindow->pos.y + ourWindow->size.y > windowSize.y) {
			ourWindow->pos.y = windowSize.y - ourWindow->size.y;
		}

		ourWindow->pos.x = (float)fmax(0, ourWindow->pos.x);
		ourWindow->pos.y = (float)fmax(0, ourWindow->pos.y);
	}

	ModuleList.clear();
	DrawUtils::flush();
}
#pragma endregion

#pragma region NewGUI

bool combatc = true;
bool renderc = false;
bool movementc = false;
bool playerc = false;
bool exploitc = false;
bool otherc = false;
void ClickGui::renderNewCategory(Category category) {
	static constexpr float textHeight = textSize * 9.f;
	const char* categoryName = ClickGui::catToName(category);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow2> ourWindow = getWindow2(categoryName);

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}
	std::string ltx;
	std::string ctx;
	std::string ptx;
	std::string otx;
	std::string etx;
	std::string rtx;
	std::string mtx;
	float zrs = 25;
	float zrsx = 4;
	static auto clickmod = moduleMgr->getModule<ClickGUIMod>();
	static auto inter = moduleMgr->getModule<Interface>();
	if (inter->Fonts.getSelectedValue() == 1)ltx = ("          Movement");
	else ltx = ("      Movement");
	if (inter->Fonts.getSelectedValue() == 1)mtx = ("       Movement");
	else mtx = ("    Movement");
	if (inter->Fonts.getSelectedValue() == 1)ctx = ("       Combat");
	else ctx = ("    Combat");
	if (inter->Fonts.getSelectedValue() == 1)rtx = ("       Render");
	else rtx = ("    Render");
	if (inter->Fonts.getSelectedValue() == 1)ptx = ("       Player");
	else ptx = ("    Player");
	if (inter->Fonts.getSelectedValue() == 1)otx = ("       Other");
	else otx = ("    Other");
	if (inter->Fonts.getSelectedValue() == 1)etx = ("       Exploit");
	else etx = ("    Exploit");

	float tate = 12;
	float yoko = 5.7;
	float sizejack = 1.2f;
	vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
	vec4_t clickGUIRect = vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 + (35 * yoko), windowSize2.y / 2 + (8 * tate));
	DrawUtils::fillRoundRectangle(clickGUIRect, MC_Color(23, 28, 35), true);
	DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH) - 10, windowSize2.y / 2 + (8 * tate)), MC_Color(18, 20, 25), true);
	DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 + (8 * tate)), MC_Color(18, 20, 25), true);

	MC_Color buttoncolor = MC_Color(8, 158, 140);
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 00 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ctx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 10 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 00 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ctx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 10 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = true;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}

	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 10 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&rtx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 20 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 10 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&rtx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 20 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = true;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 20 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&mtx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 30 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 20 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&mtx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 30 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = true;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 30 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ptx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 40 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 30 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ptx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 40 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = true;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 40 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&etx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 50 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 40 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&etx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 50 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = true;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 50 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&otx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 60 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 50 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&otx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 60 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = true;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	DrawUtils::drawImage("textures/ui/icon_recipe_equipment.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));
	DrawUtils::drawImage("textures/gui/newgui/mob_effects/night_vision_effect.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs + 10), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));
	DrawUtils::drawImage("textures/gui/newgui/mob_effects/speed_effect.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs + 20), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));
	DrawUtils::drawImage("textures/ui/icon_steve.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs + 30), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));
	DrawUtils::drawImage("textures/ui/servers.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs + 40), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));
	DrawUtils::drawImage("textures/ui/worldsIcon.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1.5f, windowSize2.y / 2 - (8 * tate) + -1.5f + zrs + 50), vec2_t(10.f, 10.f), vec2_t(0.f, 0.f), vec2_t(1.f, 1.f));

	//DrawUtils::drawRoundRectangle(clickGUIRect, MC_Color(255, 255, 255), true);
	//DrawUtils::drawRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 + (8 * tate)), MC_Color(255, 255, 255),true);
	zrs = 25;
	zrsx = 4;
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx - 1, windowSize2.y / 2 - (8 * tate) + 05), &string("Actinium"), MC_Color(0, 170, 0), 2.f, 1.f, true);
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 00 + zrs), &string("   Combat"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 10 + zrs), &string("   Render"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 20 + zrs), &string("   Movement"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 30 + zrs), &string("   Player"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 40 + zrs), &string("   Exploit"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 50 + zrs), &string("   Other"), MC_Color(255, 255, 255));
	DrawUtils::flush();
	const float yOffset = windowSize2.y / 2 - (8 * tate);
	const float xOffset = windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH) + 5;
	vec2_t* windowSize = &ourWindow->size;
	ourWindow->pos.y = windowSize2.y / 2 - (8 * tate);
	ourWindow->pos.x = windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH) + 5;

	float surenobugX = xOffset;
	float surenobugY = yOffset;

	// Get All Modules in our category
	std::vector<std::shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(category, &ModuleList);

	// Get max width of all text
	{
		for (auto& it : ModuleList) {
			std::string label = "-------------";
			windowSize->x = fmax(windowSize->x, DrawUtils::getTextWidth(&label, textSize, Fonts::SMOOTH));
		}
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}

	const float xEnd = windowSize2.x / 2 - 20;

	const float saveModuleXEnd = xEnd;

	float categoryHeaderYOffset = surenobugY;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			clickGUI->animation *= 0.85f;
			if (clickGUI->animation < 0.001f) {
				ourWindow->yOffset = 0;
				ourWindow->isInAnimation = false;
			}

		}
		else {
			clickGUI->animation = 1 - ((1 - clickGUI->animation) * 0.85f);
			if (1 - clickGUI->animation < 0.001f) ourWindow->isInAnimation = false;
		}
	}

	surenobugY += textHeight + (textPadding * 2);
	if (ourWindow->isExtended) {
		if (ourWindow->isInAnimation) {
			surenobugY -= clickGUI->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf((windowSize2.y / 2 + (8 * 12) - 17) * 0.7) + 0.5f;
		int moduleIndex = 0;
		for (auto& mod : moduleList) {
			moduleIndex++;
			if (moduleIndex < ourWindow->yOffset) continue;
			float probableYOffset = (moduleIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			if (ourWindow->isInAnimation) { // Estimate, we don't know about module settings yet
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((surenobugY - ourWindow->pos.y) > cutoffHeight || surenobugY > (windowSize2.y / 2 + (8 * tate) - 16)) {
				overflowing = true;
				break;
			}

			std::string textStr = mod->getRawModuleName();

			vec2_t textPos = vec2_t(surenobugX + textPadding, surenobugY + textPadding);
			vec4_t rectPos = vec4_t(
				surenobugX, surenobugY, xEnd * 1.7,
				surenobugY + textHeight + (textPadding * 2) * 5.5);

			float animation = 0.f;

			vec4_t rectPosAnim = vec4_t(
				surenobugX, surenobugY, animation,
				surenobugY + textHeight + (textPadding * 2));

			bool allowRender = surenobugY >= categoryHeaderYOffset;

			// Background
			if (allowRender) {
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();
					//if (clickGuiMod->showTooltips && !tooltip.empty()) renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}

			}
			DrawUtils::fillRoundRectangle(rectPos, MC_Color(18, 20, 25), true);

			// Text
			if (allowRender) {

				vec2_t textPos2 = vec2_t(surenobugX + textPadding, surenobugY + textPadding + 13.5);
				std::string tooltip = mod->getTooltip();
				//				xEnd = fmax(fmax(DrawUtils::getTextWidth(&tooltip, textSize, Fonts::SMOOTH), DrawUtils::getTextWidth(&tooltip, textSize, Fonts::SMOOTH)), fmax(DrawUtils::getTextWidth(&textStr, textSize, Fonts::SMOOTH), DrawUtils::getTextWidth(&textStr, textSize, Fonts::SMOOTH)));
				DrawUtils::drawText(textPos, &textStr, mod->isEnabled() ? MC_Color(buttoncolor) : MC_Color(255, 255, 255), 1.15f, true);
				DrawUtils::drawText(textPos2, &tooltip, MC_Color(100, 100, 100), 1, true);
			}
			float startYsettings = surenobugY;
			int itemIndex = 0;
			surenobugY += textHeight + (textPadding * 2) * 5.5;

			// Settings
			{
				std::vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					std::shared_ptr<ClickModule2> clickMod = getClickModule2(ourWindow, mod->getModuleName());
					float surenobugYfake = surenobugY;
					surenobugYfake += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						for (auto setting : *settings) {
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;
							if ((surenobugYfake - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							string len = "saturation  ";
							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								surenobugYfake += textHeight + (textPadding * 2);
								break;
							}
							case ValueType::ENUM_T: {
								surenobugYfake += textHeight + (textPadding * 2);
								break;
							}
							case ValueType::FLOAT_T: {
								surenobugYfake += textPadding + textHeight - 9;
								surenobugYfake += textHeight + (textPadding * 2) + 3.f;
							} break;
							case ValueType::INT_T: {
								surenobugYfake += textPadding + textHeight - 7;
								surenobugYfake += textHeight + (textPadding * 2) + 3.f;
							} break;
							case ValueType::KEYBIND_T: {
								surenobugYfake += textHeight + (textPadding * 2);
							} break;
							}
							float endYsettings = surenobugYfake;
							DrawUtils::fillRoundRectangle(vec4_t(surenobugX, startYsettings, xEnd * 1.7, endYsettings), MC_Color(18, 20, 25), true);
						}
					}
				}
			}


			// Settings
			{
				std::vector<SettingEntry*>* settings = mod->getSettings();
				if (settings->size() > 2 && allowRender) {
					std::shared_ptr<ClickModule2> clickMod = getClickModule2(ourWindow, mod->getModuleName());
					if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						shouldToggleRightClick = false;
						clickMod->isExtended = !clickMod->isExtended;
					}

					surenobugY += textHeight + (textPadding * 2);

					if (clickMod->isExtended) {
						float startYOffset = surenobugY;
						for (auto setting : *settings) {
							itemIndex++;
							if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0) continue;

							vec2_t textPos = vec2_t(surenobugX + textPadding + 5, surenobugY + textPadding);
							vec4_t rectPos = vec4_t(surenobugX, surenobugY, xEnd, 0);

							if ((surenobugY - ourWindow->pos.y) > cutoffHeight) {
								overflowing = true;
								break;
							}

							string len = "saturation  ";
							switch (setting->valueType) {
							case ValueType::BOOL_T: {
								rectPos.w = surenobugY + textHeight + (textPadding * 2);
								//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								vec4_t selectableSurface = vec4_t(
									rectPos.x + textPadding,
									rectPos.y + textPadding,
									rectPos.z + textHeight - textPadding - 15,
									rectPos.y + textHeight - textPadding);

								bool isFocused = selectableSurface.contains(&mousePos);
								// Logic
								{
									if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->value->_bool = !setting->value->_bool;
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								// Checkbox
								{
									vec4_t boxPos = vec4_t(
										rectPos.z + textPadding - 15,
										rectPos.y + textPadding + 2,
										rectPos.z + textHeight - textPadding - 15,
										rectPos.y + textHeight - textPadding + 2);

									DrawUtils::drawRectangle(boxPos, MC_Color(255, 255, 255), isFocused ? 1 : 0.8f, 0.5f);

									if (setting->value->_bool) {
										DrawUtils::setColor(28, 107, 201, 1);
										boxPos.x += textPadding;
										boxPos.y += textPadding;
										boxPos.z -= textPadding;
										boxPos.w -= textPadding;
										DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
										DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
									}
								}

								// Text
								{
									// Convert first letter to uppercase for more friendlieness
									char name[0x21]; sprintf_s(name, 0x21, "%s", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									string elTexto = name;

									windowSize->x = fmax(xEnd, DrawUtils::getTextWidth(&len, textSize) + 16);
									DrawUtils::drawText(textPos, &elTexto, isFocused || setting->value->_bool ? MC_Color(255, 255, 255) : MC_Color(140, 140, 140), true, true, true);
									surenobugY += textHeight + (textPadding * 2);
								}
								break;
							}
							case ValueType::ENUM_T: {  // Click setting
								// Text and background
								float settingStart = surenobugY;
								{
									char name[0x22]; sprintf_s(name, +"%s:", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);

									EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();

									char name2[0x21]; sprintf_s(name2, 0x21, " %s", i.GetName().c_str()); if (name2[0] != 0) name2[0] = toupper(name2[0]);
									string elTexto2 = name2;

									string elTexto = string(GRAY) + name + string(RESET) + elTexto2;
									rectPos.w = surenobugY + textHeight - 0.5 + (textPadding * 2);
									windowSize->x = fmax(xEnd, DrawUtils::getTextWidth(&len, textSize) + 16);
									//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true); //enum aids
									surenobugY += textHeight + (textPadding * 2 - 11.5);
								}
								int e = 0;

								if ((surenobugY - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}

								bool isEven = e % 2 == 0;

								rectPos.w = surenobugY + textHeight + (textPadding * 2);
								EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
								textPos.y = surenobugY * textPadding;
								vec4_t selectableSurface = vec4_t(textPos.x, textPos.y, xEnd, rectPos.w);
								// logic
								selectableSurface.y = settingStart;
								if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
									if (shouldToggleLeftClick) {
										shouldToggleLeftClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(false);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
									else if (shouldToggleRightClick) {
										shouldToggleRightClick = false;
										((SettingEnum*)setting->extraData)->SelectNextValue(true);
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}
									}
								}
								surenobugY += textHeight + (textPadding * 2);

								break;
							}

							case ValueType::FLOAT_T: {
								// Text and background
								{
									vec2_t textPos = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char str[10]; sprintf_s(str, 10, "%.2f", setting->value->_float);
									string text = str;
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									string elTexto = name + text;
									windowSize->x = fmax(xEnd, DrawUtils::getTextWidth(&len, textSize) + 16);
									textPos.x = rectPos.x + 5;
									DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									surenobugY += textPadding + textHeight - 9;
									rectPos.w = surenobugY;
									//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}

								if ((surenobugY - ourWindow->pos.y) > cutoffHeight) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										surenobugX + textPadding + 5, surenobugY + textPadding + 8.5,
										xEnd - textPadding + 5, surenobugY - textPadding + textHeight + 3.f);
									// Visuals & Logic
									{
										rectPos.y = surenobugY;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										rect.z = rect.z - 10;
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										const float minValue = setting->minValue->_float;
										const float maxValue = setting->maxValue->_float - minValue;
										float value = (float)fmax(0, setting->value->_float - minValue);
										if (value > maxValue) value = maxValue; value /= maxValue;
										const float endXlol = (xEnd - textPadding) - (surenobugX + textPadding + 10);
										value *= endXlol;

										{
											rect.z = rect.x + value;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);
										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_float = value;
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									surenobugY += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::INT_T: {
								// Text and background
								{
									// Convert first letter to uppercase for more friendlieness
									vec2_t textPos2 = vec2_t(rectPos.x + ((rectPos.z - rectPos.x) / 2), rectPos.y);
									char name[0x22]; sprintf_s(name, "%s: ", setting->name); if (name[0] != 0) name[0] = toupper(name[0]);
									vec2_t textPos = vec2_t(rectPos.x + 10, rectPos.y + 10);
									char str[10]; sprintf_s(str, 10, "%i", setting->value->_int);
									string text = str;
									textPos2.x -= DrawUtils::getTextWidth(&text, textSize) / 2;
									textPos2.y += 2.5f;
									string elTexto = name + text;
									windowSize->x = fmax(xEnd, DrawUtils::getTextWidth(&len, textSize) + 15);
									textPos2.x = rectPos.x + 5;
									DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), textSize, 1.f, true);
									surenobugY += textPadding + textHeight - 7;
									rectPos.w = surenobugY;
									//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
								}
								if ((surenobugY - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}
								// Slider
								{
									vec4_t rect = vec4_t(
										surenobugX + textPadding + 5,
										surenobugY + textPadding + 8.5,
										xEnd - textPadding + 5,
										surenobugY - textPadding + textHeight + 3.f);


									// Visuals & Logic
									{
										rect.z = rect.z - 10;
										rectPos.y = surenobugY;
										rectPos.w += textHeight + (textPadding * 2) + 3.f;
										// Background
										const bool areWeFocused = rectPos.contains(&mousePos);
										DrawUtils::fillRectangle(rect, MC_Color(150, 150, 150), 1);
										//DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, clickGUI->opacity));
										DrawUtils::fillRectangle(rect, MC_Color(44, 44, 44), 1);// Background

										const float minValue = (float)setting->minValue->_int;
										const float maxValue = (float)setting->maxValue->_int - minValue;
										float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
										if (value > maxValue) value = maxValue;
										value /= maxValue;  // Value is now in range 0 - 1
										const float endXlol = (xEnd - textPadding) - (surenobugX + textPadding + 5);
										value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
										{
											rect.z = rect.x + value - 5;
											DrawUtils::fillRectangle(rect, MC_Color(255, 255, 255), (areWeFocused || setting->isDragging) ? 1.f : 1.f);

										}

										// Drag Logic
										{
											if (setting->isDragging) {
												if (isLeftClickDown && !isRightClickDown)
													value = mousePos.x - rect.x;
												else
													setting->isDragging = false;
											}
											else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
												shouldToggleLeftClick = false;
												setting->isDragging = true;
												if (clickGUI->sounds) {
													auto player = g_Data.getLocalPlayer();
													PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
													level->playSound("random.click", *player->getPos(), 1, 1);
												}
											}
										}

										// Save Value
										{
											value /= endXlol;  // Now in range 0 - 1
											value *= maxValue;
											value += minValue;

											setting->value->_int = (int)roundf(value);
											setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
										}
									}
									surenobugY += textHeight + (textPadding * 2) + 3.f;
								}
							} break;
							case ValueType::KEYBIND_T: {

								rectPos.w = surenobugY + textHeight + (textPadding * 2);
								if (!isCapturingKey || (keybindMenuCurrent != setting && isCapturingKey)) {
									char name[0x21];
									sprintf_s(name, 0x21, "%s:", setting->name);
									if (name[0] != 0)
										name[0] = toupper(name[0]);

									std::string text = name;

									DrawUtils::drawText(textPos, &text, MC_Color(255, 255, 255), textSize);

									const char* key;

									if (setting->value->_int > 0 && setting->value->_int < 190)
										key = KeyNames[setting->value->_int];
									else if (setting->value->_int == 0x0)
										key = "NONE";
									else
										key = "???";

									if (keybindMenuCurrent == setting && isCapturingKey) {
										key = "...";
									}
									else if (keybindMenuCurrent == setting && isConfirmingKey) {
										if (newKeybind > 0 && newKeybind < 190)
											key = KeyNames[newKeybind];
										else if (newKeybind == 0x0)
											key = "N/A";
										else
											key = "???";
									}

									std::string keyb = key;
									float keybSz = textHeight * 0.8f;

									float length = 10.f;  // because we add 5 to text padding + keybind name
									length += DrawUtils::getTextWidth(&text, textSize);
									length += DrawUtils::getTextWidth(&keyb, textSize);

									//windowSize.x = fmax(windowSize.x, length + offset);

									DrawUtils::drawText(textPos, &text, MC_Color(255, 255, 255), textSize);

									vec2_t textPos2(rectPos.z - 5.f, textPos.y);
									textPos2.x -= DrawUtils::getTextWidth(&keyb, textSize);

									DrawUtils::drawText(textPos2, &keyb, MC_Color(((0.f + 0.f * 1.5f + 0.f) / 3 > 128) ? MC_Color(0, 0, 0) : MC_Color(100, 100, 100)), textSize);
								}
								else {
									std::string text = "Press new bind...";
									//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&text, textSize));

									DrawUtils::drawText(textPos, &text, MC_Color(1.f, 1.f, 1.f), textSize);
								}

								//DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);

								if ((surenobugY - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
									overflowing = true;
									break;
								}

								// Logic
								{
									bool isFocused = rectPos.contains(&mousePos);

									if (isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting /*don't let the user click other stuff while changing a keybind*/)) {
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}

										keybindMenuCurrent = setting;
										isCapturingKey = true;
									}

									if (isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}

										setting->value->_int = 0x0;  // Clear

										isCapturingKey = false;
									}

									if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
										if (clickGUI->sounds) {
											auto player = g_Data.getLocalPlayer();
											PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
											level->playSound("random.click", *player->getPos(), 1, 1);
										}

										shouldStopCapturing = false;
										isCapturingKey = false;
										setting->value->_int = newKeybind;
									}
								}
								surenobugY += textHeight + (textPadding * 2);
							} break;
							}
						}
						surenobugY += 10;
					}
				}
				else surenobugY += textHeight + (textPadding * 2);
			}
		}

		vec4_t winRectPos = vec4_t(xOffset, yOffset, xEnd * 1.7, surenobugY);

		if (winRectPos.contains(&mousePos)) {
			if (scrollingDirection > 0 && overflowing) ourWindow->yOffset += scrollingDirection;
			else if (scrollingDirection < 0) ourWindow->yOffset += scrollingDirection;
			scrollingDirection = 0;
			if (ourWindow->yOffset < 0) ourWindow->yOffset = 0;
		}
	}


	moduleList.clear();
	DrawUtils::flush();
}

#pragma endregion
/*
bool combatc = true;
bool renderc = false;
bool movementc = false;
bool playerc = false;
bool exploitc = false;
bool otherc = false;
#pragma region NewGUI
void ClickGui::renderNewCategory(Category category) {
	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}
	std::string ltx;
	float zrs = 25;
	float zrsx = 4;
	static auto clickmod = moduleMgr->getModule<ClickGUIMod>();
	if (clickmod->Fonts.getSelectedValue()==1)ltx = ("          Movement");
	else ltx = ("      Movement");
	float tate = 12;
	float yoko = 5.7;
	float sizejack = 1.2f;
	vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;
	vec4_t clickGUIRect = vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 + (35 * yoko), windowSize2.y / 2 + (8 * tate));
	DrawUtils::fillRoundRectangle(clickGUIRect, MC_Color(23, 28, 35), true);
	DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH)-10, windowSize2.y / 2 + (8 * tate)), MC_Color(18, 20, 25), true);
	DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 + (8 * tate)), MC_Color(18, 20, 25), true);

	MC_Color buttoncolor = MC_Color(8, 158, 140);
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 00 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 10 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 00 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 10 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = true;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}

	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 10 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 20 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 10 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 20 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = true;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 20 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 30 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 20 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 30 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = true;
			playerc = false;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 30 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 40 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 30 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 40 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = true;
			exploitc = false;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 40 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 50 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 40 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 50 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = true;
			otherc = false;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	if (vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 50 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 60 + zrs).contains(&mousePos)) {
		DrawUtils::fillRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate) + 50 + zrs, windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 - (8 * tate) + 60 + zrs), buttoncolor, true);
		if (shouldToggleLeftClick) {
			combatc = false;
			renderc = false;
			movementc = false;
			playerc = false;
			exploitc = false;
			otherc = true;
			if (clickmod->sounds) {
				auto player = g_Data.getLocalPlayer();
				PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
				level->playSound("random.click", *player->getPos(), 1, 1);
			}
			shouldToggleLeftClick = false;
		}
	}
	DrawUtils::drawImage("textures/Movement.png", vec2_t(windowSize2.x / 2 - (35 * yoko) + 1, windowSize2.y / 2 - (8 * tate) + -2 + zrs), vec2_t(10.f,10.f), vec2_t(0.f,0.f), vec2_t(1.f, 1.f));

	//DrawUtils::drawRoundRectangle(clickGUIRect, MC_Color(255, 255, 255), true);
	//DrawUtils::drawRoundRectangle(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH), windowSize2.y / 2 + (8 * tate)), MC_Color(255, 255, 255),true);

	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx - 1, windowSize2.y / 2 - (8 * tate) + 05), &string("Radium"), MC_Color(0, 170, 0), 2.f, 1.f, true);
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 00 + zrs), &string("   Combat"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 10 + zrs), &string("   Render"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 20 + zrs), &string("   Movement"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 30 + zrs), &string("   Player"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 40 + zrs), &string("   Exploit"), MC_Color(255, 255, 255));
	DrawUtils::drawText(vec2_t(windowSize2.x / 2 - (35 * yoko) + 1 + zrsx, windowSize2.y / 2 - (8 * tate) + 50 + zrs), &string("   Other"), MC_Color(255, 255, 255));
	DrawUtils::flush();
	static constexpr float textHeight = (textSize * 9.f) * 1.6f;
	const char* categoryName = ClickGui::catToName(category);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow2> ourWindow = getWindow2(categoryName);
	ourWindow->pos.y = windowSize2.y / 2 - (8 * tate);
	ourWindow->pos.x = windowSize2.x / 2 - (35 * yoko) + 1 + DrawUtils::getTextWidth(&ltx, 1.f, Fonts::SMOOTH) + 5;

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;
	vec2_t windowSize = ourWindow->size;
	currentXOffset = xOffset;
	currentYOffset = yOffset;

	struct ClickGuiItem {
		std::shared_ptr<IModule> mod;
		SettingEntry* setting;
		bool isSetting;

		ClickGuiItem(std::shared_ptr<IModule> mod) {
			this->mod = mod;
			isSetting = false;
		}
		ClickGuiItem(SettingEntry* setting) {
			this->setting = setting;
			isSetting = true;
		}
	};
#define TEXTCOLOR (MC_Color(8,158,140))
#define BLACKORWHITE(bright) (MC_Color(bright, bright, bright))

#define FILLCOLOR (MC_Color(0.5f, 0.5f, 0.5f))
#define FILLCOLORDARK (MC_Color(0.5f, 0.5f, 0.5f))
#define FILLCOLORLIGHT (MC_Color(0.5f, 0.5f, 0.5f))

#define FILLCOLORDARK1(a) (MC_Color(0.5f - a, 0.5f - a, 0.5f - a))
#define FILLCOLORLIGHT1(a) (MC_Color(0.5f + a, 0.5f + a, 0.5f + a))

#define FILLCOLORSMORT ((0.2f + 0.2f * 1.5f + 0.2f) / 3 > 128 ? (FILLCOLORDARK) : (FILLCOLORLIGHT))
#define FILLCOLORSMART(a) ((0.2f + 0.2f * 1.5f + 0.2f) / 3 > 128 ? (FILLCOLORDARK1(a)) : (FILLCOLORLIGHT1(a)))

#define OPACITY (0.f)

	// Get All Modules in our category
	std::vector<std::shared_ptr<IModule>> moduleList;
	getModuleListByCategory(category, &moduleList);
	float endXLolFIXed = 0.f;
	// Get max width of all text
	{
		for (auto& it : moduleList) {
			std::string label = "-------------";
			windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&label, textSize * sizejack, Fonts::SMOOTH));
		}
	}


	bool canClick = true;

	{
		bool funny = false;
		for (auto i : windowMap2) {
			if (i.second.get() == ourWindow.get()) {
				funny = true;
				continue;
			}
			if (funny && i.second->selectableSurface.contains(&mousePos))
				canClick = false;
		}
	}

	float categoryHeaderYOffset = currentYOffset;

	if (ourWindow->isInAnimation) {
		if (ourWindow->isExtended) {
			ourWindow->animation *= 0.85f;
			if (ourWindow->animation < 0.001f) {
				ourWindow->yOffset = 0;  // reset scroll
				ourWindow->isInAnimation = false;
			}

		}
		else {
			ourWindow->animation = 1 - ((1 - ourWindow->animation) * 0.85f);
			if (1 - ourWindow->animation < 0.001f)
				ourWindow->isInAnimation = false;
		}
	}
	std::vector<ClickGuiItem> items;

	currentYOffset += textHeight + (textPadding * 2);
	// Loop through Modules to display em
	if (ourWindow->isExtended || ourWindow->isInAnimation) {
		if (ourWindow->isInAnimation) {
			currentYOffset -= ourWindow->animation * moduleList.size() * (textHeight + (textPadding * 2));
		}

		bool overflowing = false;
		const float cutoffHeight = roundf((windowSize2.y / 2 + (8 * tate) - 16) * 0.72f);
		int moduleIndex = 0;

		int itemIndex = 0;

		static auto getSettingSize = [&](SettingEntry* entry) {
			float offset = 2.f * (float)entry->nestValue;

			switch (entry->valueType) {
			case ValueType::BOOL_T: {
				char name[0x21];
				sprintf_s(name, 0x21, "%s", entry->name);
				if (name[0] != 0)
					name[0] = toupper(name[0]);

				std::string elTexto = name;
				return vec2_t(DrawUtils::getTextWidth(&elTexto, textSize) + 10 + offset, 0.f);  // /* because we add 10 to text padding + checkbox
			} break;
			case ValueType::ENUM_SETTING_GROUP_T:
			case ValueType::ENUM_T: {
				float maxLength = 0.f;
				SettingEnum* _enum = (SettingEnum*)entry->extraData;

				// Compute largest enum so that the size doesn't change when you click
				int i = 0;
				for (auto it = _enum->Entrys.begin(); it != _enum->Entrys.end(); it++, i++) {
					maxLength = std::fmaxf(maxLength, DrawUtils::getTextWidth(&it->GetName(), textSize));
				}

				maxLength += 10.f;  // Padding between right side and value + value and name

				char name[0x22];
				sprintf_s(name, "%s:", entry->name);

				if (name[0] != 0x0)
					name[0] = toupper(name[0]);

				std::string text = name;

				maxLength += DrawUtils::getTextWidth(&text, textSize);

				maxLength += offset;

				if (entry->valueType == ValueType::ENUM_SETTING_GROUP_T) {
					for (auto it = _enum->Entrys.begin(); it != _enum->Entrys.end(); it++, i++) {
						std::string text = it->GetName();
						maxLength = fmax(DrawUtils::getTextWidth(&text, textSize) + 7 + offset, maxLength);
					}
				}

				return vec2_t(maxLength, 0.f);
			} break;
			case ValueType::INT_T:
			case ValueType::FLOAT_T: {
				// Convert first letter to uppercase for more friendlieness
				char name[0x22];
				sprintf_s(name, "%s:", entry->name);
				if (name[0] != 0)
					name[0] = toupper(name[0]);

				std::string elTexto = name;
				return vec2_t(DrawUtils::getTextWidth(&elTexto, textSize) + 5 + offset, 0.f);  // /* because we add 5 to text padding
			} break;
			case ValueType::KEYBIND_T: {
				if (!isCapturingKey || (keybindMenuCurrent != entry && isCapturingKey)) {
					char name[0x21];
					sprintf_s(name, 0x21, "%s:", entry->name);
					if (name[0] != 0)
						name[0] = toupper(name[0]);

					std::string text = name;

					const char* key;

					if (entry->value->_int > 0 && entry->value->_int < 190)
						key = KeyNames[entry->value->_int];
					else if (entry->value->_int == 0x0)
						key = "NONE";
					else
						key = "???";

					if (keybindMenuCurrent == entry && isCapturingKey) {
						key = "...";
					}
					else if (keybindMenuCurrent == entry && isConfirmingKey) {
						if (newKeybind > 0 && newKeybind < 190)
							key = KeyNames[newKeybind];
						else if (newKeybind == 0x0)
							key = "N/A";
						else
							key = "???";
					}

					std::string keyb = key;
					float keybSz = textHeight * 0.8f;

					float length = 10.f;  // because we add 5 to text padding + keybind name
					length += DrawUtils::getTextWidth(&text, textSize);
					length += DrawUtils::getTextWidth(&keyb, textSize);

					return vec2_t(length + offset, 0.f);
				}
				else {
					std::string text = "Press new bind...";
					return vec2_t(DrawUtils::getTextWidth(&text, textSize) + offset, 0.f);
				}
			} break;
			default:
				break;
			}

			return vec2_t(0.f, 0.f);
		};


		for (auto it = moduleList.begin(); it != moduleList.end(); ++it) {
			auto mod = *it;
			std::shared_ptr<ClickModule2> clickMod = getClickModule2(ourWindow, mod->getRawModuleName());
			std::vector<SettingEntry*>* settings = mod->getSettings();

			ClickGuiItem module(mod);

			items.push_back(module);

			if (clickMod->isExtended && settings->size() > 2) {
				for (auto it2 = settings->begin(); it2 != settings->end(); ++it2) {
					SettingEntry* setting = *it2;

					if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "Keybind") == 0 || strcmp(setting->name, "Hide in list") == 0)
						continue;

					items.push_back(setting);

					auto newList = setting->getAllExtendedSettings();

					for (auto sett : newList) {
						windowSize.x = fmaxf(getSettingSize(sett).x, windowSize.x);

						items.push_back(sett);
					}

					windowSize.x = fmaxf(getSettingSize(setting).x, windowSize.x);
				}
				SettingEntry* setting = (*settings)[2];
				items.push_back(setting);  // Add the "Visible" setting to the end of the settings.
				windowSize.x = fmaxf(getSettingSize(setting).x, windowSize.x);

				setting = (*settings)[0];
				items.push_back(setting);  // Add our special Keybind setting
				windowSize.x = fmaxf(getSettingSize(setting).x, windowSize.x);
			}
		}

		const float xEnd = currentXOffset + windowSize.x + paddingRight;

		//*
		for (auto& item : items) {
			itemIndex++;

			if (itemIndex < ourWindow->yOffset)
				continue;

			float probableYOffset = (itemIndex - ourWindow->yOffset) * (textHeight + (textPadding * 2));

			if (ourWindow->isInAnimation) {
				if (probableYOffset > cutoffHeight) {
					overflowing = true;
					break;
				}
			}
			else if ((currentYOffset - ourWindow->pos.y) > cutoffHeight || currentYOffset > (windowSize2.y / 2 + (8 * tate) - 16)) {
				overflowing = true;
				break;
			}

			bool allowRender = currentYOffset >= categoryHeaderYOffset;

			if (!item.isSetting) {
				auto mod = item.mod;

				std::shared_ptr<ClickModule2> clickMod = getClickModule2(ourWindow, mod->getRawModuleName());
				std::vector<SettingEntry*>* settings = mod->getSettings();

				std::string textStr = mod->getRawModuleName();

				vec2_t textPos = vec2_t(
					currentXOffset + textPadding,
					currentYOffset + textPadding);

				vec4_t rectPos = vec4_t(
					currentXOffset,
					currentYOffset,
					xEnd * 2,
					(currentYOffset + textHeight + (textPadding * 2) * 2));
				static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();

				if (rectPos.contains(&mousePos)) {  // Is the Mouse hovering above us?
					if (shouldToggleLeftClick) {  // Are we being clicked?
						mod->toggle();
						shouldToggleLeftClick = false;
					}
				}

				// Text
				DrawUtils::drawText(textPos, &textStr, mod->isEnabled() ? TEXTCOLOR : BLACKORWHITE(100), textSize * sizejack);
				DrawUtils::drawText(textPos.add(0, 9), &string(mod->getTooltip()), BLACKORWHITE(120), textSize*0.5);

				DrawUtils::fillRoundRectangle(rectPos, MC_Color(18, 20, 25), true);
				if (rectPos.contains(&mousePos) && shouldToggleRightClick) {
					shouldToggleRightClick = false;
					clickMod->isExtended = !clickMod->isExtended;
				}
			} else {
				auto setting = item.setting;

				float offset = 2.f * (float)setting->nestValue;

				setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();

				if (strcmp(setting->name, "enabled") == 0 || strcmp(setting->name, "keybind") == 0)
					continue;

				vec2_t textPos = vec2_t(
					currentXOffset + textPadding + 5 + offset,
					currentYOffset + textPadding);

				// Incomplete, because we dont know the endY yet
				vec4_t rectPos = vec4_t(
					currentXOffset,
					currentYOffset,
					xEnd,
					0);

				if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
					overflowing = true;
					break;
				}

				switch (setting->valueType) {
				case ValueType::BOOL_T: {
					rectPos.w = currentYOffset + textHeight + (textPadding * 2);
					DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);
					vec4_t selectableSurface = vec4_t(
						textPos.x + textPadding,
						textPos.y + textPadding,
						xEnd - textPadding,
						textPos.y + textHeight - textPadding);

					bool isFocused = selectableSurface.contains(&mousePos) && canClick;
					// Logic
					{
						if (isFocused && shouldToggleLeftClick ) {
							shouldToggleLeftClick = false;
							setting->value->_bool = !setting->value->_bool;
						}
					}
					// Checkbox
					{
						vec4_t boxPos = vec4_t(
							textPos.x + textPadding,
							textPos.y + textPadding,
							textPos.x + textHeight - textPadding,
							textPos.y + textHeight - textPadding);

						DrawUtils::drawRectangle(boxPos, BLACKORWHITE(255), isFocused ? 1 : 0.8f, 0.5f);

						if (setting->value->_bool) {
							auto colCross = BLACKORWHITE(255);
							DrawUtils::setColor(colCross.r, colCross.g, colCross.b, 1);
							boxPos.x += textPadding;
							boxPos.y += textPadding;
							boxPos.z -= textPadding;
							boxPos.w -= textPadding;
							DrawUtils::drawLine(vec2_t(boxPos.x, boxPos.y), vec2_t(boxPos.z, boxPos.w), 0.5f);
							DrawUtils::drawLine(vec2_t(boxPos.z, boxPos.y), vec2_t(boxPos.x, boxPos.w), 0.5f);
						}
					}
					textPos.x += textHeight + (textPadding * 2);
					// Text
					{
						// Convert first letter to uppercase for more friendlieness
						char name[0x21];
						sprintf_s(name, 0x21, "%s", setting->name);
						if (name[0] != 0)
							name[0] = toupper(name[0]);

						std::string elTexto = name;
						////windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&elTexto, textSize) + 10 + offset);  // /* because we add 10 to text padding + checkbox
						DrawUtils::drawText(textPos, &elTexto, isFocused ? TEXTCOLOR : BLACKORWHITE(100), textSize);
					}
					break;
				}
				case ValueType::ENUM_SETTING_GROUP_T:
				case ValueType::ENUM_T: {
					// minValue is whether the enum is expanded
					// value is the actual mode (int)

					SettingEnum* _enum = (SettingEnum*)setting->extraData;
					std::string selected = "";  // We are looping through all the values so we might as well set the text beforehand

					// Text and background
					{
						float maxLength = 0.f;

						// Compute largest enum so that the size doesn't change when you click
						int i = 0;
						for (auto it = _enum->Entrys.begin(); it != _enum->Entrys.end(); it++, i++) {
							if (setting->value->_int == i)
								selected = it->GetName();

							maxLength = std::fmaxf(maxLength, DrawUtils::getTextWidth(&it->GetName(), textSize));
						}

						maxLength += 10.f;  // Padding between right side and value + value and name

						char name[0x22];
						sprintf_s(name, "%s:", setting->name);

						if (name[0] != 0x0)
							name[0] = toupper(name[0]);

						std::string text = name;

						maxLength += DrawUtils::getTextWidth(&text, textSize);

						//windowSize.x = fmax(windowSize.x, maxLength + offset);

						DrawUtils::drawText(textPos, &text, BLACKORWHITE(255), textSize);
						rectPos.w = currentYOffset + textHeight + (textPadding * 2);
						if (setting->minValue->_bool)
							currentYOffset += textHeight + textPadding;
						DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);

						vec2_t textPos2 = textPos;

						float textX = rectPos.z - 5.f;
						textX -= DrawUtils::getTextWidth(&selected, textSize);

						textPos2.x = textX;

						DrawUtils::drawText(textPos2, &selected, BLACKORWHITE(100), textSize);
					}

					// Logic
					{
						bool isFocused = rectPos.contains(&mousePos) && canClick;

						if (setting->groups.empty()) {
							if (isFocused && shouldToggleLeftClick) {
								setting->value->_int = (setting->value->_int + 1) % ((SettingEnum*)setting->extraData)->Entrys.size();
								shouldToggleLeftClick = false;
							}
						}
						else {
							SettingGroup* group = setting->groups[setting->value->_int];

							bool isModsExpanded = false;

							if (group != nullptr)
								isModsExpanded = group->isExpanded;

							if (isFocused && shouldToggleLeftClick) {
								if (!isModsExpanded) {
									setting->minValue->_bool = !setting->minValue->_bool;
								}
								else {
									for (auto g : setting->groups) {
										if (g != nullptr)
											g->isExpanded = false;
									}

									setting->minValue->_bool = true;
								}
								shouldToggleLeftClick = false;
							}

							if (isFocused && shouldToggleRightClick && group != nullptr) {
								setting->minValue->_bool = false;
								bool expand = !group->isExpanded;

								for (auto g : setting->groups) {
									if (g != nullptr)
										g->isExpanded = expand;
								}

								shouldToggleRightClick = false;
							}
						}
					}

					// Drop down menu
					if (setting->minValue->_bool) {
						int i = 0;
						for (auto it = _enum->Entrys.begin(); it != _enum->Entrys.end(); it++, i++) {
							bool highlight = i == setting->value->_int;

							textPos.y += textPadding + textHeight;
							if (it + 1 != _enum->Entrys.end())
								currentYOffset += textHeight + textPadding;

							textPos.x += 2;

							std::string text = it->GetName();
							//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&text, textSize) + 7 + offset);
							DrawUtils::drawText(textPos, &text, highlight ? TEXTCOLOR : BLACKORWHITE(100), textSize);

							rectPos.y += textPadding + textHeight;
							rectPos.w += textPadding + textHeight;

							textPos.x -= 2;

							DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);

							// Logic
							if (rectPos.contains(&mousePos) && canClick && shouldToggleLeftClick) {
								setting->value->_int = i;
								shouldToggleLeftClick = false;
							}
						}
					}
				} break;
				case ValueType::FLOAT_T: {
					// Text and background
					{
						// Convert first letter to uppercase for more friendlieness
						char name[0x22];
						sprintf_s(name, "%s:", setting->name);
						if (name[0] != 0)
							name[0] = toupper(name[0]);

						std::string elTexto = name;
						//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&elTexto, textSize) + 5 + offset);  // /* because we add 5 to text padding
						DrawUtils::drawText(textPos, &elTexto, BLACKORWHITE(255), textSize);
						currentYOffset += textPadding + textHeight;
						rectPos.w = currentYOffset;
						DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);
					}

					if ((currentYOffset - ourWindow->pos.y) > cutoffHeight) {
						overflowing = true;
						break;
					}
					// Slider
					{
						vec4_t rect = vec4_t(
							currentXOffset + textPadding + 5 + offset,
							currentYOffset + textPadding,
							xEnd - textPadding,
							currentYOffset - textPadding + textHeight);

						// Visuals & Logic
						{
							rectPos.y = currentYOffset;
							rectPos.w += textHeight + (textPadding * 2);
							// Background
							const bool areWeFocused = rect.contains(&mousePos) && canClick;

							DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);                    // Background
							DrawUtils::drawRectangle(rect, BLACKORWHITE(255), 1.f, 1.f);  // Slider background

							const float minValue = setting->minValue->_float;
							const float maxValue = setting->maxValue->_float - minValue;
							float value = (float)std::fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
							if (value > maxValue)
								value = maxValue;
							value /= maxValue;  // Value is now in range 0 - 1
							const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
							value *= endXlol;  // Value is now pixel diff between start of bar and end of progress

							// Draw Int
							{
								vec2_t mid = vec2_t(
									rect.x + ((rect.z - rect.x) / 2),
									rect.y - 0.2f);
								char str[10];
								sprintf_s(str, 10, "%.2f", setting->value->_float);
								std::string text = str;
								mid.x -= DrawUtils::getTextWidth(&text, textSize) / 2;

								DrawUtils::drawText(mid, &text, BLACKORWHITE(255), textSize);
							}

							// Draw Progress
							{
								rect.z = rect.x + value;
								DrawUtils::fillRectangle(rect, FILLCOLORSMORT, (areWeFocused || setting->isDragging) ? 1.f : 0.8f);
							}

							// Drag Logic
							{
								if (setting->isDragging) {
									if (isLeftClickDown && !isRightClickDown)
										value = mousePos.x - rect.x;
									else
										setting->isDragging = false;
								}
								else if (areWeFocused && shouldToggleLeftClick) {
									shouldToggleLeftClick = false;
									setting->isDragging = true;
								}
							}

							// Save Value
							{
								value /= endXlol;  // Now in range 0 - 1
								value *= maxValue;
								value += minValue;

								setting->value->_float = value;
								setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
							}
						}
					}
				} break;
				case ValueType::INT_T: {
					// Text and background
					{
						// Convert first letter to uppercase for more friendlieness
						char name[0x22];
						sprintf_s(name, "%s:", setting->name);
						if (name[0] != 0)
							name[0] = toupper(name[0]);

						std::string elTexto = name;
						//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&elTexto, textSize) + 5 + offset);  // /* because we add 5 to text padding
						DrawUtils::drawText(textPos, &elTexto, TEXTCOLOR, textSize);
						currentYOffset += textPadding + textHeight;
						rectPos.w = currentYOffset;
						DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);
					}
					if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
						overflowing = true;
						break;
					}
					// Slider
					{
						vec4_t rect = vec4_t(
							currentXOffset + textPadding + 5 + offset,
							currentYOffset + textPadding,
							xEnd - textPadding,
							currentYOffset - textPadding + textHeight);

						// Visuals & Logic
						{
							rectPos.y = currentYOffset;
							rectPos.w += textHeight + (textPadding * 2);
							// Background
							const bool areWeFocused = rect.contains(&mousePos) && canClick;

							DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);                    // Background
							DrawUtils::drawRectangle(rect, BLACKORWHITE(255), 1.f, 1.f);  // Slider background

							const float minValue = (float)setting->minValue->_int;
							const float maxValue = (float)setting->maxValue->_int - minValue;
							float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
							if (value > maxValue)
								value = maxValue;
							value /= maxValue;  // Value is now in range 0 - 1
							const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
							value *= endXlol;  // Value is now pixel diff between start of bar and end of progress

							// Draw Int
							{
								vec2_t mid = vec2_t(
									rect.x + ((rect.z - rect.x) / 2),
									rect.y - 0.2f  // Hardcoded ghetto
								);
								char str[10];
								sprintf_s(str, 10, "%i", setting->value->_int);
								std::string text = str;
								mid.x -= DrawUtils::getTextWidth(&text, textSize) / 2;

								DrawUtils::drawText(mid, &text, BLACKORWHITE(255), textSize);
							}

							// Draw Progress
							{
								rect.z = rect.x + value;
								DrawUtils::fillRectangle(rect, FILLCOLORSMORT, (areWeFocused || setting->isDragging) ? 1.f : 0.8f);
							}

							// Drag Logic
							{
								if (setting->isDragging) {
									if (isLeftClickDown && !isRightClickDown)
										value = mousePos.x - rect.x;
									else
										setting->isDragging = false;
								}
								else if (areWeFocused && shouldToggleLeftClick) {
									shouldToggleLeftClick = false;
									setting->isDragging = true;
								}
							}

							// Save Value
							{
								value /= endXlol;  // Now in range 0 - 1
								value *= maxValue;
								value += minValue;

								setting->value->_int = (int)roundf(value);
								setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
							}
						}
					}
				} break;
				case ValueType::KEYBIND_T: {
					rectPos.w = currentYOffset + textHeight + (textPadding * 2);
					if (!isCapturingKey || (keybindMenuCurrent != setting && isCapturingKey)) {
						char name[0x21];
						sprintf_s(name, 0x21, "%s:", setting->name);
						if (name[0] != 0)
							name[0] = toupper(name[0]);

						std::string text = name;

						DrawUtils::drawText(textPos, &text, TEXTCOLOR, textSize);

						const char* key;

						if (setting->value->_int > 0 && setting->value->_int < 190)
							key = KeyNames[setting->value->_int];
						else if (setting->value->_int == 0x0)
							key = "NONE";
						else
							key = "???";

						if (keybindMenuCurrent == setting && isCapturingKey) {
							key = "...";
						}
						else if (keybindMenuCurrent == setting && isConfirmingKey) {
							if (newKeybind > 0 && newKeybind < 190)
								key = KeyNames[newKeybind];
							else if (newKeybind == 0x0)
								key = "N/A";
							else
								key = "???";
						}

						std::string keyb = key;
						float keybSz = textHeight * 0.8f;

						float length = 10.f;  // because we add 5 to text padding + keybind name
						length += DrawUtils::getTextWidth(&text, textSize);
						length += DrawUtils::getTextWidth(&keyb, textSize);

						//windowSize.x = fmax(windowSize.x, length + offset);

						DrawUtils::drawText(textPos, &text, BLACKORWHITE(255), textSize);

						vec2_t textPos2(rectPos.z - 5.f, textPos.y);
						textPos2.x -= DrawUtils::getTextWidth(&keyb, textSize);

						DrawUtils::drawText(textPos2, &keyb, BLACKORWHITE(100), textSize);
					}
					else {
						std::string text = "Press new bind...";
						//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&text, textSize));

						DrawUtils::drawText(textPos, &text, BLACKORWHITE(255), textSize);
					}

					DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);

					if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
						overflowing = true;
						break;
					}

					// Logic
					{
						bool isFocused = rectPos.contains(&mousePos) && canClick;

						if (isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting /*don't let the user click other stuff while changing a keybind)) {
							keybindMenuCurrent = setting;
							isCapturingKey = true;
						}

						if (isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
							setting->value->_int = 0x0;  // Clear

							isCapturingKey = false;
						}

						if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
							shouldStopCapturing = false;
							isCapturingKey = false;
							setting->value->_int = newKeybind;
						}
					}
				} break;

					// WIP but I'll keep it here for now
					/*case ValueType::COLOR_PICKER_T: {
						break;  // not implemented yet, gonna work on other stuff
						// TODO: finish this
						{
							// Convert first letter to uppercase for more friendlieness
							char name[0x21];
							sprintf_s(name, 0x21, "%s:", setting->name);
							if (name[0] != 0)
								name[0] = toupper(name[0]);

							std::string elTexto = name;
							//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&elTexto, textSize) + 5);  // /* because we add 5 to text padding
							DrawUtils::drawText(textPos, &elTexto, MC_Color(1.0f, 1.0f, 1.0f), textSize);
							currentYOffset += textPadding + textHeight;
							rectPos.w = currentYOffset;
							DrawUtils::fillRectangle(rectPos, moduleColor, backgroundAlpha);
						}
						if ((currentYOffset - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
							overflowing = true;
							break;
						}

						ColorSettingValue* col = reinterpret_cast<ColorSettingValue*>(setting->value->color);
						MC_Color theColor = col->displayColor;

						// Red slider
						{
							vec4_t rect = vec4_t(
								currentXOffset + textPadding + 5,
								currentYOffset + textPadding,
								xEnd - textPadding,
								currentYOffset - textPadding + textHeight);

							int red = (int)std::floorf(col->displayColor.r * 255);

							// Visuals & Logic
							{
								rectPos.y = currentYOffset;
								rectPos.w += textHeight + (textPadding * 2);
								// Background
								const bool areWeFocused = rect.contains(&mousePos);

								DrawUtils::fillRectangle(rectPos, moduleColor, backgroundAlpha);                   // Background
								DrawUtils::drawRectangle(rect, MC_Color(1.0f, 1.0f, 1.0f), 1.f, backgroundAlpha);  // Slider background

								const float minValue = (float)setting->minValue->_int;
								const float maxValue = (float)setting->maxValue->_int - minValue;
								float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
								if (value > maxValue)
									value = maxValue;
								value /= maxValue;  // Value is now in range 0 - 1
								const float endXlol = (xEnd - textPadding) - (currentXOffset + textPadding + 5);
								value *= endXlol;  // Value is now pixel diff between start of bar and end of progress

								// Draw Int
								{
									vec2_t mid = vec2_t(
										rect.x + ((rect.z - rect.x) / 2),
										rect.y - 0.2f  // Hardcoded ghetto
									);
									char str[10];
									sprintf_s(str, 10, "%i", setting->value->_int);
									std::string text = str;
									mid.x -= DrawUtils::getTextWidth(&text, textSize) / 2;

									DrawUtils::drawText(mid, &text, BLACKORWHITE, textSize);
								}

								// Draw Progress
								{
									rect.z = rect.x + value;
									DrawUtils::fillRectangle(rect, MC_Color(255, 0, 0), (areWeFocused || setting->isDragging) ? 1.f : 0.8f);
								}

								// Drag Logic
								{
									if (setting->isDragging) {
										if (isLeftClickDown && !isRightClickDown)
											value = mousePos.x - rect.x;
										else
											setting->isDragging = false;
									} else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
										shouldToggleLeftClick = false;
										setting->isDragging = true;
									}
								}

								// Save Value
								{
									value /= endXlol;  // Now in range 0 - 1
									value *= maxValue;
									value += minValue;

									setting->value->_int = (int)roundf(value);
									setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
								}
							}
						}
					} break;


				default: {
					char alc[100];
					sprintf_s(alc, 100, "Not implemented (%s)", setting->name);
					std::string elTexto = alc;
					// Adjust window size if our text is too  t h i c c
					//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&elTexto, textSize) + 5 + offset);  // /* because we add 5 to text padding

					DrawUtils::drawText(textPos, &elTexto, MC_Color(255, 0, 0), textSize);
				} break;
				}
			}
			if (!item.isSetting) {
				if (moduleMgr->getModule<ClickGUIMod>()->showTooltips) currentYOffset += (textHeight + (textPadding * 2)) * 2;
				else currentYOffset += textHeight + (textPadding * 2);
			}
			else {
				currentYOffset += textHeight + (textPadding * 2);
				sizejack = 0.f;
			}
		}
		//

		vec4_t winRectPos = vec4_t(
			xOffset,
			yOffset,
			xEnd,
			currentYOffset);

		ourWindow->selectableSurface = winRectPos;

		if (winRectPos.contains(&mousePos)) {
			if (scrollingDirection > 0 && overflowing) {
				ourWindow->yOffset += scrollingDirection;
			}
			else if (scrollingDirection < 0) {
				ourWindow->yOffset += scrollingDirection;
			}
			scrollingDirection = 0;
			if (ourWindow->yOffset < 0) {
				ourWindow->yOffset = 0;
			}
		}
	}

	moduleList.clear();
	DrawUtils::flush();
}*/
#pragma endregion
float spacing2 = 45.f;
void ClickGui::onOpened() {
	selectedCategoryAnimY.reset();
	scrollAnim.reset();
	scrollingDirection = 0;
}
#pragma region newBadlion
int itemIndex = 0;
void ClickGui::renderNewBadLion() {

	static constexpr float textHeight = textSize * 9.f;
	const char* categoryName = catToName(selectedCategory);
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	const std::shared_ptr<ClickWindow2> ourWindow = getWindow2(categoryName);

	if (GameData::isKeyDown(VK_ESCAPE)) clickGUI->isSettingOpened = false;

	vec2_t mousePos = *g_Data.getClientInstance()->getMousePos();
	{
		vec2_t windowSize = g_Data.getClientInstance()->getGuiData()->windowSize;
		vec2_t windowSizeReal = g_Data.getClientInstance()->getGuiData()->windowSizeReal;
		mousePos = mousePos.div(windowSizeReal);
		mousePos = mousePos.mul(windowSize);
	}

	std::string ltx;
	float zrs = 25;
	float zrsx = 4;
	static auto clickmod = moduleMgr->getModule<ClickGUIMod>();

	float tate = 12;
	float yoko = 5.7;
	float sizejack = 1.2f;
	vec2_t windowSize2 = g_Data.getClientInstance()->getGuiData()->windowSize;

	float xEnd = windowSize2.x / 2 - 20;

	vec4_t clickGUIRect = vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 + (35 * yoko), windowSize2.y / 2 + (10 * tate));
	//module
	std::vector<std::shared_ptr<IModule>> ModuleList;
	getModuleListByCategory(selectedCategory, &ModuleList);

	// Get max width of all text
	{
		for (auto& it : ModuleList) {
			std::string label = "-------------";
			windowSize2.x = fmax(windowSize2.x, DrawUtils::getTextWidth(&label, textSize, Fonts::SMOOTH));
		}
	}

	vector<shared_ptr<IModule>> moduleList;
	{
		vector<int> toIgniore;
		int moduleCount = (int)ModuleList.size();
		for (int i = 0; i < moduleCount; i++) {
			float bestWidth = 1.f;
			int bestIndex = 1;

			for (int j = 0; j < ModuleList.size(); j++) {
				bool stop = false;
				for (int bruhwth = 0; bruhwth < toIgniore.size(); bruhwth++)
					if (j == toIgniore[bruhwth]) {
						stop = true;
						break;
					}
				if (stop)
					continue;

				string t = ModuleList[j]->getRawModuleName();
				float textWidthRn = DrawUtils::getTextWidth(&t, textSize, Fonts::SMOOTH);
				if (textWidthRn > bestWidth) {
					bestWidth = textWidthRn;
					bestIndex = j;
				}
			}
			moduleList.push_back(ModuleList[bestIndex]);
			toIgniore.push_back(bestIndex);
		}
	}
	//

	DrawUtils::drawText(vec2_t(clickGUIRect.x + 10, clickGUIRect.y - 20), &string("Actinium"), MC_Color(255, 255, 255), 1.4f, 1.f, true);
	vec2_t textPos = vec2_t(clickGUIRect.x + 92, clickGUIRect.y + 9);

	DrawUtils::fillRectangleA(clickGUIRect, MC_Color(0, 0, 0, 40));
	DrawUtils::fillRectangleA(vec4_t(clickGUIRect.x, clickGUIRect.y - 30, clickGUIRect.z, clickGUIRect.y), MC_Color(0, 0, 0, 90));
	DrawUtils::fillRectangleA(vec4_t(windowSize2.x / 2 - (35 * yoko), windowSize2.y / 2 - (8 * tate), windowSize2.x / 2 - (35 * yoko) + 30 * 2.5f, windowSize2.y / 2 + (10 * tate)), MC_Color(0, 0, 0, 65));

	vec4_t rectPos = vec4_t(
		textPos.x,
		textPos.y,
		textPos.x + 86,
		textPos.y + 86
	);

	const float xOffset = ourWindow->pos.x;
	const float yOffset = ourWindow->pos.y;

	float surenobugX = xOffset;
	float surenobugY = yOffset;

	currentYOffset = yOffset;



	vector<std::string> configList;

	if (scrollingDirection < 0)
		scrollingDirection = 0;
	if (scrollingDirection > configList.size() - 3)
		scrollingDirection = configList.size() - 3;
	int index1 = -1;
	int realIndex1 = -1;

	int index2 = -1;
	int realIndex2 = -1;

	float categoryHeaderYOffset = currentYOffset;

	bool allowRender = currentYOffset >= categoryHeaderYOffset;

	bool overflowing = false;

	int moduleIndex = 0;

	const float cutoffHeight = roundf((windowSize2.y / 2 + (8 * 12) - 17) * 0.7) + 0.5f;

	vector<Category> categories = {
		Category::ALL,
		Category::COMBAT,
		Category::VISUAL,
		Category::MOVEMENT,
		Category::PLAYER,
		Category::EXPLOIT,
		Category::OTHER,
		Category::CONFIG,
	};

	vec2_t categoryPos = vec2_t(windowSize2.x / 2 - (35 * yoko) + 6, windowSize2.y / 2 - (8 * tate) + 6);

	for (auto& cs : categories) {

		vec4_t categoryRect = vec4_t(categoryPos.x - 6, categoryPos.y + 10, categoryPos.x + 69, categoryPos.y + 25);

		const char* categoryName = ClickGui::catToName(cs);

		string cat = categoryName;

		DrawUtils::drawText(vec2_t(categoryPos.x + 1, categoryPos.y + 12), &cat, MC_Color(255, 255, 255), 1.f, 1.f);

		if (categoryRect.contains(&mousePos)) {
			DrawUtils::fillRectangleA(vec4_t(categoryRect), MC_Color(255, 255, 255, 45));
			if (isLeftClickDown && shouldToggleLeftClick) {
				selectedCategory = cs;
				clickGUI->isSettingOpened = false;
				shouldToggleLeftClick = false;
			}
		}

		categoryPos.y += 16.5f;
	}

	for (auto& mod : moduleList) {

		realIndex1++;
		if (realIndex1 < scrollingDirection)
			continue;
		index1++;
		if (index1 >= 10)
			break;

		if (!clickGUI->isSettingOpened) {
			if (rectPos.w <= clickGUIRect.w) {
				std::string moduleName = mod->getRawModuleName();


				DrawUtils::drawCenteredString(vec2_t(rectPos.x + 43, rectPos.y + 17), &moduleName, 0.92f, MC_Color(255, 255, 255, 60), 1.f);
				DrawUtils::fillRectangleA(rectPos, MC_Color(0, 0, 0, 60));
				DrawUtils::drawRectangle(rectPos, MC_Color(96, 104, 119), 0.8f, 0.5f);

				//toggle
				if (!ourWindow->isInAnimation && !isDragging && rectPos.contains(&mousePos)) {
					std::string tooltip = mod->getTooltip();
					static auto clickGuiMod = moduleMgr->getModule<ClickGUIMod>();
					if (clickGuiMod->showTooltips && !tooltip.empty()) renderTooltip(&tooltip);
					if (shouldToggleLeftClick && !ourWindow->isInAnimation) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
						mod->toggle();
						selectedModule = mod;
						shouldToggleLeftClick = false;
					}
				}
				//

				DrawUtils::fillRoundRectangle(vec4_t(rectPos.x + 32, rectPos.y + 65, rectPos.x + 52, rectPos.y + 70), mod->isEnabled() ? MC_Color(44, 173, 220) : MC_Color(2, 15, 14), true);

				DrawUtils::drawCircle(mod->isEnabled() ? vec4_t(rectPos.x + 43, rectPos.y + 65, rectPos.x + 51, rectPos.y + 70) : vec4_t(rectPos.x + 33, rectPos.y + 65, rectPos.x + 41, rectPos.y + 70), MC_Color(255, 255, 255));

			}

			if (rectPos.contains(&mousePos) && shouldToggleRightClick && !ourWindow->isInAnimation) {
				if (clickGUI->sounds) {
					auto player = g_Data.getLocalPlayer();
					PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
					level->playSound("random.click", *player->getPos(), 1, 1);
				}
				shouldToggleRightClick = false;
				selectedModule = mod;
				clickGUI->isSettingOpened = true;
			}

			rectPos.x += spacing2 * 2.2f;
			rectPos.z += spacing2 * 2.2f;

			if (rectPos.z >= clickGUIRect.z) {
				rectPos.x = textPos.x;
				rectPos.z = textPos.x + 86;

				rectPos.y += textPos.y + 11.6f;
				rectPos.w += textPos.y + 11.6f;
			}
		}
	}

	vec4_t settingRect = vec4_t(
		clickGUIRect.x + 75,
		clickGUIRect.y,
		clickGUIRect.z,
		clickGUIRect.w
	);

	auto inter = moduleMgr->getModule<Interface>();

	int indexC = 0;
	indexC++; int curIndex = -indexC * inter->spacing;
	auto interfaceColor = ColorUtil::interfaceColor(curIndex);

	vec2_t configPos = vec2_t(settingRect.x + 12, settingRect.y + 8);

	std::string dir_path = (getenv("AppData") + (std::string)"\\..\\Local\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe\\RoamingState\\PacketSkid\\Configs\\");

	if (selectedCategory == Category::CONFIG) {

		configList.clear();

		for (const auto& entry : std::filesystem::directory_iterator(dir_path))
		{
			if (!entry.is_directory())
			{
				std::string filename = entry.path().filename().string();
				configList.push_back(filename);
			}
		}

		for (auto& config : configList) {
			realIndex1++;
			if (realIndex1 < scrollingDirection)
				continue;
			index1++;
			if (index1 >= 10)
				break;

			DrawUtils::drawText(configPos, &config, MC_Color(255, 255, 255), 1.f, 0.9f, true);
			DrawUtils::drawText(vec2_t(configPos.x + 240, configPos.y), &string("Load"), MC_Color(255, 255, 255), 1.f, 0.83f, true);
			DrawUtils::drawText(vec2_t(configPos.x + 270, configPos.y), &string("Delete"), MC_Color(255, 255, 255), 1.f, 0.83f, true);

			vec4_t loadRect = vec4_t(configPos.x + 237, configPos.y, configPos.x + 264, configPos.y + 9);
			vec4_t delRect = vec4_t(configPos.x + 268, configPos.y, configPos.x + 298, configPos.y + 9);

			if (loadRect.contains(&mousePos) && isLeftClickDown && shouldToggleLeftClick) {

				clickGUI->setEnabled(false);
				g_Data.getClientInstance()->grabMouse();

				std::string filename = config;

				std::regex re("\\.txt$");

				std::string replaced = std::regex_replace(filename, re, "");

				configMgr->loadConfig(replaced, false);
				shouldToggleLeftClick = false;
			}

			if (delRect.contains(&mousePos) && isLeftClickDown && shouldToggleLeftClick) {

				if (configMgr->currentConfig != config) {
					clickGUI->setEnabled(false);
					g_Data.getClientInstance()->grabMouse();

					string filepath = dir_path + config;

					config.pop_back();

					std::filesystem::remove(filepath);

					shouldToggleLeftClick = false;
				}
			}

			configPos.y += 30.f;
		}
	}

	if (clickGUI->isSettingOpened) {

		std::vector<SettingEntry*>* settings = selectedModule->getSettings();
		if (settings->size() > 2 && allowRender) {
			std::shared_ptr<ClickModule2> clickMod = getClickModule2(ourWindow, selectedModule->getModuleName());
		}
		float startYOffset = settingRect.y;

		if (scrollingDirection < 0)
			scrollingDirection = 0;
		if (scrollingDirection > settings->size() - 2)
			scrollingDirection = settings->size() - 2;

		int index3 = -1;
		int realIndex3 = -1;

		for (auto setting : *settings) {
			if (!strcmp(setting->name, "enabled"))
				continue;
			realIndex3++;
			if (realIndex3 < scrollingDirection)
				continue;
			index3++;
			if (index3 >= 10)
				break;

			if (!strcmp(setting->name, "enabled") || strcmp(setting->name, "keybind") == 0) continue;

			vec2_t textPos2 = vec2_t(settingRect.x + 20, settingRect.y + 45);
			vec4_t rectPos2 = vec4_t(settingRect.x + 20, settingRect.y + 45, xEnd + 180, 80);
			vec4_t sliderRect = vec4_t(settingRect.x + 180, settingRect.y + 150, xEnd + 180, 200);

			if ((settingRect.y - ourWindow->pos.y) > cutoffHeight) {
				overflowing = true;
				break;
			}

			DrawUtils::drawText(vec2_t(clickGUIRect.x + 94, clickGUIRect.y + 7), &string(selectedModule->getModuleName()), MC_Color(255, 255, 255), 1.1f, 1.f);
			DrawUtils::drawText(vec2_t(clickGUIRect.x + 94, clickGUIRect.y + 19), &string(string(GRAY) + selectedModule->getTooltip()), MC_Color(255, 255, 255), 0.86f, 1.f);

			DrawUtils::setColor(0.1f, 0.1f, 0.1f, 0.1f);
			DrawUtils::drawLine(vec2_t(clickGUIRect.x + 83, clickGUIRect.y + 29), vec2_t(clickGUIRect.z - 10, clickGUIRect.y + 29), 0.35f);

			vec4_t lineRect = vec4_t(clickGUIRect.x + 80, clickGUIRect.y + 12.5f, clickGUIRect.x + 84, clickGUIRect.y + 12.5f);

			vec4_t exitRect = vec4_t(lineRect.x + 1.95f, lineRect.y - 3.68f, lineRect.x + 7.7f, lineRect.y + 2.7f);

			if (exitRect.contains(&mousePos) && isLeftClickDown && shouldToggleLeftClick) {
				clickGUI->isSettingOpened = false;
				shouldToggleLeftClick = false;
			}

			DrawUtils::drawText(vec2_t(lineRect.x + 2.4f, lineRect.y - 3.48f), &string("<"), exitRect.contains(&mousePos) ? MC_Color(100, 100, 100) : MC_Color(255, 255, 255), 0.76f, 1.f);


			switch (setting->valueType) {

			case ValueType::BOOL_T: {
				rectPos2.w = settingRect.y + textHeight + (textPadding * 2);
				string len = "saturation              ";
				float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
				if (!clickGUI->cFont) len = "saturation      ";
				if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
				vec4_t selectableSurface = vec4_t(
					rectPos2.x + textPadding,
					rectPos2.y + textPadding,
					rectPos2.z + textHeight - textPadding - 15,
					rectPos2.y + textHeight - textPadding);

				vec4_t boxPos = vec4_t(
					rectPos2.x + textPadding + 270.5f,
					textPos2.y - 15,
					rectPos2.z + textHeight - textPadding,
					rectPos2.y + textHeight + 2.5f);

				bool isFocused = selectableSurface.contains(&mousePos);
				bool isFocused2 = vec4_t(boxPos.x, boxPos.y + 15, boxPos.x + 20, boxPos.y + 20).contains(&mousePos);
				// Logic
				{
					if (isFocused && shouldToggleLeftClick && !ourWindow->isInAnimation || isFocused2 && shouldToggleLeftClick && !ourWindow->isInAnimation) {
						shouldToggleLeftClick = false;
						setting->value->_bool = !setting->value->_bool;
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}
				}
				// Checkbox
				{
					DrawUtils::fillRoundRectangle(vec4_t(boxPos.x, boxPos.y + 15, boxPos.x + 20, boxPos.y + 20), setting->value->_bool ? MC_Color(44, 173, 220) : MC_Color(2, 15, 14), true);

					DrawUtils::drawCircle(setting->value->_bool ? vec4_t(boxPos.x + 11, boxPos.y + 15, boxPos.x + 19, boxPos.y + 20) : vec4_t(boxPos.x + 1, boxPos.y + 15, boxPos.x + 9, boxPos.y + 20), MC_Color(255, 255, 255));
				}

				// Text
				{
					char name[0x21];
					sprintf_s(name, 0x21, "%s", setting->name);
					string elTexto = name;
					DrawUtils::toLower(elTexto);
					DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255));
					settingRect.y += textHeight + (textPadding * 10);
				}
				break;
			}
			case ValueType::ENUM_T: {  // Click setting
				float settingStart = settingRect.y;
				{
					char name[0x21];
					sprintf_s(name, +"%s:", setting->name);
					string elTexto = string(GRAY) + name;
					DrawUtils::toLower(elTexto);
					EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
					char name2[0x21];
					sprintf_s(name2, 0x21, " %s", i.GetName().c_str());
					string elTexto2 = name2;
					DrawUtils::toUpper(elTexto2);
					float lenth2 = DrawUtils::getTextWidth(&elTexto2, clickGUI->txtsize) + 2;
					elTexto2 = string(RESET) + elTexto2;
					rectPos2.w = settingRect.y + textHeight + 2 + (textPadding * 3);
					DrawUtils::drawText(textPos2, &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize, 1.f); //enum aids
					DrawUtils::drawRightAlignedString(&elTexto2, rectPos2, MC_Color(255, 255, 255), false);
					string len = "saturation  ";
					float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
					if (!clickGUI->cFont) len = "saturation  ";
					if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
					settingRect.y += textHeight + (textPadding * 2 - 11.5);
				}
				int e = 0;

				bool isEven = e % 2 == 0;

				rectPos2.w = settingRect.y + textHeight + (textPadding * 2);
				EnumEntry& i = ((SettingEnum*)setting->extraData)->GetSelectedEntry();
				textPos2.y = settingRect.y * textPadding;
				vec4_t selectableSurface = vec4_t(settingRect.x + 1, settingRect.y + 47, settingRect.z, settingRect.y + 57);
				// logic

				if (!ourWindow->isInAnimation && selectableSurface.contains(&mousePos)) {
					if (shouldToggleLeftClick) {
						shouldToggleLeftClick = false;
						((SettingEnum*)setting->extraData)->SelectNextValue(false);
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}
					else if (shouldToggleRightClick) {
						shouldToggleRightClick = false;
						((SettingEnum*)setting->extraData)->SelectNextValue(true);
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}
					}
				}
				settingRect.y += textHeight + (textPadding * 12);

				break;
			}
			case ValueType::KEYBIND_T: {

				vec4_t r = vec4_t(textPos2.x, textPos2.y, textPos2.x + 220, textPos2.y + 10);

				vec2_t textPos2 = vec2_t(rectPos2.x + ((rectPos2.z - rectPos2.x) / 2), rectPos2.y + 2);

				char name[0x21];
				sprintf_s(name, 0x21, "%s:", setting->name);
				if (name[0] != 0)
					name[0] = toupper(name[0]);

				std::string text = name;

				DrawUtils::drawText(vec2_t(r.x, r.y), &text, MC_Color(255, 255, 255), textSize);

				if (!isCapturingKey || (keybindMenuCurrent != setting && isCapturingKey)) {

					const char* key;

					if (setting->value->_int > 0 && setting->value->_int < 190)
						key = KeyNames[setting->value->_int];
					else if (setting->value->_int == 0x0)
						key = "NONE";
					else
						key = "???";

					if (keybindMenuCurrent == setting && isCapturingKey) {
						key = "...";
					}
					else if (keybindMenuCurrent == setting && isConfirmingKey) {
						if (newKeybind > 0 && newKeybind < 190)
							key = KeyNames[newKeybind];
						else if (newKeybind == 0x0)
							key = "N/A";
						else
							key = "???";
					}

					std::string keyb = key;
					float keybSz = textHeight * 0.8f;

					float length = 10.f;  // because we add 5 to text padding + keybind name
					length += DrawUtils::getTextWidth(&text, textSize);
					length += DrawUtils::getTextWidth(&keyb, textSize);

					//windowSize.x = fmax(windowSize.x, length + offset);

					DrawUtils::drawText(vec2_t(r.z, r.y), &text, MC_Color(255, 255, 255), textSize);

					DrawUtils::drawText(vec2_t(r.z + 50, r.y), &keyb, MC_Color(255, 255, 255), textSize);
				}
				else {
					std::string text = "Press new bind...";
					//windowSize.x = fmax(windowSize.x, DrawUtils::getTextWidth(&text, textSize));

					DrawUtils::drawText(vec2_t(r.z, r.y), &text, MC_Color(255, 255, 255), textSize);
				}

				//DrawUtils::fillRectangle(rectPos, FILLCOLOR, OPACITY);

				if ((surenobugY - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
					overflowing = true;
					break;
				}

				// Logic
				{
					bool isFocused = vec4_t(r.x, r.y, r.z + 50, r.w).contains(&mousePos);

					if (isFocused && shouldToggleLeftClick && !(isCapturingKey && keybindMenuCurrent != setting /*don't let the user click other stuff while changing a keybind*/)) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}

						keybindMenuCurrent = setting;
						isCapturingKey = true;
					}

					if (isFocused && shouldToggleRightClick && !(isCapturingKey && keybindMenuCurrent != setting)) {
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}

						setting->value->_int = 0x0;  // Clear

						isCapturingKey = false;
					}

					if (shouldStopCapturing && keybindMenuCurrent == setting) {  // The user has selected a key
						if (clickGUI->sounds) {
							auto player = g_Data.getLocalPlayer();
							PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
							level->playSound("random.click", *player->getPos(), 1, 1);
						}

						shouldStopCapturing = false;
						isCapturingKey = false;
						setting->value->_int = newKeybind;
					}
				}
				settingRect.y += textHeight + (textPadding * 12);
			} break;
			case ValueType::FLOAT_T: {
				// Text and background
				{
					vec2_t textPos2 = vec2_t(rectPos2.x + ((rectPos2.z - rectPos2.x) / 2), rectPos2.y + 2);
					char str[10];
					sprintf_s(str, 10, "%.2f", setting->value->_float);
					string text = str;
					char name[0x22];
					sprintf_s(name, "%s: ", setting->name);
					string elTexto = name;
					string texto = str;
					DrawUtils::toLower(elTexto);
					textPos2.x = rectPos2.x + 5;
					DrawUtils::drawText(vec2_t(textPos2.x, textPos2.y - 10), &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize, 1.f);
					DrawUtils::drawRightAlignedString(&texto, vec4_t(rectPos2.x, rectPos2.y - 10, rectPos2.z, rectPos2.w), MC_Color(255, 255, 255), false);
					settingRect.y += textPadding + textHeight - 9;
					rectPos2.w = settingRect.y;
					string len = "saturation           ";
					float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
					if (!clickGUI->cFont) len = "saturation      ";
					if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
				}

				if ((settingRect.y - ourWindow->pos.y) > cutoffHeight) {
					overflowing = true;
					break;
				}
				// Slider
				{

					vec4_t rect = vec4_t(
						rectPos2.x + textPadding + 2.5f,
						rectPos2.y,
						rectPos2.z + textHeight - textPadding - 1.5f,
						rectPos2.y + textHeight + 4.f);
					// Visuals & Logic
					{

						rectPos2.w += textHeight;

						rect.z = rect.z - 10;

						rect.y = rect.y + 3;
						rect.w = rect.w - 3;

						const bool areWeFocused = rect.contains(&mousePos);
						DrawUtils::fillRectangleA(rect, MC_Color(0, 0, 0, 80));

						const float minValue = setting->minValue->_float;
						const float maxValue = setting->maxValue->_float - minValue;
						float value = (float)fmax(0, setting->value->_float - minValue);  // Value is now always > 0 && < maxValue
						if (value > maxValue) value = maxValue;
						value /= maxValue;  // Value is now in range 0 - 1
						//const float endXlol = settingRect.z - 246.4f;
						const float endXlol = settingRect.z - 251.6f;
						value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
						{
							rect.z = rect.x + value;

							vec4_t circle = vec4_t(rect.z, rect.y - 1.7f, rect.z + 3.7f, rect.y + 7.8f);

							DrawUtils::fillRectangleA(circle, MC_Color(230, 230, 230));
						}

						// Drag Logic
						{
							if (setting->isDragging) {
								if (isLeftClickDown && !isRightClickDown)
									value = mousePos.x - rect.x;
								else
									setting->isDragging = false;
							}
							else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
								shouldToggleLeftClick = false;
								setting->isDragging = true;
								if (clickGUI->sounds) {
									auto player = g_Data.getLocalPlayer();
									PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
									level->playSound("random.click", *player->getPos(), 1, 1);
								}
							}
						}

						// Save Value
						{
							value /= endXlol;  // Now in range 0 - 1
							value *= maxValue;
							value += minValue;

							setting->value->_float = value;
							setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
						}
					}
					settingRect.y += textHeight + (textPadding * 12);
				}
			} break;
			case ValueType::INT_T: {
				// Text and background
				{
					vec2_t textPos2 = vec2_t(rectPos2.x + ((rectPos2.z - rectPos2.x) / 2), rectPos2.y);
					char name[0x22];
					sprintf_s(name, "%s: ", setting->name);
					vec2_t textPos = vec2_t(rectPos2.x + 10, rectPos2.y + 10);
					char str[10];
					sprintf_s(str, 10, "%i", setting->value->_int);
					string text = str;
					string elTexto = name;
					DrawUtils::toLower(elTexto);
					textPos2.x -= DrawUtils::getTextWidth(&text, clickGUI->txtsize) / 2;
					textPos2.y += 2.5f;
					textPos2.x = rectPos2.x + 5;
					DrawUtils::drawText(vec2_t(textPos2.x, textPos2.y - 10), &elTexto, MC_Color(255, 255, 255), clickGUI->txtsize, 1.f);
					DrawUtils::drawRightAlignedString(&text, vec4_t(rectPos2.x, rectPos2.y - 10, rectPos2.z, rectPos2.w), MC_Color(255, 255, 255), false);
					settingRect.y += textPadding + textHeight - 7;
					rectPos2.w = settingRect.y;
					string len = "saturation  ";
					float lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 20.5f;
					if (!clickGUI->cFont) len = "saturation  ";
					if (!clickGUI->cFont)lenth = DrawUtils::getTextWidth(&len, clickGUI->txtsize) + 17.5f;
				}
				if ((settingRect.y - ourWindow->pos.y) > (g_Data.getGuiData()->heightGame * 0.75)) {
					overflowing = true;
					break;
				}
				// Slider
				{
					vec4_t rect = vec4_t(
						rectPos2.x + textPadding + 2.5f,
						rectPos2.y,
						rectPos2.z + textHeight - textPadding - 1.5f,
						rectPos2.y + textHeight + 4.f);


					// Visuals & Logic
					{
						rectPos2.w += textHeight;

						rect.z = rect.z - 10;

						rect.y = rect.y + 3;
						rect.w = rect.w - 3;

						const bool areWeFocused = rect.contains(&mousePos);
						DrawUtils::fillRectangleA(rect, MC_Color(0, 0, 0, 80));

						const float minValue = (float)setting->minValue->_int;
						const float maxValue = (float)setting->maxValue->_int - minValue;
						float value = (float)fmax(0, setting->value->_int - minValue);  // Value is now always > 0 && < maxValue
						if (value > maxValue)
							value = maxValue;
						value /= maxValue;  // Value is now in range 0 - 1
						const float endXlol = settingRect.z - 251.6;
						value *= endXlol;  // Value is now pixel diff between start of bar and end of progress
						{
							rect.z = rect.x + value;
							vec4_t circle = vec4_t(rect.z, rect.y - 1.7f, rect.z + 3.7f, rect.y + 7.8f);

							DrawUtils::fillRectangleA(circle, MC_Color(230, 230, 230));
						}

						// Drag Logic
						{
							if (setting->isDragging) {
								if (isLeftClickDown && !isRightClickDown)
									value = mousePos.x - rect.x;
								else
									setting->isDragging = false;
							}
							else if (areWeFocused && shouldToggleLeftClick && !ourWindow->isInAnimation) {
								shouldToggleLeftClick = false;
								setting->isDragging = true;
								if (clickGUI->sounds) {
									auto player = g_Data.getLocalPlayer();
									PointingStruct* level = g_Data.getLocalPlayer()->pointingStruct;
									level->playSound("random.click", *player->getPos(), 1, 1);
								}
							}
						}

						// Save Value
						{
							value /= endXlol;  // Now in range 0 - 1
							value *= maxValue;
							value += minValue;

							setting->value->_int = (int)roundf(value);
							setting->makeSureTheValueIsAGoodBoiAndTheUserHasntScrewedWithIt();
						}
					}
					settingRect.y += textHeight + (textPadding * 12);
				}
			} break;
			}
		}
		float endYOffset = settingRect.y;
	}

	vec4_t winRectPos = clickGUIRect;

	DrawUtils::drawRectangle(winRectPos, MC_Color(255, 255, 255), 0.8f, 0.5f);
}
#pragma endregion
void ClickGui::render() {
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	if (!moduleMgr->isInitialized()) return;

	if (timesRendered < 1) timesRendered++;

	// Fill Background
	{
		DrawUtils::fillRectangle(vec4_t(0, 0,
			g_Data.getClientInstance()->getGuiData()->widthGame,
			g_Data.getClientInstance()->getGuiData()->heightGame),
			MC_Color(0, 0, 0), 0.3);
	}

	// Render all categorys
	switch (clickGUI->theme.getSelectedValue()) {
	case 0: // Packet
		renderPacketCategory(Category::COMBAT, MC_Color(255, 255, 255));
		renderPacketCategory(Category::VISUAL, MC_Color(255, 255, 255));
		renderPacketCategory(Category::MOVEMENT, MC_Color(255, 255, 255));
		renderPacketCategory(Category::PLAYER, MC_Color(255, 255, 255));
		renderPacketCategory(Category::EXPLOIT, MC_Color(255, 255, 255));
		renderPacketCategory(Category::OTHER, MC_Color(255, 255, 255));
		break;
	case 1: // Vape
		renderVapeCategory(Category::COMBAT);
		renderVapeCategory(Category::VISUAL);
		renderVapeCategory(Category::MOVEMENT);
		renderVapeCategory(Category::PLAYER);
		renderVapeCategory(Category::EXPLOIT);
		renderVapeCategory(Category::OTHER);
		break;
	case 2: // Astolfo
		renderAstolfoCategory(Category::COMBAT, MC_Color(231, 76, 60), MC_Color(231 - 20, 76 - 20, 60 - 20));
		renderAstolfoCategory(Category::VISUAL, MC_Color(55, 0, 206), MC_Color(55 - 20, 0, 206 - 20));
		renderAstolfoCategory(Category::MOVEMENT, MC_Color(46, 204, 113), MC_Color(46 - 20, 204 - 20, 113 - 20));
		renderAstolfoCategory(Category::PLAYER, MC_Color(146, 68, 173), MC_Color(146 - 20, 68 - 20, 173 - 20));
		renderAstolfoCategory(Category::EXPLOIT, MC_Color(52, 152, 219), MC_Color(52 - 20, 152 - 20, 219 - 20));
		renderAstolfoCategory(Category::OTHER, MC_Color(243, 156, 18), MC_Color(243 - 20, 156 - 20, 20 - 20));
		break;
	case 3: // PacketOld
		renderPacketOldCategory(Category::COMBAT, MC_Color(255, 255, 255));
		renderPacketOldCategory(Category::VISUAL, MC_Color(255, 255, 255));
		renderPacketOldCategory(Category::MOVEMENT, MC_Color(255, 255, 255));
		renderPacketOldCategory(Category::PLAYER, MC_Color(255, 255, 255));
		renderPacketOldCategory(Category::EXPLOIT, MC_Color(255, 255, 255));
		renderPacketOldCategory(Category::OTHER, MC_Color(255, 255, 255));
		break;
	case 4: // Tenacity
		renderTenacityCategory(Category::COMBAT, MC_Color(231, 76, 60), MC_Color(231 - 20, 76 - 20, 60 - 20));
		renderTenacityCategory(Category::VISUAL, MC_Color(55, 0, 206), MC_Color(55 - 20, 0, 206 - 20));
		renderTenacityCategory(Category::MOVEMENT, MC_Color(46, 204, 113), MC_Color(46 - 20, 204 - 20, 113 - 20));
		renderTenacityCategory(Category::PLAYER, MC_Color(146, 68, 173), MC_Color(146 - 20, 68 - 20, 173 - 20));
		renderTenacityCategory(Category::EXPLOIT, MC_Color(52, 152, 219), MC_Color(52 - 20, 152 - 20, 219 - 20));
		renderTenacityCategory(Category::OTHER, MC_Color(243, 156, 18), MC_Color(243 - 20, 156 - 20, 20 - 20));
		break;
	case 5: // tena2
		if (combatc)renderNewCategory(Category::COMBAT);
		if (renderc)renderNewCategory(Category::VISUAL);
		if (movementc)renderNewCategory(Category::MOVEMENT);
		if (playerc)renderNewCategory(Category::PLAYER);
		if (exploitc)renderNewCategory(Category::EXPLOIT);
		if (otherc)renderNewCategory(Category::OTHER);
		break;
	case 6: // Lunar
		renderLunarCategory();
		break;
	case 7: // Badlion
		renderNewBadLion();
		break;
	case 8: // ONECONFIG
		renderONECONFIG();
		break;
	case 9: //Tenacity New
		renderTenacityNew();
		break;
	}

	shouldToggleLeftClick = false;
	shouldToggleRightClick = false;
	shouldToggleKeyDown = false;
	resetStartPos = false;
	DrawUtils::flush();
}

void ClickGui::init() { initialised = true; }

void ClickGui::onMouseClickUpdate(int key, bool isDown) {
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	if (clickGUI->isEnabled() && g_Data.isInGame()) {
		switch (key) {
		case 1:  // Left Click
			if (!clickGUI->skipClick) {
				isLeftClickDown = isDown;
				if (isDown)
					shouldToggleLeftClick = true;
			}
			else
				isLeftClickDown = false;
			if (!isDown)
				clickGUI->skipClick = false;
			break;
		case 2:  // Right Click
			if (!clickGUI->skipClick) {
				isRightClickDown = isDown;
				if (!isDown)
					shouldToggleRightClick = true;
			}
			else
				isRightClickDown = false;
			if (!isDown)
				clickGUI->skipClick = false;
		}
	}
}

void ClickGui::onWheelScroll(bool direction) {
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();
	if (clickGUI->theme.getSelectedValue() == 6 && focusConfigRect) {
		if (!direction) configScrollingDirection++;
		else configScrollingDirection--;
	}
	else {
		if (!direction) scrollingDirection++;
		else scrollingDirection--;
	}
}

void ClickGui::onKeyUpdate(int key, bool isDown) {
	if (!initialised) return;
	static auto clickGUI = moduleMgr->getModule<ClickGUIMod>();

	if (!isDown) return;

	if (!clickGUI->isEnabled()) {
		timesRendered = 0;
		return;
	}

	if (isCapturingKey && !shouldStopCapturing) {
		if (VK_DELETE == key || VK_ESCAPE == key)
			newKeybind = 0x0;
		else
			newKeybind = key;
		shouldStopCapturing = true;
	}

	if (timesRendered < 1) return;
	timesRendered = 0;

	if (isCapturingKey)
		return;
	isKeyDown = isDown;
	if (isDown) {
		pressedKey = key;
		shouldToggleKeyDown = true;
	}
	switch (key) {
	case VK_ESCAPE:
		clickGUI->setEnabled(false);
		return;
	default:
		if (key == clickGUI->getKeybind()) clickGUI->setEnabled(false);
	}
}

#pragma region Config Stuff
using json = nlohmann::json;
void ClickGui::onLoadConfig(void* confVoid) {
	savedWindowSettings.clear();
	windowMap.clear();
	json* conf = reinterpret_cast<json*>(confVoid);
	if (conf->contains("ClickGuiMenu")) {
		auto obj = conf->at("ClickGuiMenu");
		if (obj.is_null())
			return;
		for (int i = 0; i <= (int)Category::CUSTOM; i++) {
			auto catName = ClickGui::catToName((Category)i);
			if (obj.contains(catName)) {
				auto value = obj.at(catName);
				if (value.is_null())
					continue;
				try {
					SavedWindowSettings windowSettings = {};
					windowSettings.name = catName;
					if (value.contains("pos")) {
						auto posVal = value.at("pos");
						if (!posVal.is_null() && posVal.contains("x") && posVal["x"].is_number_float() && posVal.contains("y") && posVal["y"].is_number_float()) {
							try {
								windowSettings.pos = { posVal["x"].get<float>(), posVal["y"].get<float>() };
							}
							catch (exception e) {
							}
						}
					}
					if (value.contains("isExtended")) {
						auto isExtVal = value.at("isExtended");
						if (!isExtVal.is_null() && isExtVal.is_boolean()) {
							try {
								windowSettings.isExtended = isExtVal.get<bool>();
							}
							catch (exception e) {
							}
						}
					}
					savedWindowSettings[Utils::getCrcHash(catName)] = windowSettings;
				}
				catch (exception e) {
					logF("Config Load Error (ClickGuiMenu): %s", e.what());
				}
			}
		}
	}
}
void ClickGui::onSaveConfig(void* confVoid) {
	json* conf = reinterpret_cast<json*>(confVoid);
	// First update our map
	for (const auto& wind : windowMap) {
		savedWindowSettings[wind.first] = { wind.second->pos, wind.second->isExtended, wind.second->name };
	}

	// Save to json
	if (conf->contains("ClickGuiMenu"))
		conf->erase("ClickGuiMenu");

	json obj = {};

	for (const auto& wind : savedWindowSettings) {
		json subObj = {};
		subObj["pos"]["x"] = wind.second.pos.x;
		subObj["pos"]["y"] = wind.second.pos.y;
		subObj["isExtended"] = wind.second.isExtended;
		obj[wind.second.name] = subObj;
	}

	conf->emplace("ClickGuiMenu", obj);
}
#pragma endregion
