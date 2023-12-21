#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

#include <fastlz.h>

#include "binops.hpp"

std::string read_file(const char* path)
{
    std::string out;
    std::ifstream stream(path, std::ios::binary);
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

    if (compressed.size() < 8)
    {
        printf("Error opening file %s:\n    Missing file header.\n", path);
        return std::string("");
    }

    auto compressed_size = read_le<std::uint32_t>(compressed.data());
    auto decompressed_size = read_le<std::uint32_t>(compressed.data() + 4);

    if (compressed.size() - 8 != compressed_size)
    {
        printf("Error opening file %s:\n    Bad compressed size: file was %zi bytes, expected %i.\n", path, compressed.size() - 8, compressed_size);
        return std::string("");
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
        return std::string("");
    }

    if (actual_size != output_buffer.size())
    {
        printf("Error opening file %s:\n    Unexpected decompressed size: file was %zi bytes, expected %i.\n", path, output_buffer.size(), actual_size);
        return std::string("");
    }

    return output_buffer;
}
