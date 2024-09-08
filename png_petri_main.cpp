#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include <format>
#ifdef _MSC_VER
#include <conio.h>
#else

#endif

#include <SFML/Graphics.hpp>

#include "binops.hpp"
#include "colors.hpp"
#include "utils.h"

namespace fs = std::filesystem;

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

struct StreaminfoBackground {
	float a;
	std::uint32_t b;
	std::uint32_t count;
	float x;
	float y;
	std::string path;
	StreaminfoBackground(const char*& ptr) {
		a = read_be<float>(ptr);
		b = read_be<std::uint32_t>(ptr);
		count = read_be<std::uint32_t>(ptr);
		x = read_be<float>(ptr);
		y = read_be<float>(ptr);
		path = read_be<std::string>(ptr);

		tex.loadFromFile(path);
	}
	sf::Texture tex;
};

struct PixelsceneBackground {
	int x;
	int y;
	std::string mat;
	std::string visual;
	std::string bg;

	bool exists;

	PixelsceneBackground(const char*& ptr) {
		x = read_be<std::uint32_t>(ptr);
		y = read_be<std::uint32_t>(ptr);
		mat = read_be<std::string>(ptr);
		visual = read_be<std::string>(ptr);
		bg = read_be<std::string>(ptr);
		ptr += 6;
		read_be<std::string>(ptr);
		ptr += 5;
		int n_extra = read_be<std::uint8_t>(ptr);
		ptr += n_extra * 8;

		exists = bg.size();
		if (exists) {
			tex.loadFromFile(bg);
		}
	}
	sf::Texture tex;
};

std::vector<StreaminfoBackground*> ParseStreaminfo(fs::path path) {
	std::string file_contents = read_compressed_file(path.string().c_str());
	const char* data = file_contents.c_str();
	const char* data_end = data + file_contents.size();

	uint32_t version = read_be<std::uint32_t>(data);
	uint32_t seed = read_be<std::uint32_t>(data);
	uint32_t playtime_frames = read_be<std::uint32_t>(data);
	uint32_t unk1 = read_be<std::uint32_t>(data);

	if (version != 24) {
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		std::cerr << "  Seed: " << seed << '\n';
		std::cerr << "  Frame counter: " << playtime_frames << '\n';
		std::cerr << "  UNK1: " << unk1 << '\n';
		exit(-1);
	}


	std::vector<StreaminfoBackground*> bgs;
	bgs.emplace_back(new StreaminfoBackground(data));
	int count = bgs[0]->count;
	for (int i = 1; i < count; i++) bgs.emplace_back(new StreaminfoBackground(data));

	//TODO finish!
	return bgs;
}

std::vector<PixelsceneBackground*> ParsePixelScenes(fs::path path) {
	std::string file_contents = read_compressed_file(path.string().c_str());
	const char* data = file_contents.c_str();
	const char* data_end = data + file_contents.size();

	uint32_t version = read_be<std::uint32_t>(data);
	uint32_t unk = read_be<std::uint32_t>(data);
	uint32_t count = read_be<std::uint32_t>(data);

	if (version != 3) {
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		exit(-1);
	}

	std::vector<PixelsceneBackground*> bgs;
	for (int i = 0; i < count; i++) {
		bgs.emplace_back(new PixelsceneBackground(data));
	}
	return bgs;
}

const float degrees_in_radians = 57.2957795131f;
template <typename T> static std::vector<T> read_vec(const char*& ptr)
{
	auto count = read_be<std::uint32_t>(ptr);
	std::vector<T> result(count);

	for (auto& out : result) {
		out = read_be<T>(ptr);
	}

	return result;
}

static int GetMaterialIndex(const char* name)
{
	for (int i = 0; i < allMaterials.size(); i++)
	{
		if (name == allMaterials[i].name)
		{
			return i;
		}
	}
	printf("ERR: Missing texture for material: %s\n", name);
	return 0;
}
struct MatIdx
{
	std::string mat;
	int idx;
	MatIdx() = default;
	MatIdx(std::string _m, int _i) : mat(_m), idx(_i) {}
};
static int IndexOrAdd(std::vector<MatIdx>& mats, const char* mat)
{
	for (int i = 0; i < mats.size(); i++)
	{
		if (!strcmp(mats[i].mat.c_str(), mat)) return i;
	}
	mats.push_back(MatIdx(std::string(mat), GetMaterialIndex(mat)));
	return mats.size() - 1;
}

struct Chunk
{
	std::string cpath;
	int cx;
	int cy;
	bool image_loaded = false;
	sf::Texture* tex;

	bool data_loaded = false;
	std::vector<MatIdx> matNames;
	std::vector<PhysicsObject> physObjs;
	uint8_t* data_buffer;
	//uint8_t materials[512 * 512];
	//uint32_t customColors[512 * 512];
	//uint32_t texBuffer[512 * 512];

	bool g_dirty = false;
	bool s_dirty = false;
	bool marked_del = false;
	

	Chunk() = default;
	Chunk(const char* path, int _cx, int _cy) : cpath(path), cx(_cx), cy(_cy) {
		cpath = std::string(path);

		load_data();
		load_image();
	}

	void load_image() {
		if (!data_loaded) {
			load_data();

			redraw_mats();
			redraw_physics();

			unload_data();
		}
		else {
			redraw_mats();
			redraw_physics();
		}
	}

	void load_data() {
		data_buffer = (uint8_t*)malloc(9 * 512 * 512);
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;

		std::string file_contents = read_compressed_file(cpath.c_str());
		const char* ptr = file_contents.c_str();
		const char* data_end = ptr + file_contents.size();

		uint32_t version = read_be<std::uint32_t>(ptr);
		uint32_t width = read_be<std::uint32_t>(ptr);
		uint32_t height = read_be<std::uint32_t>(ptr);

		if (version != 24 || width != 512 || height != 512)
		{
			std::cerr << "Unexpected header:\n";
			std::cerr << "  version: " << version << '\n';
			std::cerr << "  width: " << width << '\n';
			std::cerr << "  height: " << height << '\n';
			exit(-1);
		}

		std::memcpy(materials, ptr, 512 * 512);
		ptr += 512 * 512;

		uint32_t material_name_count = read_be<std::uint32_t>(ptr);

		for (int i = 0; i < material_name_count; ++i)
		{
			std::string s = read_be<std::string>(ptr);
			matNames.push_back(MatIdx(s, GetMaterialIndex(s.c_str())));
		}

		std::vector<std::uint32_t> custom_world_colors = read_vec<std::uint32_t>(ptr);

		int ccIt = 0;
		for (int i = 0; i < 512 * 512; i++)
		{
			if (materials[i] & 0x80) customColors[i] = custom_world_colors[ccIt++];
			else customColors[i] = 0;
		}

		std::uint32_t physics_object_count = read_be<std::uint32_t>(ptr);

		for (auto i = 0; i < physics_object_count; ++i)
		{
			PhysicsObject into;

			into.a = read_be<std::uint64_t>(ptr);
			into.b = read_be<std::uint32_t>(ptr);
			into.x = read_be<float>(ptr);
			into.y = read_be<float>(ptr);
			into.rot_radians = read_be<float>(ptr);
			into.f = read_be<double>(ptr);
			into.g = read_be<double>(ptr);
			into.h = read_be<double>(ptr);
			into.i = read_be<double>(ptr);
			into.j = read_be<double>(ptr);
			into.k = read_be<bool>(ptr);
			into.l = read_be<bool>(ptr);
			into.m = read_be<bool>(ptr);
			into.n = read_be<bool>(ptr);
			into.o = read_be<bool>(ptr);
			into.z = read_be<float>(ptr);
			into.width = read_be<std::uint32_t>(ptr);
			into.height = read_be<std::uint32_t>(ptr);

			auto image_size = into.width * into.height;
			into.colors.resize(image_size);

			for (int j = 0; j < image_size; ++j)
			{
				into.colors[j] = read_be<std::uint32_t>(ptr);
			}
			physObjs.push_back(into);
		}

		data_loaded = true;
		image_loaded = true;
	}

	void unload_data() {
		free(data_buffer);
		physObjs.clear();
		matNames.clear();

		data_loaded = false;
	}

	void save()
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		s_dirty = false;
		std::ostringstream s;

		write_be<std::uint32_t>(s, 24);
		write_be<std::uint32_t>(s, 512);
		write_be<std::uint32_t>(s, 512);

		s.write((const char*)materials, 512 * 512);
		write_be<std::uint32_t>(s, matNames.size());
		for (int i = 0; i < matNames.size(); i++)
		{
			write_be<std::uint32_t>(s, matNames[i].mat.size());
			s.write(matNames[i].mat.c_str(), matNames[i].mat.size());
		}
		int count = 0;
		for (int i = 0; i < 512 * 512; i++) if (materials[i] & 0x80) count++;
		write_be<std::uint32_t>(s, count);
		for (int i = 0; i < 512 * 512; i++)
		{
			if (materials[i] & 0x80) write_be<std::uint32_t>(s, customColors[i]);
		}

		write_be<std::uint32_t>(s, physObjs.size());
		for (auto i = 0; i < physObjs.size(); ++i)
		{
			PhysicsObject out = physObjs[i];
			write_be<std::uint64_t>(s, out.a);
			write_be<std::uint32_t>(s, out.b);
			write_be<float>(s, out.x);
			write_be<float>(s, out.y);
			write_be<float>(s, out.rot_radians);
			write_be<double>(s, out.f);
			write_be<double>(s, out.g);
			write_be<double>(s, out.h);
			write_be<double>(s, out.i);
			write_be<double>(s, out.j);
			write_be<bool>(s, out.k);
			write_be<bool>(s, out.l);
			write_be<bool>(s, out.m);
			write_be<bool>(s, out.n);
			write_be<bool>(s, out.o);
			write_be<float>(s, out.z);
			write_be<std::uint32_t>(s, out.width);
			write_be<std::uint32_t>(s, out.height);

			auto image_size = out.width * out.height;
			for (int j = 0; j < image_size; ++j)
			{
				write_be<std::uint32_t>(s, out.colors[j]);
			}
		}
		write_compressed_file(cpath.c_str(), s.str());

		printf("finished saving chunk at (%i, %i)\n", cx, cy);
	}
	void destroy(fs::path save00_path)
	{
                fs::remove(cpath);
                fs::remove(save00_path / std::format("world/area_{}.bin", cy * 2000 + cx));
                fs::remove(save00_path / std::format("world/entities_{}.bin", cy * 2000 + cx));
	}

        int id() {
          return cy * 2000 + cx;
        }

	void redraw_mats()
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		for (int y = 0; y < 512; y++)
		{
			for (int x = 0; x < 512; x++)
			{
				int i = y * 512 + x;
				uint8_t material = materials[i] & 0x7f;
				bool custom_color = (materials[i] & 0x80) != 0;
				if (custom_color)
					texBuffer[i] = customColors[i];
				else
				{
					Material m = allMaterials[matNames[material].idx];
					int gx = x + cx * 512;
					int gy = y + cy * 512;
					gx *= 6;
					gy *= 6;
					int texX = ((gx % 252) + 252) % 252;
					int texY = ((gy % 252) + 252) % 252;
					uint32_t color = m.tex[texY * 252 + texX];
					texBuffer[i] = color;
				}
			}
		}
	}
	void redraw_physics()
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		for (const auto& physics_object : physObjs)
		{
			int lx = rint(physics_object.x) - 512 * cx;
			int ly = rint(physics_object.y) - 512 * cy;
			int ux = lx;
			int uy = ly;

			float cosine = cosf(physics_object.rot_radians);
			float sine = sinf(physics_object.rot_radians);

			if (cosine > 0)
			{
				ux += physics_object.width * cosine;
				uy += physics_object.height * cosine;
			}
			else
			{
				lx += physics_object.width * cosine;
				ly += physics_object.height * cosine;
			}

			if (sine > 0)
			{
				lx -= physics_object.height * sine;
				uy += physics_object.width * sine;
			}
			else
			{
				ux -= physics_object.height * sine;
				ly += physics_object.width * sine;
			}
			lx = std::min(std::max(lx, 0), 511);
			ly = std::min(std::max(ly, 0), 511);
			ux = std::min(std::max(ux, 0), 511);
			uy = std::min(std::max(uy, 0), 511);

			for (int pixY = ly; pixY < uy; pixY++)
				for (int pixX = lx; pixX < uy; pixX++)
				{
					float offsetPixX = pixX - (rintf(physics_object.x) - 512 * cx);
					float offsetPixY = pixY - (rintf(physics_object.y) - 512 * cy);

					int texX = rint(offsetPixX * cosine + offsetPixY * sine);
					int texY = rint(-offsetPixX * sine + offsetPixY * cosine);

					if (texX < 0 || physics_object.width <= texX || texY < 0 || physics_object.height <= texY) continue;

					int idx = pixY * 512 + pixX;
					uint32_t c2 = physics_object.colors.data()[physics_object.width * texY + texX];
					if ((c2 >> 24) == 0) continue;
					texBuffer[idx] = c2;
				}
		}
	}
	void update_tex()
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		tex->update((unsigned char*)texBuffer, 512, 512, 0, 0);
	}
	void update()
	{
		g_dirty = false;
		redraw_mats();
		redraw_physics();
		update_tex();
	}
	const char* get(int x, int y)
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		return matNames[materials[512 * y + x] & 0x7f].mat.c_str();
	}
	void set(int x, int y, const char* material)
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		materials[512 * y + x] = IndexOrAdd(matNames, material);
		g_dirty = true;
		s_dirty = true;
	}
	void set(int x, int y, const char* material, uint32_t color)
	{
		uint8_t* materials = data_buffer;
		uint32_t* customColors = (uint32_t*)(materials + 512 * 512);
		uint32_t* texBuffer = customColors + 512 * 512;
		materials[512 * y + x] = IndexOrAdd(matNames, material) | 0x80;
		customColors[512 * y + x] = color;
		g_dirty = true;
		s_dirty = true;
	}
};

struct PetriPath
{
	std::string path;
	int cx;
	int cy;
};
static std::vector<PetriPath> GetPngPetris(fs::path save00_path)
{
	std::vector<PetriPath> outVec;

        for (auto &p : fs::directory_iterator(save00_path / "world")) {
          if (p.is_regular_file() && p.path().filename().string().starts_with("world_") && p.path().extension() == ".png_petri") {
            std::stringstream filename(p.path().filename().string());
            std::string segment;
            std::getline(filename, segment, '_');  // skip to first number
            std::getline(filename, segment, '_');  // read first number
            int px = std::stoi(segment);
            std::getline(filename, segment, '_');  // read second number
            int py = std::stoi(segment);
            int cx = px / 512;
            int cy = py / 512;
            PetriPath path = { p.path().string(), cx, cy };
            outVec.push_back(path);
          }
        }
	return outVec;
}
static void ThreadLoadChunks(int tIdx, int tStride, std::vector<PetriPath> paths, std::vector<Chunk*>* outVec, std::mutex* lock)
{
	for (int i = tIdx; i < paths.size(); i += tStride)
	{
		Chunk* c = new Chunk(paths[i].path.c_str(), paths[i].cx, paths[i].cy);
		lock->lock();
		outVec->push_back(c);
		lock->unlock();
	}
	return;
}

static void ExportMapImage(std::vector<Chunk*>& chunks, int downscale = 2)
{
	int minX = 10000;
	int maxX = -10000;
	int minY = 10000;
	int maxY = -10000;
	for (Chunk* c : chunks)
	{
		if (c->cx < minX) minX = c->cx;
		if (c->cx > maxX) maxX = c->cx;
		if (c->cy < minY) minY = c->cy;
		if (c->cy > maxY) maxY = c->cy;
	}
	size_t width = (maxX - minX + 1) * 512 / downscale;
	size_t height = (maxY - minY + 1) * 512 / downscale;
	png_byte** rows = (png_byte**)malloc(sizeof(png_byte*) * height);
	for (int y = 0; y < height; y++)
	{
		rows[y] = (png_byte*)malloc(4 * width);
		memset(rows[y], 0, 4 * width);
	}
	
	for (Chunk* c : chunks)
	{
		size_t dx = (c->cx - minX) * 512;
		size_t dy = (c->cy - minY) * 512;
		for (size_t py = 0; py < 512; py += downscale)
		{
			size_t y = (dy + py) / downscale;
			for (size_t px = 0; px < 512; px += downscale)
			{
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

sf::Vector2f viewportCenter(512, 512);
float zoomLevel = 1;

static sf::Vector2f mouseLocal(sf::RenderWindow& window)
{
	return sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
}
static sf::Vector2f localToGlobal(sf::Vector2f local)
{
	return local / zoomLevel + viewportCenter;
}
static sf::Vector2i roundGlobal(sf::Vector2f global)
{
	return { (int)floorf(global.x), (int)floorf(global.y) };
}
static sf::Vector2i globalToChunk(sf::Vector2i global)
{
	return { (int)floorf((float)global.x / 512), (int)floorf((float)global.y / 512) };
}
static sf::Vector2i globalToOffset(sf::Vector2i global)
{
	sf::Vector2i cc = globalToChunk(global);
	return { global.x - 512 * cc.x, global.y - 512 * cc.y };
}
static Chunk* chunkIdx(std::vector<Chunk*> chunks, sf::Vector2i index)
{
	for (Chunk* chunk : chunks)
	{
		if (chunk->cx == index.x && chunk->cy == index.y)
			return chunk;
	}
	return NULL;
}

static void set_circle(std::vector<Chunk*> chunks, sf::Vector2f center, float radius, const char* material)
{
	for (int dy = -radius - 1; dy < radius + 1; dy++)
	{
		for (int dx = -radius - 1; dx < radius + 1; dx++)
		{
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
			{
				chunk->set(cOff.x, cOff.y, material);
			}
		}
	}
}
static void drawTextAligned(const char* text, sf::Vector2f position, uint32_t size, int hAlign, int vAlign, sf::Font font, sf::RenderWindow& window)
{
	sf::Text t(text, font, size);
	t.setPosition(position);
	sf::FloatRect bounds = t.getGlobalBounds();
	int xPos = position.x;
	int yPos = position.y;
	if (hAlign == 2) xPos = position.x - bounds.width;
	else if (hAlign == 1) xPos = position.x - bounds.width * 0.5f;
	if (vAlign == 2) yPos = position.y - bounds.height;
	else if (vAlign == 1) yPos = position.y - bounds.height * 0.5f;
	t.setPosition(xPos, yPos);
	window.draw(t);
}

int main(int argc, char** argv)
{
	fs::path save00_path;
	if (argc > 1)
	{
		save00_path = argv[1];
	}
	else
	{
#ifdef _WIN32
          save00_path = std::format("{}/../LocalLow/Nolla_Games_Noita/save00", getenv("APPDATA"));
#else
          save00_path = std::format("{}/.local/share/steam/steamapps/compatdata/881100/pfx/drive_c/users/steamuser/AppData/LocalLow/Nolla_Games_Noita/save00", getenv("HOME"));
#endif
	}

        std::string streamInfoPath = std::format("{}/world/.stream_info", save00_path.string());

	std::ifstream save00ExistenceStream(streamInfoPath, std::ios::binary);
	if (save00ExistenceStream.fail())
	{
		std::cerr << "ERR: stream_info file not found at " << streamInfoPath << "! Ensure that there is a valid save present, or if not a Windows user, run the program again with NoitaMapViewer <path-to-save00>\n";
		// _kbhit();
		return -1;
	}
	save00ExistenceStream.close();
	std::vector<StreaminfoBackground*> sibgs = ParseStreaminfo(streamInfoPath);
        streamInfoPath = std::format("{}/world/world_pixel_scenes.bin", save00_path.string());
	std::vector<PixelsceneBackground*> psbgs = ParsePixelScenes(streamInfoPath);

	LoadMats("data/mats/");
	auto paths = GetPngPetris(save00_path);
	std::vector<Chunk*> chunks;
	std::vector<std::thread> threads;
	std::mutex lock;
	for (int i = 0; i < std::thread::hardware_concurrency(); i++)
		threads.emplace_back(std::thread(ThreadLoadChunks, i, std::thread::hardware_concurrency(), paths, &chunks, &lock));
	for (int i = 0; i < std::thread::hardware_concurrency(); i++)
	{
		while (!threads[i].joinable());
		threads[i].join();
	}

	sf::Vector2f initial_display_sz(950.f, 800.f);
	sf::RenderWindow window(
		sf::VideoMode(initial_display_sz.x, initial_display_sz.y),
		"World Viewer"
	);

	window.setVerticalSyncEnabled(true);
	auto handle_resize = [&](sf::Vector2f new_size) {
		sf::View view(new_size / 2.f, new_size);
		window.setView(view);
	};
	handle_resize(initial_display_sz);

	for (Chunk* c : chunks)
	{
		c->tex = new sf::Texture();
		c->tex->create(512, 512);
		c->update_tex();
	}

	sf::Font font = sf::Font();
	font.loadFromFile("NoitaPixel.ttf");

	constexpr float scrollZoomSensitivity = 1.3f;
	constexpr float keyZoomSensitivity = 1.05f;
	constexpr float keyPanSensitivity = 15;

	sf::Vector2f lastMousePos;
	bool mouseDownLastFrame = false;
	
	int mode = 0;
	bool tooltip = true;
	bool keybinds = true;
	bool outline = true;

	bool matInput = false;
	std::string matEntered;

	const char* material = "gold";
	float drawRadius = 2;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::Resized)
			{
				handle_resize(sf::Vector2f(event.size.width, event.size.height));
			}

			if (event.type == sf::Event::MouseWheelMoved)
			{
				if (mode == 1 && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
				{
					drawRadius *= powf(scrollZoomSensitivity, event.mouseWheel.delta);
				}
				else
				{
					if (event.mouseWheel.delta > 0)
					{
						sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
						sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
						zoomLevel *= scrollZoomSensitivity;
						sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
						sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
						viewportCenter += delta;
					}
					else
					{
						sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
						sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
						zoomLevel /= scrollZoomSensitivity;
						sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
						sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
						viewportCenter += delta;
					}
				}
			}

			if (event.type == sf::Event::TextEntered)
			{
				if (matInput)
				{
					char character = event.text.unicode;
					if (character == 0x08)
					{
						if (matEntered.size() > 0) matEntered.erase(matEntered.size() - 1);
						continue;
					}
					bool isNum = 0x30 <= character && character <= 0x39;
					bool isLower = 0x61 <= character && character <= 0x7a;
					bool isUpper = 0x41 <= character && character <= 0x5a;
					if (isNum || isUpper || isLower || character == '_')
					{
						matEntered.push_back(character);
						continue;
					}
				}
			}

			if (event.type == sf::Event::KeyPressed)
			{
				if (matInput)
				{
					if (event.key.code == sf::Keyboard::Enter)
					{
						material = matEntered.c_str();
						matInput = false;
						continue;
					}
				}
				if (mode == 1 && event.key.code == sf::Keyboard::SemiColon && event.key.shift)
				{
					matInput = true;
					matEntered.clear();
				}
				else if (event.key.code == sf::Keyboard::P && event.key.control)
				{
					printf("Saving map to map.png...\n");
					ExportMapImage(chunks, event.key.shift ? 1: 4);
				}
				else if (mode == 1 && event.key.code == sf::Keyboard::S && event.key.control)
				{
					for (Chunk* chunk : chunks)
						if (chunk->s_dirty)
							chunk->save();
				}
				else if (mode == 2 && event.key.code == sf::Keyboard::S && event.key.control)
				{
					for (int i = 0; i < chunks.size(); i++)
						if (chunks[i]->marked_del)
						{
							chunks[i]->destroy(save00_path);
							chunks.erase(chunks.begin() + i);
							i--;
						}
				}
				else if (event.key.code == sf::Keyboard::LAlt || event.key.code == sf::Keyboard::RAlt)
					tooltip = !tooltip;
				else if (event.key.code == sf::Keyboard::Tab)
					keybinds = !keybinds;
				else if (event.key.code == sf::Keyboard::Q)
					outline = !outline;
				else if (event.key.code == sf::Keyboard::V && event.key.shift)
					mode = 0;
				else if (event.key.code == sf::Keyboard::E && event.key.shift)
					mode = 1;
				else if (event.key.code == sf::Keyboard::R && event.key.shift)
					mode = 2;
			}
		
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (mode == 2 && event.mouseButton.button == sf::Mouse::Right)
				{
					sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
					sf::Vector2i c = globalToChunk(gPos);
					Chunk* chunk = chunkIdx(chunks, c);
					if (chunk)
						chunk->marked_del = !chunk->marked_del;
				}
			}
		}
		if (window.hasFocus() && !matInput)
		{
			float actualPanSensitivity = keyPanSensitivity;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) actualPanSensitivity *= 3;

			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem))
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) viewportCenter.x -= actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) viewportCenter.x += actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) viewportCenter.y -= actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) viewportCenter.y += actualPanSensitivity / zoomLevel;

				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal))
				{
					sf::Vector2f distFromCenter = mouseLocal(window);
					sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
					zoomLevel *= keyZoomSensitivity;
					sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
					sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
					viewportCenter += delta;
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) || sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen))
				{
					sf::Vector2f distFromCenter = mouseLocal(window);
					sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
					zoomLevel /= keyZoomSensitivity;
					sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
					sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
					viewportCenter += delta;
				}
			}
			if (mode == 0 || mode == 2)
			{
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					sf::Vector2f newMousePos = sf::Vector2f(sf::Mouse::getPosition());
					if (mouseDownLastFrame)
					{
						sf::Vector2f posDiff = newMousePos - lastMousePos;
						viewportCenter -= posDiff / zoomLevel;
					}
					lastMousePos = newMousePos;
					mouseDownLastFrame = true;
				}
				else mouseDownLastFrame = false;
			}
			else if (mode == 1)
			{
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					sf::Vector2f gPos = localToGlobal(mouseLocal(window));

					if (mouseDownLastFrame)
					{
						sf::Vector2f diff = gPos - lastMousePos;
						float len = sqrtf(diff.x * diff.x + diff.y * diff.y);
						for (float offset = 0; offset < len; offset = fminf(offset + drawRadius / 4, len))
							set_circle(chunks, gPos - diff * (offset / len), drawRadius, material);
					}
					else
						set_circle(chunks, gPos, drawRadius, material);

					lastMousePos = gPos;
					mouseDownLastFrame = true;
				}
				else mouseDownLastFrame = false;

				if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
				{
					sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
					sf::Vector2i cPos = globalToChunk(gPos);
					sf::Vector2i cOff = globalToOffset(gPos);
					Chunk* chunk = chunkIdx(chunks, cPos);
					if (chunk)
					{
						material = chunk->get(cOff.x, cOff.y);
					}
				}
			}
		}

		window.clear(sf::Color::Black);

		sf::Vector2f screenSize = sf::Vector2f(window.getSize());
		sf::Vector2f topLeftOffset = screenSize * 0.5f;
		for (StreaminfoBackground* bg : sibgs) {
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (PixelsceneBackground* bg : psbgs) {
			if (!bg->exists) continue;
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (Chunk* chunk : chunks)
		{
			if (chunk->g_dirty) chunk->update();
			sf::Sprite s;
			s.setTexture(*chunk->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((chunk->cx * 512 - viewportCenter.x) * zoomLevel + topLeftOffset.x, (chunk->cy * 512 - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (Chunk* chunk : chunks)
		{
			if (mode == 2 && chunk->marked_del)
			{
				sf::RectangleShape r(sf::Vector2f(512 * zoomLevel, 512 * zoomLevel));
				r.setPosition((chunk->cx * 512 - viewportCenter.x) * zoomLevel + topLeftOffset.x, (chunk->cy * 512 - viewportCenter.y) * zoomLevel + topLeftOffset.y);
				r.setFillColor(sf::Color::Transparent);
				r.setOutlineThickness(1);
				r.setOutlineColor(sf::Color::Red);
				window.draw(r);
			}
		}

		if (tooltip)
		{
			sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
			sf::Vector2i cPos = globalToChunk(gPos);
			sf::Vector2i cOff = globalToOffset(gPos);
			int matIdx = -1;
			Chunk* chunk = chunkIdx(chunks, cPos);
			if (chunk) {
				matIdx = chunk->matNames[chunk->data_buffer[512 * cOff.y + cOff.x] & 0x7f].idx;
			}
			
			char tooltipText[255];
                        int chunkId = 0,
                            worldX = 0,
                            worldY = 0;
                        if (chunk != nullptr) {
                          chunkId = chunk->id();
                          worldX = chunk->cx * 512;
                          worldY = chunk->cy * 512;
                        }


			sprintf(tooltipText, "(%i, %i: chunk %i, world %i_%i)\n%s", (int)gPos.x, (int)gPos.y, chunkId, worldX, worldY, matIdx >= 0 ? allMaterials[matIdx].name.c_str() : "");
			sf::Text text = sf::Text(sf::String((const char*)tooltipText), font);
			text.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)) + sf::Vector2f(10, 2));
			window.draw(text);
		}

		if (mode == 1 && outline)
		{
			sf::CircleShape circle(drawRadius * zoomLevel);
			circle.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(drawRadius * zoomLevel,drawRadius * zoomLevel));
			circle.setFillColor(sf::Color::Transparent);
			circle.setOutlineThickness(1);
			circle.setOutlineColor(sf::Color::White);
			window.draw(circle);
		}

		if (mode == 1)
		{
			char matBuffer[128];
			if (matInput)
				sprintf(matBuffer, "Material: [%s]", matEntered.c_str());
			else
				sprintf(matBuffer, "Material: %s", material);
			drawTextAligned(matBuffer, sf::Vector2f(window.getSize().x / 2, window.getSize().y - 50), 48, 1, 2, font, window);
		}

		if (keybinds)
		{
			const char* modeNames[] = {
				"[[ View Mode ]]",
				"[[ Edit Mode ]]",
				"[[ Delete Mode]]"
			};
			drawTextAligned(modeNames[mode], sf::Vector2f(window.getSize().x / 2, 0), 48, 1, 0, font, window);

			drawTextAligned(
                            "== Global ==\n"
                            "WASD/Arrow Keys - Camera Panning\n"
                            "Scroll/+- - Zoom\n"
                            "SHIFT+V - Switch to View Mode\n"
                            "SHIFT+E - Switch to Edit Mode\n"
                            "SHIFT+R - Switch to Delete Mode\n"
                            "CTRL+P - Export to PNG\n"
                            "CTRL+SHIFT+P - Export to HD PNG\n"
                            "ALT - Toggle Tooltips\n"
                            "Tab - Toggle this display\n"
                            "\n"
                            "== View Mode ==\n"
                            "LMB - Camera Panning\n"
                            "\n"
                            "== Edit Mode ==\n"
                            "LMB - Draw Material\n"
                            "RMB - Copy Material\n"
                            "Q - Toggle Outline\n"
                            "SHIFT+Scroll - Change Draw Radius\n"
                            "SHIFT+; ... Enter - Choose Material by Name\n"
                            "CTRL+S - Save Edited Chunks\n"
                            "\n"
                            "== Delete Mode ==\n"
                            "LMB - Camera Panning\n"
                            "RMB - Mark Chunk for Deletion\n"
                            "CTRL+S - Delete Marked Chunks\n",
                            sf::Vector2f(10, 0), 30, 0, 0, font, window
                        );
		}

		window.display();
	}
}
