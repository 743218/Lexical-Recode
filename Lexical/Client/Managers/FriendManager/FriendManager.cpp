#include "FriendManager.h"

std::vector<std::string> friendList;

std::vector<std::string> FriendManager::getList() {
	return friendList;
}

bool FriendManager::isInList(std::string name) {
	if (friendList.empty()) return false;
	for (int i = 0; i < friendList.size(); i++) {
		std::string curFriend = friendList[i];
		if (curFriend == name) return true;
	}
	return false;
}

bool FriendManager::addToList(std::string name) {
	if (!isInList(name)) {
		friendList.push_back(name);
		return true;
	}
	return false;
}

bool FriendManager::removeFromList(std::string name) {
	if (friendList.empty()) return false;
	if (isInList(name)) {
		for (int i = 0; i < friendList.size(); i++) {
			std::string curFriend = friendList[i];
			if (curFriend == name) {
				friendList.erase(friendList.begin() + i);
				return true;
			}
		}
	}
	return false;
}

void FriendManager::setFriendList(std::vector<std::string> newList) {
	friendList = newList;
}