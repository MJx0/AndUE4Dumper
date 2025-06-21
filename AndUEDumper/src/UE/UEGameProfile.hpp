#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "../Utils/Logger.hpp"

#include "UEMemory.hpp"
#include "UEOffsets.hpp"

enum class PATTERN_MAP_TYPE : int8_t
{
    ANY_R,  // Search in any private readable map

    ANY_X,  // Search in any private readable & executable map

    ANY_W,  // Search in any private readable & writeable map

    BSS,  // Search in .bss maps
};

class IGameProfile
{
public:
protected:
    UEVars _UEVars;

public:
    virtual ~IGameProfile() = default;

    UEVarsInitStatus InitUEVars();
    const UEVars *GetUEVars() const { return &_UEVars; }

    virtual ElfScanner GetUnrealELF() const;

    // arch support check
    virtual bool ArchSupprted() const = 0;

    virtual std::string GetAppName() const = 0;

    virtual std::vector<std::string> GetAppIDs() const = 0;

    virtual bool isUsingCasePreservingName() const = 0;

    virtual bool IsUsingFNamePool() const = 0;

    virtual bool isUsingOutlineNumberName() const = 0;

    virtual UE_Offsets *GetOffsets() const = 0;

protected:
    virtual uintptr_t GetGUObjectArrayPtr() const = 0;

    // GNames / NamePoolData
    virtual uintptr_t GetNamesPtr() const = 0;

    virtual uint8_t *GetNameEntry(int32_t id) const;
    // can override if decryption is needed
    virtual std::string GetNameEntryString(uint8_t *entry) const;
    virtual std::string GetNameByID(int32_t id) const;

    virtual bool isEmulator() const;

    virtual uintptr_t findIdaPattern(PATTERN_MAP_TYPE map_type,
                                     const std::string &pattern, const int step,
                                     uint32_t skip_result = 0) const;
};
