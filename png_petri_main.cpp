#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "binops.hpp"
#include "utils.h"
#include "colors.h"

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

struct RenderObject {
	sf::Sprite sprite;
	std::unique_ptr<sf::Texture> texture;
	const PhysicsObject* physics_object;
};

const float degrees_in_radians = 57.2957795131;

struct read_vec_uint32_result {
	const char* ptr;
	std::vector<std::uint32_t> data;
};

read_vec_uint32_result read_vec_uint32(const char* ptr)
{
	auto count = read_be<std::uint32_t>(ptr);
	std::vector<std::uint32_t> result(count);

	ptr += 4;
	for (auto& out : result) {
		out = read_be<std::uint32_t>(ptr);
		ptr += 4;
	}

	return {ptr, result};
}

int GetMaterialIndex(const char* color)
{
	for (int i = 0; i < numMats; i++)
	{
		if (allMaterials[i].name != NULL && strcmp(color, allMaterials[i].name) == 0)
		{
			return i;
		}
	}
	printf("ERR: Missing texture for material: %s\n", color);
	return 0;
}

struct ChunkSprite
{
	int cx;
	int cy;
	sf::Texture tex;
};

ChunkSprite RenderChunk(const char* save00_path, int cx, int cy)
{
	char pathBuffer[200];
	sprintf_s(pathBuffer, "%s/world/world_%i_%i.png_petri", save00_path, cx * 512, cy * 512);
	std::ifstream fileExistenceStream(pathBuffer, std::ios::binary);
	if (fileExistenceStream.fail())
	{
		//std::cerr << "File not found: " << pathBuffer << '\n';
		return { cx, cy, sf::Texture() };
	}
	fileExistenceStream.close();

	auto file_contents = read_compressed_file(pathBuffer);
	auto data = file_contents.c_str();
	auto data_end = data + file_contents.size();

	auto version = read_be<std::uint32_t>(data);
	auto width_q = read_be<std::uint32_t>(data + 4);
	auto height_q = read_be<std::uint32_t>(data + 8);

	if (version != 24 || width_q != 512 || height_q != 512)
	{
		std::cerr << "Unexpected header:\n";
		std::cerr << "  version: " << version << '\n';
		std::cerr << "  width?: " << width_q << '\n';
		std::cerr << "  height?: " << height_q << '\n';
		return { cx, cy, sf::Texture() };
	}

	auto world_cells_start = data + 12;

	std::vector<std::uint8_t> hp_values_q(512 * 512);
	std::memcpy(hp_values_q.data(), world_cells_start, 512 * 512);

	auto material_names_start = world_cells_start + 512 * 512;
	auto material_name_count = read_be<std::uint32_t>(material_names_start);
	std::vector<std::string> material_names(material_name_count);

	auto material_names_ptr = material_names_start + 4;
	for (int i = 0; i < material_name_count; ++i)
	{
		auto size = read_be<std::uint32_t>(material_names_ptr);
		material_names[i].resize(size);
		std::memcpy((void*)material_names[i].data(), material_names_ptr + 4, size);
		material_names_ptr += 4 + size;
	}

	int* material_indices = (int*)malloc(4 * material_name_count);
	for (int i = 0; i < material_name_count; i++)
		material_indices[i] = GetMaterialIndex(material_names[i].c_str());

	auto [physics_objects_start, custom_world_colors] = read_vec_uint32(material_names_ptr);

	auto physics_object_count = read_be<std::uint32_t>(physics_objects_start);
	auto current_object = physics_objects_start + 4;

	// assert version == 24
	std::vector<PhysicsObject> physics_objects(physics_object_count);

	for (auto i = 0; i < physics_object_count; ++i)
	{
		auto& into = physics_objects[i];

		into.a = read_be<std::uint64_t>(current_object);
		into.b = read_be<std::uint32_t>(current_object + 8);
		into.x = read_be<float>(current_object + 12);
		into.y = read_be<float>(current_object + 16);
		into.rot_radians = read_be<float>(current_object + 20);
		into.f = read_be<double>(current_object + 24);
		into.g = read_be<double>(current_object + 32);
		into.h = read_be<double>(current_object + 40);
		into.i = read_be<double>(current_object + 48);
		into.j = read_be<double>(current_object + 56);
		into.k = read_be<bool>(current_object + 64);
		into.l = read_be<bool>(current_object + 65);
		into.m = read_be<bool>(current_object + 66);
		into.n = read_be<bool>(current_object + 67);
		into.o = read_be<bool>(current_object + 68);
		into.z = read_be<float>(current_object + 69);
		into.width = read_be<std::uint32_t>(current_object + 73);
		into.height = read_be<std::uint32_t>(current_object + 77);

		auto image_size = into.width * into.height;
		into.colors.resize(image_size);

		auto image_data = current_object + 81;
		for (int j = 0; j < image_size; ++j)
		{
			into.colors[j] = read_be<std::uint32_t>(image_data);
			image_data += 4;
		}
		current_object = image_data;
	}

	//auto unknown2_start = current_object;
	//auto unknown2_count = read_be<std::uint32_t>(unknown2_start);

	//std::cout << "Unhandled offset: " << current_object - data << '\n';
	//std::cout << "Unhandled bytes: " << data_end - current_object << '\n';

	/////////////////
	// Write image //
	/////////////////

	uint32_t* RGBABuffer = (uint32_t*)malloc(4 * 512 * 512);
	auto custom_color_it = custom_world_colors.begin();
	for (int i = 0; i != 512 * 512; ++i)
	{
		auto posx = i % 512;
		auto posy = i / 512;
		auto material = hp_values_q[i] & (~0x80);
		auto custom_color = (hp_values_q[i] & 0x80) != 0;
		if (custom_color)
		{
			RGBABuffer[i] = *custom_color_it;
			++custom_color_it;
		}
		else
		{
			Material m = allMaterials[material_indices[material]];
			int gx = posx + cx * 512;
			int gy = posy + cy * 512;
			gx *= 6;
			gy *= 6;
			int texX = ((gx % 252) + 252) % 252;
			int texY = ((gy % 252) + 252) % 252;
			uint32_t color = m.tex[texY * 252 + texX];
			RGBABuffer[i] = color;//swapEndianness(color);
		}
	}
	for (const auto& physics_object : physics_objects)
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
				uint32_t c = physics_object.colors.data()[physics_object.width * texY + texX];
				if ((c >> 24) == 0) continue;
				RGBABuffer[idx] = c;
			}
	}

	sf::Texture world_texture;
	world_texture.create(0x200, 0x200);
	world_texture.update((unsigned char*)RGBABuffer, 512, 512, 0, 0);
	free(material_indices);
	free(RGBABuffer);
	printf("finished rendering chunk at (%i, %i)\n", cx, cy);
	return { cx, cy, world_texture };
}

void IteratePngPetris(const char* save00_path, std::vector<ChunkSprite>& outVec)
{
	WIN32_FIND_DATA fd;

	char buffer[_MAX_PATH];
	int offset = 0;
	_putstr_offset(save00_path, buffer, offset);
	_putstr_offset("/world/world_*.png_petri", buffer, offset);
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

				_putstr_offset(save00_path, buffer, offset);
				_putstr_offset("/world/", buffer, offset);
				_putstr_offset(buffer2, buffer, offset);
				buffer[offset] = '\0';

				int secondUnderscore = offset - 12;
				for (;secondUnderscore >= 0; secondUnderscore--) if (buffer[secondUnderscore] == '_') break;
				int firstUnderscore = secondUnderscore - 2;
				for (;firstUnderscore >= 0; firstUnderscore--) if (buffer[firstUnderscore] == '_') break;
				int px = atoi(buffer + firstUnderscore + 1);
				int py = atoi(buffer + secondUnderscore + 1);
				int cx = px / 512;
				int cy = py / 512;

				outVec.push_back(RenderChunk(save00_path, cx, cy));
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}

int main(int argc, char** argv)
{
	char save00_path[_MAX_PATH];
	if (argc > 1)
	{
		strcpy_s(save00_path, argv[1]);
	}
	else
	{
		strcpy_s(save00_path, "%appdata%");
		size_t bufferSize = _MAX_PATH;
		getenv_s(&bufferSize, save00_path, "appdata");
		std::string tempStr(save00_path);
		tempStr = tempStr.substr(0, tempStr.length() - 8);
		sprintf_s(save00_path, "%s/LocalLow/Nolla_Games_Noita/save00", tempStr.c_str());
	}

	char streamInfoPath[_MAX_PATH];
	sprintf_s(streamInfoPath, "%s/world/.stream_info", save00_path);

	std::ifstream save00ExistenceStream(streamInfoPath, std::ios::binary);
	if (save00ExistenceStream.fail())
	{
		std::cerr << "ERR: stream_info file not found at " << streamInfoPath << "! Ensure that there is a valid save present, or if not a Windows user, run the program again with NoitaMapViewer <path-to-save00>\n";
		std::getchar();
		return -1;
	}
	save00ExistenceStream.close();


	//char pathBuffer[200];
	//sprintf_s(pathBuffer, "%s/world/area_0.bin", save00_path);
	//for (int i = 0; i < read_compressed_file(pathBuffer).length(); i++) printf("%c", ((char*)read_compressed_file(pathBuffer).c_str())[i]);
	//printf("\n");

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

	LoadMats("mats/");
	std::vector<ChunkSprite> chunks;
	IteratePngPetris(save00_path, chunks);

	sf::Vector2f viewportCenter(512, 512);

	constexpr float scrollZoomSensitivity = 1.2f;
	constexpr float keyZoomSensitivity = 1.05f;
	constexpr float keyPanSensitivity = 15;

	float zoomLevel = 1;

	sf::Vector2f lastMousePos;
	bool mouseDownLastFrame = false;

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
				if (event.mouseWheel.delta == 1)
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
		if (window.hasFocus())
		{
			float actualPanSensitivity = keyPanSensitivity;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) actualPanSensitivity *= 3;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) viewportCenter.x -= actualPanSensitivity / zoomLevel;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) viewportCenter.x += actualPanSensitivity / zoomLevel;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) viewportCenter.y -= actualPanSensitivity / zoomLevel;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) viewportCenter.y += actualPanSensitivity / zoomLevel;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal))
			{
				sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
				sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
				zoomLevel *= keyZoomSensitivity;
				sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
				sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
				viewportCenter += delta;
			}
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) || sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen))
			{
				sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
				sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
				zoomLevel /= keyZoomSensitivity;
				sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
				sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
				viewportCenter += delta;
			}

			if (sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right))
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

		window.clear(sf::Color::Black);

		sf::Vector2f screenSize = sf::Vector2f(window.getSize());
		sf::Vector2f topLeftOffset = screenSize * 0.5f;
		for (ChunkSprite& chunk : chunks)
		{
			sf::Sprite s;
			s.setTexture(chunk.tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((chunk.cx * 512 - viewportCenter.x) * zoomLevel + topLeftOffset.x, (chunk.cy * 512 - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}

		window.display();
	}
}
