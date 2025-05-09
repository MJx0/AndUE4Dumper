#include "ioutils.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
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

int mkdir_recursive(const std::string &dirPath, mode_t mode)
{
    std::string tmp = dirPath;
    for (char *p = strchr(tmp.data() + 1, '/'); p; p = strchr(p + 1, '/'))
    {
        *p = '\0';
        errno = 0;
        if (mkdir(tmp.c_str(), mode) == -1)
        {
            if (errno != EEXIST)
            {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    errno = 0;
    return mkdir(tmp.c_str(), mode);
}
} // namespace ioutils