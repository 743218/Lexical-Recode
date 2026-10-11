#include "Client.h"

#include "../Utils/FileUtil.h"
#include "../Utils/NetworkUtil.h"
#include "../Utils/Logger.h"
#include "../Renderer/D2D.h"
#include "../Renderer/ImGuiRenderer.h"

void AddFont(const std::string& fontPath) {
	std::wstring temp = std::wstring(fontPath.begin(), fontPath.end());
	AddFontResource(temp.c_str());
}

void Client::DisplayClientMessage(const char* fmt, ...) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	va_list arg;
	va_start(arg, fmt);
	char message[300];
	vsprintf_s(message, 300, fmt, arg);
	va_end(arg);

	std::string headerMessage;
	if (headerMessage.empty()) {
		char headerCStr[50];
		sprintf_s(headerCStr, 50, "%s[%s%s%s] %s", MCTF::GRAY, MCTF::BLUE, getClientName().c_str(), MCTF::GRAY, MCTF::WHITE);
		headerMessage = std::string(headerCStr);
	}

	std::string messageStr = headerMessage + std::string(message);
	localPlayer->displayClientMessage(messageStr);
}

void Client::init() {
	if (!NetworkUtil::canAccessInternet()) {
		abort();
		return;
	}
	
	std::string ClientPath = FileUtil::getClientPath();
	if (!FileUtil::doesFilePathExist(ClientPath)) {
		FileUtil::createPath(ClientPath);
	}
	else {
		if (!FileUtil::doesFilePathExist(ClientPath + "Maparts\\")) {
			FileUtil::createPath(ClientPath + "Maparts\\");
		}
	}
	Logger::init();
	ConfigManager::init();
	
	/*
	
	
	std::string FontsPath = ClientPath + "Fonts\\";
	if (!FileUtil::doesFilePathExist(FontsPath)) {
		FileUtil::createPath(FontsPath);
	}
	
	std::string ProductSansFontPath = FontsPath + "ProductSans.ttf";
	if (!FileUtil::doesFilePathExist(ProductSansFontPath)) {
		NetworkUtil::downloadFile("ProductSans.ttf", FontsPath, "https://github.com/MerciGG/Font-Repository/releases/download/v1/Product.Sans.Regular.ttf");
	}

	AddFont(ProductSansFontPath);
	*/
	Addresses::init();
	MCTF::initMCTF();

	ModuleManager::init();
	CommandManager::init();
	HookManager::init();

	if (Game::clientInstance == nullptr)
		return;
	/*
	logF("%llx", Game::clientInstance);
	logF("%llx", Game::clientInstance->mcGame);
	logF("%llx", Game::getLocalPlayer());
	*/

	initialized = true;
	NotificationManager::addNotification("Successfully injected Lexical!", 5.f);

	if (!ConfigManager::doesConfigExist("default"))
		ConfigManager::createNewConfig("default");

	ConfigManager::loadConfig("default");

}

void Client::shutdown() {
	// Phase 1: safe to call from any thread (chat command, render thread, level thread).
	// Stops all future hook entries and manager dispatches immediately.
	if (!initialized)
		return;

	DisplayClientMessage("%sEjected", MCTF::RED);
	ConfigManager::saveConfig();
	initialized = false;
	MH_DisableHook(MH_ALL_HOOKS);
}

void Client::teardown() {
	// Phase 2: full cleanup. Runs once on the init thread before FreeLibrary,
	// or on DLL_PROCESS_DETACH (game closing). Never call from inside a hook.
	static bool teardownDone = false;
	if (teardownDone)
		return;
	teardownDone = true;

	initialized = false;
	MH_DisableHook(MH_ALL_HOOKS);
	Sleep(300); // let any in-flight hook invocation drain
	HookManager::shutdown();
	ModuleManager::shutdown();
	CommandManager::shutdown();
	ImGuiRenderer::Cleanup();
	D2D::Clean();
}