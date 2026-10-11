#pragma once
#include "Managers/ConfigManager/ConfigManager.h"
#include "Managers/ModuleManager/ModuleManager.h"
#include "Managers/CommandManager/CommandManager.h"
#include "Managers/HookManager/HookManager.h"
#include "Managers/NotificationManager/NotificationManager.h"

class Client {
private:
	static inline bool initialized = false;
	static inline std::string clientName = "Lexical";
	static inline std::string version = "1.0";
public:
	static std::string getVersion() { return version; };
	static inline bool isInitialized() { return initialized; }
	static std::string getClientName() { return clientName; }
	static void setClientName(std::string newName) { clientName = newName;  }
	static void DisplayClientMessage(const char* fmt, ...);
	static void init();
	static void shutdown();
	static void teardown();
};
