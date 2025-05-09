#pragma once

#include "../GameProfile.h"

// ARK
// UE 4.17

class ArkProfile : public IGameProfile
{
public:
    ArkProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        // arm & arm64
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "Ark Survival";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.studiowildcard.wardrumstudios.ark" };
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
        if(guobjectarray == 0)
        {
            LOGE("Failed to find GUObjectArray symbol.");
            return 0;
        }
        return guobjectarray;
    }

    uintptr_t GetNamesPtr() const override
    {
        // GFNameTableForDebuggerVisualizers_MT = &GNames
        uintptr_t name_table_p = GetUnrealEngineELF().findSymbol("GFNameTableForDebuggerVisualizers_MT");
        if (name_table_p == 0)
        {
            LOGE("Failed to find GFNameTableForDebuggerVisualizers_MT symbol.");
            return 0;
        }

        kMgr.readMem(name_table_p, &name_table_p, sizeof(uintptr_t));
        return name_table_p;
    }

    UE_Offsets *GetOffsets() const override
    {
        // ===============  64bit offsets  =============== //
        struct
        {
            uint16_t Stride = 0;          // not needed in versions older than UE4.23
            uint16_t FNamePoolBlocks = 0; // not needed in versions older than UE4.23
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 4;
            } FName;
            struct
            {
                uint16_t Name = 0x10;
            } FNameEntry;
            struct
            { // not needed in versions older than UE4.23
                uint16_t Header = 0;
                std::function<bool(uint16_t)> GetIsWide = nullptr;
                std::function<size_t(uint16_t)> GetLength = nullptr;
            } FNameEntry23;
            struct
            {
                uint16_t ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16_t Objects = 0;
                uint16_t NumElements = 0xC;
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
                uint16_t SuperStruct = 0x30; // sizeof(UField)
                uint16_t Children = 0x38;    // UField*
                uint16_t ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16_t PropertiesSize = 0x40;
            } UStruct;
            struct
            {
                uint16_t Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0x88; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x6;
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
                uint16_t Offset_Internal = 0x50;
                uint16_t Size = 0x78; // sizeof(FProperty)
            } UProperty;
        } static profile64;
        static_assert(sizeof(profile64) == sizeof(UE_Offsets));

        // ===============  32bit offsets  =============== //
        struct
        {
            uint16_t Stride = 0;          // not needed in versions older than UE4.23
            uint16_t FNamePoolBlocks = 0; // not needed in versions older than UE4.23
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 4;
            } FName;
            struct
            {
                uint16_t Name = 0x8;
            } FNameEntry;
            struct
            { // not needed in versions older than UE4.23
                uint16_t Header = 0;
                std::function<bool(uint16_t)> GetIsWide = nullptr;
                std::function<size_t(uint16_t)> GetLength = nullptr;
            } FNameEntry23;
            struct
            {
                uint16_t ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16_t Objects = 0;
                uint16_t NumElements = 0x8;
            } TUObjectArray;
            struct
            {
                uint16_t Object = 0;
                uint16_t Size = 0x10;
            } FUObjectItem;
            struct
            {
                uint16_t ObjectFlags = 0x4;
                uint16_t InternalIndex = 0x8;
                uint16_t ClassPrivate = 0xC;
                uint16_t NamePrivate = 0x10;
                uint16_t OuterPrivate = 0x18;
            } UObject;
            struct
            {
                uint16_t Next = 0x1C; // sizeof(UObject)
            } UField;
            struct
            {
                uint16_t SuperStruct = 0x20; // sizeof(UField)
                uint16_t Children = 0x24;    // UField*
                uint16_t ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16_t PropertiesSize = 0x28;
            } UStruct;
            struct
            {
                uint16_t Names = 0x2C; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0x58; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x6;
                uint16_t ParamSize = NumParams + 0x2;
                uint16_t Func = EFunctionFlags + 0x1C; // +0x1C (32bit) from flags location.
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
                uint16_t ArrayDim = 0x20; // sizeof(UField)
                uint16_t ElementSize = 0x24;
                uint16_t PropertyFlags = 0x28;
                uint16_t Offset_Internal = 0x40;
                uint16_t Size = 0x58; // sizeof(UProperty)
            } UProperty;
        } static profile32;
        static_assert(sizeof(profile32) == sizeof(UE_Offsets));

#ifdef __LP64__
        return (UE_Offsets *)&profile64;
#else
        return (UE_Offsets *)&profile32;
#endif
    }
};