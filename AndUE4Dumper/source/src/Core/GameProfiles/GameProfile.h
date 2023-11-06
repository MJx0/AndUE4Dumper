#pragma once

#include <cstdint>

#include <types.h>

#include <Logger.h>

#include "../Offsets.h"

#include "../memory.h"

#include "../KittyArm64.hpp"

#define INSN_PAGE_OFFSET(x) ((uintptr_t)x & ~(uintptr_t)(4096 - 1));

enum class PATTERN_MAP_TYPE : uint8_t
{
    MAP_BASE = 0,
    MAP_RXP,
    MAP_RWP,
    MAP_BSS
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
    // returned address will be absolute not relative.
    uintptr_t findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result = 0) const;
};