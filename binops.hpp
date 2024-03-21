#pragma once
#include <cstdint>
#include <cstring>

template<typename T> T read_le(const char*&);
template<typename T> void write_le(std::ostream&, const T);
template<typename T> T read_be(const char*&);
template<typename T> void write_be(std::ostream&, const T);

template<>
inline std::uint8_t read_be(const char*& ptr) {
    return *(ptr++);
}

template<>
inline std::uint8_t read_le(const char*& ptr) {
    return *(ptr++);
}

template<>
inline std::uint32_t read_be(const char*& ptr) {
    auto it = reinterpret_cast<const std::uint8_t*>(ptr);
    std::uint32_t r =
          (std::uint32_t)it[3] << 0
        | (std::uint32_t)it[2] << 8
        | (std::uint32_t)it[1] << 16
        | (std::uint32_t)it[0] << 24;
    *(uintptr_t*)&ptr += 4;
    return r;
}

template<>
inline std::uint32_t read_le(const char*& ptr)
{
    auto it = reinterpret_cast<const std::uint8_t*>(ptr);
    std::uint32_t r =
          (std::uint32_t)it[0] << 0
        | (std::uint32_t)it[1] << 8
        | (std::uint32_t)it[2] << 16
        | (std::uint32_t)it[3] << 24;
    *(uintptr_t*)&ptr += 4;
    return r;
}

template<>
inline std::uint64_t read_be(const char*& ptr)
{
    auto it = reinterpret_cast<const std::uint8_t*>(ptr);
    std::uint64_t r =
          (std::uint64_t)it[7] << 0
        | (std::uint64_t)it[6] << 8
        | (std::uint64_t)it[5] << 16
        | (std::uint64_t)it[4] << 24
        | (std::uint64_t)it[3] << 32
        | (std::uint64_t)it[2] << 40
        | (std::uint64_t)it[1] << 48
        | (std::uint64_t)it[0] << 56;
    *(uintptr_t*)&ptr += 8;
    return r;
}

template<>
inline std::uint64_t read_le(const char*& ptr)
{
    auto it = reinterpret_cast<const std::uint8_t*>(ptr);
    std::uint64_t r =
          (std::uint64_t)it[0] << 0
        | (std::uint64_t)it[1] << 8
        | (std::uint64_t)it[2] << 16
        | (std::uint64_t)it[3] << 24
        | (std::uint64_t)it[4] << 32
        | (std::uint64_t)it[5] << 40
        | (std::uint64_t)it[6] << 48
        | (std::uint64_t)it[7] << 56;
    *(uintptr_t*)&ptr += 8;
    return r;
}

template<>
inline float read_be(const char*& ptr)
{
    float value;
    auto data = read_be<std::uint32_t>(ptr);
    std::memcpy(&value, &data, sizeof(data));
    return value;
}

template<>
inline double read_be(const char*& ptr)
{
    double value;
    auto data = read_be<std::uint64_t>(ptr);
    std::memcpy(&value, &data, sizeof(data));
    return value;
}

template<>
inline bool read_be(const char*& ptr)
{
    bool b = ((const char*)ptr)[0] != 0;
    *(uintptr_t*)&ptr += 1;
    return b;
}

template<>
inline std::string read_be(const char*& ptr) {
    std::uint32_t size = read_be<std::uint32_t>(ptr);
    std::string s = std::string(ptr, size);
    *(uintptr_t*)&ptr += size;
    return s;
}

template<>
inline void write_le<std::uint32_t>(std::ostream& s, uint32_t val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++) s << it2[i];
}

template<>
inline void write_le<std::uint64_t>(std::ostream& s, uint64_t val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++) s << it2[i];
}

template<>
inline void write_be<std::uint32_t>(std::ostream& s, uint32_t val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++) s << it2[3 - i];
}
template<>
inline void write_be<std::uint64_t>(std::ostream& s, uint64_t val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++) s << it2[7 - i];
}
template<>
inline void write_be<double>(std::ostream& s, double val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++) s << it2[7 - i];
}
template<>
inline void write_be<float>(std::ostream& s, float val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++) s << it2[3 - i];
}
template<>
inline void write_be<bool>(std::ostream& s, bool val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    s << *it2;
}