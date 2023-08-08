#include "Spammer.h"
#include "../pch.h"
#include <locale>
#include <codecvt>
Spammer::Spammer() : IModule(0, Category::OTHER, "Spams a message in a specified delay") {
	registerEnumSetting("Mode", &mode, 0);
	mode.addEntry("Custom", 0);
	mode.addEntry("Alphabets", 1);
	mode.addEntry("aLPhaBeTs", 2);
	mode.addEntry(u8"ひらがな", 3);
	registerIntSetting("delay", &delay, delay, 0, 30);
	registerIntSetting("minLength", &minLength, minLength, 1, 200);
	registerIntSetting("maxLength", &maxLength, maxLength, 1, 200);
}

const char* Spammer::getModuleName() {
	return ("Spammer");
}

void Spammer::onTick(C_GameMode* gm) {
	Odelay++;
	if (Odelay > delay * 20) {
		C_TextPacket textPacket;
		if (mode.getSelectedValue() == 0) {
			textPacket.message.setText(message);
		}
		else if (mode.getSelectedValue() == 1) {
			std::string message;
			for (size_t i = 0; i < random(minLength, maxLength); i++)
			{
				message += 'a' + rand() % 26;
			}
			textPacket.message.setText(message);
		}
		else if (mode.getSelectedValue() == 2) {
			std::string message;
			for (size_t i = 0; i < random(minLength, maxLength); i++)
			{
				int r = rand() % 52;
				char base = (r < 26) ? 'A' : 'a';
				message += (char)(base + r % 26);
			}
			textPacket.message.setText(message);
		}
		else if (mode.getSelectedValue() == 3) {
			std::string message;
			for (size_t i = 0; i < random(minLength, maxLength); i++)
			{
				int randomCode = std::rand() % 72 + 12353;
				std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
				message += converter.to_bytes(static_cast<char32_t>(randomCode));
			}
			textPacket.message.setText(message);
		}
		textPacket.sourceName.setText(g_Data.getLocalPlayer()->getNameTag()->getText());
		g_Data.getClientInstance()->loopbackPacketSender->sendToServer(&textPacket);
		Odelay = 0;
	}
}