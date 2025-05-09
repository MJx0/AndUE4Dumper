#pragma once

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <string>

#include <KittyMemoryMgr.hpp>

extern KittyMemoryMgr kMgr;

bool vm_rpm_ptr(void *address, void *result, size_t len);

template <typename T>
T vm_rpm_ptr(void *address)
{
    T buffer{};
    vm_rpm_ptr(address, &buffer, sizeof(T));
    return buffer;
}

std::string vm_rpm_str(void *address, size_t max_len = 0xff);
std::wstring vm_rpm_strw(void *address, size_t max_len = 0xff);
