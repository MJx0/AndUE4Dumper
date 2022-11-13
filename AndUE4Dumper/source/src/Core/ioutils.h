
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

    size_t copy_file(const std::string &src, const std::string &dst);

    size_t pread_fully(int fd, void *buffer, int64_t offset, size_t numBytes);

    size_t pwrite_fully(int fd, void *buffer, int64_t offset, size_t numBytes);
}