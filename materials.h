#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "binops.h"
#include "winapi_wrappers.h"
#include "pngutils.h"
#include "wak.h"

struct Material {
	std::string name;
	uint32_t color;
	int w, h;
	uint32_t* tex;
};

std::vector<Material> allMaterials;

static uint32_t to_int(const std::string_view s) {
	uint32_t result;
	auto err = std::from_chars(s.data(), s.data() + s.size(), result, 16);
	if (err.ec != std::errc{} || err.ptr != s.data() + s.size()) {
		fprintf(stderr, "Value '%.*s' could not be converted to a number.\n", (uint32_t)s.size(), s.data());
		std::exit(-1);
	}
	return result;
}
void LoadMats(const char* path) {
	uint32_t* missingTexture = (uint32_t*)malloc(4 * 84 * 84);
	for (int py = 0; py < 84; py++) {
		for (int px = 0; px < 84; px++) {
			uint32_t color = 0;
			if (((px / 42) + (py / 42)) % 2 == 0) color = 0xffdc00ff;
			missingTexture[py * 84 + px] = color;
		}
	}
	allMaterials.emplace_back(Material(std::string("MISSING_RESOURCE"), 0, 84, 84, missingTexture));

	std::string_view mats = get_file("data/materials.xml");
	while (true) {
		uint32_t color = 0;
		std::string texture{};
		auto it = mats.find("<CellData");
		if (it == (size_t)-1)
			break;
		mats = mats.substr(it);
		it = mats.find("</CellData");
		auto mat = mats.substr(0, it);
		mats = mats.substr(it);
		it = mat.find("\tname=");
		mat = mat.substr(it + 7);
		std::string name(mat.data(), mat.find('"'));
		it = mat.find("<Graphics");
		if (it == (size_t)-1) {
			auto col = mat.find("\twang_color=");
			if (col != (size_t)-1) {
				auto col_str = mat.substr(col + 13);
				auto end = col_str.find('"');
				color = to_int(col_str.substr(0, end));
			}
		} else {
			mat = mat.substr(it);
			auto graphics = mat.substr(0, mat.find('>'));

			auto tex = graphics.find("\ttexture_file=");
			if (tex != (size_t)-1) {
				auto tex_str = graphics.substr(tex + 15);
				auto end = tex_str.find('"');
				texture = std::string(tex_str.data(), end);
			}
			auto col = graphics.find("\tcolor=");
			if (col != (size_t)-1) {
				auto col_str = graphics.substr(col + 8);
				auto end = col_str.find('"');
				color = to_int(col_str.substr(0, end));
			}
		}
		if (texture.size()) {
			std::string& img = get_file(texture);
			Vec2i dims = GetBufferImageDimensions((const uint8_t*)img.c_str());
			uint8_t* data = (uint8_t*)malloc(4 * dims.x * dims.y);
			ReadBufferImage((const uint8_t*)img.c_str(), data, true);
			allMaterials.emplace_back(Material(name, color, dims.x, dims.y, (uint32_t*)data));
			printf("Adding %s = %s\n", name.c_str(), texture.c_str());
		} else {
			allMaterials.emplace_back(Material(name, color, 0, 0, nullptr));
			printf("Adding %s = %06x\n", name.c_str(), color);
		}
	}
	allMaterials[1].color = 0; //air
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