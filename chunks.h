#pragma once
#include "binops.h"
#include "file_access.h"
#include "streaminfo.h"
#include "wak.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include "materials.h"

constexpr float degrees_in_radians = 57.2957795131f;

struct PhysicsObject {
	std::uint64_t a;
	std::uint32_t b;
	float x;
	float y;
	float rot_radians;
	double f;
	double g;
	double h;
	double i;
	double j;
	bool k;
	bool l;
	bool m;
	bool n;
	bool o;
	float z;
	std::uint32_t width;
	std::uint32_t height;
	std::vector<std::uint32_t> colors;
};

template <>
PhysicsObject read_be<PhysicsObject>(std::istream& s) {
	PhysicsObject out;

	out.a = read_be<std::uint64_t>(s);
	out.b = read_be<std::uint32_t>(s);
	out.x = read_be<float>(s);
	out.y = read_be<float>(s);
	out.rot_radians = read_be<float>(s);
	out.f = read_be<double>(s);
	out.g = read_be<double>(s);
	out.h = read_be<double>(s);
	out.i = read_be<double>(s);
	out.j = read_be<double>(s);
	out.k = read_be<bool>(s);
	out.l = read_be<bool>(s);
	out.m = read_be<bool>(s);
	out.n = read_be<bool>(s);
	out.o = read_be<bool>(s);
	out.z = read_be<float>(s);
	out.width = read_be<std::uint32_t>(s);
	out.height = read_be<std::uint32_t>(s);

	auto image_size = out.width * out.height;
	out.colors.resize(image_size);

	for (auto& c : out.colors)
		c = read_be<std::uint32_t>(s);

	return out;
}
template <>
void write_be<PhysicsObject>(std::ostream& s, const PhysicsObject& val) {
	write_be<std::uint64_t>(s, val.a);
	write_be<std::uint32_t>(s, val.b);
	write_be<float>(s, val.x);
	write_be<float>(s, val.y);
	write_be<float>(s, val.rot_radians);
	write_be<double>(s, val.f);
	write_be<double>(s, val.g);
	write_be<double>(s, val.h);
	write_be<double>(s, val.i);
	write_be<double>(s, val.j);
	write_be<bool>(s, val.k);
	write_be<bool>(s, val.l);
	write_be<bool>(s, val.m);
	write_be<bool>(s, val.n);
	write_be<bool>(s, val.o);
	write_be<float>(s, val.z);
	write_be<std::uint32_t>(s, val.width);
	write_be<std::uint32_t>(s, val.height);

	auto image_size = val.width * val.height;
	for (int j = 0; j < image_size; ++j) {
		write_be<std::uint32_t>(s, val.colors[j]);
	}
}

struct Chunk {
	std::string cpath;
	int cx;
	int cy;
	bool image_loaded = false;
	sf::Texture* tex;

	bool data_loaded = false;
	std::vector<CachedMat> mat_names;
	std::vector<PhysicsObject> phys_objs;
	uint8_t* data_buffer;
	std::vector<uint8_t> unhandled_bytes;

	bool image_dirty = false;
	bool file_dirty = false;
	bool marked_for_delete = false;
};

Chunk ParseChunkData(const char* path) {
	Chunk c = {path};
	std::string stem = std::filesystem::path(path).stem().string();
	for (int i = 0, n = 0; stem[i]; i++) {
		if (stem[i] == '_') {
			if (n == 0)
				c.cx = atoi(stem.data() + i + 1) / 512;
			else if (n == 1)
				c.cy = atoi(stem.data() + i + 1) / 512;
			n++;
		}
	}
	c.data_buffer = (uint8_t*)malloc(9 * 512 * 512);

	std::string contents = read_compressed_file(path);
	std::istringstream data(contents);

	uint8_t* materials = c.data_buffer;
	uint32_t* customColors = (uint32_t*)(materials + 512 * 512);

	uint32_t version = read_be<std::uint32_t>(data);
	uint32_t width = read_be<std::uint32_t>(data);
	uint32_t height = read_be<std::uint32_t>(data);

	if (version != 24 || width != 512 || height != 512) {
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		std::cerr << "  width: " << width << '\n';
		std::cerr << "  height: " << height << '\n';
		exit(-1);
	}

	data.read((char*)materials, 512 * 512);
	c.mat_names = read_vec_be<CachedMat>(data);
	std::vector<std::uint32_t> custom_world_colors = read_vec_be<std::uint32_t>(data);

	int ccIt = 0;
	for (int i = 0; i < 512 * 512; i++) {
		if (materials[i] & 0x80)
			customColors[i] = custom_world_colors[ccIt++];
		else
			customColors[i] = 0;
	}

	c.phys_objs = read_vec_be<PhysicsObject>(data);

	while (!data.eof())
		c.unhandled_bytes.emplace_back(read_le<std::uint8_t>(data));

	c.data_loaded = true;

	return c;
}

void SaveChunk(Chunk& c) {
	uint8_t* materials = c.data_buffer;
	uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
	uint32_t* texBuffer = customColors + 512 * 512;
	std::ostringstream s;

	write_be<std::uint32_t>(s, 24);
	write_be<std::uint32_t>(s, 512);
	write_be<std::uint32_t>(s, 512);

	s.write((const char*)materials, 512 * 512);
	write_vec_be<CachedMat>(s, c.mat_names);

	int count = 0;
	for (int i = 0; i < 512 * 512; i++)
		if (materials[i] & 0x80)
			count++;
	write_be<std::uint32_t>(s, count);
	for (int i = 0; i < 512 * 512; i++) {
		if (materials[i] & 0x80)
			write_be<std::uint32_t>(s, customColors[i]);
	}

	write_vec_be<PhysicsObject>(s, c.phys_objs);
	for (auto b : c.unhandled_bytes)
		write_le<std::uint8_t>(s, b);

	write_file((c.cpath + ".hex").c_str(), s.str());
	write_compressed_file(c.cpath.c_str(), s.str());

	//printf("finished saving chunk at (%i, %i) to %s\n", c.cx, c.cy, c.cpath.c_str());
	c.file_dirty = false;
}

void ReloadChunkImage(Chunk& c) {
	if (!c.image_loaded) {
		c.tex = new sf::Texture();
		c.tex->create(512, 512);
	}
	if (!c.data_loaded)
		return;

	uint8_t* materials = c.data_buffer;
	uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
	uint32_t* texBuffer = customColors + 512 * 512;
	for (int y = 0; y < 512; y++) {
		for (int x = 0; x < 512; x++) {
			int i = y * 512 + x;
			uint8_t material = materials[i] & 0x7f;
			bool custom_color = (materials[i] & 0x80) != 0;
			if (custom_color)
				texBuffer[i] = customColors[i];
			else {
				Material m = allMaterials[c.mat_names[material].index];
				if (m.tex) {
					int gx = x + c.cx * 512;
					int gy = y + c.cy * 512;
					int texX = ((gx % m.w) + m.w) % m.w;
					int texY = ((gy % m.h) + m.h) % m.h;
					texBuffer[i] = m.tex[texY * m.w + texX];
				} else
					texBuffer[i] = (m.color & 0xff00ff00) | ((m.color & 0xff) << 16) | ((m.color & 0xff0000) >> 16);
			}
		}
	}

	for (const auto& physics_object : c.phys_objs) {
		int lx = rint(physics_object.x) - 512 * c.cx;
		int ly = rint(physics_object.y) - 512 * c.cy;
		int ux = lx;
		int uy = ly;

		float cosine = cosf(physics_object.rot_radians);
		float sine = sinf(physics_object.rot_radians);

		if (cosine > 0) {
			ux += physics_object.width * cosine;
			uy += physics_object.height * cosine;
		} else {
			lx += physics_object.width * cosine;
			ly += physics_object.height * cosine;
		}

		if (sine > 0) {
			lx -= physics_object.height * sine;
			uy += physics_object.width * sine;
		} else {
			ux -= physics_object.height * sine;
			ly += physics_object.width * sine;
		}
		lx = std::min(std::max(lx, 0), 511);
		ly = std::min(std::max(ly, 0), 511);
		ux = std::min(std::max(ux, 0), 511);
		uy = std::min(std::max(uy, 0), 511);

		for (int pixY = ly; pixY < uy; pixY++) {
			for (int pixX = lx; pixX < uy; pixX++) {
				float offsetPixX = pixX - (rintf(physics_object.x) - 512 * c.cx);
				float offsetPixY = pixY - (rintf(physics_object.y) - 512 * c.cy);

				int texX = rint(offsetPixX * cosine + offsetPixY * sine);
				int texY = rint(-offsetPixX * sine + offsetPixY * cosine);

				if (texX < 0 || physics_object.width <= texX || texY < 0 || physics_object.height <= texY)
					continue;

				int idx = pixY * 512 + pixX;
				uint32_t c2 = physics_object.colors.data()[physics_object.width * texY + texX];
				if ((c2 >> 24) == 0)
					continue;
				texBuffer[idx] = c2;
			}
		}
	}
	c.image_loaded = true;
	c.image_dirty = false;
	c.tex->update((unsigned char*)texBuffer, 512, 512, 0, 0);
}

const char* ChunkGet(const Chunk& c, int x, int y) {
	uint8_t* materials = c.data_buffer;
	uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
	uint32_t* texBuffer = customColors + 512 * 512;
	return c.mat_names[materials[512 * y + x] & 0x7f].name.c_str();
}

void ChunkSet(Chunk& c, int x, int y, const char* material, uint32_t color = 0) {
	uint8_t* materials = c.data_buffer;
	uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
	uint32_t* texBuffer = customColors + 512 * 512;
	materials[512 * y + x] = IndexOrAdd(c.mat_names, material);
	if (color)
		customColors[512 * y + x] = color;
	c.image_dirty = true;
	c.file_dirty = true;
}

void DestroyChunk(Chunk& c, PixelScenes& scenes, const char* save00_path) {
	remove(c.cpath.c_str());
	char buffer[MAX_PATH];
	sprintf(buffer, "%s/world/area_%i.bin", save00_path, c.cy * 2000 + c.cx);
	remove(buffer);
	sprintf(buffer, "%s/world/entities_%i.bin", save00_path, c.cy * 2000 + c.cx);
	remove(buffer);

	for (int i = 0; i < scenes.placed.size(); i++) {
		PixelSceneBackground* b = scenes.placed[i];
		if (b->x >= 512 * c.cx && b->x < 512 * c.cx + 512 && b->y >= 512 * c.cy && b->y < 512 * c.cy + 512) {
			scenes.placed.erase(scenes.placed.begin() + i);
			--i;
			std::string str = b->bg.size() ? b->bg : b->mat;
			if (str.starts_with("data/biome_impl/spliced") || scenes.reference_whitelist.find(str) != -1ull)
				scenes.pending.emplace_back(b);
		}
	}
}

