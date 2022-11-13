#include "ioutils.h"

#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ioutils
{
    std::string get_filename(const std::string &filePath)
    {
        std::string filename;
        const size_t last_slash_idx = filePath.find_last_of("/\\");
        if (std::string::npos != last_slash_idx)
        {
            filename = filePath.substr(last_slash_idx + 1);
        }
        return filename;
    }

    std::string get_file_directory(const std::string &filePath)
    {
        std::string directory;
        const size_t last_slash_idx = filePath.find_last_of("/\\");
        if (std::string::npos != last_slash_idx)
        {
            directory = filePath.substr(0, last_slash_idx);
        }
        return directory;
    }

    std::string get_file_extension(const std::string &filePath)
    {
        std::string ext;
        const size_t last_slash_idx = filePath.find_last_of(".");
        if (std::string::npos != last_slash_idx)
        {
            ext = filePath.substr(last_slash_idx + 1);
        }
        return ext;
    }

    bool file_path_contains(const std::string &filePath, const std::string &subPath)
    {
        return (!filePath.empty() && (filePath.find(subPath) != std::string::npos));
    }

    std::string remove_specials(std::string s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s.erase(s.begin() + i);
            --i;
        }
        return s;
    }

    std::string replace_specials(std::string s, char c)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s[i] = c;
        }
        return s;
    }

    void delete_directory(const std::string &directory)
    {
        DIR *dp;
        struct dirent *rd;
        char buff[512] = {0};

        dp = opendir(directory.c_str());
        if (!dp)
            return;

        while ((rd = readdir(dp)) != NULL)
        {
            if (!strcmp(rd->d_name, ".") || !strcmp(rd->d_name, ".."))
                continue;

            sprintf(buff, "%s/%s", directory.c_str(), rd->d_name);
            if (path_is_directory(buff))
                delete_directory(buff);
            else
                unlink(buff);
        }

        closedir(dp);
        rmdir(directory.c_str());
    }

    int path_is_directory(const std::string &path)
    {
        struct stat st;

        if (stat(path.c_str(), &st))
            return 0;

        return S_ISDIR(st.st_mode);
    }

    size_t copy_file(const std::string &src, const std::string &dst)
    {
        size_t size = 0;
        char buffer[4096] = {0};

        int sfd = open(src.c_str(), O_RDONLY);
        int dfd = open(dst.c_str(), O_CREAT | O_WRONLY, 0644);

        for (;;)
        {
            size = read(sfd, buffer, sizeof(buffer));
            if (size > 0)
                write(dfd, buffer, size);
            else
                break;
        }

        close(sfd);
        close(dfd);

        return size;
    }

    size_t pread_fully(int fd, void *buffer, int64_t offset, size_t numBytes)
    {
        char *ptr = (char*)buffer;
        size_t bytesRead = 0;
        do
        {
            ssize_t readSize = pread64(fd, ptr + bytesRead, numBytes - bytesRead, offset);
            if (readSize > 0)
                bytesRead += readSize;
            else
                break;
        } while (bytesRead < numBytes);
        return bytesRead;
    }

    size_t pwrite_fully(int fd, void *buffer, int64_t offset, size_t numBytes)
    {
        char *ptr = (char*)buffer;
        size_t bytesWritten = 0;
        do
        {
            ssize_t writeSize = pwrite64(fd, ptr + bytesWritten, numBytes - bytesWritten, offset);
            if (writeSize > 0)
                bytesWritten += writeSize;
            else
                break;
        } while (bytesWritten < numBytes);
        return bytesWritten;
    }
}