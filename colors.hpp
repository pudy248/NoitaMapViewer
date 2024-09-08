#pragma once

#include <cstdio>
#include <fstream>
#include <filesystem>
#include "pngutils.h"

namespace fs = std::filesystem;

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
	std::string name;
	uint32_t* tex;
};

std::vector<Material> allMaterials = {
    { "ERR_MAT", nullptr },
};

Material LoadMaterial(fs::path path)
{
	Vec2i dims = GetImageDimensions(path.string().c_str());
	auto* data = (uint8_t*)malloc(4 * dims.x * dims.y);
	ReadImageRGBA(path.string().c_str(), data);
	//printf("loaded texture for %s\n", name);
	return { path.filename().replace_extension().string(), (uint32_t*)data };
}

void LoadMats(const char* path)
{
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

	allMaterials[0].tex = missingTexture;

        for (auto &p : fs::directory_iterator(path)) {
          if (p.is_regular_file() && p.path().extension() == ".png") {
            allMaterials.push_back(LoadMaterial(p.path()));
          }
        }
}