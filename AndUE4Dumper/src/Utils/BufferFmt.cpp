#include "BufferFmt.h"

bool BufferFmt::_writeBufferToFile(const std::string &filePath, const char *mode) const
{
    FILE *file = std::fopen(filePath.c_str(), mode);
    if (!file)
        return false;

    size_t buffer_size = _buffer.size();
    size_t chunk_size = std::min<size_t>(buffer_size, 1024 * 1024); // Cap at 1MB

    // Write buffer in chunks
    if (buffer_size > 0)
    {
        const char *data = _buffer.data();
        size_t remaining = buffer_size;
        size_t offset = 0;

        while (remaining > 0)
        {
            size_t to_write = std::min(remaining, chunk_size);
            size_t written = std::fwrite(data + offset, 1, to_write, file);
            if (written != to_write)
            {
                std::fclose(file);
                return false;
            }
            offset += to_write;
            remaining -= to_write;
        }
    }

    std::fclose(file);
    return true;
}

std::vector<std::string> BufferFmt::readLines() const
{
    std::vector<std::string> lines;
    std::string_view buffer_view(_buffer.data(), _buffer.size());
    size_t start = 0;
    for (size_t i = 0; i < buffer_view.size(); ++i)
    {
        if (buffer_view[i] == '\n')
        {
            lines.emplace_back(buffer_view.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < buffer_view.size())
    {
        lines.emplace_back(buffer_view.substr(start));
    }
    return lines;
}
