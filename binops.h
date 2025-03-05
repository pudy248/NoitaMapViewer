#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>

template<typename T> T read_le(std::istream&);
template<typename T> void write_le(std::ostream&, const T&);
template<typename T> T read_be(std::istream&);
template<typename T> void write_be(std::ostream&, const T&);

template<>
inline std::uint8_t read_be(std::istream& s) {
    uint8_t val;
    s.read((char*)&val, sizeof(val));
    return val;
}
template<>
inline std::uint8_t read_le(std::istream& s) {
    uint8_t val;
    s.read((char*)&val, sizeof(val));
    return val;
}

template<>
inline std::uint32_t read_be(std::istream& s) {
    uint32_t val;
    auto it = (uint8_t*)&val;
    for (int i = 0; i < 4; i++)
        it[3 - i] = read_le<uint8_t>(s);
    return val;
}
template<>
inline std::uint32_t read_le(std::istream& s)
{
    uint32_t val;
    auto it = (uint8_t*)&val;
    for (int i = 0; i < 4; i++)
        it[i] = read_le<uint8_t>(s);
    return val;
}

template<>
inline std::uint64_t read_be(std::istream& s)
{
    uint64_t val;
    auto it = (uint8_t*)&val;
    for (int i = 0; i < 8; i++)
        it[7 - i] = read_le<uint8_t>(s);
    return val;
}
template<>
inline std::uint64_t read_le(std::istream& s)
{
    uint64_t val;
    auto it = (uint8_t*)&val;
    for (int i = 0; i < 8; i++)
        it[i] = read_le<uint8_t>(s);
    return val;
}

template<>
inline float read_be(std::istream& s)
{
    float value;
    auto data = read_be<std::uint32_t>(s);
    std::memcpy(&value, &data, sizeof(data));
    return value;
}
template<>
inline double read_be(std::istream& s)
{
    double value;
    auto data = read_be<std::uint64_t>(s);
    std::memcpy(&value, &data, sizeof(data));
    return value;
}
template<>
inline bool read_be(std::istream& s)
{
    return read_be<uint8_t>(s);
}

template<>
inline std::string read_be(std::istream& s) {
    std::uint32_t size = read_be<std::uint32_t>(s);
    std::string str;
    str.resize(size);
    s.read(str.data(), size);
    return str;
}
template<>
inline std::string read_le(std::istream& s) {
    std::uint32_t size = read_le<std::uint32_t>(s);
    std::string str;
    str.resize(size);
    s.read(str.data(), size);
    return str;
}

template <typename T> static std::vector<T> read_vec_be(std::istream& s) {
    auto count = read_be<std::uint32_t>(s);
    std::vector<T> result(count);

    for (auto& out : result) {
        out = read_be<T>(s);
    }

    return result;
}
template <typename T> static std::vector<T> read_vec_le(std::istream& s) {
    auto count = read_le<std::uint32_t>(s);
    std::vector<T> result(count);

    for (auto& out : result) {
        out = read_le<T>(s);
    }

    return result;
}
template <typename T> static std::vector<T*> read_vec_ptrs_be(std::istream& s) {
    auto count = read_be<std::uint32_t>(s);
    std::vector<T*> result(count);

    for (auto& out : result) {
        out = new T(read_be<T>(s));
    }

    return result;
}
template <typename T> static std::vector<T*> read_vec_ptrs_le(std::istream& s) {
    auto count = read_le<std::uint32_t>(s);
    std::vector<T*> result(count);

    for (auto& out : result) {
        out = new T(read_le<T>(s));
    }

    return result;
}


template<>
inline void write_le<uint8_t>(std::ostream& s, const uint8_t& val) {
    s.write((char*)&val, 1);
}
template<>
void write_be<std::uint32_t>(std::ostream& s, const uint32_t& val) {
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++)
        write_le(s, it2[3 - i]);
}
template<>
void write_le<std::uint32_t>(std::ostream& s, const uint32_t& val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++)
        write_le(s, it2[i]);
}

template<>
void write_be<std::uint64_t>(std::ostream& s, const uint64_t& val) {
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++)
        write_le(s, it2[7 - i]);
}
template<>
inline void write_le<std::uint64_t>(std::ostream& s, const uint64_t& val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++)
        write_le(s, it2[i]);
}

template<>
inline void write_be<double>(std::ostream& s, const double& val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 8; i++)
        write_le(s, it2[7 - i]);
}
template<>
inline void write_be<float>(std::ostream& s, const float& val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    for (int i = 0; i < 4; i++)
        write_le(s, it2[3 - i]);
}
template<>
inline void write_be<bool>(std::ostream& s, const bool& val)
{
    auto it2 = reinterpret_cast<const std::uint8_t*>(&val);
    write_le(s, *it2);
}

template<>
inline void write_be<std::string>(std::ostream& s, const std::string& val) {
    write_be<std::uint32_t>(s, val.size());
    s.write(val.data(), val.size());
}
template<>
inline void write_le<std::string>(std::ostream& s, const std::string& val) {
    write_le<std::uint32_t>(s, val.size());
    s.write(val.data(), val.size());
}

template <typename T> static void write_vec_be(std::ostream& s, const std::vector<T>& vec) {
    write_be<std::uint32_t>(s, vec.size());
    for (const auto& elem : vec) {
        write_be<T>(s, elem);
    }
}
template <typename T> static void write_vec_le(std::ostream& s, const std::vector<T>& vec) {
    write_le<std::uint32_t>(s, vec.size());
    for (const auto& elem : vec) {
        write_le<T>(s, elem);
    }
}
template <typename T> static void write_vec_ptrs_be(std::ostream& s, const std::vector<T*>& vec) {
    write_be<std::uint32_t>(s, vec.size());
    for (const auto& elem : vec) {
        write_be<T>(s, *elem);
    }
}
template <typename T> static void write_vec_ptrs_le(std::ostream& s, const std::vector<T*>& vec) {
    write_le<std::uint32_t>(s, vec.size());
    for (const auto& elem : vec) {
        write_le<T>(s, *elem);
    }
}
