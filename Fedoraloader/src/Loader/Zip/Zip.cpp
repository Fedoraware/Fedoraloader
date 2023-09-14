#include "Zip.h"

#include <stdexcept>
#include <miniz/miniz.h>

void Zip::UnpackFile(Binary& file, const char* fileName)
{
	mz_zip_archive zipArchive{};

	// Init zip reader
	if (!mz_zip_reader_init_mem(&zipArchive, file.Data, file.Size, 0))
	{
		throw std::runtime_error("Failed to initialize zip reader");
	}

	// Find the dll file | TODO: Look for the first .dll file
	const int fileIndex = mz_zip_reader_locate_file(&zipArchive, fileName, nullptr, 0);
	if (fileIndex < 0)
	{
		throw std::runtime_error("Dll file not found in archive");
	}

	// Get dll file info
	mz_zip_archive_file_stat fileStat;
	if (!mz_zip_reader_file_stat(&zipArchive, fileIndex, &fileStat))
	{
		mz_zip_reader_end(&zipArchive);
		throw std::runtime_error("Failed to retrieve archive file info");
	}

	// Extract the dll file
	const size_t bufferSize = static_cast<size_t>(fileStat.m_uncomp_size) * sizeof(BYTE);
	const auto buffer = static_cast<BYTE*>(std::malloc(bufferSize));
	if (!mz_zip_reader_extract_to_mem(&zipArchive, fileIndex, buffer, bufferSize, 0))
	{
		std::free(buffer);
		mz_zip_reader_end(&zipArchive);
		throw std::runtime_error("Failed to extract file from ZIP");
	}

	// Cleanup
	mz_zip_reader_end(&zipArchive);
	std::free(file.Data);

	// Update the file
	file.Data = buffer;
	file.Size = bufferSize;
}
