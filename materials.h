#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "binops.h"
#include "winapi_wrappers.h"
#include "pngutils.h"

struct Material {
	std::string name;
	int w, h;
	uint32_t* tex;
};

std::vector<Material> allMaterials;


void LoadMats(const char* path) {
	uint32_t* missingTexture = (uint32_t*)malloc(4 * 84 * 84);
	for (int py = 0; py < 84; py++) {
		for (int px = 0; px < 84; px++) {
			uint32_t color = 0;
			if (((px / 42) + (py / 42)) % 2 == 0) color = 0x7fdc00ff;
			missingTexture[py * 84 + px] = color;
		}
	}
	allMaterials.emplace_back(Material(std::string("MISSING_RESOURCE"), 84, 84, missingTexture));
	uint32_t* air = (uint32_t*)malloc(4 * 1 * 1);
	air[0] = 0;
	allMaterials.emplace_back(Material(std::string("air"), 1, 1, air));
	
	for_each_file(path, ".png", true, [](const std::filesystem::path& p) {
			Vec2i dims = GetImageDimensions(p.string().c_str());
			uint8_t* data = (uint8_t*)malloc(4 * dims.x * dims.y);
			ReadImageRGBA(p.string().c_str(), data);
			//printf("loaded texture: %s\n", filename);
			allMaterials.emplace_back(Material(p.stem().string(), dims.x, dims.y, (uint32_t*)data));
		}
	);
}

struct CachedMat {
	std::string name;
	int index;
};

int FindMaterial(const char* mat) {
	for (int i = 0; i < allMaterials.size(); i++)
		if (std::string_view(mat) == allMaterials[i].name)
			return i;
	printf("missing material resource: \"%s\"\n", mat);
	return 0;
}
int IndexOrAdd(std::vector<CachedMat>& mats, const char* mat) {
	for (int i = 0; i < mats.size(); i++) {
		if (mats[i].name == std::string_view(mat)) return i;
	}
	mats.emplace_back(mat, FindMaterial(mat));
	return mats.size() - 1;
}

template<>
CachedMat read_be(std::istream& s) {
	std::string name = read_be<std::string>(s);
	return { name, FindMaterial(name.c_str()) };
}
template<>
void write_be(std::ostream& s, const CachedMat& val) {
	write_be<std::string>(s, val.name);
}

/*
uint32_t createRGB(const uint8_t r, const uint8_t g, const uint8_t b) {
	return (r << 16) | (g << 8) | b;
}
uint32_t swapEndianness(uint32_t n) {
	uint8_t a = (n >> 24) & 0xff;
	uint8_t b = (n >> 16) & 0xff;
	uint8_t c = (n >> 8) & 0xff;
	uint8_t d = (n >> 0) & 0xff;
	return (d << 24) | (c << 16) | (b << 8) | (a << 0);
}
*/