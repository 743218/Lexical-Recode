#pragma once
#include "../../Utils/MemoryUtil.h"
#include "Effect/MobEffect.h"
#include "Effect/MobEffectInstance.h"
#include "Item/ItemStack.h"
#include "Level/Level.h"
#include "EntityId.h"

#include "ActorFlags.h"
#include "Components/AttributesComponent.h"
#include "Components/AABBShapeComponent.h"
#include "Components/ActorEquipmentComponent.h"
#include "Components/ActorHeadRotationComponent.h"
#include "Components/ActorRotationComponent.h"
#include "Components/ActorTypeComponent.h"
#include "Components/AnimationComponent.h"
#include "Components/DimensionTypeComponent.h"
#include "Components/MobBodyRotationComponent.h"
#include "Components/MoveInputComponent.h"
#include "Components/RenderPositionComponent.h"
#include "Components/RuntimeIDComponent.h"
#include "Components/StateVectorComponent.h"
#include "Components/ActorOwnerComponent.h"
#include "Components/FlagComponent.h"

class Actor {
public:
    EntityContext entityContext;
    CLASS_MEMBER(Level*, level, 0x250);
    CLASS_MEMBER(StateVectorComponent*, stateVector, 0x290);
    CLASS_MEMBER(AABBShapeComponent*, aabbShape, 0x298);
    CLASS_MEMBER(ActorRotationComponent*, rotation, 0x2A0);
public:
    bool onGround() {
        return entityContext.hasComponent<OnGroundFlagComponent>();
    }
    void jumpFromGround() {
        entityContext.getOrAddComponent<FlagComponent<JumpFromGroundRequestFlag>>();
    }
    void setOnGround(bool state) {
        if (state) {
            entityContext.getOrAddComponent<OnGroundFlagComponent>();
        }
        else {
            entityContext.removeComponent<OnGroundFlagComponent>();
        }
    };
    bool isInWater() const {
        return entityContext.hasComponent<FlagComponent<InWaterFlag>>();
    }
    bool isInLava() const {
        return entityContext.hasComponent<FlagComponent<InLavaFlag>>();
    }
    bool isInLiquid() const {
        return this->isInWater() || this->isInLava();
    }

    ActorEquipmentComponent* getActorEquipmentComponent() {
        using func_t = ActorEquipmentComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getActorEquipmentComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    ActorHeadRotationComponent* getActorHeadRotationComponent() {
        using func_t = ActorHeadRotationComponent * (__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getActorHeadRotationComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    ActorTypeComponent* getActorTypeComponent() {
        using func_t = ActorTypeComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getActorTypeComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    DimensionTypeComponent* getDimensionTypeComponent() {
        // commands.tp.notSameDimension
        using func_t = DimensionTypeComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getDimensionTypeComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    MobBodyRotationComponent* getMobBodyRotationComponent() {
        using func_t = MobBodyRotationComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getMobBodyRotationComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    MoveInputComponent* getMoveInputComponent() {
        using func_t = MoveInputComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getMoveInputComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    RenderPositionComponent* getRenderPositionComponent() {
        using func_t = RenderPositionComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getRenderPositionComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    RuntimeIDComponent* getRuntimeIDComponent() {
        using func_t = RuntimeIDComponent*(__cdecl*)(__int64, __int64);
        static func_t Func = (func_t)(Addresses::try_getRuntimeIDComponent);
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

    AttributesComponent* getAttributesComponent() {
        return *entityContext.tryGetComponent<AttributesComponent*>();
    }

    AttributeInstance* getAttribute(AttributeId id) {
        std::unordered_map<int, AttributeInstance> attrMap = getAttributesComponent()->baseMap.attributes;
        std::unordered_map<int, AttributeInstance>::iterator it = attrMap.find((int)id);
        if (it != attrMap.end()) return &it->second;
        return nullptr;

    }
    Vec3<float> getPos() {
        return stateVector->pos;
    }

    Vec3<float> getEyePos() {
        return getRenderPositionComponent()->eyePos;
    }

    float calculateAttackDamage(Actor* target) {
        using func_t = float(__thiscall*)(Actor*, Actor*);
        static func_t Func = (func_t)Addresses::Actor_calculateAttackDamage;
        return Func(this, target);
    }

    std::string* getNameTag() {
        using func_t = std::string*(__thiscall*)(Actor*);
        static func_t Func = (func_t)Addresses::Actor_getNameTag;
        return Func(this);
    }

    std::string getNameTag2() {
        using func_t = std::string* (__thiscall*)(Actor*);
        static func_t Func = (func_t)Addresses::Actor_getNameTag;
        return *Func(this);
    }

    ItemStack* getOffhandSlot() {
        ActorEquipmentComponent* actorEquipmentComponent = getActorEquipmentComponent();
        if (actorEquipmentComponent == nullptr)
            return nullptr;

        SimpleContainer* offhandContainer = actorEquipmentComponent->offhandContainer;

        return (ItemStack*)(*(__int64(__fastcall**)(SimpleContainer*, __int64))(*(uintptr_t*)offhandContainer + 56i64))(
            offhandContainer,
            1i64);
    }

    ItemStack* getArmor(int slot) {
        ActorEquipmentComponent* actorEquipmentComponent = getActorEquipmentComponent();
        if (actorEquipmentComponent == nullptr)
            return nullptr;

        return actorEquipmentComponent->armorContainer->getItem(slot);
    }

    bool hasEffect(MobEffect* effect) {
        using hasEffect_t = bool(__thiscall*)(Actor*, MobEffect*);
        static hasEffect_t func = (hasEffect_t)Addresses::Actor_hasEffect;
        return func(this, effect);
    }

    MobEffectInstance* getEffect(MobEffect* effect) {
        using getEffect_t = MobEffectInstance*(__thiscall*)(Actor*, MobEffect*);
        static getEffect_t func = (getEffect_t)Addresses::Actor_getEffect;
        return func(this, effect);
    }

    void setPos(const Vec3<float>& pos) {
        using func_t = __int64(__thiscall*)(Actor*, const Vec3<float>&);
        static func_t Func = (func_t)(Addresses::Actor_setPos);
        Func(this, pos);
    }

    bool hasHorizontalCollision() {
        using func_t = bool(__thiscall*)(__int64, __int64);
        static func_t Func = (func_t)Addresses::ActorCollision_hasHorizontalCollision;
        __int64 a1 = *(__int64*)((__int64)this + 0x10);
        __int64 a2 = (__int64)&(*(__int64*)((__int64)this + 0x18));
        return Func(a1, a2);
    }

   // void setOnGround(bool isOnGround) {
        // Search "1 MovePlayerPacket - server no vehicle" -> sub_XXXXXX(v7 + 8, *(unsigned __int8 *)(a3 + XX));
   //     using func_t = void(__thiscall*)(__int64, bool);
   //     static func_t Func = (func_t)Addresses::ActorCollision_setOnGround;
   //     Func((__int64)((uintptr_t)this + 0x8), isOnGround);
   // }
    

    void lerpMotion(const Vec3<float>& delta) {
        MemoryUtil::CallVFunc<24, void, const Vec3<float>&>(this, delta);
    }

    std::string getEntityLocNameString() {
        return *MemoryUtil::CallVFunc<32, std::string*>(this, std::string());
    }

    bool isAlive() {
        return MemoryUtil::CallVFunc<50, bool>(this);
    }

    ItemStack* getCarriedItem() {
        return MemoryUtil::CallVFunc<83, ItemStack*>(this);
    }

    void setOffhandSlot(ItemStack* itemStack) {
        MemoryUtil::CallVFunc<84, void, ItemStack*>(this, itemStack);
    }

    AnimationComponent* getAnimationComponent() {
        return MemoryUtil::CallVFunc<115, AnimationComponent*>(this);
    }

    void swing() {
        MemoryUtil::CallVFunc<117, void>(this);
    }
    int getDeathTime() {
        return MemoryUtil::CallVFunc<110, int>(this);
    }
public:
    virtual bool getStatusFlag(ActorFlags flag);
    virtual void setStatusFlag(ActorFlags flag, bool value);
};
