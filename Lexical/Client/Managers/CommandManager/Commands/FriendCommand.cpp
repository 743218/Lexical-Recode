#include "FriendCommand.h"
#include "../../Utils/Minecraft/TargetUtil.h"

FriendCommand::FriendCommand() : CommandBase("friend", "Add/Remove people to friend list", "<add/remove> <name>", { "f" }) {
}

bool FriendCommand::execute(const std::vector<std::string>& args) {
	if (args.size() < 3)
		return false;

	std::string searchedName = args[2];
	if (searchedName.empty())
		return false;

	std::transform(searchedName.begin(), searchedName.end(), searchedName.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });

	if (args[1] == "remove") {
		std::string trgtName;
		const auto list = FriendManager::getList();
		for (const auto& name : list) {
			std::string curName = name;
			std::transform(curName.begin(), curName.end(), curName.begin(),
				[](unsigned char c) { return (char)std::tolower(c); });
			if (curName.find(searchedName) != std::string::npos) {
				trgtName = name;
				break;
			}
		}

		if (!trgtName.empty()) {
			if (FriendManager::removeFromList(trgtName))
				Game::DisplayClientMessage("%sRemoved %s%s%s from list.", MCTF::GREEN, MCTF::YELLOW, trgtName.c_str(), MCTF::GREEN);
			else
				Game::DisplayClientMessage("%s%s%s was not in your friendslist", MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
			return true;
		}

		Game::DisplayClientMessage("%s%s%s was not in your friendslist", MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
		return true;
	}
	else if (args[1] == "add") {
		LocalPlayer* localPlayer = Game::getLocalPlayer();
		if (localPlayer == nullptr || localPlayer->level == nullptr) {
			Game::DisplayClientMessage("%sUnable to resolve local player or level.", MCTF::RED);
			return false;
		}

		Actor* target = nullptr;
		const auto actorList = localPlayer->level->getRuntimeActorList();
		for (Actor* actor : actorList) {
			if (actor == nullptr)
				continue;
			if (!TargetUtil::isTargetValid(actor, false, false))
				continue;

			std::string* nameTag = actor->getNameTag();
			if (nameTag == nullptr)
				continue;

			std::string actorName = *nameTag;
			std::transform(actorName.begin(), actorName.end(), actorName.begin(),
				[](unsigned char c) { return (char)std::tolower(c); });

			if (actorName.find(searchedName) != std::string::npos || actorName == searchedName) {
				target = actor;
				break;
			}
		}

		if (target != nullptr) {
			std::string* targetTag = target->getNameTag();
			if (targetTag == nullptr) {
				Game::DisplayClientMessage("%sTarget has no nametag.", MCTF::RED);
				return false;
			}
			std::string trgtName = *targetTag;
			if (FriendManager::addToList(trgtName))
				Game::DisplayClientMessage("%sAdded %s%s%s to list.", MCTF::GREEN, MCTF::YELLOW, trgtName.c_str(), MCTF::GREEN);
			else
				Game::DisplayClientMessage("%s%s%s is already in your friendslist", MCTF::YELLOW, trgtName.c_str(), MCTF::RED);
			return true;
		}

		Game::DisplayClientMessage("%sEntity matching the name of %s%s%s was not found, please make sure they are on your render distance", MCTF::RED, MCTF::YELLOW, searchedName.c_str(), MCTF::RED);
		return true;
	}

	return false;
}