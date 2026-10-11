#pragma once
#include <string>
#include <unordered_map>

enum AttributeId {
    ZombieSpawnReinforcementsChange = -1,
    PlayerHunger = 2,
    PlayerSaturation = 3,
    PlayerExhaustion = 4,
    PlayerLevel = 5,
    PlayerExperience = 6,
    Health = 7,
    FollowRange = 8,
    KnockbackResistance = 9,
    MovementSpeed = 10,
    UnderwaterMovementSpeed = 11,
    LavaMovementSpeed = 12,
    AttackDamage = 13,
    Absorption = 14,
    Luck = 15,
    JumpStrength = 16
};

class AttributeInstance {
private:
    char __padding[0x74];
public:
    float minimumValue;
    float maximumValue;
    float currentValue;
};

class Attribute {
public:
    __int64 hash;
    __int64 hashedStringHash;
    std::string attributeName;
private:
    char __padding[0x32];
public:

    Attribute() {
        memset(this, 0x0, sizeof(Attribute));
    }

    Attribute(__int64 hash) {
        memset(this, 0x0, sizeof(Attribute));
        this->hash = hash;
    }
};

enum AttributeHashes : unsigned __int64 {
    HEALTH = 30064771328 - 4294967296,
    HUNGER = 8589934848 - 4294967296,
    ABSORPTION = 60129542401 - 4294967296,
};

class HealthAttribute : public Attribute {
public:
    HealthAttribute() { this->hash = AttributeHashes::HEALTH; }
};

class PlayerHungerAttribute : public Attribute {
public:
    PlayerHungerAttribute() { this->hash = AttributeHashes::HUNGER; }
};

class AbsorptionAttribute : public Attribute {
public:
    AbsorptionAttribute() { this->hash = AttributeHashes::ABSORPTION; }
};