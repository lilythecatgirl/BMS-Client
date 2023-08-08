#include "Killsults.h"

using namespace std;
Killsults::Killsults() : IModule(0, Category::OTHER, "Insults people you kill lol") {
	registerEnumSetting("Mode", &mode, 0);
	mode.addEntry("Normal", 0);
	mode.addEntry("Sigma", 1);
	mode.addEntry("Funny", 2);
	mode.addEntry(u8"メスガキ", 3);
	mode.addEntry("Japanese810", 4);
	mode.addEntry("UwUSpeak", 5);
	mode.addEntry("Health", 6);
	mode.addEntry(u8"おにまい", 7);
	mode.addEntry(u8"おみくじ", 8);
	registerBoolSetting("Sound", &sound, sound);
	registerBoolSetting("Notification", &notification, notification);
}

const char* Killsults::getRawModuleName() {
	return "Killsults";
}

const char* Killsults::getModuleName() {
	if (mode.getSelectedValue() == 0) name = string("Killsults ") + string(GRAY) + string("Normal");
	if (mode.getSelectedValue() == 1) name = string("Killsults ") + string(GRAY) + string("Sigma");
	if (mode.getSelectedValue() == 2) name = string("Killsults ") + string(GRAY) + string("Funny");
	if (mode.getSelectedValue() == 3) name = string("Killsults ") + string(GRAY) + string(u8"メスガキ");
	if (mode.getSelectedValue() == 4) name = string("Killsults ") + string(GRAY) + string("Japanese810");
	if (mode.getSelectedValue() == 5) name = string("Killsults ") + string(GRAY) + string("UwUSpeak");
	if (mode.getSelectedValue() == 6) name = string("Killsults ") + string(GRAY) + string("Health");
	if (mode.getSelectedValue() == 7) name = string("Killsults ") + string(GRAY) + string(u8"おにまい");
	if (mode.getSelectedValue() == 8) name = string("Killsults ") + string(GRAY) + string(u8"おみくじ");
	return name.c_str();
}

string normalMessages[32] = {
	"Download Radium today to kick azs while aiding to some abstractional!",
	"I found you in task manager and I ended your process",
	"What's yellow and can't swim? A bus full of children",
	"You are more disappointing than an unsalted pretzel",
	"Take a shower, you smell like your grandpa's toes",
	"You are not Radium Clent approved :rage:",
	"I'm not flying, I'm just defying gravity!",
	"Stop running, you weren't going to win",
	"Knock knock, who's there? Your life",
	"Your client has stopped working",
	"Warning, I have detected haram",
	"I don't hack, I just radium",
	"You should end svchost.exe!",
	"just aided my pantiez",
	"You were an accident",
	"Abstractional Aidz",
	"JACKALOPE TURD BOX",
	"Get dogwatered on",
	"Get 360 No-Scoped",
	"You afraid of me?",
	"Go do the dishes",
	"Job Immediately",
	"Delete System32",
	"I Alt-F4'ed you",
	"Radium is Free",
	"Touch grass",
	"jajajaja",
	"Minority",
	"kkkkkk",
	"clean",
	"idot",
	"F",
};

string cheaterMessages[9] = {
	"How does this bypass ?!?!?",
	"Switch to Radium today!",
	"Violently bhopping I see!",
	"Why Toolbox when Radium?",
	"You probably use Zephyr",
	"Must be using Kek.Club+",
	"SelfHarm Immediately.",
	"Man you're violent",
	"ToolBox moment"
};

string hvhMessages[19] = {
	"Your client has stopped working",
	"Switch to Radium today!",
	"Violently bhopping I see!",
	"Your aim is like derp",
	"You probably use Toolbox",
	"You're so slow, use speed",
	"You're probably playing on VR",
	"Did you know? This server doesn't have Anti-Cheat",
	"Sentinel can only detect speed",
	"Flareon bans legit players",
	"Gwen can't detect most cheats",
	"Why are you flying? oh you were already dead",
	"Press CTRL + L",
	"20 health left",
	"Press Alt + F4 to enable Godmode",
	"you're probably using a $3 mouse",
	"You only have 3IQ",
	"You died too fast",
	"Toolbox moment"
};

string sigmaMessages[2] = {
	"Eat My",
	"Funny Funny Abstractional"
};

string japaneseMessages[14] = {
	u8"Nexus勢ざぁーこ♡ ",
	u8"ざぁーーこ！♡ ",
	u8"Nexusとか使ってる情弱居るんだぁ♡ ",
	u8"ざぁこ♡ ",
	u8"Nexus使ってるとか情けないね〜♡ ",
	u8"レポートしてやってもいいのよ？ ",
	u8"すく死んじゃったね♡ ",
	u8"Hiveのざぁこ♡ ",
	u8"よわよわぁ～♡ ",
	u8"負けちゃってはずかし～♡ ",
	u8"ざぁこ♡ざぁこ♡ ",
	u8"あれ～？ざぁこすぎて泣いちゃった？♡",
	u8"エイムざぁこ♡",
	u8"アンチチートよわ～い♡"
};
string japanese810Messages[8] = {
	"縺ｾ縺壹＞縺ｧ縺吶ｈ!",
	"繝輔ぃ!?",
	"繧､繧ｭ繧ｹ繧ｮ繧｣!",
	"114514",
	"縺ゅ▲(蟇溘＠)",
	"繧､繧､繧ｾ繝ｼ繧ｳ繝ｬ",
	"繧､繧ｭ繧ｹ繧ｮ繧､!",
	"繧ｪ繝翫す繝｣繧ｹ",
	/*"縺励ｃ縺ｶ繧後ｈ",
	"縺吶∩縺ｾ縺帙ｓ險ｱ縺励※縺上□縺輔＞!菴輔〒繧ゅ＠縺ｾ縺吶ｓ縺ｧ!",
	"縺ｪ繧薙□縺雁燕",
	"繝後ャ!",
	"縺ｯ縺茨ｼｾ繝ｼ縺吶▲縺斐＞螟ｧ縺阪＞...",
	"繝ｳ繧｢繝ｼ縺｣!"*/
};
string uwuspeakMessage[13] = {
	"Thanks for the fwee woot~",
	"Heyyy OwU!",
	"hehe~~",
	"Thanks for letting me touch you! Hehe!",
	"You're so gentle UwU!",
	"OwO!",
	"Hey! Thanks for letting me kill you~",
	"aahhhhh~",
	"You're so cute!",
	"mmmmmmmm~",
	"You're such a sussy baka",
	"OwO! You're so easy!",
	"I got stuck in the washing machine~"
};
string onimai[8] = {
	u8"だめだめえっちすぎます！",
	u8"出る..出ちゃうぅ....///",
	u8"緒山が悪いんだぞ..",
	u8"み、みはりぃそれはだめ...////",
	u8"お兄ちゃんここが弱いんだぁ♡...//",
	u8"そんなぁ...出る....////",
	u8"あうあうあー！",
	u8"はうぅ.."
};

string omikuziMessage[7] = {
	u8"大凶",
	u8"凶",
	u8"末吉",
	u8"小吉",
	u8"中吉",
	u8"吉",
	u8"大吉"
};
void Killsults::onEnable() {
	killed = false;
}

void Killsults::onPlayerTick(C_Player* plr) {
	auto player = g_Data.getLocalPlayer();
	if (player == nullptr) return;

	int randomVal = 0;
	srand(time(NULL));
	if (killed) {
		C_TextPacket textPacket;
		PointingStruct* level = player->pointingStruct;
		vec3_t* pos = player->getPos();
		if (sound) {
			level->playSound("random.orb", *pos, 1, 1);
			level->playSound("firework.blast", *pos, 1, 1);
		}
		if (notification) {
			auto notification = g_Data.addNotification("Killsult:", "Killed player"); notification->duration = 5;
		}
		switch (mode.getSelectedValue()) {
		case 0: // Normal
			randomVal = rand() % 32;
			textPacket.message.setText(normalMessages[randomVal]);
			break;
		case 1: // Sigma
			randomVal = rand() % 2;
			textPacket.message.setText(sigmaMessages[randomVal]);
			break;
		case 2: // HvH
			randomVal = rand() % 19;
			textPacket.message.setText(hvhMessages[randomVal]);
			break;
		case 3: // Japanese
			randomVal = random(0, 13);
			textPacket.message.setText(japaneseMessages[randomVal]);
			break;
		case 4: // Japanese810
			randomVal = rand() % 8;
			textPacket.message.setText(japanese810Messages[randomVal]);
			break;
		case 5: // UwU
			randomVal = rand() % 13;
			textPacket.message.setText(uwuspeakMessage[randomVal]);
			break;
		case 6:
			textPacket.message.setText(to_string(player->getHealth() + player->getAbsorption()) + " Health remaining, Want power? radiumclient.com");
			break;
		case 7: // おにまい
			randomVal = rand() % 7;
			textPacket.message.setText(onimai[randomVal] + u8" | お兄ちゃんはおしまい！");
			break;
		case 8:
			randomVal = rand() % 6;
			int randomVal2 = random(0, 13);
			textPacket.message.setText(u8"あなたの運勢は " + omikuziMessage[randomVal] + " - " + japaneseMessages[randomVal2]);
			break;
		}
		textPacket.sourceName.setText(player->getNameTag()->getText());
		textPacket.xboxUserId = to_string(player->getUserId());
		g_Data.getClientInstance()->loopbackPacketSender->sendToServer(&textPacket);
		killed = false;
	}
}
