#pragma once
#include "../Libs/xorstr.hpp"
#include <string>

#define CURL_STATICLIB
#define WEBHOOK_ADDRESS xorstr_("https://discord.com/api/webhooks/1263761353064321085/stwZNFoZWczRJONbZsx7wz_eCFyk1GMR9fIUhi6eE1LybtX8kXHOzd02-21pAClntHyQ")

namespace NetworkUtil {
	bool canAccessInternet();
	bool downloadFile(std::string name, std::string path, std::string url);
	bool sendWebhook(const char* botName, const char* content, const char* webhookAddr);
	std::string getStrings(const std::string& url);
}
