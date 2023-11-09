
#pragma once

#include <string>
#include <cstdint>

namespace ioutils
{
    std::string get_filename(const std::string &filePath);
    
    std::string get_file_directory(const std::string &filePath);

    std::string get_file_extension(const std::string &filePath);

    bool file_path_contains(const std::string &filePath, const std::string &subPath);
    
    std::string remove_specials(std::string s);

    std::string replace_specials(std::string s, char c);

    void delete_directory(const std::string &directory);

    int path_is_directory(const std::string &path);

    int mkdir_recursive(const std::string &dirPath, mode_t mode);
}