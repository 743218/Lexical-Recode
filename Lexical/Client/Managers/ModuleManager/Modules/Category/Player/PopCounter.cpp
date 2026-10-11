#include "PopCounter.h"
#include "../../Client/Client.h"
#include "../../../../../../Libs/json.hpp"

using json = nlohmann::json;
using namespace StringUtil;

PopCounter::PopCounter() : Module("PopCounter", "Tracks players pops", Category::PLAYER) {
	registerSetting(new BoolSetting("Send Pop Message", "Sends a message in chat when someone pops", &sendPops, false));
	registerSetting(new BoolSetting("Send Death Message", "Sends a message in chat when someone dies that tells how many pops they got", &sendPopsOnDeath, false));
	registerSetting(new BoolSetting("Track Friends", "Track friends when they pop", &countFriends, false));
	registerSetting(new BoolSetting("Track Self", "Track yourself when you pop", &countSelf, false));
	registerSetting(new BoolSetting("ActorEvent", "Uses actor event packets to detect pops,this is 100% accurate but some servers may not support it(ign)", &actorEventOnly, false));
}

std::string PopCounter::sanitize(const std::string& message, const std::string name, const int pops) {
	std::string result = message;
	std::string totem = "totem";

	if (pops > 1) totem += "s";

	static const std::regex clientName("!clientname!");
	static const std::regex player("!player!");
	static const std::regex tPops("!pops!");
	static const std::regex rTotem("!totem!");
	static const std::regex dollarSign("$");
	result = std::regex_replace(result, dollarSign, "§");
	result = std::regex_replace(result, clientName, Client::getClientName());
	result = std::regex_replace(result, player, name);
	result = std::regex_replace(result, tPops, std::to_string(pops));
	result = std::regex_replace(result, rTotem, totem);

	std::string out;
	for (char c : result) {
		if (c != '$') out += c;
		else if (c == '$') out += "§";
	}
	return out;
}

void PopCounter::onActorPop(Actor* actor) {
	const uint64_t& rId = actor->getRuntimeIDComponent()->runtimeId.id;
	totemMap[rId]++;
	if (!this->isEnabled()) return;
	const std::string& message = sanitize(popMessage, *actor->getNameTag(), totemMap[rId]);
	const std::string& messageFriend = sanitize(friendMessage, *actor->getNameTag(), totemMap[rId]);
	const std::string& messageSelf = sanitize(selfMessage, *actor->getNameTag(), totemMap[rId]);
	const std::string& publicMessage = sanitize(popMessage2, *actor->getNameTag(), totemMap[rId]);
	if (sendPops && actor != Game::getLocalPlayer() && !FriendManager::isInList(*actor->getNameTag())) PlayerUtil::SendTextMessage(publicMessage);
	if (FriendManager::isInList(*actor->getNameTag()) && countFriends) Game::DisplayClientMessage("%s", StringUtil::formatToGame(messageFriend).c_str());
	else if (actor == Game::getLocalPlayer() && countSelf) Game::DisplayClientMessage("%s", StringUtil::formatToGame(messageSelf).c_str());
	else {
		if (actor != Game::getLocalPlayer() && !FriendManager::isInList(*actor->getNameTag())) Game::DisplayClientMessage("%s", StringUtil::formatToGame(message).c_str());
	}
}

void PopCounter::onNormalTick(LocalPlayer* localPlayer) {
	if (!localPlayer->isAlive()) totemMap[localPlayer->getRuntimeIDComponent()->runtimeId.id] = 0;
	for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
		if (!actor) continue;
		if (actor->getActorTypeComponent()->id != 319) continue;
		if (actor->getOffhandSlot()->item.get() != nullptr) didHaveTotem[actor->getRuntimeIDComponent()->runtimeId.id] = true;
	}
}

void PopCounter::onRecievePacket(Packet* packet, bool& cancel) {
	if (!Game::getLocalPlayer()) return;
	static std::vector<Actor*> actorList;
	actorList.clear();
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!actorEventOnly) {
		if (packet->getId() == PacketID::LevelEvent) {
			LevelEventPacket* eventPacket = (LevelEventPacket*)packet;
			if (eventPacket->mEventId == LevelEventPacket::EventID::SoundTotemUsed && eventPacket->mPos != Vec3<float>(0.f, 0.f, 0.f)) {
				for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
					if (!actor) continue;
					if (actor->getActorTypeComponent()->id != 319) continue;
					if (actor->getPos().floor().sub(0, 1, 0).dist(eventPacket->mPos) > 10) continue;
					if (actor->getOffhandSlot()->item.get() == nullptr && didHaveTotem[actor->getRuntimeIDComponent()->runtimeId.id]) actorList.push_back(actor);
				}
				if (!actorList.empty()) {
					std::sort(actorList.begin(), actorList.end(), [eventPacket](Actor* a1, Actor* a2) {
						return a1->getPos().floor().sub(0, 1, 0).dist(eventPacket->mPos) < a2->getPos().floor().sub(0, 1, 0).dist(eventPacket->mPos);
						});
					Actor* actor = actorList.front();
					didHaveTotem[actor->getRuntimeIDComponent()->runtimeId.id] = false;
					onActorPop(actor);
				}
			}
		}
	}
	else {
		if (packet->getId() == PacketID::ActorEvent) {
			ActorEventPacket* eventPkt = (ActorEventPacket*)packet;
			if (eventPkt->mEventId == ActorEventPacket::ActorEventID::TotemUse) {
				for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
					if (!actor) continue;
					if (actor->getActorTypeComponent()->id != 319) continue;
					if (actor->getRuntimeIDComponent()->runtimeId.id != eventPkt->mRuntimeId) continue;
					onActorPop(actor);
				}
			}
		}
	}
	if (packet->getId() == PacketID::RemoveActor) {
		RemoveActorPacket* removePacket = (RemoveActorPacket*)packet;
		for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
			if (!actor) continue;
			if (actor == localPlayer) continue;
			if (actor->getActorTypeComponent()->id != 319) continue;
			if (actor->getRuntimeIDComponent()->runtimeId.id != removePacket->mRuntimeId) continue;
			const uint64_t& rId = actor->getRuntimeIDComponent()->runtimeId.id;
			if (totemMap[rId] == 0) continue;
			if ((!FriendManager::isInList(*actor->getNameTag()) && actor != localPlayer && this->isEnabled())) {
				const std::string& message = sanitize(deathMessage, *actor->getNameTag(), totemMap[rId]);
				const std::string& publicMessage = sanitize(deathMessage2, *actor->getNameTag(), totemMap[rId]);
				if (sendPopsOnDeath) PlayerUtil::SendTextMessage(publicMessage);
				Game::DisplayClientMessage("%s", StringUtil::formatToGame(message).c_str());
			}
		}
		totemMap[removePacket->mRuntimeId] = 0;
	}
}

void PopCounter::onLoadConfig(void* confVoid) {
	json* config = reinterpret_cast<json*>(confVoid);
	std::string modName = this->getModuleName();
	try {
		if (config->contains(modName)) {
			json obj = config->at(getModuleName());
			if (obj.is_null()) return;
			if (obj.contains("enabled")) this->setEnabled(obj["enabled"].get<bool>());
			for (Setting* setting : this->getSettingList()) {
				if (obj.contains(setting->name)) {
					if (setting->type == SettingType::KEYBIND_S) {
						KeybindSetting* keySetting = (KeybindSetting*)setting;
						*keySetting->value = obj[keySetting->name].get<int>();
					}
					else if (setting->type == SettingType::BOOL_S) {
						BoolSetting* boolSetting = (BoolSetting*)setting;
						*boolSetting->value = obj[boolSetting->name].get<bool>();
					}
					else if (setting->type == SettingType::ENUM_S) {
						EnumSetting* enumSetting = (EnumSetting*)setting;
						*enumSetting->value = obj[enumSetting->name].get<int>();
					}
					else if (setting->type == SettingType::COLOR_S) {
						ColorSetting* colorSetting = (ColorSetting*)setting;
						json colorObj = obj[colorSetting->name];
						if (colorObj.contains("Red")) colorSetting->colorPtr->r = colorObj["Red"].get<int>();
						if (colorObj.contains("Green")) colorSetting->colorPtr->g = colorObj["Green"].get<int>();
						if (colorObj.contains("Blue")) colorSetting->colorPtr->b = colorObj["Blue"].get<int>();
						if (colorObj.contains("Alpha")) colorSetting->colorPtr->a = colorObj["Alpha"].get<int>();
						if (colorObj.contains("Synced")) colorSetting->colorSynced = colorObj["Synced"].get<bool>();
					}
					else if (setting->type == SettingType::SLIDER_S) {
						SliderSettingBase* sliderSetting = (SliderSettingBase*)setting;
						if (sliderSetting->valueType == ValueType::INT_T) {
							SliderSetting<int>* intSlider = (SliderSetting<int>*)sliderSetting;
							*intSlider->valuePtr = obj[intSlider->name].get<int>();
						}
						else if (sliderSetting->valueType == ValueType::FLOAT_T) {
							SliderSetting<float>* floatSlider = (SliderSetting<float>*)sliderSetting;
							*floatSlider->valuePtr = obj[floatSlider->name].get<float>();
						}
					}
				}
			}
			if (obj.contains("Messages")) {
				json confValue = obj.at("Messages");
				if (confValue.is_null()) return;
				if (confValue.contains("Client Chat")) {
					json confValue2 = confValue.at("Client Chat");
					if (confValue2.is_null()) return;
					if (confValue2.contains("On Pop")) {
						json confValue3 = confValue2.at("On Pop");
						if (confValue3.is_null()) return;
						if (confValue3.contains("Enemy")) popMessage = confValue3["Enemy"].get<std::string>();
						if (confValue3.contains("Friend")) friendMessage = confValue3["Friend"].get<std::string>();
						if (confValue3.contains("Self")) selfMessage = confValue3["Self"].get<std::string>();
					}
					if (confValue2.contains("On Death")) deathMessage = confValue2["On Death"].get<std::string>();
				}
				if (confValue.contains("Game Chat")) {
					json confValue2 = confValue.at("Game Chat");
					if (confValue2.is_null()) return;
					if (confValue2.contains("On Pop")) popMessage2 = confValue2["On Pop"].get<std::string>();
					if (confValue2.contains("On Death")) deathMessage2 = confValue2["On Death"].get<std::string>();
				}
			}
		}
	}
	catch (const std::exception& e) {}
}

void PopCounter::onSaveConfig(void* confVoid) {
	json* currentConfig = reinterpret_cast<json*>(confVoid);
	try {
		json obj;
		obj["enabled"] = this->isEnabled();
		for (Setting* setting : this->getSettingList()) {
			if (setting->type == SettingType::KEYBIND_S) {
				KeybindSetting* keySetting = (KeybindSetting*)setting;
				obj[keySetting->name] = *keySetting->value;
			}
			else if (setting->type == SettingType::BOOL_S) {
				BoolSetting* boolSetting = (BoolSetting*)setting;
				obj[boolSetting->name] = *boolSetting->value;
			}
			else if (setting->type == SettingType::ENUM_S) {
				EnumSetting* enumSetting = (EnumSetting*)setting;
				obj[enumSetting->name] = *enumSetting->value;
			}
			else if (setting->type == SettingType::COLOR_S) {
				ColorSetting* colorSetting = (ColorSetting*)setting;
				obj[colorSetting->name]["Red"] = colorSetting->colorPtr->r;
				obj[colorSetting->name]["Green"] = colorSetting->colorPtr->g;
				obj[colorSetting->name]["Blue"] = colorSetting->colorPtr->b;
				obj[colorSetting->name]["Alpha"] = colorSetting->colorPtr->a;
				obj[colorSetting->name]["Synced"] = colorSetting->colorSynced;
			}
			else if (setting->type == SettingType::SLIDER_S) {
				SliderSettingBase* sliderSetting = (SliderSettingBase*)setting;
				if (sliderSetting->valueType == ValueType::INT_T) {
					SliderSetting<int>* intSlider = (SliderSetting<int>*)sliderSetting;
					obj[intSlider->name] = *intSlider->valuePtr;
				}
				else if (sliderSetting->valueType == ValueType::FLOAT_T) {
					SliderSetting<float>* floatSlider = (SliderSetting<float>*)sliderSetting;
					obj[floatSlider->name] = *floatSlider->valuePtr;
				}
			}
		}
		obj["Help"]["Formating"] = "Since the json cannot convert the section sign(the one minecraft uses for colors), we will be using $ as an alternative. (eg: $4 would output a RED text)";
		obj["Help"]["Key Words"] = { "!clientname! = The clients current client name", "!pops! = The amount of totems the player has popped", "!player! = The players name", "!totem! = the word totem or totems depending on their pops" };
		obj["Messages"]["Client Chat"]["On Pop"]["Enemy"] = popMessage;
		obj["Messages"]["Client Chat"]["On Pop"]["Friend"] = friendMessage;
		obj["Messages"]["Client Chat"]["On Pop"]["Self"] = selfMessage;
		obj["Messages"]["Client Chat"]["On Death"] = deathMessage;

		obj["Messages"]["Game Chat"]["On Pop"] = popMessage2;
		obj["Messages"]["Game Chat"]["On Death"] = deathMessage2;

		(*currentConfig)[this->getModuleName()] = obj;
	}
	catch (const std::exception& e) {}
}