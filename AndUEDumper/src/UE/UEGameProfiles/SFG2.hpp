#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class SFG2Profile : public IGameProfile
{
public:
    SFG2Profile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        return e_machine == EM_ARM || e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Special Forces Group 2";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.ForgeGames.SpecialForcesGroup2"};
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
        uintptr_t GFNameTableForDebuggerVisualizers_MT = GetUnrealEngineELF().findSymbol("GFNameTableForDebuggerVisualizers_MT");
        if (GFNameTableForDebuggerVisualizers_MT == 0)
        {
            LOGE("Failed to find GFNameTableForDebuggerVisualizers_MT symbol.");
            return 0;
        }

        return GFNameTableForDebuggerVisualizers_MT;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_22(isUsingCasePreservingName());
        return &offsets;
    }
};
