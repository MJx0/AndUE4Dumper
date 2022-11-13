#include "memory.h"

#include <fcntl.h>
#include <dirent.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

#include <Logger.h>

#include "ioutils.h"

// ======================== MEMORY FUNCTIONS ======================== //
namespace PMemory
{
    pid_t target_pid = 0;
    std::string target_pkg;
    std::string target_output_dir;

    void Initialize(pid_t pid, const std::string& pkg, const std::string& out_dir)
    {
        target_pid = pid;
        target_pkg = pkg;
        target_output_dir = out_dir;
    }

    pid_t get_target_pid() { return target_pid; }
    std::string get_target_pkg() { return target_pkg; }
    std::string get_target_outdir() { return target_output_dir; }

    bool vm_rpm_ptr(void *address, void *result, size_t len)
    {
        if (address == nullptr)
            return false;

        struct iovec local[1];
        local[0].iov_base = result;
        local[0].iov_len = len;

        struct iovec remote[1];
        remote[0].iov_base = address;
        remote[0].iov_len = len;

        errno = 0;
        bool success = process_vm_readv_(PMemory::target_pid, local, 1, remote, 1, 0) == len;
        if (!success)
        {
            int err = errno;
            switch (err)
            {
            case 0:
                return true;
            case EPERM:
                LOGE("Can't access the address space of process ID (%d).", PMemory::target_pid);
                exit(1);
                break;
            case ESRCH:
                LOGE("No process with ID (%d) exists.", PMemory::target_pid);
                exit(1);
                break;
            default:
                LOGD("Warning, reading address (%p) with len (%zu), error=%d | %s.",  address, len, err, strerror(err));
                break;
            }
        }
        return success;
    }

    std::string vm_rpm_str(void *address, int max_len)
    {
        std::vector<char> chars(max_len);
        if (!vm_rpm_ptr(address, chars.data(), max_len))
            return "";

        std::string str = "";
        for (size_t i = 0; i < chars.size(); i++)
        {
            if (chars[i] == '\0')
                break;
            str.push_back(chars[i]);
        }

        chars.clear();
        chars.shrink_to_fit();

        if ((int)str[0] == 0 && str.size() == 1)
            return "";

        return str;
    }

    void dumpMaps()
    {
#if defined(_EXECUTABLE)
        char srcFile[64] = {0};
        sprintf(srcFile, "/proc/%d/maps", target_pid);
#else
        char srcFile[] = "/proc/self/maps";
#endif

        char dstFile[256] = {0};
        sprintf(dstFile, "%s/maps.txt", target_output_dir.c_str());

        ioutils::copy_file(srcFile, dstFile);
    }

    void dumpMemory(size_t fromAddress, size_t toAddress)
    {
        if (fromAddress >= toAddress)
        {
            LOGE("DumpMemory: from(%p) is equal or greater than to(%p).", (void *)fromAddress, (void *)toAddress);
            return;
        }

        size_t displaySize = (toAddress - fromAddress);
        static const char *units[] = {"B", "KB", "MB", "GB"};
        int u;
        for (u = 0; displaySize > 1024; u++)
        {
            displaySize /= 1024;
        }

        size_t dumpSize = (toAddress - fromAddress);
        void *dmmap = mmap(NULL, dumpSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (dmmap == nullptr)
        {
            LOGE("DumpMemory: failed to allocate memory for dump of size %zu%s.", dumpSize, units[u]);
            return;
        }

        LOGI("Dumping memory: [ %p - %p | Size: %zu%s ] ...", (void *)fromAddress, (void *)toAddress, displaySize, units[u]);

        char dstFile[255] = {0};
        sprintf(dstFile, "%s/%p-%p_dump.bin", target_output_dir.c_str(), (void *)fromAddress, (void *)toAddress);

#if defined(_EXECUTABLE)
        char srcFile[64] = {0};
        sprintf(srcFile, "/proc/%d/mem", target_pid);
#else
        char srcFile[] = "/proc/self/mem";
#endif

        int sfd = open(srcFile, O_RDONLY);

        errno = 0;
        size_t read_sz = ioutils::pread_fully(sfd, dmmap, fromAddress, dumpSize);
        if (read_sz != dumpSize)
            LOGW("Dumping memory: warning, dump size %zu but bytes read %zu. error=%d | %s.", dumpSize, read_sz, errno, strerror(errno));

        close(sfd);

        int dfd = open(dstFile, O_CREAT | O_WRONLY, 0644);

        errno = 0;
        size_t write_sz = ioutils::pwrite_fully(dfd, dmmap, 0 ,dumpSize);
        if (write_sz != dumpSize)
            LOGW("Dumping memory: warning, dump size %zu but bytes write %zu. error=%d | %s.", dumpSize, write_sz, errno, strerror(errno));

        close(dfd);

        LOGI("Dumped memory at %s", dstFile);

        munmap(dmmap, dumpSize);
    }
}

// ======================== elf utils ================================ //

namespace elf_utils
{
    // elf load size
    size_t get_elfSize(ElfW(Ehdr) ehdr, uintptr_t base)
    {
        ElfW(Word) loadSize = 0;

        // read all program headers
        std::vector<char> phdr_buff(ehdr.e_phnum * ehdr.e_phentsize);
        PMemory::vm_rpm_ptr((void *)(base + ehdr.e_phoff), phdr_buff.data(), ehdr.e_phnum * ehdr.e_phentsize);

        for (ElfW(Half) i = 0; i < ehdr.e_phnum; i++)
        {
            ElfW(Phdr) phdr = {};
            memcpy(&phdr, phdr_buff.data() + (i * ehdr.e_phentsize), ehdr.e_phentsize);
            if (phdr.p_type == PT_LOAD)
            {
                loadSize = phdr.p_vaddr + phdr.p_memsz;
                break;
            }
        }
        return loadSize;
    }
}

// ======================== PROCESS FUNCTIONS ======================== //

std::string getCurrentProcName()
{
    std::string rt;
    char cmdline[128] = {0};
    FILE *fp = fopen("/proc/self/cmdline", "r");
    if (fp)
    {
        fgets(cmdline, sizeof(cmdline), fp);
        fclose(fp);
        rt = cmdline;
    }
    else
    {
        LOGE("Could't open file {\"/proc/self/cmdline\"}");
    }
    return rt;
}

#if defined(_EXECUTABLE)

#include <KittyMemory/KittyMemory.h>

namespace KittyMemory
{
    std::vector<ProcMap> getAllMapsEx()
    {
        FILE *fp;
        char mapFile[32] = {0}, line[512] = {0};
        std::vector<ProcMap> retMaps;

        sprintf(mapFile, "/proc/%d/maps", PMemory::target_pid);
        fp = fopen(mapFile, "r");
        if (fp)
        {
            while (fgets(line, sizeof(line), fp))
            {
                ProcMap map;

                char perms[5] = {0}, dev[11] = {0}, pathname[256] = {0};

                sscanf(line, "%llx-%llx %s %llx %s %lu %s",
                       &map.startAddress, &map.endAddress,
                       perms, &map.offset, dev, &map.inode, pathname);

                map.length = map.endAddress - map.startAddress;
                map.dev = dev;
                map.pathname = pathname;

                if (perms[0] == 'r')
                {
                    map.protection |= PROT_READ;
                    map.readable = true;
                }
                if (perms[1] == 'w')
                {
                    map.protection |= PROT_WRITE;
                    map.writeable = true;
                }
                if (perms[2] == 'x')
                {
                    map.protection |= PROT_EXEC;
                    map.executable = true;
                }

                map.is_private = (perms[3] == 'p');
                map.is_shared = (perms[3] == 's');

                map.is_rx = (strncmp(perms, "r-x", 3) == 0);
                map.is_rw = (strncmp(perms, "rw-", 3) == 0);
                map.is_ro = (strncmp(perms, "r--", 3) == 0);

                retMaps.push_back(map);
            }
            fclose(fp);
        }
        else
        {
            LOGE("Could't open file {%s}", mapFile);
        }

        if (retMaps.empty())
        {
            LOGE("getMaps err couldn't find any map");
        }
        return retMaps;
    }

    std::vector<ProcMap> getMapsByNameEx(const std::string &name)
    {
        std::vector<ProcMap> retMaps;

        auto allMaps = KittyMemory::getAllMapsEx();
        for (auto &it : allMaps)
        {
            if (!it.pathname.empty() && strstr(it.pathname.c_str(), name.c_str()))
            {
                retMaps.push_back(it);
            }
        }

        if (retMaps.empty())
        {
            LOGE("getMapsByNameEx err couldn't find any map with name (%s)", name.c_str());
        }

        return retMaps;
    }

}

pid_t findProcID(const std::string &procName)
{
    pid_t pid = -1;
    int id;
    char cmdlineFile[32] = {0}, cmdline[128] = {0};
    FILE *fp;
    dirent *entry;

    DIR *dir = opendir("/proc/");
    if (!dir)
    {
        LOGE("Couldn't open /proc directory...");
        return pid;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        id = atoi(entry->d_name);
        if (id != 0)
        {
            sprintf(cmdlineFile, "/proc/%d/cmdline", id);
            fp = fopen(cmdlineFile, "r");
            if (fp)
            {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);

                if (strcmp(procName.c_str(), cmdline) == 0)
                {
                    pid = id;
                    break;
                }
            }
            else
            {
                LOGE("Could't open file {%s}", cmdlineFile);
            }
        }
    }
    closedir(dir);
    return pid;
}

std::string findProcName(pid_t pid)
{
    std::string rt;
    char cmdlineFile[32] = {0}, cmdline[128] = {0};
    FILE *fp;
    sprintf(cmdlineFile, "/proc/%d/cmdline", pid);
    fp = fopen(cmdlineFile, "r");
    if (fp)
    {
        fgets(cmdline, sizeof(cmdline), fp);
        fclose(fp);
        rt = cmdline;
    }
    else
    {
        LOGE("Could't open file {%s}", cmdlineFile);
    }
    return rt;
}

#endif