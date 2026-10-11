#pragma once
#include <windows.h>
#include <Psapi.h>
#include <intrin.h>
#include <memory>
#include <string_view>
#include <unordered_set>
#include "../Libs/xorstr.hpp"
#include "../SDK/Addresses.h"
#include <string>

#define CLASS_MEMBER(type, name, offset)																			\
__declspec(property(get = get##name, put = set##name)) type name;													\
inline type& get##name() { return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); }			\
inline void set##name(type v) { *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset) = v; }

#define FindSignature(str) MemoryUtil::findSignature(xorstr_(str));
#define FindVTable(str) MemoryUtil::getVtableFromSig(xorstr_(str));
#define FindFuncFromCall(str) MemoryUtil::getFuncFromCall(MemoryUtil::findSignature(xorstr_(str)));

#define HASH_KEY xorstr_("TekitFunc")
#define DLL_HASH std::to_string(StringUtil::getFileHash(MemoryUtil::locateDirectory().c_str(), StringUtil::hash(HASH_KEY).c_str())) // hash should be known at compile time

namespace MemoryUtil {
	uintptr_t getGameAddress();
	uintptr_t findSignature(std::string_view sig);
	uintptr_t** getVtableFromSig(std::string_view sig);

	template <unsigned int IIdx, typename TRet, typename... TArgs>
	inline TRet CallVFunc(void* thisptr, TArgs... argList) {
		using Fn = TRet(__thiscall*)(void*, decltype(argList)...);
		return (*static_cast<Fn**>(thisptr))[IIdx](thisptr, argList...);
	}

	inline uintptr_t getFuncFromCall(uintptr_t address) {
		if (address == 0) 
			return 0;

		return address + 1 + 4 + *(int*)(address + 1); // Trong smart code
	}

	inline void nopBytes(void* dst, unsigned int size) {
		DWORD oldprotect;
		VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
		memset(dst, 0x90, size);
		VirtualProtect(dst, size, oldprotect, &oldprotect);
	}

	inline void copyBytes(void* src, void* dst, unsigned int size) {
		DWORD oldprotect;
		VirtualProtect(src, size, PAGE_EXECUTE_READWRITE, &oldprotect);
		memcpy(dst, src, size);
		VirtualProtect(src, size, oldprotect, &oldprotect);
	}

	inline void patchBytes(void* dst, void* src, unsigned int size) {
		DWORD oldprotect;
		VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
		memcpy(dst, src, size);
		VirtualProtect(dst, size, oldprotect, &oldprotect);
	}

	inline std::unordered_set<std::string> getRunningApp() {
		std::unordered_set<std::string> titles;
		HWND hwnd = GetTopWindow(NULL);
		while (hwnd != NULL) {
			if (IsWindowVisible(hwnd)) {
				int length = GetWindowTextLength(hwnd);
				if (length > 0) {
					std::vector<char> buffer(length + 1);
					GetWindowTextA(hwnd, buffer.data(), length + 1);
					titles.insert(buffer.data());
				}
			}
			hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
		}
		return titles;
	}

	inline std::string getCPUId() {
		int CPUInfo[4] = { -1 };
		__cpuid(CPUInfo, 1);
		std::string rtn;

		for (int i = 0; i < 4; i++) {
			std::stringstream sstream;
			sstream << std::hex << CPUInfo[i];
			std::string part = sstream.str();

			if (i == 1) {
				while (part.length() < 8) {
					part = '0' + part;
				}
				for (int i = 0; i < 3; ++i) {
					part[i] = '*';
				}
			}
			rtn += part;
			if (i != 3) rtn += ':';
		}
		return rtn;
	}

	inline std::string getProfile() {
		HW_PROFILE_INFO hwInfo;

		std::wstring hardware_guid;
		GetCurrentHwProfile(&hwInfo);
		hardware_guid = hwInfo.szHwProfileGuid;

		std::string hwid(hardware_guid.begin(), hardware_guid.end());
		return hwid;
	}

	inline void makeLeak() { // makes a memory leak
		std::vector<int> vec;
		while (true) {
			vec.push_back(1);
		}
	}

	inline std::string locateDirectory() {
		char path[MAX_PATH];
		HMODULE hMod = NULL;

		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&locateDirectory, &hMod) == 0) {
			return (xorstr_("GMH failure"));
		}
		if (GetModuleFileNameA(hMod, path, sizeof(path)) == 0) {
			return (xorstr_("GMFN failure"));
		}

		std::string out = (path);
		return (out);
	}
}

static const char* const KeyNames[] = {
	"Unknown",
	"VK_LBUTTON",
	"VK_RBUTTON",
	"VK_CANCEL",
	"VK_MBUTTON",
	"VK_XBUTTON1",
	"VK_XBUTTON2",
	"Unknown",
	"Backspace",
	"Tab",
	"Unknown",
	"Unknown",
	"VK_CLEAR",
	"VK_RETURN",
	"Unknown",
	"Unknown",
	"Shift",
	"Ctrl",
	"Alt",
	"VK_PAUSE",
	"VK_CAPITAL",
	"VK_KANA",
	"Unknown",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_KANJI",
	"Unknown",
	"VK_ESCAPE",
	"VK_CONVERT",
	"VK_NONCONVERT",
	"VK_ACCEPT",
	"VK_MODECHANGE",
	"VK_SPACE",
	"VK_PRIOR",
	"VK_NEXT",
	"VK_END",
	"VK_HOME",
	"VK_LEFT",
	"VK_UP",
	"VK_RIGHT",
	"VK_DOWN",
	"VK_SELECT",
	"VK_PRINT",
	"VK_EXECUTE",
	"VK_SNAPSHOT",
	"Insert",
	"Delete",
	"VK_HELP",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"VK_LWIN",
	"VK_RWIN",
	"VK_APPS",
	"Unknown",
	"VK_SLEEP",
	"NUMPAD0",
	"NUMPAD1",
	"NUMPAD2",
	"NUMPAD3",
	"NUMPAD4",
	"NUMPAD5",
	"NUMPAD6",
	"NUMPAD7",
	"NUMPAD8",
	"NUMPAD9",
	"VK_MULTIPLY",
	"VK_ADD",
	"VK_SEPARATOR",
	"VK_SUBTRACT",
	"VK_DECIMAL",
	"VK_DIVIDE",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"VK_F13",
	"VK_F14",
	"VK_F15",
	"VK_F16",
	"VK_F17",
	"VK_F18",
	"VK_F19",
	"VK_F20",
	"VK_F21",
	"VK_F22",
	"VK_F23",
	"VK_F24",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"VK_NUMLOCK",
	"VK_SCROLL",
	"VK_OEM_NEC_EQUAL",
	"VK_OEM_FJ_MASSHOU",
	"VK_OEM_FJ_TOUROKU",
	"VK_OEM_FJ_LOYA",
	"VK_OEM_FJ_ROYA",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"LeftShift",
	"RightShift",
	"VK_LCONTROL",
	"VK_RCONTROL",
	"VK_LMENU",
	"VK_RMENU",
	"Browser Back",
	"Browser Forward",
	"Browser Refresh",
	"Browser Stop",
	"Browser Search",
	"Browser Favourites",
	"Browser Home",
	"Volume Mute",
	"Volume Down",
	"Volume Up",
	"Media_Next",
	"Media_Prev",
	"Media_Stop",
	"Media_Pause",
	"Mail",
	"Media",
	"App1",
	"App2",
	"Unknown",
	"Unknown",
	"OEM_1",
	"PLUS",
	"COMMA",
	"MINUS",
	"DOT",
	"OEM_2",
	"OEM_3" 
};