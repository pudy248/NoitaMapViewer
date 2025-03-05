#pragma once
#include <filesystem>
#include <SFML/Graphics.hpp>
#include "materials.h"
#include "png_petri.h"
#include "pngutils.h"
#include "streaminfo.h"
#include "chunks.h"

static sf::Vector2i roundGlobal(sf::Vector2f global) {
	return { (int)floorf(global.x), (int)floorf(global.y) };
}
static sf::Vector2i globalToChunk(sf::Vector2i global) {
	return { (int)floorf((float)global.x / 512), (int)floorf((float)global.y / 512) };
}
static sf::Vector2i globalToOffset(sf::Vector2i global) {
	sf::Vector2i cc = globalToChunk(global);
	return { global.x - 512 * cc.x, global.y - 512 * cc.y };
}
static Chunk* chunkIdx(std::vector<Chunk*> chunks, sf::Vector2i index) {
	for (Chunk* chunk : chunks) {
		if (chunk->cx == index.x && chunk->cy == index.y)
			return chunk;
	}
	return NULL;
}

static void set_circle(std::vector<Chunk*> chunks, sf::Vector2f center, float radius, const char* material) {
	for (int dy = -radius - 1; dy < radius + 1; dy++) {
		for (int dx = -radius - 1; dx < radius + 1; dx++) {
			int x = center.x + dx;
			int y = center.y + dy;
			sf::Vector2f diff = sf::Vector2f(x + 0.5f, y + 0.5f) - center;
			float dist = diff.x * diff.x + diff.y * diff.y;
			if (dist > radius * radius) continue;

			sf::Vector2i gPos = roundGlobal(sf::Vector2f((int)x, (int)y));
			sf::Vector2i cPos = globalToChunk(gPos);
			sf::Vector2i cOff = globalToOffset(gPos);
			Chunk* chunk = chunkIdx(chunks, cPos);
			if (chunk)
				ChunkSet(*chunk, cOff.x, cOff.y, material, 0);
		}
	}
}

static void ExportMapImage(std::vector<Chunk*>& chunks, int downscale = 2) {
	int minX = 10000;
	int maxX = -10000;
	int minY = 10000;
	int maxY = -10000;
	for (Chunk* c : chunks) {
		if (c->cx < minX) minX = c->cx;
		if (c->cx > maxX) maxX = c->cx;
		if (c->cy < minY) minY = c->cy;
		if (c->cy > maxY) maxY = c->cy;
	}
	size_t width = (maxX - minX + 1) * 512 / downscale;
	size_t height = (maxY - minY + 1) * 512 / downscale;
	png_byte** rows = (png_byte**)malloc(sizeof(png_byte*) * height);
	for (int y = 0; y < height; y++) {
		rows[y] = (png_byte*)malloc(4 * width);
		memset(rows[y], 0, 4 * width);
	}

	for (Chunk* c : chunks) {
		size_t dx = (c->cx - minX) * 512;
		size_t dy = (c->cy - minY) * 512;
		for (size_t py = 0; py < 512; py += downscale) {
			size_t y = (dy + py) / downscale;
			for (size_t px = 0; px < 512; px += downscale) {
				size_t x = (dx + px) / downscale;
				uint8_t* materials = c->data_buffer;
				uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
				uint32_t* texBuffer = customColors + 512 * 512;
				((uint32_t**)rows)[y][x] = texBuffer[py * 512 + px];
			}
		}
	}
	WriteImageRows("map.png", rows, width, height);
}