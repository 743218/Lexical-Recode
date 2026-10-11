#pragma once

//COMBAT
#include "Modules/Category/Combat/AutoCrystal.h"
#include "Modules/Category/Combat/KillAura.h"
#include "Modules/Category/Combat/Surround.h"

//MOVEMENT
#include "Modules/Category/Movement/ReverseStep.h"
#include "Modules/Category/Movement/Speed.h"
#include "Modules/Category/Movement/Clip.h"
#include "Modules/Category/Movement/Fly.h"
#include "Modules/Category/Movement/Jetpack.h"
#include "Modules/Category/Movement/NoSlowDown.h"
#include "Modules/Category/Movement/NoClip.h"
#include "Modules/Category/Movement/Velocity.h"

//RENDER
#include "Modules/Category/Render/CustomFov.h"
#include "Modules/Category/Render/ESP.h"
#include "Modules/Category/Render/Fullbright.h"
#include "Modules/Category/Render/NameTags.h"
#include "Modules/Category/Render/NoHurtCam.h"
#include "Modules/Category/Render/NoRender.h"
#include "Modules/Category/Render/ViewModel.h"

//PLAYER
#include "Modules/Category/Player/HotbarSorter.h"
#include "Modules/Category/Player/AntiCrystal.h"
#include "Modules/Category/Player/PopCounter.h"
#include "Modules/Category/Player/PacketMine.h"
#include "Modules/Category/Player/Offhand.h"
#include "Modules/Category/Player/TRyDupe.h"


//WORLD
#include "Modules/Category/World/Scaffold.h"

//MISC
#include "Modules/Category/Misc/ChatSuffix.h"
#include "Modules/Category/Misc/Timer.h"

//CLIENT
#include "Modules/Category/Client/Arraylist.h"
#include "Modules/Category/Client/ClickGUI.h"
#include "Modules/Category/Client/HUD.h"
#include "Modules/Category/Client/Colors.h"
#include "Modules/Category/Client/CustomFont.h"
#include "Modules/Category/Client/TestModule.h"

class ModuleManager {
public:
	static inline std::vector<Module*> moduleList;

	static void init();
	static void shutdown();

	template <typename TRet>
	static TRet* getModule() {
		for (Module* mod : moduleList) {
			TRet* result = dynamic_cast<TRet*>(mod);
			if (result == nullptr)
				continue;
			return result;
		}
		return nullptr;
	}

	static void onKeyUpdate(int key, bool isDown);
	static void onClientTick();
	static void onNormalTick(LocalPlayer* localPlayer);
	static void onLevelTick(Level* level);
	static void onUpdateRotation(LocalPlayer* localPlayer);
	static void onSendPacket(Packet* packet, bool& cancel);
	static void onRecievePacket(Packet* packet, bool& cancel);
	static void onD2DRender();
	static void onImGuiRender(struct ImDrawList* drawList);
	static void onMCRender(MinecraftUIRenderContext* renderCtx);
	static void onLevelRender();
	static void onLoadConfig(void* conf);
	static void onSaveConfig(void* conf);
	
	// Events
	static void onAttack(Actor* actor, bool& cancel);
	static void onBuild(const BlockPos& blockPos, const uint8_t face, bool& cancel);
};