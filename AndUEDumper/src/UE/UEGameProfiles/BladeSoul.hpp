#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class BladeSoulProfile : public IGameProfile
{
public:
    BladeSoulProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "Blade Soul Revolution";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.netmarble.bnsmasia"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool IsUsingFNamePool() const override
    {
        return false;
    }
    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        uintptr_t guobjectarray = GetUnrealEngineELF().findSymbol("GUObjectArray");
        if (guobjectarray == 0)
        {
            LOGE("Failed to find GUObjectArray symbol.");
            return 0;
        }
        return guobjectarray;
    }

    uintptr_t GetNamesPtr() const override
    {
        uintptr_t GNamesInit = GetUnrealEngineELF().findSymbol("_ZN5FName16GetIsInitializedEv");
        if (GNamesInit == 0)
        {
            LOGE("Failed to find _ZN5FName16GetIsInitializedEv symbol.");
            return 0;
        }

        uintptr_t adrl = Arm64::Decode_ADRP_ADD(GNamesInit);
        return adrl ? (adrl + GetPtrAlignedOf(sizeof(bool))) : 0;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_20(isUsingCasePreservingName());
        return &offsets;
    }
};
