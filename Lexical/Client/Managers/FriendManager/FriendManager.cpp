#include "FriendManager.h"

static std::vector<std::string> friendList;
static std::mutex friendListMutex;

std::vector<std::string> FriendManager::getList() {
	std::lock_guard<std::mutex> lock(friendListMutex);
	return friendList;
}

bool FriendManager::isInList(const std::string& name) {
	std::lock_guard<std::mutex> lock(friendListMutex);
	for (const auto& curFriend : friendList) {
		if (curFriend == name)
			return true;
	}
	return false;
}

bool FriendManager::addToList(const std::string& name) {
	std::lock_guard<std::mutex> lock(friendListMutex);
	for (const auto& curFriend : friendList) {
		if (curFriend == name)
			return false;
	}
	friendList.push_back(name);
	return true;
}

bool FriendManager::removeFromList(const std::string& name) {
	std::lock_guard<std::mutex> lock(friendListMutex);
	for (auto it = friendList.begin(); it != friendList.end(); ++it) {
		if (*it == name) {
			friendList.erase(it);
			return true;
		}
	}
	return false;
}

void FriendManager::setFriendList(const std::vector<std::string>& newList) {
	std::lock_guard<std::mutex> lock(friendListMutex);
	friendList = newList;
}