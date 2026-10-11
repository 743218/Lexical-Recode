#pragma once
#include "../FuncHook.h"

enum class MouseButton : uint8_t {
	None = 0,
	Left = 1,
	Right = 2
};

enum class MouseAction : bool {
	Press = true,
	Release = false
};

class KeyMouseHook : public FuncHook {
private:
	using func_t = void(__fastcall*)(__int64, char, char, __int16, __int16, __int16, __int16, char);
	static inline func_t oFunc;

	static void mouseInputCallback(__int64 a1, char mouseButton, char isDown, __int16 mouseX, __int16 mouseY, __int16 relativeMovementX, __int16 relativeMovementY, char a8) {

		Game::keyMousePtr = (void*)(a1 + 0x10);

		static ClickGUI* clickGuiMod = ModuleManager::getModule<ClickGUI>();
		if (clickGuiMod->isEnabled()) {
			clickGuiMod->onMouseUpdate(Vec2<float>((float)mouseX, (float)mouseY), mouseButton, isDown);
			return;
		}
		oFunc(a1, mouseButton, isDown, mouseX, mouseY, relativeMovementX, relativeMovementY, a8);
	}
public:
	static void SendClick(const MouseButton& button, const MouseAction& action, const Vec2<float>& mousePos = { 0, 0 }) {
		return oFunc(0, (uint8_t)button, (bool)action, static_cast<__int16>(mousePos.x), static_cast<__int16>(mousePos.y), 0, 0, 0);
	}
public:
	KeyMouseHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&mouseInputCallback;
	}
};