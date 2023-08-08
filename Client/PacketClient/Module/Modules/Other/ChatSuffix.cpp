#include "ChatSuffix.h"
#include <locale>
#include <codecvt>
#include "../../ModuleManager.h"
ChatSuffix::ChatSuffix() : IModule(0, Category::OTHER, "Adds the client suffix") {
	registerBoolSetting("Bypass", &bypass, bypass);
}

ChatSuffix::~ChatSuffix() {
}

const char* ChatSuffix::getModuleName() {
	return "ChatSuffix";
}

void ChatSuffix::onSendPacket(C_Packet* packet) {
	if (packet->isInstanceOf<C_TextPacket>()) {
		C_TextPacket* funy = reinterpret_cast<C_TextPacket*>(packet);
		std::string Sentence;
		std::string end;
		int i = random(1, 40);
		if (i >= 0 && i <= 3) end = " | A C T I N I U M";
		if (i > 3) end = " | Actinium";
#ifdef _DEBUG
#endif // _DEBUG
		std::string start;
		if (bypass && !moduleMgr->getModule<ChatBypass>()->isEnabled()) {
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
			std::string bypassStr = converter.to_bytes(static_cast<char32_t>(random(262144, 917503)));
			start += bypassStr;
			std::string bypassStr2 = converter.to_bytes(static_cast<char32_t>(random(262144, 917503)));
			end += bypassStr2;
		}
		Sentence = start + funy->message.getText() + end;
		funy->message.resetWithoutDelete();
		funy->message = Sentence;
	}
}

