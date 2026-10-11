#pragma once
#include <vector>
#include "Packet.h"

class SetScorePacket : public Packet {
public:
	enum class ScoreType : uint8_t {
		Change = 0x0,
		Remove = 0x1
	};
	struct ScoreboardId {
	public:
		int64_t mRawId;
		class IdentityDefinition* mIdentityDef;
	};
	class IdentityDefinition {
	public:
		enum class Type : uint8_t {
			Invalid = 0x0,
			Player = 0x1,
			Entity = 0x2,
			FakePlayer = 0x3
		};
		ScoreboardId mScoreBoardId;
		bool mIsHiddenFakePlayer;
		uint64_t mUniqueId;
		int64_t mEntityId;
		std::string mPlayerName;
		Type identityType;
	};

	struct ScoreInfo {
	public:
		ScoreboardId mScoreboardId;
		std::string mObjectiveName;
		int mScoreValue;
		IdentityDefinition::Type mIdentityType;
		uint64_t mUniqueId;
		int64_t mEntityId;
		std::string fakePlayerName;
	};

	ScoreType scoreType;
	std::vector<ScoreInfo> mInfo;
};