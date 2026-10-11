#pragma once
#include "../../Utils/MemoryUtil.h"
#include "../../Utils/Maths.h"
#include <functional>

class Actor;
class Player;
class InputMode;
class ItemStack;
class Block;

class GameMode {
public:
	CLASS_MEMBER(Player*, player, 0x8);
	CLASS_MEMBER(BlockPos, lastBreakPos, 0x10);
	CLASS_MEMBER(int32_t, lastBreakFace, 0x1C);
	CLASS_MEMBER(float, lastDestroyProgress, 0x20);
	CLASS_MEMBER(float, destroyProgress, 0x24);
public:
	virtual ~GameMode();
	virtual bool startDestroyBlock(BlockPos const& pos, uint8_t face, bool& hasDestroyedBlock);
	virtual bool destroyBlock(BlockPos const& pos, uint8_t face);
	virtual bool continueDestroyBlock(BlockPos const& pos, uint8_t face, Vec3<float> const& playerPos, bool& hasDestroyedBlock);
	virtual void stopDestroyBlock(BlockPos const& pos);
	virtual void startBuildBlock(BlockPos const& pos, uint8_t face);
	virtual bool buildBlock(BlockPos const& pos, uint8_t face, bool);
	virtual void continueBuildBlock(BlockPos const& pos, uint8_t face);
	virtual void stopBuildBlock();
	virtual void tick();
	virtual float getPickRange(InputMode const& currentInputMode, bool isVR);
	virtual bool useItem(ItemStack& item);
	virtual __int64 useItemOn(ItemStack& item, BlockPos const& at, uint8_t face, Vec3<float> const& hit, Block const* targetBlock);
	virtual bool interact(Actor* entity, Vec3<float> const& location);
	virtual bool attack(Actor* entity);
	virtual void releaseUsingItem();
	virtual void setTrialMode(bool isEnabled);
	virtual bool isInTrialMode();
	virtual void registerUpsellScreenCallback(std::function<void(bool)> callback);
public:
	bool baseUseItem(ItemStack* item) {
		using func_t = bool(__thiscall*)(GameMode*, ItemStack*);
		static func_t Func = reinterpret_cast<func_t>(Addresses::GameMode_baseUseItem);
		return Func(this, item);
	}

	float getDestroyRate(Block* block) {
		using func_t = float(__thiscall*)(GameMode*, Block*);
		static func_t Func = reinterpret_cast<func_t>(Addresses::GameMode_getDestroyRate);
		return Func(this, block);
	}
};
