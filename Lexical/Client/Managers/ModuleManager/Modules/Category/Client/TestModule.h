#pragma once
#include "../../ModuleBase/Module.h"

class TestModule : public Module {
private:
	bool packetLogger = false;
	bool noAuth = false;
	bool noMove = false;
public:
	TestModule();
	virtual void onNormalTick(LocalPlayer* lp) override;
	virtual void onSendPacket(Packet* packet, bool& cancel) override;
	virtual void onMCRender(MinecraftUIRenderContext* renderCtx) override;
	virtual void onLevelRender() override;
	virtual void onD2DRender() override;
	virtual void onRecievePacket(Packet* packet, bool& cancel) override;
	virtual void onEnable() override;
	virtual void onDisable() override;
};