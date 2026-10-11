#include <Windows.h>
#include "Client/Client.h"
#include "Renderer/D2D.h"
#include "Renderer/ImGuiRenderer.h"
#include "Renderer/MCR.h"

DWORD WINAPI initClient(LPVOID lpParameter) {
    Client::init();

    while (Client::isInitialized()) {
        ModuleManager::onClientTick();
        Sleep(50);
    }

    // Eject path: hooks are already disabled by Client::shutdown(),
    // full cleanup here before unloading so no hook can fire into unmapped code.
    Client::teardown();

    FreeLibraryAndExitThread((HMODULE)lpParameter, 1);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)initClient, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        // No-op if the eject path already tore everything down.
        // No chat message here: game objects may be gone during process detach.
        Client::teardown();
        break;
    }
    return TRUE;
}
