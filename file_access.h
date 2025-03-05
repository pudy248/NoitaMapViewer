#pragma once
#include <cstdint>
#include <string>
#include <fstream>
#include <sstream>

#include <fastlz.h>
#include "binops.h"

template <typename CharT>
bool file_exists(const CharT* path) {
    std::ifstream s(path, std::ios::binary);
    return !s.fail();
}

std::string read_file(const char* path)
{
    std::string out;
    std::ifstream stream(path, std::ios::binary);
    if (stream.fail()) {
        printf("[%s] does not exist.\n", path);
        exit(-1);
    }
    while (stream)
    {
        char buffer[1024];
        stream.read(buffer, sizeof(buffer));
        out.append(buffer, stream.gcount());
    }

    return out;
}

std::string read_compressed_file(const char* path)
{
    std::string compressed = read_file(path);
    std::ifstream stream(path, std::ios::binary);

    if (compressed.size() < 8)
    {
        printf("Error opening file %s:\n    Missing file header.\n", path);
        exit(-1);
    }

    auto compressed_size = read_le<std::uint32_t>(stream);
    auto decompressed_size = read_le<std::uint32_t>(stream);

    if (compressed.size() - 8 != compressed_size)
    {
        printf("Error opening file %s:\n    Bad compressed size: file was %zi bytes, expected %i.\n", path, compressed.size() - 8, compressed_size);
        exit(-1);
    }

    std::string output_buffer(decompressed_size, '\0');
    auto actual_size = fastlz_decompress(
        compressed.data() + 8,
        compressed_size,
        output_buffer.data(),
        output_buffer.size());

    if (actual_size == 0)
    {
        printf("Error opening file %s:\n    Failed to decompress.\n", path);
        exit(-1);
    }

    if (actual_size != output_buffer.size())
    {
        printf("Error opening file %s:\n    Unexpected decompressed size: file was %zi bytes, expected %i.\n", path, output_buffer.size(), actual_size);
        exit(-1);
    }

    return output_buffer;
}

void write_file(const char* path, const std::string& in) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(in.c_str(), in.length());
}

void write_compressed_file(const char* path, const std::string& in)
{
    size_t actual_size = in.size();
    std::string compressed_wide(in.size(), 0);
    size_t compressed_size = fastlz_compress_level(1, in.c_str(), in.size(), compressed_wide.data());
    std::string compressed(compressed_size, 0);
    std::memcpy(compressed.data(), compressed_wide.data(), compressed_size);

    std::ofstream stream(path, std::ios::binary);
    write_le<uint32_t>(stream, compressed_size);
    write_le<uint32_t>(stream, actual_size);
    stream.write(compressed.c_str(), compressed.length());
}