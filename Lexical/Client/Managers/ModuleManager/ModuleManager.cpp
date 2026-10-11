#include "ModuleManager.h"
#include "../../Client.h"

void ModuleManager::init() {

	//COMBAT
	moduleList.push_back(new AutoCrystal());
	moduleList.push_back(new KillAura());
	moduleList.push_back(new Surround());

	//MOVEMENT
	moduleList.push_back(new ReverseStep());
	moduleList.push_back(new Speed());
	moduleList.push_back(new Clip());
	moduleList.push_back(new NoClip());
	moduleList.push_back(new Fly());
	moduleList.push_back(new Jetpack());
	moduleList.push_back(new NoSlowDown());
	moduleList.push_back(new Velocity());

	//RENDER
	moduleList.push_back(new CustomFov());
	moduleList.push_back(new ESP());
	moduleList.push_back(new Fullbright());
	moduleList.push_back(new NameTags());
	moduleList.push_back(new NoHurtCam());
	moduleList.push_back(new NoRender);
	moduleList.push_back(new ViewModel());

	//PLAYER
	moduleList.push_back(new AntiCrystal());
	moduleList.push_back(new PopCounter());
	moduleList.push_back(new PacketMine());
	moduleList.push_back(new TryDupe1());
	moduleList.push_back(new HotbarSorter());
	moduleList.push_back(new Offhand());

	//WORLD
	moduleList.push_back(new Scaffold());

	//MISC
	moduleList.push_back(new ChatSuffix());
	moduleList.push_back(new Timer());

	//CLIENT
	moduleList.push_back(new Arraylist());
	moduleList.push_back(new ClickGUI());
	moduleList.push_back(new HUD());
	moduleList.push_back(new Colors());
	moduleList.push_back(new CustomFont());
	moduleList.push_back(new TestModule());

	std::sort(moduleList.begin(), moduleList.end(), [](Module* lhs, Module* rhs) {
		return lhs->getModuleName() < rhs->getModuleName();
	});

	getModule<ClickGUI>()->InitClickGUI();

	getModule<Arraylist>()->setEnabled(true);
	getModule<HUD>()->setEnabled(true);
}

void ModuleManager::shutdown() {
	for (auto& mod : moduleList) {
		mod->setEnabled(false);
	}
	for (auto& mod : moduleList) {
		delete mod;
		mod = nullptr;
	} 
	moduleList.clear();
}

void ModuleManager::onKeyUpdate(int key, bool isDown) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		mod->onKeyUpdate(key, isDown);
	}
}

void ModuleManager::onClientTick() {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onClientTick();
		}
	}
}

void ModuleManager::onNormalTick(LocalPlayer* localPlayer) {
	if (!Client::isInitialized()) return;
	if (!localPlayer->level) return;
	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onNormalTick(localPlayer);
		}
	}
}

void ModuleManager::onLevelTick(Level* level) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onLevelTick(level);
		}
	}
}

void ModuleManager::onUpdateRotation(LocalPlayer* localPlayer) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onUpdateRotation(localPlayer);
		}
	}
}

void ModuleManager::onSendPacket(Packet* packet, bool& cancel) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onSendPacket(packet, cancel);
		}
		if (cancel) break;
	}
}


void ModuleManager::onRecievePacket(Packet* packet, bool& cancel) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onRecievePacket(packet, cancel);
		}
		if (cancel) break;
	}
}

void ModuleManager::onD2DRender() {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onD2DRender();
		}
	}
}

void ModuleManager::onImGuiRender(struct ImDrawList* drawList) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onImGuiRender(drawList);
		}
	}
}

void ModuleManager::onMCRender(MinecraftUIRenderContext* renderCtx) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onMCRender(renderCtx);
		}
	}
}

void ModuleManager::onLevelRender() {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onLevelRender();
		}
	}
}

void ModuleManager::onLoadConfig(void* conf) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		mod->onLoadConfig(conf);
	}
}

void ModuleManager::onSaveConfig(void* conf) {
	if (!Client::isInitialized())
		return;

	for (auto& mod : moduleList) {
		mod->onSaveConfig(conf);
	}
}

void ModuleManager::onAttack(Actor* actor, bool& cancel) {
	if (!Client::isInitialized()) return;
	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onAttack(actor, cancel);
		}
		if (cancel) break;
	}
}

void ModuleManager::onBuild(const BlockPos& blockPos, const uint8_t face, bool& cancel) {
	if (!Client::isInitialized()) return;
	for (auto& mod : moduleList) {
		if (mod->isEnabled() || mod->runOnBackground()) {
			mod->onBuild(blockPos, face, cancel);
		}
		if (cancel) break;
	}
}