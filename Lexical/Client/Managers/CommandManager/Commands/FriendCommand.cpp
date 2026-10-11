#include "FriendCommand.h"
#include "../../Utils/Minecraft/TargetUtil.h"

FriendCommand::FriendCommand() : CommandBase("friend", "Add/Remove people to friend list", "<add/remove> <name>", { "f" }) {
}

bool FriendCommand::execute(const std::vector<std::string>& args) {
	if (args.size() < 2) return false;
	std::string searchedName = args[2];
	if (searchedName.empty()) return false;
	if (searchedName.size() == 0) return false;
	std::transform(searchedName.begin(), searchedName.end(), searchedName.begin(), ::tolower);
	if (args[1] == "remove") {
		std::string trgtName;
		for (std::string name : FriendManager::getList()) {
			std::string curName = name;
			std::transform(curName.begin(), curName.end(), curName.begin(), ::tolower);
			if (curName.find(searchedName) != std::string::npos) {
				trgtName = name;
				break;
			}
		}
		if (!trgtName.empty()) {
			if (FriendManager::removeFromList(trgtName)) Game::DisplayClientMessage("%sRemoved %s%s%s to list.", MCTF::GREEN, MCTF::YELLOW, trgtName, MCTF::GREEN);
			else Game::DisplayClientMessage("%s%s%s was not in your friendslist", MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
			return true;
		}
		else {
			Game::DisplayClientMessage("%s%s%s was not in your friendslist", MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
			return true;
		}
	}
	else if (args[1] == "add") {
		if (args[2].empty()) return false;
		Actor* target = nullptr;
		for (Actor* actor : Game::getLocalPlayer()->level->getRuntimeActorList()) {
			if (!TargetUtil::isTargetValid(actor, false, false)) continue;
			std::string actorName = *actor->getNameTag();
			std::transform(actorName.begin(), actorName.end(), actorName.begin(), ::tolower);
			if (actorName.find(searchedName) != std::string::npos || actorName == searchedName) {
				target = actor;
				break;
			}
		}
		if (target != nullptr) {
			std::string trgtName = *target->getNameTag();
			if (FriendManager::addToList(trgtName)) Game::DisplayClientMessage("%sAdded %s%s%s to list.", MCTF::GREEN, MCTF::YELLOW, trgtName, MCTF::GREEN);
			else Game::DisplayClientMessage("%s%s%s is already in your friendslist", MCTF::YELLOW, trgtName.c_str(), MCTF::RED);
			return true;
		}
		else {
			Game::DisplayClientMessage("%sEntity matching the name of %s%s%s was not found, please make sure they are on your render distance", MCTF::RED, MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
			return true;
		}
	}
	return false;
}