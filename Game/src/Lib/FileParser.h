#pragma once

#include "Core.h"

namespace FileParser
{
constexpr static char END_LINE = '\n';
constexpr static char END_COLUMN = ';';
constexpr static const char* EXTENSION = ".scal";

struct FileParserState
{

	u8* Data;
	int DataLength;
	int FileVersion;
	int StateError;
	int CurrentLine;
};

FileParserState FileParserInit(const char* filePath)
{
	SAssert(filePath);
	
	if (!FileExists(filePath))
	{
		SError("File %s does not exist!", filePath);
	}

	if (!strcmp(GetFileExtension(filePath), EXTENSION))
	{
		SError("File extensions do not match!");
	}

	zpl_file file;
	zpl_file_error err = zpl_file_open(&file, filePath);
	if (err)
	{
		SError("Error occured opening file %s, error id: %d", filePath, (int)err);
	}

	zpl_file_read_lines

}

}
