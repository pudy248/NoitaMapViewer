#pragma once

#include "pngutils.h"
#include "Windows.h"

void _putstr_offset(const char* str, char* buffer, int& offset)
{
	int i = 0;
	while (str[i] != '\0')
	{
		buffer[offset++] = str[i++];
	}
}
uint32_t createRGB(const uint8_t r, const uint8_t g, const uint8_t b)
{
	return (r << 16) | (g << 8) | b;
}
uint32_t swapEndianness(uint32_t n)
{
	uint8_t a = (n >> 24) & 0xff;
	uint8_t b = (n >> 16) & 0xff;
	uint8_t c = (n >> 8) & 0xff;
	uint8_t d = (n >> 0) & 0xff;
	return (d << 24) | (c << 16) | (b << 8) | (a << 0);
}

struct Material
{
	char* name;
	uint32_t* tex;
};

constexpr int numMats = 457;
Material allMaterials[numMats];

Material LoadMaterial(const char* path, char* outputStr)
{
	Vec2i dims = GetImageDimensions(path);
	uint8_t* data = (uint8_t*)malloc(4 * dims.x * dims.y);
	ReadImageRGBA(path, data);
	for (int i = 0; i < 50; i++)
	{
		if (outputStr[i] == '\0')
		{
			outputStr[i - 4] = '\0';
			break;
		}
	}
	char* name = (char*)malloc(50);
	strcpy_s(name, 50, outputStr);

	//printf("loaded texture for %s\n", name);
	return { name, (uint32_t*)data };
}

void LoadMats(const char* path)
{
	WIN32_FIND_DATA fd;

	char* str = (char*)malloc(10);
	strcpy_s(str, 10, "ERR_MAT");
	uint32_t* missingTexture = (uint32_t*)malloc(4 * 252 * 252);
	for (int py = 0; py < 252; py++)
	{
		for (int px = 0; px < 252; px++)
		{ 
			uint32_t color = 0;
			if (((px / 42) + (py / 42)) % 2 == 0) color = 0xffdc00ff;
			missingTexture[py * 252 + px] = color;
		}
	}
	Material missingMat = { str, missingTexture };

	allMaterials[0] = missingMat;
	int counter = 1;
	char buffer[_MAX_PATH];
	int offset = 0;
	_putstr_offset(path, buffer, offset);
	_putstr_offset("*.png", buffer, offset);
	buffer[offset] = '\0';

	WCHAR wSearchName[_MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, buffer, _MAX_PATH, wSearchName, _MAX_PATH);
	HANDLE hFind = ::FindFirstFile(wSearchName, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				offset = 0;
				char buffer2[_MAX_PATH];
				WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, _MAX_PATH, buffer2, _MAX_PATH, NULL, NULL);

				_putstr_offset(path, buffer, offset);
				_putstr_offset(buffer2, buffer, offset);

				buffer[offset] = '\0';
				allMaterials[counter++] = LoadMaterial(buffer, buffer2);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}