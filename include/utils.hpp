#pragma once

#include <filesystem>
#include <fstream>

inline auto file_get_contents(std::filesystem::path path) {
    size_t fileSz = std::filesystem::file_size(path);
    std::ifstream file{ path, std::ios_base::binary };
    std::vector<std::byte> data(fileSz);
    file.read(reinterpret_cast<char*>(data.data()), fileSz);
    return data;
}
