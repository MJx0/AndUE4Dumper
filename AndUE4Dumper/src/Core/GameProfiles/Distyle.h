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
        auto e_machine = GetUnrealEngineELF().header().e_machine;
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
        uintptr_t guobjectarray = GetUnrealEngineELF().findSymbol("GUObjectArray");
        if(guobjectarray == 0)
        {
            LOGE("Failed to find GUObjectArray symbol.");
            return 0;
        }
        return guobjectarray;
    }

    uintptr_t GetNamesPtr() const override
    {
        // GNameBlocksDebug = &NamePoolData + Blocks offset
        uintptr_t blocks_p = GetUnrealEngineELF().findSymbol("GNameBlocksDebug");
        if (blocks_p == 0)
        {
            LOGE("Failed to find GNameBlocksDebug symbol.");
            return 0;
        }

        kMgr.readMem(blocks_p, &blocks_p, sizeof(uintptr_t));
        return ((blocks_p == 0) ? 0 : (blocks_p - GetOffsets()->FNamePoolBlocks));
    }

    UE_Offsets *GetOffsets() const override
    {
        // ===============  64bit offsets  =============== //
        struct
        {
            uint16_t Stride = 2;             // alignof(FNameEntry)
            uint16_t FNamePoolBlocks = 0x40; // usually ios at 0xD0 and android at 0x40
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 4;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16_t Name = 0;
            } FNameEntry;
            struct
            {
                uint16_t Header = 0; // Offset to Memory filled with info about type and size of string
                std::function<bool(uint16_t)> GetIsWide = [](uint16_t header){return (header&1)!=0;};
                std::function<size_t(uint16_t)> GetLength = [](uint16_t header){return header>>6;};
            } FNameEntry23;
            struct
            {
                uint16_t ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16_t Objects = 0;
                uint16_t NumElements = 0x14;
            } TUObjectArray;
            struct
            {
                uint16_t Object = 0;
                uint16_t Size = 0x18;
            } FUObjectItem;
            struct
            {
                uint16_t ObjectFlags = 0x8;
                uint16_t InternalIndex = 0xC;
                uint16_t ClassPrivate = 0x10;
                uint16_t NamePrivate = 0x18;
                uint16_t OuterPrivate = 0x20;
            } UObject;
            struct
            {
                uint16_t Next = 0x28; // sizeof(UObject)
            } UField;
            struct
            {
                uint16_t SuperStruct = 0x40;
                uint16_t Children = 0x48;    // UField*
                uint16_t ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16_t PropertiesSize = 0x50;
            } UStruct;
            struct
            {
                uint16_t Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0x98; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x4;
                uint16_t ParamSize = NumParams + 0x2;
                uint16_t Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location.
            } UFunction;
            struct
            { // not needed in versions older than UE4.25
                uint16_t ClassPrivate = 0;
                uint16_t Next = 0;
                uint16_t NamePrivate = 0;
                uint16_t FlagsPrivate = 0;
            } FField;
            struct
            { // not needed in versions older than UE4.25
                uint16_t ArrayDim = 0;
                uint16_t ElementSize = 0;
                uint16_t PropertyFlags = 0;
                uint16_t Offset_Internal = 0;
                uint16_t Size = 0;
            } FProperty;
            struct
            {
                uint16_t ArrayDim = 0x30; // sizeof(UField)
                uint16_t ElementSize = 0x34;
                uint16_t PropertyFlags = 0x38;
                uint16_t Offset_Internal = 0x44;
                uint16_t Size = 0x70; // sizeof(FProperty)
            } UProperty;
        } static profile;
        static_assert(sizeof(profile) == sizeof(UE_Offsets));

        return (UE_Offsets *)&profile;
    }
};