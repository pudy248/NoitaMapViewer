#pragma once
#include "binops.h"
#include "materials.h"
#include "pngutils.h"
#include "wak.h"
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct Biome {
	std::string path;
	uint32_t color;
	uint32_t w, h;
	sf::Texture* tex;
};

struct BiomeMap {
	uint32_t w, h;
	std::vector<Biome*> entries;
};

std::vector<Biome> LoadBiomes() {
	std::vector<Biome> biomes;
	std::string_view biomes_xml = get_file("data/biome/_biomes_all.xml");
	while (true) {
		Biome b = {};
		auto it = biomes_xml.find("\tbiome_filename=");
		if (it == (size_t)-1)
			break;
		biomes_xml = biomes_xml.substr(it + 17);
		b.path = std::string(biomes_xml.substr(0, biomes_xml.find('"')));
		it = biomes_xml.find("\tcolor=");
		biomes_xml = biomes_xml.substr(it + 8);
		b.color = to_int(biomes_xml.substr(0, biomes_xml.find('"')));

		std::string_view this_biome = get_file(b.path);
		it = this_biome.find("\tbackground_image=");
		if (it == (size_t)-1)
			it = this_biome.find(" background_image=");
		if (it != (size_t)-1) {
			this_biome = this_biome.substr(it + 19);
			std::string path2(this_biome.data(), this_biome.find('"'));
			if (path2.size()) {
				std::string& img = get_file(path2);
				Vec2i dims = GetBufferImageDimensions((const uint8_t*)img.c_str());
				b.w = dims.x;
				b.h = dims.y;
				uint8_t* data = (uint8_t*)malloc(4 * dims.x * dims.y);
				ReadBufferImage((const uint8_t*)img.c_str(), data, true);
				b.tex = new sf::Texture();
				b.tex->create(dims.x, dims.y);
				b.tex->update((unsigned char*)data, dims.x, dims.y, 0, 0);
				b.tex->setRepeated(true);
				free(data);
				printf("Loaded biome %s: %08x -> %s\n", b.path.c_str(), b.color, path2.c_str());
			}
		} else
			printf("Loaded biome %s: %08x\n", b.path.c_str(), b.color);
		biomes.emplace_back(b);
	}
	return biomes;
}

BiomeMap LoadBiomeMap(std::vector<Biome>& ref) {
	std::string& map = get_file("data/biome_impl/biome_map.png");
	Vec2i dims = GetBufferImageDimensions((const uint8_t*)map.c_str());
	uint8_t* data = (uint8_t*)malloc(4 * dims.x * dims.y);
	ReadBufferImage((const uint8_t*)map.c_str(), data, true);
	std::vector<Biome*> out(dims.x * dims.y, 0);
	for (int i = 0; i < dims.x * dims.y; i++) {
		uint32_t c = data[i * 4 + 2] | (data[i * 4 + 1] << 8) | (data[i * 4 + 0] << 16) | (data[i * 4 + 3] << 24);
		for (auto& b : ref) {
			if (b.color == c) {
				out[i] = &b;
				break;
			}
		}
	}
	free(data);
	return {(uint32_t)dims.x, (uint32_t)dims.y, out};
}