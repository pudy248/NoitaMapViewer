#pragma once
#include "binops.h"
#include "file_access.h"
#include "pngutils.h"
#include "winapi_wrappers.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, std::string> globalWakContents;
std::string empty_str;

void read_wak(const char* wak_path) {
	globalWakContents.clear();
	std::string contents = read_file(wak_path);
	std::istringstream data(contents);

	uint32_t z1 = read_le<std::uint32_t>(data);
	uint32_t fileCount = read_le<std::uint32_t>(data);
	uint32_t dataStart = read_le<std::uint32_t>(data);
	uint32_t z2 = read_le<std::uint32_t>(data);

	for (int i = 0; i < fileCount; i++) {
		uint32_t offset = read_le<std::uint32_t>(data);
		uint32_t size = read_le<std::uint32_t>(data);
		std::string name = read_le<std::string>(data);
		globalWakContents.emplace(name, std::string(contents.c_str() + offset, size));
	}
}

std::string& get_file(const std::string& path) {
	if (globalWakContents.find(path) != globalWakContents.end())
		return globalWakContents.at(path);
	else
		return empty_str;
}

void load_texture(sf::Texture& tex, const std::string& path) {
	std::string& contents = get_file(path);
	if (!contents.size())
		return;
	Vec2i dims = GetBufferImageDimensions((uint8_t*)contents.data());
	tex.loadFromMemory(contents.data(), contents.size(), {0, 0, dims.x, dims.y});
}

std::filesystem::path find_wak(const char* wakpath) {
	if (file_exists(wakpath))
		return read_file(wakpath);

	std::filesystem::path dialog_ret = locate_file_dialog(
		"", "data.wak\0data.wak\0", "Locate your install's data.wak, in the steam install directory");
	write_file(wakpath, dialog_ret.string().c_str());

	if (dialog_ret.filename().string() != std::string_view("data.wak"))
		exit(-1);

	return dialog_ret;
}