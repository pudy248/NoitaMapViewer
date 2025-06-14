#pragma once
#include <filesystem>
#include <string_view>
#include <string>

#ifdef WIN32
#include "Windows.h"
// evil windows is evil
#undef min
#undef max
#endif

// Portable solution probably doesn't exist, sucks to suck
std::string locate_file_dialog(const char* hint, const char* filter, const char* title) {
#ifdef WIN32
	char buf[2048] = {};
	OPENFILENAMEA fn = {};
	fn.lStructSize = sizeof(OPENFILENAMEA);
	fn.lpstrFilter = (LPSTR)filter;
	fn.lpstrFile = (LPSTR)buf;
	fn.nMaxFile = 2048;
	fn.lpstrTitle = (LPSTR)title;
	fn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	GetOpenFileNameA(&fn);
	return std::string(buf);
#else
	// yep, it's terrible. Linux people, make this better if possible maybe
	std::string s;
	printf("%s\n", title);
	s << std::cin;
	return s;
#endif
}

template <typename F> void for_each_file(std::filesystem::path directory, const char* ext, bool recursive, F what) {
	/*
	char buffer[2048];
	WCHAR wSearchName[2048];

	strcpy(buffer, directory);
	strcat(buffer, filter);

	MultiByteToWideChar(CP_UTF8, 0, buffer, 2048, wSearchName, 2048);
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW(wSearchName, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			char buffer2[2048];
			WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, 2048, buffer2, 2048, NULL, NULL);
			strcpy(buffer, directory);
			strcat(buffer, buffer2);
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				what(buffer, buffer2);
			else if(recursive)
				for_each_file(buffer, filter, true, what);
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
	else {
		printf("ERR\n");
		abort();
	}*/
	try {
	for (auto p : std::filesystem::directory_iterator(directory)) {
		if (p.is_directory()) {
			if (recursive)
				for_each_file(p, ext, recursive, what);
		}
		else if (p.path().extension().string() == std::string_view(ext))
			what(p.path());
		
	}
	} catch (std::filesystem::filesystem_error e) {
		printf("%s\n", e.what());
		exit(-1);
	}
}