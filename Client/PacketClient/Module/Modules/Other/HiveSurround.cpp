#include "HiveSurround.h"
#include "../../../../Utils/TimerUtil.h"
#include "../pch.h"
using namespace std;

HiveSurround::HiveSurround() : IModule(0, Category::OTHER, u8"宝戦争で自動的に宝をブロックで囲います") {
}
const char* HiveSurround::getModuleName() {
	return ("TreasureProtect");
}
int HiveSurround::placeBlock(vec3_t pos) {
	static vector<vec3_ti*> checklist;
	auto player = g_Data.getLocalPlayer();
	int i = 0;
	vec3_t blockBelow = pos.floor();
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
		bool foundBlock = false;
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
			return g_Data.getCGameMode()->buildBlock(&blok, i);
		}
	}
}


vector<vec3_t> blocks;
void HiveSurround::onEnable() {
	blocks.clear();
	C_Entity* entity;
	bool foundEntity = false;
	g_Data.forEachEntity([&entity, &foundEntity, this](C_Entity* ent, bool b) {
		string name = ent->getNameTag()->getText();
		int id = ent->getEntityTypeId();
		if (id != 319 && name.find("Treasure") != string::npos && g_Data.getLocalPlayer()->getPos()->dist(*ent->getPos()) <= 5) {
			entity = ent;
			foundEntity = true;
		}
		});
	if (foundEntity) {
		vec3_t pos = vec3_t(floorf(entity->getPos()->x), floorf(entity->getPos()->y), floorf(entity->getPos()->z));
		blocks.push_back(vec3_t(pos.x + 1, pos.y, pos.z));
		blocks.push_back(vec3_t(pos.x - 1, pos.y, pos.z));
		blocks.push_back(vec3_t(pos.x, pos.y, pos.z + 1));
		blocks.push_back(vec3_t(pos.x, pos.y, pos.z - 1));
		blocks.push_back(vec3_t(pos.x + 1, pos.y + 1, pos.z));
		blocks.push_back(vec3_t(pos.x - 1, pos.y + 1, pos.z));
		blocks.push_back(vec3_t(pos.x + 2, pos.y, pos.z));
		blocks.push_back(vec3_t(pos.x - 2, pos.y, pos.z));
		blocks.push_back(vec3_t(pos.x, pos.y, pos.z + 2));
		blocks.push_back(vec3_t(pos.x, pos.y, pos.z - 2));
		blocks.push_back(vec3_t(pos.x, pos.y + 1, pos.z + 1));
		blocks.push_back(vec3_t(pos.x, pos.y + 1, pos.z - 1));
		blocks.push_back(vec3_t(pos.x + 1, pos.y, pos.z + 1));
		blocks.push_back(vec3_t(pos.x + 1, pos.y, pos.z - 1));
		blocks.push_back(vec3_t(pos.x - 1, pos.y, pos.z + 1));
		blocks.push_back(vec3_t(pos.x - 1, pos.y, pos.z - 1));
		blocks.push_back(vec3_t(pos.x, pos.y + 1, pos.z));
		blocks.push_back(vec3_t(pos.x, pos.y + 2, pos.z));
		reverse(blocks.begin(), blocks.end());
	}
	else {
		this->setEnabled(false);
	}

}
int index = 0;
void HiveSurround::onTick(C_GameMode* gm) {
	if (blocks.size() == 0) {
		this->setEnabled(false);
		return;
	}
	if (blocks.size() <= index)
		index = 0;
	vec3_t block = blocks[index];
	if (placeBlock(block) == 1) {
		blocks.erase(blocks.begin() + index);
		index--;
	}
	index++;
}
