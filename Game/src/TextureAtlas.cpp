#include "TextureAtlas.h"

#include "GameState.h"

#define HashString(str, len) zpl_fnv32a(str, len);

SpriteAtlas SpriteAtlasLoad(const char* dirPath, const char* atlasFile)
{
	SASSERT(dirPath);
	SASSERT(atlasFile);

	const char* atlasFiles[2] = { dirPath, atlasFile };
	const char* filepath = TextJoin(atlasFiles, 2, 0);

	SpriteAtlas atlas = {};

	if (!FileExists(filepath))
	{
		SERR("[ SpriteAtlas ] File does not exist. Path: %s", filepath);
		return atlas;
	}

	char* atlasData = LoadFileText(filepath);
	SASSERT(atlasData);

	int count = 0;
	int bufferSize = Kilobytes(10);
	char* buffer = (char*)SAllocTemp(bufferSize);
	int splitBufferSize = Kilobytes(1);
	char** split = (char**)SAllocTemp(splitBufferSize * sizeof(char*));
	TextSplitBuffered(atlasData, '\n', &count, buffer, bufferSize, split, splitBufferSize);

	// 1st line empty
	int line = 1;
	const char* imgName = split[line++];
	const char* size = split[line++];
	const char* format = split[line++];
	const char* filter = split[line++];
	const char* repear = split[line++];

	const char* imgFiles[2] = { dirPath, imgName };
	const char* imgPath = TextJoin(imgFiles, 2, 0);

	int length = (count / 7) - 1;
	if (length <= 0)
	{
		SERR("Atlas of size 0");
		return {};
	}

	zpl_array_init_reserve(atlas.Rects, zpl_heap_allocator(), length);
	zpl_array_resize(atlas.Rects, length);

	atlas.NameToIndex.Initialize(sizeof(uint16_t), length, DefaultAllocFunc, DefaultFreeFunc);
	atlas.Texture = LoadTexture(imgPath);

	char s0[16];
	char s1[16];

	for (uint16_t entryCounter = 0; entryCounter < length; ++entryCounter)
	{
		if (line >= count)
			break;
		// Weird hack to make sure we don't process a space at the end if the atlas format happens to do so.
		//if (entryCounter == atlas.Length - 1 && (split[line][0] == ' ' || split[line][0] == '\0'))
			//break;

		const char* name = split[line];
		const char* rotate = split[line + 1];
		const char* xy = split[line + 2];
		const char* size = split[line + 3];
		const char* orig = split[line + 4];
		const char* offset = split[line + 5];
		const char* index = split[line + 6];

		line += 7;

		zpl_u32 hash = HashString(name, strlen(name));
		atlas.NameToIndex.Put(hash, &entryCounter);

		int err;

		int x;
		int y;
		{
			const char* find = xy + 5;
			int comma = TextFindIndex(find, ",");

			SClear(s0, 16);
			SClear(s1, 16);
			SCopy(s0, find, comma);
			SCopy(s1, find + comma + 1, strlen(find + comma + 1));

			RemoveWhitespace(s0);
			err = Str2Int(&x, s0, 10);
			SASSERT(err == STR2INT_SUCCESS);
			RemoveWhitespace(s1);
			err = Str2Int(&y, s1, 10);
			SASSERT(err == STR2INT_SUCCESS);

		}

		int w;
		int h;
		{
			const char* find = size + 7;
			int comma = TextFindIndex(find, ",");

			SClear(s0, 16);
			SClear(s1, 16);
			SCopy(s0, find, comma);
			SCopy(s1, find + comma + 1, strlen(find + comma + 1));

			RemoveWhitespace(s0);
			err = Str2Int(&w, s0, 10);
			SASSERT(err == STR2INT_SUCCESS);
			RemoveWhitespace(s1);
			err = Str2Int(&h, s1, 10);
			SASSERT(err == STR2INT_SUCCESS);
		}

		Rect16 r;
		r.x = (uint16_t)x;
		r.y = (uint16_t)y;
		r.w = (uint16_t)w;
		r.h = (uint16_t)h;

		atlas.Rects[entryCounter] = r;

		SLOG_DEBUG("[ SpriteAtlas ] Loaded rect, %s, x:%u, y:%u, w:%u, h:%u", name, x, y, w, h);
	}

	UnloadFileText(atlasData);

	SLOG_INFO("[ SpriteAtlas ] Successfully loaded sprite atlas, %s, with %i sprites", filepath, length);

	return atlas;
}

void SpriteAtlasUnload(SpriteAtlas* atlas)
{
	zpl_array_free(atlas->Rects);
	atlas->NameToIndex.Destroy();
	UnloadTexture(atlas->Texture);
}

Rectangle SpriteAtlasGet(SpriteAtlas* atlas, const char* name)
{
	Rectangle result = {};

	zpl_u32 hash = HashString(name, strlen(name));
	uint16_t* idx = HashMapGet(atlas->NameToIndex, hash, uint16_t);
	if (idx)
	{
		Rect16 textRect = atlas->Rects[*idx];

		result.x = textRect.x;
		result.y = textRect.y;
		result.width = textRect.w;
		result.height = textRect.h;
	}

	return result;
}
