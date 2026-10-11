#pragma once
#include <thread>
#include "../../FuncHook.h"

class LevelTickHook : public FuncHook {
protected:
	static inline bool hasChecked = false;
	static inline float lastCheck = -600;
private:
	using func_t = void(__thiscall*)(Level*);
	static inline func_t oFunc;

	static void LevelTickCallback(Level* _this) {
		if (Game::getLocalPlayer() != nullptr && Game::getLocalPlayer()->level == _this) {
			/*
			if (TimerUtil::hasReached<seconds>(&lastCheck, 30)) {
				if (hasChecked) {
					std::thread check([]() {
						unsigned int result = 0xDEADBEEF;
						std::string server = NetworkUtil::getStrings(xorstr_("https://raw.githubusercontent.com/DeathsPlayground/Server/main/L/hash.txt"));
						std::vector<std::string> hashes = StringUtil::getAllLines(server);
						std::vector<std::string>::iterator it = std::find_if(hashes.begin(), hashes.end(), [&](const std::string& curHash) {
							return strcmp(curHash.c_str(), DLL_HASH.c_str()) == 0;
						});
						if (it != hashes.end()) result = 0xBEEFDEAD;
						if (result == 0xDEADBEEF) {
							const std::string& message = xorstr_("`Invalid DLL Hash | Name: ") + *Game::getLocalPlayer()->getNameTag() + xorstr_(" | DLL ID: ") + DLL_HASH + '`';
							NetworkUtil::sendWebhook(xorstr_("Lexical"), message.c_str(), WEBHOOK_ADDRESS);
							MemoryUtil::makeLeak();
							Client::DisplayClientMessage(xorstr_("%sFailed to validate DLL"), MCTF::DARK_RED);
							Client::shutdown();
							return;
						}
					});
					check.detach();
				}
				if (!hasChecked) hasChecked = true;
			}
			*/
			ModuleManager::onLevelTick(_this);
		}
		return oFunc(_this);
	}
public:
	LevelTickHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&LevelTickCallback;
	}
	~LevelTickHook() {
		lastCheck = -600;
	}
};