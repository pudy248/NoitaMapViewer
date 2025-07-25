#pragma once
#include <vector>
#include <sstream>
#include <iostream>
#include <SFML/Graphics.hpp>
#include "binops.h"
#include "file_access.h"
#include "wak.h"

struct StreaminfoBackground {
	float x;
	float y;
	std::string path;
	float a;
	std::uint32_t b;
	std::uint32_t c;

	sf::Texture tex;
};

template<> StreaminfoBackground read_be<StreaminfoBackground>(std::istream& s) {
	StreaminfoBackground out;
	out.x = read_be<float>(s);
	out.y = read_be<float>(s);
	out.path = read_be<std::string>(s);
	out.a = read_be<float>(s);
	out.b = read_be<std::uint32_t>(s);
	out.c = read_be<std::uint32_t>(s);
	load_texture(out.tex, out.path);
	return out;
}

struct PixelSceneBackground {
	int x;
	int y;
	std::string mat;
	std::string visual;
	std::string bg;
	uint8_t pad[6];
	std::string pad2;
	uint8_t pad3[5];
	std::vector<uint64_t> color_mats;

	bool exists;
	sf::Texture tex;
};

template<> PixelSceneBackground read_be<PixelSceneBackground>(std::istream& s) {
	PixelSceneBackground out;
	out.x = read_be<std::uint32_t>(s);
	out.y = read_be<std::uint32_t>(s);
	out.mat = read_be<std::string>(s);
	out.visual = read_be<std::string>(s);
	out.bg = read_be<std::string>(s);
	for (int i = 0; i < 6; i++)
		out.pad[i] = read_le<std::uint8_t>(s);
	out.pad2 = read_be<std::string>(s);
	for (int i = 0; i < 5; i++)
		out.pad3[i] = read_le<std::uint8_t>(s);
	int n_extra = read_be<std::uint8_t>(s);
	for (int i = 0; i < n_extra; i++)
		out.color_mats.emplace_back(read_be<std::uint64_t>(s));

	out.exists = out.bg.size();
	if (out.exists)
		load_texture(out.tex, out.bg);
	return out;
}
template<> void write_be<PixelSceneBackground>(std::ostream& s, const PixelSceneBackground& bg) {
	write_be<std::uint32_t>(s, bg.x);
	write_be<std::uint32_t>(s, bg.y);
	write_be<std::string>(s, bg.mat);
	write_be<std::string>(s, bg.visual);
	write_be<std::string>(s, bg.bg);
	for (int i = 0; i < 6; i++)
		write_le<std::uint8_t>(s, bg.pad[i]);
	write_be<std::string>(s, bg.pad2);
	for (int i = 0; i < 5; i++)
		write_le<std::uint8_t>(s, bg.pad3[i]);
	write_le<std::uint8_t>(s, bg.color_mats.size());
	for (auto i : bg.color_mats)
		write_be<std::uint64_t>(s, i);
}

struct PixelScenes {
	std::vector<PixelSceneBackground*> pending;
	std::vector<PixelSceneBackground*> placed;
	std::vector<PixelSceneBackground*> backgrounds;
};

std::vector<StreaminfoBackground*> ParseStreaminfo(const char* path) {
	std::string contents = read_compressed_file(path);
	std::istringstream data(contents);

	uint32_t version = read_be<std::uint32_t>(data);
	uint32_t seed = read_be<std::uint32_t>(data);
	uint32_t playtime_frames = read_be<std::uint32_t>(data);
	uint32_t playtime_secs = read_be<float>(data);
	uint32_t unk = read_be<std::uint64_t>(data);

	if (version != 24) {
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		std::cerr << "  Seed: " << seed << '\n';
		std::cerr << "  Frame counter: " << playtime_frames << '\n';
		exit(-1);
	}

	std::vector<StreaminfoBackground*> bgs = read_vec_ptrs_be<StreaminfoBackground>(data);

	std::string schema_hash = read_be<std::string>(data);
	uint32_t gamemode_num = read_be<std::uint32_t>(data);
	std::string gamemode_name = read_be<std::string>(data);
	uint64_t gamemode_steam_id = read_be<std::uint64_t>(data);
	data.ignore(13);
	std::string ui_newgame_name = read_be<std::string>(data);
	data.ignore(16);

	return bgs;
}

PixelScenes ParsePixelScenes(const char* path) {
	std::string contents = read_compressed_file(path);
	std::istringstream data(contents);

	uint32_t version = read_be<std::uint32_t>(data);
	uint32_t magic_num = read_be<std::uint32_t>(data);

	if (version != 3) {
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		std::cerr << "  magic_num: " << magic_num << std::endl;
		exit(-1);
	}

	std::vector<PixelSceneBackground*> pending = read_vec_ptrs_be<PixelSceneBackground>(data);
	std::vector<PixelSceneBackground*> placed = read_vec_ptrs_be<PixelSceneBackground>(data);
	std::vector<PixelSceneBackground*> backgrounds = read_vec_ptrs_be<PixelSceneBackground>(data);
	return { pending, placed, backgrounds };
}

void WritePixelScenes(const char* path, PixelScenes& scenes) {
	std::ostringstream s;
	write_be<std::uint32_t>(s, 3);
	write_be<std::uint32_t>(s, 0x2F0AA9F);
	write_vec_ptrs_be<PixelSceneBackground>(s, scenes.pending);
	write_vec_ptrs_be<PixelSceneBackground>(s, scenes.placed);
	write_vec_ptrs_be<PixelSceneBackground>(s, scenes.backgrounds);

	write_compressed_file(path, s.str());
}
