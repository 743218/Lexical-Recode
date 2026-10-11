#pragma once
#include "../FuncHook.h"

// this is homeless ik boohoo

class TextPacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	TextPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};

class LevelEventPacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	LevelEventPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};


class LevelSoundEventPacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	LevelSoundEventPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};


class ActorEventPacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	ActorEventPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};

class RemoveActorPacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	RemoveActorPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};



class SetScorePacketHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(const float*, const float*, const float*, std::shared_ptr<Packet>);
	static inline func_t oFunc;

	static void callback(const float* a1, const float* a2, const float* a3, std::shared_ptr<Packet> packet) {
		bool cancel = false;
		ModuleManager::onRecievePacket(packet.get(), cancel);
		if (!cancel) oFunc(a1, a2, a3, packet);
	}
public:
	SetScorePacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};

