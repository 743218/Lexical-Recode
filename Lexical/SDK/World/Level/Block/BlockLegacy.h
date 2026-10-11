#pragma once
#include <string>
#include "../../../../Utils/MemoryUtil.h"
#include "../../../../Utils/Maths.h"

class BlockSource;
class Block;

class BlockLegacy {
public:
	//CLASS_MEMBER(std::string, blockName, 0x28); tile.something.something
	CLASS_MEMBER(std::string, mName, 0xA0); // minecraft:something_something
	CLASS_MEMBER(uint16_t, blockId, 0x1A6);
public:
	AABB* getOutline(Block* block, BlockSource* blockSource, BlockPos* blockPos, AABB* aabbOut) {
		return MemoryUtil::CallVFunc<9, AABB*>(this, block, blockSource, blockPos, aabbOut);
	}

	bool canBeBuiltOver(BlockSource* region, BlockPos const& blockPos) {
		return MemoryUtil::CallVFunc<104, bool>(this, region, blockPos);
	}

	bool isSolid() {
		return *(bool*)((uintptr_t)this + 0x164);
	}
};
