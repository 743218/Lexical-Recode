#include "ChatSuffix.h"
#include <random>
#include "../../Client/Client.h"

void ChatSuffix::onSendPacket(Packet* packet, bool& cancel) {
	if (packet->getId() != PacketID::Text) return;
	TextPacket* textPacket = (TextPacket*)packet;
	std::string message = textPacket->mMessage;
	if (greenText) message = "> " + message;
	if (suffix) message += " | " + Client::getClientName();
	if (randomText) {
		std::string yea("ABCDEFGHIJKLMNOPQRXTUVWXYZabcdefghijklmnopqrstuvwxyz123456789");
		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(yea.begin(), yea.end(), gen);
		message += " | " + yea.substr(0, 12);
	}
	
	static std::unordered_map<std::string, std::u8string> textMap = {
		{"A", u8"ᴀ"},
		{"B", u8"ʙ"},
		{"C", u8"ᴄ"},
		{"D", u8"ᴅ"},
		{"E", u8"ᴇ"},
		{"F", u8"ꜰ"},
		{"G", u8"ɢ"},
		{"H", u8"ʜ"},
		{"I", u8"ɪ"},
		{"J", u8"ᴊ"},
		{"K", u8"ᴋ"},
		{"L", u8"ʟ"},
		{"M", u8"ᴍ"},
		{"N", u8"ɴ"},
		{"O", u8"ᴏ"},
		{"P", u8"ᴘ"},
		{"Q", u8"ǫ"},
		{"R", u8"ʀ"},
		{"S", u8"ꜱ"},
		{"T", u8"ᴛ"},
		{"U", u8"ᴜ"},
		{"V", u8"ᴠ"},
		{"W", u8"ᴡ"},
		{"X", u8"x"},
		{"Y", u8"ʏ"},
		{"Z", u8"ᴢ"},
		{"1", u8"₁"},
		{"2", u8"₂"},
		{"3", u8"₃"},
		{"4", u8"₄"},
		{"5", u8"₅"},
		{"6", u8"₆"},
		{"7", u8"₇"},
		{"8", u8"₈"},
		{"9", u8"₉"},
		{"0", u8"₀"}
	};
	std::string out = message;
	if (fancyText) {
		for (const std::pair pair : textMap) {
			std::string lower = pair.first;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			out = std::regex_replace(out, std::regex(pair.first), reinterpret_cast<const char*>(pair.second.c_str()));
			out = std::regex_replace(out, std::regex(lower), reinterpret_cast<const char*>(pair.second.c_str()));
		}
	}
	textPacket->mMessage = out;
}