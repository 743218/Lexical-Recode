#pragma once
#include "../../SDK/MCTextFormat.h"
#include <random>
#include <regex>

namespace StringUtil {
	inline bool invalidChar(char c) {
		return !(c >= 0 && *reinterpret_cast<unsigned char*>(&c) < 128);
	}

	inline std::string sanitize(const std::string& string) {
		std::string out;
		bool wasValid = true;
		for (char c : string) {
			bool isValid = !invalidChar(c);
			if (wasValid) {
				if (!isValid) wasValid = false;
				else out += c;
			}
			else wasValid = isValid;
		}
		return out;
	}

	inline bool isNumber(char c) {
		return c >= '0' && c <= '9';
	}

	inline std::wstring format(const std::string& string) {
		const int wChars_num = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, NULL, 0);
		wchar_t* wstr = new wchar_t[wChars_num];
		MultiByteToWideChar(CP_UTF8, 0, string.c_str(), -1, wstr, wChars_num);
		const std::wstring wString(wstr);
		delete[] wstr;
		return wString;
	}

	inline std::string format(const std::wstring& wString) {
		const size_t len = std::wcstombs(nullptr, wString.c_str(), 0) + 1;
		char* buffer = new char[len];
		std::wcstombs(buffer, wString.c_str(), len);
		const std::string string(buffer);
		delete[] buffer;
		return string;
	}

	inline std::string formatToGame(const std::string& string) {
		static const std::unordered_map<std::string, std::string> colorPair = {
		{ std::string(MCTF::BLACK), "§0" },
		{ std::string(MCTF::DARK_BLUE), "§1" },
		{ std::string(MCTF::DARK_GREEN), "§2" },
		{ std::string(MCTF::DARK_AQUA), "§3" },
		{ std::string(MCTF::DARK_RED), "§4" },
		{ std::string(MCTF::DARK_PURPLE), "§5" },
		{ std::string(MCTF::GOLD), "§6" },
		{ std::string(MCTF::GRAY), "§7" },
		{ std::string(MCTF::DARK_GRAY), "§8" },
		{ std::string(MCTF::BLUE), "§9" },
		{ std::string(MCTF::GREEN), "§a" },
		{ std::string(MCTF::AQUA), "§b" },
		{ std::string(MCTF::RED), "§c" },
		{ std::string(MCTF::LIGHT_PURPLE), "§d" },
		{ std::string(MCTF::YELLOW), "§e" },
		{ std::string(MCTF::WHITE), "§f" },
		{ std::string(MCTF::OBFUSCATED), "§k" },
		{ std::string(MCTF::BOLD), "§l" },
		{ std::string(MCTF::STRIKETHROUGH), "§m" },
		{ std::string(MCTF::UNDERLINE), "§n" },
		{ std::string(MCTF::ITALIC), "§o" },
		{ std::string(MCTF::RESET), "§r" }
		};
		std::string out = string;
		for (const std::pair<std::string, std::string>& pair : colorPair) {
			const std::regex regex(pair.second);
			out = std::regex_replace(out, regex, pair.first);
		}
		return out;
	}

	inline std::string hash(const std::string& input) {
		uint64_t hash1 = 0xcbf29ce484222325;
		uint64_t hash2 = 0x14650fb0739d0383;
		static constexpr uint64_t prime1 = 6016682199633653;
		static constexpr uint64_t prime2 = 7909364740601323;
		for (char c : input) {
			hash1 ^= c;
			hash1 *= prime1;
			hash2 ^= c;
			hash2 *= prime2;

			hash1 ^= hash1 >> 33;
			hash1 *= 0xff51afd7ed558ccd;
			hash1 ^= hash1 >> 33;
			hash1 *= 0xc4ceb9fe1a85ec53;
			hash1 ^= hash1 >> 33;

			hash2 ^= hash2 >> 33;
			hash2 *= 0xff51afd7ed558ccd;
			hash2 ^= hash2 >> 33;
			hash2 *= 0xc4ceb9fe1a85ec53;
			hash2 ^= hash2 >> 33;

			unsigned int count = 5;
			hash1 += (hash1 << count) | (hash1 >> (64 - count));
			hash2 += (hash2 << count) | (hash2 >> (64 - count));

			count = 15;
			hash1 ^= (hash1 << count) | (hash1 >> (64 - count));
			hash2 ^= (hash2 << count) | (hash2 >> (64 - count));

			count = 27;
			hash1 += (hash1 << count) | (hash1 >> (64 - count));
			hash2 += (hash2 << count) | (hash2 >> (64 - count));
		}

		std::stringstream ss;
		ss << std::hex << std::setw(16) << std::setfill('0') << hash1
			<< std::setw(16) << std::setfill('0') << hash2;

		return ss.str();
	}

	inline std::vector<std::string> getAllLines(std::string& str) {
		std::vector<std::string> tokens;
		std::stringstream ss(str);
		std::string token;
		while (std::getline(ss, token, '\n')) {
			tokens.push_back(token);
		}
		str.clear();
		return tokens;
	}

	inline std::string random(const int size) {
		std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
		std::random_device rd;
		std::mt19937 generator(rd());
		std::shuffle(str.begin(), str.end(), generator);
		return str.substr(0, size);
	}

	inline unsigned int getFileHash(const char* path, int seed) {
		static unsigned int crc32_lut[256] = { 0 };
		if (!crc32_lut[1]) {
			const unsigned int polynomial = 0xEDB88320;
			for (unsigned int i = 0; i < 256; i++) {
				unsigned int crc = i;
				for (unsigned int j = 0; j < 8; j++) {
					crc = (crc >> 1) ^ (((unsigned int)(-((int)(crc & 1)))) & polynomial);
				}
				crc32_lut[i] = crc;
			}
		}

		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) return 0;

		seed = ~seed;
		unsigned int crc = seed;
		char byte;
		while (file.get(byte)) {
			crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ static_cast<uint8_t>(byte)];
		}
		file.close();
		return ~crc;
	}

	inline unsigned int getHashedCRC(const char* str, int seed = 0) {
		static unsigned int crc32_lut[256] = { 0 };
		if (!crc32_lut[1]) {
			const unsigned int polynomial = 0xEDB88320;
			for (unsigned int i = 0; i < 256; i++) {
				unsigned int crc = i;
				for (unsigned int j = 0; j < 8; j++)
					crc = (crc >> 1) ^ (((unsigned int)(-((int)(crc & 1)))) & polynomial);
				crc32_lut[i] = crc;
			}
		}

		seed = ~seed;
		unsigned int crc = seed;
		const uint8_t* current = (const uint8_t*)str;
		while (uint8_t c = *current++) {
			if (c == '#' && current[0] == '#' && current[1] == '#') crc = seed;
			crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
		}
		return ~crc;
	}

	inline unsigned int getFileHash(const char* path, const char* seed) {
		return getFileHash(path, getHashedCRC(seed, 0x6a747764));
	}
}