#include "SpammerCommand.h"

#include "../../Module/ModuleManager.h"
#include "pch.h"

SpammerCommand::SpammerCommand() : IMCCommand("spammer", "Edit spammer delay/text", "<text/delay/bypass/manual> <string/int/bool>") {
	registerAlias("spam");
}

bool SpammerCommand::execute(std::vector<std::string>* args) {
	assertTrue(g_Data.getLocalPlayer() != nullptr);
	std::string option = args->at(1);
	std::transform(option.begin(), option.end(), option.begin(), ::tolower);

	auto spammer = moduleMgr->getModule<Spammer>();

	if (option == "text") {
		std::ostringstream os;
		for (int i = 2; i < args->size(); i++) {
			if (i > 1)
				os << " ";
			os << args->at(i);
		}
		std::string text = os.str().substr(1);
		spammer->getMessage() = text;
		clientMessageF("[Actinium] %sSpammer message set to %s%s%s!", GREEN, GRAY, text.c_str(), GREEN);
		return true;
	}
	else if (option == "delay") {
		int delay = assertInt(args->at(2));
		if (delay < 1) {
			clientMessageF("[Actinium] %sDelay needs to be 1 or more!", RED);
			return true;
		}
		else {
			return true;
		}
	}
	else if (option == "manual") {
		int times = assertInt(args->at(2));
		std::ostringstream os;
		for (int i = 3; i < args->size(); i++) {
			if (i > 1)
				os << " ";
			os << args->at(i);
		}
		std::string text = os.str().substr(1);
		for (int i = 0; i < times; i++) {
			C_TextPacket textPacket;
			textPacket.message.setText(text);
			textPacket.sourceName = *g_Data.getLocalPlayer()->getNameTag();
			//textPacket.xboxUserId = TextHolder(std::to_string(g_Data.getLocalPlayer()->getUserId()));
			g_Data.getClientInstance()->loopbackPacketSender->sendToServer(&textPacket);
		}
		return true;
	}
	return false;
}
