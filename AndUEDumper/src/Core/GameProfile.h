#pragma once

#include <cstdint>

#include "UE_Offsets.h"

#include "../Utils/Logger.h"
#include "../Utils/memory.h"

#define INSN_PAGE_OFFSET(x) ((uintptr_t)x & ~(uintptr_t)(4096 - 1));

enum class PATTERN_MAP_TYPE : int8_t
{
    ANY_R = 0, // Search in any private readable map

    ANY_X, // Search in any private readable & executable map

    ANY_W, // Search in any private readable & writeable map

    BSS, // Search in .bss maps
};

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual ElfScanner GetUnrealEngineELF() const;

    // arch support check
    virtual bool ArchSupprted() const = 0;

    virtual std::string GetAppName() const = 0;

    virtual std::vector<std::string> GetAppIDs() const = 0;

    // ue4 4.23+
    virtual bool IsUsingFNamePool() const = 0;

    virtual bool isUsingOutlineNumberName() const = 0;

    // absolute address of GUObjectArray
    virtual uintptr_t GetGUObjectArrayPtr() const = 0;

    // absolute address of GNames / NamePoolData
    virtual uintptr_t GetNamesPtr() const = 0;

    // UE Offsets
    virtual UE_Offsets *GetOffsets() const = 0;

    virtual std::string GetNameByID(int32_t id) const;

protected:
    virtual uint8_t *GetNameEntry_Internal(int32_t id) const;
    virtual std::string GetNameEntryString_Internal(uint8_t *entry) const;
    virtual std::string GetNameByID_Internal(int32_t id) const;

    virtual bool isEmulator() const;

    virtual uintptr_t findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result = 0) const;
};