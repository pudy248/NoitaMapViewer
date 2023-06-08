#include <cstdint>
#include <fstream>
#include <stdexcept>

#include <fastlz.h>

#include "binops.hpp"

#include "utils.hpp"

std::string read_file(const char* path)
{
    std::string out;
    std::ifstream stream(path, std::ios::binary);
    while (stream) {
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
        throw std::runtime_error{"No compression file header"};

    auto compressed_size = read_le<std::uint32_t>(compressed.data());
    auto decompressed_size = read_le<std::uint32_t>(compressed.data() + 4);

    if (compressed.size() - 8 != compressed_size)
        throw std::runtime_error{"Bad compressed size"};

    std::string output_buffer(decompressed_size, '\0');
    auto actual_size = fastlz_decompress(
            compressed.data() + 8,
            compressed_size,
            output_buffer.data(),
            output_buffer.size());

    if (actual_size == 0)
        throw std::runtime_error{"Couldn't decompress file.\n"};

    if (actual_size != output_buffer.size())
        throw std::runtime_error{"Unexpected decompressed size"};

    return output_buffer;
}
