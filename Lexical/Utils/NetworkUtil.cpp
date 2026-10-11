#include "NetworkUtil.h"
#include "FileUtil.h"
// why the fuck do i HAVE to put the includes in the source file, why why WHY
#include "../Libs/curl/curl.h"
#include <windows.h>
#include <WinInet.h>
#include <urlmon.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Libs/curl/libcurl_a.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "advapi32.lib")

bool NetworkUtil::canAccessInternet() {
	char url[] = "https://www.google.com/";
	return InternetCheckConnectionA(url, FLAG_ICC_FORCE_CONNECTION, 0);
}

bool NetworkUtil::downloadFile(std::string name, std::string path, std::string url) {
	std::string filePath = path + name;

	HINTERNET webH = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
	HINTERNET urlFile;

	if (!webH)
		return false;

	urlFile = InternetOpenUrlA(webH, url.c_str(), NULL, NULL, NULL, NULL);

	if (!urlFile) {
		InternetCloseHandle(webH);
		return false;
	}

	std::ofstream outputFile(filePath, std::ios::binary);

	if (!outputFile.is_open()) {
		InternetCloseHandle(webH);
		InternetCloseHandle(urlFile);
		return false;
	}

	char buffer[2000];
	DWORD bytesRead;

	do
	{
		InternetReadFile(urlFile, buffer, 2000, &bytesRead);
		outputFile.write(buffer, bytesRead);
		memset(buffer, 0, 2000);
	} while (bytesRead);

	outputFile.close();
	InternetCloseHandle(webH);
	InternetCloseHandle(urlFile);

	return true;
}

std::string NetworkUtil::getStrings(const std::string& url) {
	HINTERNET webH = InternetOpenA(xorstr_("Mozila/5.0"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);

	if (!webH) return xorstr_("ERROR");

	std::string out;
	HINTERNET urlFile;
	DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE;
	urlFile = InternetOpenUrlA(webH, url.c_str(), NULL, NULL, flags, NULL);

	if (!urlFile) return xorstr_("ERROR");

	DWORD statusCode = 0;
	DWORD statusCodeSize = sizeof(statusCode);
	HttpQueryInfo(urlFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);

	if (statusCode != 200) return xorstr_("ERROR");

	char buffer[2000];
	DWORD bytesRead;
	do {
		InternetReadFile(urlFile, buffer, sizeof(buffer), &bytesRead);
		out.append(buffer, bytesRead);
	} while (bytesRead > 0);

	InternetCloseHandle(urlFile);
	InternetCloseHandle(webH);
	return out;
}

bool NetworkUtil::sendWebhook(const char* botName, const char* content, const char* webhookAddr) {
	CURL* curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);
	if (!curl) return false;
	curl_easy_setopt(curl, CURLOPT_URL, webhookAddr);
	curl_slist* headers = curl_slist_append(NULL, xorstr_("Content-Type: application/json"));

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	std::string json = (std::string(xorstr_("{\"content\": \"")) + content + xorstr_("\", ")) + (std::string(xorstr_("\"username\": \"")) + botName + xorstr_("\"}"));

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) return false;

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return true;
}