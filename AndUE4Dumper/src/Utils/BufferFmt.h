#pragma once

#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

class BufferFmt
{
private:
    fmt::memory_buffer _buffer; // memory buffer

    // Write buffer to a file
    bool _writeBufferToFile(const std::string &filePath, const char *mode) const;

public:
    BufferFmt() = default;

    // Write a formatted string to the buffer, overwriting existing content
    template <typename... Args>
    void write(fmt::format_string<Args...> format_str, Args &&...args)
    {
        _buffer.clear();
        fmt::format_to(std::back_inserter(_buffer), format_str, std::forward<Args>(args)...);
    }

    // Append a formatted string to the buffer
    template <typename... Args>
    void append(fmt::format_string<Args...> format_str, Args &&...args)
    {
        fmt::format_to(std::back_inserter(_buffer), format_str, std::forward<Args>(args)...);
    }

    // Read the entire buffer as a string (copies data)
    inline std::string read() const
    {
        return std::string(_buffer.data(), _buffer.size());
    }

    inline std::string_view readView() const
    {
        return std::string_view(_buffer.data(), _buffer.size());
    }

    // Read the buffer as lines, split by newline
    inline std::vector<std::string> readLines() const;

    // Clear the buffer
    inline void clear()
    {
        _buffer.clear();
    }

    // Check if buffer is empty
    inline bool empty() const
    {
        return _buffer.size() == 0;
    }

    // Get buffer size
    inline size_t size() const
    {
        return _buffer.size();
    }

    inline bool writeBufferToFile(const std::string &filePath) const
    {
        return _writeBufferToFile(filePath, "wb");
    }

    inline bool appendBufferToFile(const std::string &filePath) const
    {
        return _writeBufferToFile(filePath, "ab");
    }
};
