#pragma once
#include "../../Utils/MemoryUtil.h"
#include "../World/Level/BlockSource.h"
#include "../World/LocalPlayer.h"
#include "../Render/LevelRenderer.h"
#include "MinecraftGame.h"
#include "Minecraft.h"
#include "GuiData.h"
#include "ClientHMDState.h"
#include "mce.h"

class ClientInstance {
public:
	CLASS_MEMBER(MinecraftGame*, mcGame, 0xC8); // "textures/ui/pointer" -> xrefs first function -> xref second line -> what acces to that vtable -> what acces to a2
	CLASS_MEMBER(Minecraft*, minecraft, 0xD0);
	CLASS_MEMBER(GuiData*, guiData, 0x558);	// "ui_invert_overlay" -> xrefs first function -> continue xrefs -> continue xrefs -> offset below!
public:
	ClientHMDState* getHMDState() {
		// Offset: 48 8d 8e ? ? ? ? 48 8b d6 e8 ? ? ? ? 90 48 8d 8e ? ? ? ? e8 + 3
		return reinterpret_cast<ClientHMDState*>((uintptr_t)this + 0x5A8);
	}

	BlockSource* getRegion() {
		// getLocalPlayer index - 1
		return MemoryUtil::CallVFunc<28, BlockSource*>(this);
	}

	LocalPlayer* getLocalPlayer() {
		// IDA: search for string "variable.player_x_rotation" -> xrefs first function -> find what access to a3
		return MemoryUtil::CallVFunc<29, LocalPlayer*>(this);
	}

	LevelRenderer* getLevelRenderer() {
		// IDA: search for string "variable.player_x_rotation" -> xrefs first function -> find what function have 4 param and access with a3 -> what acces a3
		return MemoryUtil::CallVFunc<204, LevelRenderer*>(this);
	}

	void grabMouse() {
		// IDA: search for string "#hotbar_size_y" -> xrefs first function -> vtable of a1 -> the function that below the second "_guard_check_icall_nop" -> idk lol
		MemoryUtil::CallVFunc<332, void>(this);
	}

	void releasebMouse() {
		// grabMouse index + 1
		MemoryUtil::CallVFunc<333, void>(this);
	}
};
