#include "Web.h"
#include <wininet.h>
#include <cstdlib>
#include <iostream>

BinData Web::DownloadFile(LPCWSTR url)
{
	HINTERNET hInternet = InternetOpenA("HTTP Request", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
	if (!hInternet) { return {}; }

	HINTERNET hConnect = InternetOpenUrlW(hInternet, url, nullptr, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hConnect)
	{
		InternetCloseHandle(hInternet);
		return {};
	}

	char* fileData = nullptr;
	size_t fileSize = 0;

	char buffer[4096];
	DWORD bytesRead = 0;
	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead))
	{
		// Did we read all bytes?
		if (bytesRead == 0) { break; }

		// Allocate more memory
		fileData = static_cast<char*>(std::realloc(fileData, fileSize + bytesRead));
		if (!fileData)
		{
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hInternet);
			return {};
		}

		// Copy the buffer
		std::memcpy(&fileData[fileSize], buffer, bytesRead);

		fileSize += bytesRead;
	}

	// Cleanup
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return {
		.Data = reinterpret_cast<BYTE*>(fileData),
		.Size = fileSize
	};
}
