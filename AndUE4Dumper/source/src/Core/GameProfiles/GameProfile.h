#pragma once

#include <cstdint>

#include <types.h>

#include <Elf/elf.h>

#include <Logger.h>

#include <KittyMemory/KittyMemory.h>
#include <KittyMemory/KittyScanner.h>
#include <KittyMemory/KittyArm64.h>
#include <KittyMemory/KittyUtils.h>

#include "../Offsets.h"
#include "../memory.h"
#include "../ioutils.h"

using KittyMemory::ProcMap;

#ifdef _EXECUTABLE
class ProcMapEx
{
public:
    uintptr_t local_ptr;
    ProcMap remote_map;
};
using MapInfoArray = std::vector<ProcMapEx>;
#else
using MapInfoArray = std::vector<ProcMap>;
#endif

#define INSN_PAGE_OFFSET(x) ((uintptr_t)x & ~(uintptr_t)(4096 - 1));

class BaseInfo
{
public:
    union
    {
        char magic[4];
        ElfW(Ehdr) ehdr;
    };
    ProcMap map;

    BaseInfo()
    {
        memset(&ehdr, 0, sizeof(ehdr));
        map = {};
    }
};

enum class PATTERN_MAP_TYPE : uint8_t
{
    MAP_RXP = 0,
    MAP_RWP = 1,
    MAP_BSS = 2
};

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual std::vector<ProcMap> GetMaps() const
    {
#ifdef _EXECUTABLE
        static std::vector<ProcMap> ue4Maps = KittyMemory::getMapsByNameEx("libUE4.so");
#else
        static std::vector<ProcMap> ue4Maps = KittyMemory::getMapsByName("libUE4.so");
#endif
        return ue4Maps;
    }

    // ue4 base
    virtual BaseInfo GetBaseInfo() const;

    // start - end of ue4 library
    virtual std::pair<uintptr_t, uintptr_t> GetRange() const;

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

    // memory dump ue4 library
    void DumpLibrary() const;

protected:
    // returned address will be absolute not relative.
    uintptr_t findSymbol(const std::string &symbol) const;
    // returned address will be absolute not relative.
    uintptr_t findPattern(PATTERN_MAP_TYPE map_type, const std::string &hex, const std::string &mask, const int step, uint32_t skip_result = 0) const;

private:
    // r-xp game maps locally mapped
    MapInfoArray GetMappedRXPs() const;
    // rw-p game maps locally mapped
    MapInfoArray GetMappedRWPs() const;
    // bss game locally mapped
    MapInfoArray GetMappedBSSs() const;
};