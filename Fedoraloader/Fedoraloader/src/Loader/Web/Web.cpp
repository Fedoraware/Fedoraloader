#include "Web.h"
#include <wininet.h>

Binary Web::DownloadFile(const std::wstring& url)
{
	// Init WinINet
	const HINTERNET hInternet = InternetOpen(TEXT("HTTP Request"), INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
	if (!hInternet)
	{
		throw std::system_error(GetLastError(), std::system_category(), "Failed to initialize connection");
	}

	// Open the connection
	const HINTERNET hConnect = InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hConnect)
	{
		throw std::system_error(GetLastError(), std::system_category(), "Failed to open URL");
	}

	Binary fileData = {};

	BYTE buffer[4096];
	DWORD bytesRead = 0;
	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead))
	{
		// Did we read all bytes?
		if (bytesRead == 0) { break; }

		// Copy the buffer
		fileData.insert(fileData.end(), buffer, buffer + bytesRead);
	}

	// Cleanup
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return fileData;
}
