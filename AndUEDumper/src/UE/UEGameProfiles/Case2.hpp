#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class Case2Profile : public IGameProfile
{
public:
    Case2Profile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealELF().header().e_machine;
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "Case 2 Animatronics";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.WallnutLLC.Case2"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        uintptr_t guobjectarray = GetUnrealELF().findSymbol("GUObjectArray");
        if (guobjectarray != 0)
            return guobjectarray;

        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"91 E1 03 ? AA E0 03 08 AA E2 03 1F 2A", -7},
            {"B4 21 0C 40 B9 ? ? ? ? ? ? ? 91", 5},
            {"9F E5 00 ? 00 E3 FF ? 40 E3 ? ? A0 E1", -2},
            {"96 df 02 17 ? ? ? ? 54 ? ? ? ? ? ? ? 91 e1 03 13 aa", 9},
            {"f4 03 01 2a ? 00 00 34 ? ? ? ? ? ? ? ? ? ? 00 54 ? 00 00 14 ? ? ? ? ? ? ? 91", 0x18},
            {"69 3e 40 b9 1f 01 09 6b ? ? ? 54 e1 03 13 aa ? ? ? ? f4 4f ? a9 ? ? ? ? ? ? ? 91", 0x18},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            uintptr_t adrl = Arm64::Decode_ADRP_ADD(findIdaPattern(map_type, ida_pattern, step));
            if (adrl != 0) return adrl;
        }

        return 0;
    }

    uintptr_t GetNamesPtr() const override
    {
        // GNameBlocksDebug = &NamePoolData + Blocks offset
        uintptr_t blocks_p = GetUnrealELF().findSymbol("GNameBlocksDebug");
        if (blocks_p != 0)
        {
            blocks_p = vm_rpm_ptr<uintptr_t>((void *)blocks_p);
            if (blocks_p != 0)
                return (blocks_p - GetOffsets()->FNamePool.BlocksOff);
        }

        std::vector<std::pair<std::string, int>> idaPatterns = {
            // FNameEntry const* FName::GetEntry(FNameEntryId id);
            {"F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ? ? ? ? ? ? ? ? A8 02 ? 39", 0x18},
            {"F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 ? ? ? ? A8 02 ? 39", 0x24},

            // DebugDump
            {"fd 7b 01 a9 fd 43 00 91 ? ? ? ? 89 ? ? 39 f3 03 08 aa c9 00 00 37 ? ? ? ? ? ? ? 91", 0x18},

            // GetPlainName ToString AppendString GetStringLength
            {"02 ? 91 C8 00 00 37 ? ? ? ? ? ? ? 91", 7},

            {"39 C8 00 00 37 ? ? ? ? ? ? ? 91 ? ? ? 97 ? 00 80 52 ? ? ? 39", 5},
            {"C8 00 00 37 ? ? ? ? ? ? ? 91 ? ? ? 97 ? 00 80 52", 4},
            {"C8 00 00 37 ? ? ? ? ? ? ? 91 ? ? ? 97", 4},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            uintptr_t adrl = Arm64::Decode_ADRP_ADD(findIdaPattern(map_type, ida_pattern, step));
            if (adrl != 0) return adrl;
        }

        return 0;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());
        return &offsets;
    }
};
