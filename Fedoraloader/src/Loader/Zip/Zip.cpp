#include "Zip.h"

#include <stdexcept>
#include <miniz/miniz.h>

bool FindBySuffix(mz_zip_archive& archive, LPCSTR suffix, mz_uint& outIndex)
{
	mz_zip_archive_file_stat fileStat;
	for (mz_uint i = 0; i < archive.m_total_files; i++)
	{
		// Retrieve file info
		if (!mz_zip_reader_file_stat(&archive, i, &fileStat))
		{
			continue;
		}

		// Check file suffix
		const auto curFile = std::string(fileStat.m_filename);
		if (curFile.ends_with(suffix))
		{
			outIndex = i;
			return true;
		}
	}

	return false;
}

void Zip::UnpackFile(Binary& file)
{
	mz_zip_archive zipArchive{};

	// Init zip reader
	if (!mz_zip_reader_init_mem(&zipArchive, file.Data, file.Size, 0))
	{
		throw std::runtime_error("Failed to initialize zip reader");
	}

	// Find the dll file
	mz_uint fileIndex;
	if (!FindBySuffix(zipArchive, ".dll", fileIndex))
	{
		throw std::runtime_error("Dll file not found in archive");
	}

	// Extract the dll file
	size_t bufferSize;
	void* buffer = mz_zip_reader_extract_to_heap(&zipArchive, fileIndex, &bufferSize, 0);
	if (!buffer)
	{
		mz_zip_reader_end(&zipArchive);
		throw std::runtime_error("Failed to extract file from ZIP");
	}

	// Cleanup
	mz_zip_reader_end(&zipArchive);
	std::free(file.Data);

	// Update the file
	file.Data = static_cast<BYTE*>(buffer);
	file.Size = bufferSize;
}
