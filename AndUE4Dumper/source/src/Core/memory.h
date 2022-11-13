#pragma once

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <string>

#include <types.h>

#include <Elf/elf.h>

#include <Logger.h>

#define VERIFY_ELF_HEADER(data) (!memcmp(data, "\177ELF", 4))
#define IS_ELF32(data) (*(char*)(data+4) == 0)
#define IS_ELF64(data) (*(char*)(data+4) == 1)

// process_vm_readv, process_vm_writev 
#if defined(__aarch64__)
#define syscall_rpmv_n 270
#define syscall_wpmv_n 271
#elif defined(__arm__)
#define syscall_rpmv_n 376
#define syscall_wpmv_n 377
#elif defined(__i386__)
#define syscall_rpmv_n 347
#define syscall_wpmv_n 348
#endif

#define process_vm_readv_(pid, liov, liovcnt, riov, riovcnt, flags) \
    syscall(syscall_rpmv_n, pid, liov, liovcnt, riov, riovcnt, flags)

#define process_vm_writev_(pid, liov, liovcnt, riov, riovcnt, flags) \
    syscall(syscall_wpmv_n, pid, liov, liovcnt, riov, riovcnt, flags)

namespace PMemory
{
    void Initialize(pid_t pid, const std::string& pkg, const std::string& out_dir);

    pid_t get_target_pid();
    std::string get_target_pkg();
    std::string get_target_outdir();

    bool vm_rpm_ptr(void *address, void *result, size_t len);

    template <typename T>
    T vm_rpm_ptr(void *address)
    {
        T buffer{};
        vm_rpm_ptr(address, &buffer, sizeof(T));
        return buffer;
    }

    std::string vm_rpm_str(void *address, int max_len = 0xff);

    void dumpMaps();
    void dumpMemory(size_t fromAddress, size_t toAddress);
}

namespace elf_utils
{
    // elf load size
    size_t get_elfSize(ElfW(Ehdr) ehdr, uintptr_t base);
}

std::string getCurrentProcName();

#if defined(_EXECUTABLE)

namespace KittyMemory
{
    std::vector<class ProcMap>getAllMapsEx();
    std::vector<class ProcMap> getMapsByNameEx(const std::string &name);
}

pid_t findProcID(const std::string& procName);
std::string findProcName(pid_t pid);

#endif