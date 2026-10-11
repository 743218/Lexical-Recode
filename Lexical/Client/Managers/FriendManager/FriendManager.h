#pragma once
#include "../../SDK/Game.h"

class FriendManager {
public:
	static std::vector<std::string> getList();
	static bool isInList(std::string name);
	static bool addToList(std::string name);
	static bool removeFromList(std::string name);
	static void setFriendList(std::vector<std::string> newList);
};