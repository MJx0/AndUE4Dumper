#pragma once

#include "../GameProfile.h"

// Mortal Kombat
// UE 4.25+

class MortalKProfile : public IGameProfile
{
public:
    MortalKProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        // arm & arm64
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "Mortal Kombat";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.wb.goog.mkx" };
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
            uint16_t Stride = 4;
            uint16_t FNamePoolBlocks = 0x40; // usually ios at 0xD0 and android at 0x40
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 8;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16_t Name = 0;
            } FNameEntry;
            struct
            {
                uint16_t Header = 4; // Offset to Memory filled with info about type and size of string
                std::function<bool(uint16_t)> GetIsWide = [](uint16_t header){return (header&1)!=0;};
                std::function<size_t(uint16_t)> GetLength = [](uint16_t header){return header>>1;};
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
                uint16_t OuterPrivate = 0x28;
            } UObject;
            struct
            {
                uint16_t Next = 0x30; // sizeof(UObject)
            } UField;
            struct
            {
                uint16_t SuperStruct = 0x48;
                uint16_t Children = 0x50;      // UField*
                uint16_t ChildProperties = 0x58; // FField*
                uint16_t PropertiesSize = 0x60;
            } UStruct;
            struct
            {
                uint16_t Names = 0x48; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0xB8; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x4;
                uint16_t ParamSize = NumParams + 0x2;
                uint16_t Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location
            } UFunction;
            struct
            {
                uint16_t ClassPrivate = 0x8;
                uint16_t Next = 0x20;
                uint16_t NamePrivate = 0x28;
                uint16_t FlagsPrivate = 0x30;
            } FField;
            struct
            {
                uint16_t ArrayDim = 0x38;
                uint16_t ElementSize = 0x3C;
                uint16_t PropertyFlags = 0x40;
                uint16_t Offset_Internal = 0x4C;
                uint16_t Size = 0x80; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.25+
                uint16_t ArrayDim = 0;
                uint16_t ElementSize = 0;
                uint16_t PropertyFlags = 0;
                uint16_t Offset_Internal = 0;
                uint16_t Size = 0;
            } UProperty;
        } static profile64;
        static_assert(sizeof(profile64) == sizeof(UE_Offsets));

        // ===============  32bit offsets  =============== //
        struct
        {
            uint16_t Stride = 4;
            uint16_t FNamePoolBlocks = 0x30; // usually at 0x30 (32bit)
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 8;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16_t Name = 0;
            } FNameEntry;
            struct
            {
                uint16_t Header = 4; // Offset to Memory filled with info about type and size of string
                std::function<bool(uint16_t)> GetIsWide = [](uint16_t header){return (header&1)!=0;};
                std::function<size_t(uint16_t)> GetLength = [](uint16_t header){return header>>1;};
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
                uint16_t Size = 0x10;
            } FUObjectItem;
            struct
            {
                uint16_t ObjectFlags = 0x4;
                uint16_t InternalIndex = 0x8;
                uint16_t ClassPrivate = 0xC;
                uint16_t NamePrivate = 0x10;
                uint16_t OuterPrivate = 0x1C;
            } UObject;
            struct
            {
                uint16_t Next = 0x20; // sizeof(UObject)
            } UField;
            struct
            {
                uint16_t SuperStruct = 0x2C;
                uint16_t Children = 0x30;      // UField*
                uint16_t ChildProperties = 0x34; // FField*
                uint16_t PropertiesSize = 0x38;
            } UStruct;
            struct
            {
                uint16_t Names = 0x30; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0x70; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x4;
                uint16_t ParamSize = NumParams + 0x2;
                uint16_t Func = EFunctionFlags + 0x1C; // ue3-ue4, always +0x1C (32bit) from flags location
            } UFunction;
            struct
            {
                uint16_t ClassPrivate = 0x4;
                uint16_t Next = 0x10;
                uint16_t NamePrivate = 0x14;
                uint16_t FlagsPrivate = 0x18;
            } FField;
            struct
            {
                uint16_t ArrayDim = 0x24;
                uint16_t ElementSize = 0x28;
                uint16_t PropertyFlags = 0x30;
                uint16_t Offset_Internal = 0x3C;
                uint16_t Size = 0x5C; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.25+
                uint16_t ArrayDim = 0;
                uint16_t ElementSize = 0;
                uint16_t PropertyFlags = 0;
                uint16_t Offset_Internal = 0;
                uint16_t Size = 0;
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