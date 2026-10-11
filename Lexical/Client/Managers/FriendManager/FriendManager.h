#pragma once
#include "../../SDK/Game.h"
#include <string>
#include <vector>
#include <mutex>

class FriendManager {
public:
	static std::vector<std::string> getList();
	static bool isInList(const std::string& name);
	static bool addToList(const std::string& name);
	static bool removeFromList(const std::string& name);
	static void setFriendList(const std::vector<std::string>& newList);
};