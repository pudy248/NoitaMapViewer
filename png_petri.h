#pragma once

#include "chunks.h"
#include "winapi_wrappers.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


struct PetriPath {
	std::string path;
	//int cx;
	//int cy;
};

static std::vector<PetriPath> GetPngPetris(const char* save00_path) {
	std::vector<PetriPath> out;
	for_each_file(save00_path, ".png_petri", false, [&](const std::filesystem::path& p) {
		PetriPath petri = {p.string()};
		//for (int i = 0, n = 0; path[i]; i++) {
		//	if (path[i] == '_') {
		//		if (n == 0) p.cx = atoi(path + i + 1);
		//		else if (n == 1) p.cy = atoi(path + i + 1);
		//		n++;
		//	}
		//}
		out.emplace_back(petri);
	});
	return out;
}

void ThreadLoadChunks(
	int tIdx, int tStride, std::vector<PetriPath>* paths, std::vector<Chunk*>* outVec, std::mutex* lock) {
	for (int i = tIdx; i < paths->size(); i += tStride) {
		Chunk* c = new Chunk();
		*c = ParseChunkData((*paths)[i].path.c_str());
		ReloadChunkImage(*c);
		lock->lock();
		outVec->push_back(c);
		lock->unlock();
	}
}

std::vector<Chunk*> LoadPngPetris(const char* save00_path) {
	auto paths = GetPngPetris(save00_path);
	std::vector<Chunk*> chunks;
	std::mutex lock;
	{
		std::vector<std::jthread> threads;
		for (int i = 0; i < std::thread::hardware_concurrency(); i++)
			threads.emplace_back(
				std::jthread(&ThreadLoadChunks, i, (int)std::thread::hardware_concurrency(), &paths, &chunks, &lock));
	}
	return chunks;
}

std::filesystem::path FindSave00(const char* default_path) {
	if (file_exists((std::filesystem::path(default_path) / "world/.stream_info").string().c_str()))
		return default_path;
#ifdef _WIN32
	std::filesystem::path save00_path =
		std::filesystem::path(getenv("appdata")).parent_path() / "LocalLow/Nolla_Games_Noita/save00/";
#else
	std::filesystem::path save00_path =
		std::filesystem::path(getenv("HOME")) /
		"/.local/share/steam/steamapps/compatdata/881100/pfx/drive_c/users/steamuser/AppData/LocalLow/Nolla_Games_Noita/save00/";
#endif
	if (file_exists((save00_path / "world/.stream_info").string().c_str()))
		return save00_path;
	else {
		std::filesystem::path dialog_ret = locate_file_dialog(save00_path.string().c_str(),
			".stream_info\0*.stream_info\0", "Locate your save's stream info in the world folder");
		if (dialog_ret.stem().string() != std::string_view(".stream_info"))
			exit(-1);
		return dialog_ret.parent_path().parent_path();
	}
}