#define _CRT_SECURE_NO_WARNINGS
#include <filesystem>
#include <SFML/Graphics.hpp>
#include "materials.h"
#include "png_petri.h"
#include "streaminfo.h"
#include "chunks.h"
#include "edit.h"


sf::Vector2f viewportCenter(512, 512);
float zoomLevel = 1;

static sf::Vector2f mouseLocal(sf::RenderWindow& window) {
	return sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
}
static sf::Vector2f localToGlobal(sf::Vector2f local) {
	return local / zoomLevel + viewportCenter;
}

static void drawTextAligned(const char* text, sf::Vector2f position, uint32_t size, int hAlign, int vAlign, sf::Font font, sf::RenderWindow& window) {
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

int main(int argc, char** argv) {
	std::filesystem::path current = std::filesystem::current_path();
	std::filesystem::path save00_path = FindSave00(argc > 1 ? argv[1] : "");
	read_wak(find_wak((current/".wakpath").string().c_str()).string().c_str());
	std::filesystem::current_path() = current;
	std::filesystem::path pixelscene_path = save00_path / "world/world_pixel_scenes.bin";
	auto streaminfo_bgs = ParseStreaminfo((save00_path / "world/.stream_info").string().c_str());
	auto pixelscene_bgs = ParsePixelScenes(pixelscene_path.string().c_str());
	LoadMats((current / "data/mats/").string().c_str());
	auto chunks = LoadPngPetris((save00_path / "world/").string().c_str());


	sf::Font font = sf::Font();
	font.loadFromFile((current / "data/NoitaPixel.ttf").string());

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

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::Resized) {
				handle_resize(sf::Vector2f(event.size.width, event.size.height));
			}

			if (event.type == sf::Event::MouseWheelMoved) {
				if (mode == 1 && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
					drawRadius *= powf(scrollZoomSensitivity, event.mouseWheel.delta);
				}
				else {
					if (event.mouseWheel.delta > 0) {
						sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
						sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
						zoomLevel *= scrollZoomSensitivity;
						sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
						sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
						viewportCenter += delta;
					}
					else {
						sf::Vector2f distFromCenter = sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(window.getSize()) * 0.5f;
						sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
						zoomLevel /= scrollZoomSensitivity;
						sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
						sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
						viewportCenter += delta;
					}
				}
			}

			if (event.type == sf::Event::TextEntered) {
				if (matInput) {
					char character = event.text.unicode;
					if (character == 0x08) {
						if (matEntered.size() > 0) matEntered.erase(matEntered.size() - 1);
						continue;
					}
					bool isNum = 0x30 <= character && character <= 0x39;
					bool isLower = 0x61 <= character && character <= 0x7a;
					bool isUpper = 0x41 <= character && character <= 0x5a;
					if (isNum || isUpper || isLower || character == '_') {
						matEntered.push_back(character);
						continue;
					}
				}
			}

			if (event.type == sf::Event::KeyPressed) {
				if (matInput) {
					if (event.key.code == sf::Keyboard::Enter) {
						material = matEntered.c_str();
						matInput = false;
						continue;
					}
				}
				if (mode == 1 && event.key.code == sf::Keyboard::SemiColon && event.key.shift) {
					matInput = true;
					matEntered.clear();
				}
				else if (event.key.code == sf::Keyboard::P && event.key.control) {
					printf("Saving map to map.png...\n");
					ExportMapImage(chunks, event.key.shift ? 1 : 4);
				}
				else if (mode == 1 && event.key.code == sf::Keyboard::S && event.key.control) {
					for (Chunk* chunk : chunks)
						if (chunk->file_dirty)
							SaveChunk(*chunk);
				}
				else if (mode == 2 && event.key.code == sf::Keyboard::S && event.key.control) {
					for (int i = 0; i < chunks.size(); i++)
						if (chunks[i]->marked_for_delete) {
							DestroyChunk(*chunks[i], pixelscene_bgs, save00_path.string().c_str());
							chunks.erase(chunks.begin() + i);
							i--;
						}
					WritePixelScenes(pixelscene_path.string().c_str(), pixelscene_bgs);
				}
				else if (event.key.code == sf::Keyboard::LAlt || event.key.code == sf::Keyboard::RAlt)
					tooltip = !tooltip;
				else if (event.key.code == sf::Keyboard::Slash && event.key.shift)
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

			if (event.type == sf::Event::MouseButtonPressed) {
				if (mode == 2 && event.mouseButton.button == sf::Mouse::Right) {
					sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
					sf::Vector2i c = globalToChunk(gPos);
					Chunk* chunk = chunkIdx(chunks, c);
					if (chunk)
						chunk->marked_for_delete = !chunk->marked_for_delete;
				}
			}
		}
		if (window.hasFocus() && !matInput) {
			float actualPanSensitivity = keyPanSensitivity;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) actualPanSensitivity *= 3;

			if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem)) {
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) viewportCenter.x -= actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) viewportCenter.x += actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) viewportCenter.y -= actualPanSensitivity / zoomLevel;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) viewportCenter.y += actualPanSensitivity / zoomLevel;

				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add) || sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
					sf::Vector2f distFromCenter = mouseLocal(window);
					sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
					zoomLevel *= keyZoomSensitivity;
					sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
					sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
					viewportCenter += delta;
				}
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract) || sf::Keyboard::isKeyPressed(sf::Keyboard::Hyphen)) {
					sf::Vector2f distFromCenter = mouseLocal(window);
					sf::Vector2f distFromCenterWorldCoords = distFromCenter / zoomLevel;
					zoomLevel /= keyZoomSensitivity;
					sf::Vector2f distFromCenterWorldCoords2 = distFromCenter / zoomLevel;
					sf::Vector2f delta = distFromCenterWorldCoords - distFromCenterWorldCoords2;
					viewportCenter += delta;
				}
			}
			if (mode == 0 || mode == 2) {
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
					sf::Vector2f newMousePos = sf::Vector2f(sf::Mouse::getPosition());
					if (mouseDownLastFrame) {
						sf::Vector2f posDiff = newMousePos - lastMousePos;
						viewportCenter -= posDiff / zoomLevel;
					}
					lastMousePos = newMousePos;
					mouseDownLastFrame = true;
				}
				else mouseDownLastFrame = false;
			}
			else if (mode == 1) {
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
					sf::Vector2f gPos = localToGlobal(mouseLocal(window));

					if (mouseDownLastFrame) {
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

				if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
					sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
					sf::Vector2i cPos = globalToChunk(gPos);
					sf::Vector2i cOff = globalToOffset(gPos);
					Chunk* chunk = chunkIdx(chunks, cPos);
					if (chunk) {
						material = ChunkGet(*chunk, cOff.x, cOff.y);
					}
				}
			}
		}

		window.clear(sf::Color::Black);

		sf::Vector2f screenSize = sf::Vector2f(window.getSize());
		sf::Vector2f topLeftOffset = screenSize * 0.5f;
		for (StreaminfoBackground* bg : streaminfo_bgs) {
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (PixelSceneBackground* bg : pixelscene_bgs.pending) {
			if (!bg->exists) continue;
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (PixelSceneBackground* bg : pixelscene_bgs.placed) {
			if (!bg->exists) continue;
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (PixelSceneBackground* bg : pixelscene_bgs.backgrounds) {
			if (!bg->exists) continue;
			sf::Sprite s;
			s.setTexture(bg->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((bg->x - viewportCenter.x) * zoomLevel + topLeftOffset.x, (bg->y - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (Chunk* chunk : chunks) {
			if (chunk->image_dirty || !chunk->image_loaded) ReloadChunkImage(*chunk);
			sf::Sprite s;
			s.setTexture(*chunk->tex);
			s.setScale(zoomLevel, zoomLevel);
			s.setPosition((chunk->cx * 512 - viewportCenter.x) * zoomLevel + topLeftOffset.x, (chunk->cy * 512 - viewportCenter.y) * zoomLevel + topLeftOffset.y);
			window.draw(s);
		}
		for (Chunk* chunk : chunks) {
			if (mode == 2 && chunk->marked_for_delete) {
				sf::RectangleShape r(sf::Vector2f(512 * zoomLevel, 512 * zoomLevel));
				r.setPosition((chunk->cx * 512 - viewportCenter.x) * zoomLevel + topLeftOffset.x, (chunk->cy * 512 - viewportCenter.y) * zoomLevel + topLeftOffset.y);
				r.setFillColor(sf::Color::Transparent);
				r.setOutlineThickness(1);
				r.setOutlineColor(sf::Color::Red);
				window.draw(r);
			}
		}

		if (tooltip) {
			sf::Vector2i gPos = roundGlobal(localToGlobal(mouseLocal(window)));
			sf::Vector2i cPos = globalToChunk(gPos);
			sf::Vector2i cOff = globalToOffset(gPos);
			int matIdx = -1;
			Chunk* chunk = chunkIdx(chunks, cPos);
			if (chunk) {
				matIdx = chunk->data_buffer[512 * cOff.y + cOff.x] & 0x7f;
			}

			char tooltipText[255];
			sprintf(tooltipText, "(%i, %i)\n%s", (int)gPos.x, (int)gPos.y, matIdx >= 0 ? chunk->mat_names[matIdx].name.c_str() : "");
			sf::Text text = sf::Text(sf::String((const char*)tooltipText), font);
			text.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)) + sf::Vector2f(10, 2));
			window.draw(text);
		}

		if (mode == 1 && outline) {
			sf::CircleShape circle(drawRadius * zoomLevel);
			circle.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)) - sf::Vector2f(drawRadius * zoomLevel, drawRadius * zoomLevel));
			circle.setFillColor(sf::Color::Transparent);
			circle.setOutlineThickness(1);
			circle.setOutlineColor(sf::Color::White);
			window.draw(circle);
		}

		if (mode == 1) {
			char matBuffer[128];
			if (matInput)
				sprintf(matBuffer, "Material: [%s]", matEntered.c_str());
			else
				sprintf(matBuffer, "Material: %s", material);
			drawTextAligned(matBuffer, sf::Vector2f(window.getSize().x / 2, window.getSize().y - 50), 48, 1, 2, font, window);
		}

		const char* modeNames[] = {
			"[[ View Mode ]]",
			"[[ Edit Mode ]]",
			"[[ Delete Mode]]"
		};
		drawTextAligned(modeNames[mode], sf::Vector2f(window.getSize().x / 2, 0), 48, 1, 0, font, window);
		if (keybinds) {
			drawTextAligned("\
== Global ==\n\
WASD/Arrow Keys - Camera Panning\n\
Scroll/+- - Zoom\n\
SHIFT+V - Switch to View Mode\n\
SHIFT+E - Switch to Edit Mode\n\
SHIFT+R - Switch to Delete Mode\n\
CTRL+P - Export to PNG\n\
CTRL+SHIFT+P - Export to HD PNG\n\
ALT - Toggle Tooltips\n\
? - Toggle this display\n\
\n\
== View Mode ==\n\
LMB - Camera Panning\n\
\n\
== Edit Mode ==\n\
LMB - Draw Material\n\
RMB - Copy Material\n\
Q - Toggle Outline\n\
SHIFT+Scroll - Change Draw Radius\n\
SHIFT+; ... Enter - Choose Material by Name\n\
CTRL+S - Save Edited Chunks\n\
\n\
== Delete Mode ==\n\
LMB - Camera Panning\n\
RMB - Mark Chunk for Deletion\n\
CTRL+S - Delete Marked Chunks\n\
", sf::Vector2f(10, 0), 30, 0, 0, font, window);
		}

		window.display();
	}
}
