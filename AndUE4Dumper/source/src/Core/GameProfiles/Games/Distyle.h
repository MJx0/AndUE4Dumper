#pragma once

#include "../GameProfile.h"

// Distyle
// UE 4.23 / UE 4.24

class DistyleProfile : public IGameProfile
{
public:
    DistyleProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetBaseInfo().ehdr.e_machine;
        // only arm64
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Distyle";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.lilithgames.xgame.gp" };
    }

    std::vector<ProcMap> virtual GetMaps() const override
    {
        static std::vector<ProcMap> ue4Maps = IGameProfile::GetMaps();
        static bool once = false;

        // filter out ue4 lib from split apk by checking elf load size > 80MB
        if (!once && ue4Maps.empty())
        {
            LOGI("Scanning for UE4 lib in split apk...");
            // just an estimated size to filter out the ue4 lib from base.apk maps
            const size_t libUE4EstimatedSize = 80000000;
#ifdef _EXECUTABLE
            auto allMaps = KittyMemory::getAllMapsEx();
#else
            auto allMaps = KittyMemory::getAllMaps();
#endif
            for (auto &it : allMaps)
            {
                if (!ioutils::file_path_contains(it.pathname, PMemory::get_target_pkg()))
                    continue;
                if (!ioutils::file_path_contains(it.pathname, "/split_config") && ioutils::get_filename(it.pathname) != "base.apk")
                    continue;
                if (ioutils::get_file_extension(it.pathname) != "apk")
                    continue;

                union
                {
                    char magic[4];
                    ElfW(Ehdr) ehdr;
                } elf;
                if (!(it.readable && PMemory::vm_rpm_ptr((void *)(it.startAddress), &elf, sizeof(elf)) && VERIFY_ELF_HEADER(elf.magic)))
                    continue;

                if (elf_utils::get_elfSize(elf.ehdr, it.startAddress) >= libUE4EstimatedSize)
                {
                    ue4Maps.push_back(it);
                    break;
                }
            }

            // didn't find any
            if (!ue4Maps.size())
            {
                LOGE("Couldn't find UE4 lib within all aplit apk maps.");
                once = true;
                return ue4Maps;
            }

            // get the rest of maps
            bool processing = false;
            for (auto &it : allMaps)
            {
                auto back = ue4Maps.back();
                if ((it.inode == ue4Maps.front().inode || back.isUnknown()) && it.startAddress == back.endAddress)
                {
                    processing = true;
                    ue4Maps.push_back(it);
                    continue;
                }

                // ---p map
                if (it.isUnknown() && it.startAddress == back.endAddress && it.is_private && !it.writeable && !it.readable && !it.executable)
                {
                    processing = true;
                    ue4Maps.push_back(it);
                    continue;
                }

                if (processing)
                    break;
            }

            once = true;
        }

        return ue4Maps;
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        uintptr_t guobjectarray = findSymbol("GUObjectArray");
        if(guobjectarray == 0)
        {
            LOGE("Failed to find GUObjectArray symbol.");
            return 0;
        }
        return guobjectarray;
        //return GetBaseInfo().map.startAddress + 0x0000000;
    }

    uintptr_t GetNamesPtr() const override
    {
        // GNameBlocksDebug = &NamePoolData + Blocks offset
        uintptr_t blocks_p = findSymbol("GNameBlocksDebug");
        if (blocks_p == 0)
        {
            LOGE("Failed to find GNameBlocksDebug symbol.");
            return 0;
        }
        blocks_p = PMemory::vm_rpm_ptr<uintptr_t>((void *)blocks_p);
        return ((blocks_p == 0) ? 0 : (blocks_p - GetOffsets()->FNamePoolBlocks));

        // return GetBaseInfo().map.startAddress + 0x0000000;
    }

    UE_Offsets *GetOffsets() const override
    {
        // ===============  64bit offsets  =============== //
        struct
        {
            uint16 Stride = 2;             // alignof(FNameEntry)
            uint16 FNamePoolBlocks = 0x40; // usually ios at 0xD0 and android at 0x40
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Number = 4;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16 Name = 0;
            } FNameEntry;
            struct
            {
                uint16 Info = 0;       // Offset to Memory filled with info about type and size of string
                uint16 WideBit = 0;    // Offset to bit which shows if string uses wide characters
                uint16 LenBit = 6;     // Offset to bit which has lenght of string
                uint16 HeaderSize = 2; // Size of FNameEntry header (offset where a string begins)
            } FNameEntry23;
            struct
            {
                uint16 ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16 NumElements = 0x14;
            } TUObjectArray;
            struct
            {
                uint16 Size = 0x18;
            } FUObjectItem;
            struct
            {
                uint16 ObjectFlags = 0x8;
                uint16 InternalIndex = 0xC;
                uint16 ClassPrivate = 0x10;
                uint16 NamePrivate = 0x18;
                uint16 OuterPrivate = 0x20;
            } UObject;
            struct
            {
                uint16 Next = 0x28; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x40;
                uint16 Children = 0x48;    // UField*
                uint16 ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16 PropertiesSize = 0x50;
            } UStruct;
            struct
            {
                uint16 Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x98; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location.
            } UFunction;
            struct
            { // not needed in versions older than UE4.25
                uint16 ClassPrivate = 0;
                uint16 Next = 0;
                uint16 NamePrivate = 0;
                uint16 FlagsPrivate = 0;
            } FField;
            struct
            { // not needed in versions older than UE4.25
                uint16 ArrayDim = 0;
                uint16 ElementSize = 0;
                uint16 PropertyFlags = 0;
                uint16 Offset_Internal = 0;
                uint16 Size = 0;
            } FProperty;
            struct
            {
                uint16 ArrayDim = 0x30; // sizeof(UField)
                uint16 ElementSize = 0x34;
                uint16 PropertyFlags = 0x38;
                uint16 Offset_Internal = 0x44;
                uint16 Size = 0x70; // sizeof(FProperty)
            } UProperty;
        } static profile;
        static_assert(sizeof(profile) == sizeof(UE_Offsets));

        return (UE_Offsets *)&profile;
    }
};