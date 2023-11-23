#pragma once

#include <cstdint>

#include <types.h>

#include <Logger.h>

#include "../Offsets.h"

#include "../memory.h"

#define INSN_PAGE_OFFSET(x) ((uintptr_t)x & ~(uintptr_t)(4096 - 1));

enum PATTERN_MAP_TYPE : int8_t
{
    ANY_R = 0, // Search in any private readable map

    ANY_X,     // Search in any private readable & executable map

    ANY_W,     // Search in any private readable & writeable map

    BSS,       // Search in .bss maps
};

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual ElfScanner GetUE4ELF() const;

    // arch support check
    virtual bool ArchSupprted() const = 0;

    virtual std::string GetAppName() const = 0;

    virtual std::vector<std::string> GetAppIDs() const = 0;

    // ue4 4.23+
    virtual bool IsUsingFNamePool() const = 0;

    // absolute address of GUObjectArray
    virtual uintptr_t GetGUObjectArrayPtr() const = 0;

    // absolute address of GNames / NamePoolData
    virtual uintptr_t GetNamesPtr() const = 0;

    // ue4 offsets
    virtual UE_Offsets *GetOffsets() const = 0;

protected:
    bool isEmulator() const;

    uintptr_t findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result = 0) const;
};