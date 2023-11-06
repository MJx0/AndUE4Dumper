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
        auto e_machine = GetUE4ELF().header().e_machine;
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

    uintptr_t GetGUObjectArrayPtr() const override
    {
        uintptr_t guobjectarray = GetUE4ELF().findSymbol("GUObjectArray");
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
        uintptr_t blocks_p = GetUE4ELF().findSymbol("GNameBlocksDebug");
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
            uint16 Stride = 4;
            uint16 FNamePoolBlocks = 0x40; // usually ios at 0xD0 and android at 0x40
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Number = 8;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16 Name = 0;
            } FNameEntry;
            struct
            {
                uint16 Info = 4;       // Offset to Memory filled with info about type and size of string
                uint16 WideBit = 0;    // Offset to bit which shows if string uses wide characters
                uint16 LenBit = 1;     // Offset to bit which has lenght of string
                uint16 HeaderSize = 6; // Size of FNameEntry header (offset where a string begins)
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
                uint16 OuterPrivate = 0x28;
            } UObject;
            struct
            {
                uint16 Next = 0x30; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x48;
                uint16 Children = 0x50;      // UField*
                uint16 ChildProperties = 0x58; // FField*
                uint16 PropertiesSize = 0x60;
            } UStruct;
            struct
            {
                uint16 Names = 0x48; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0xB8; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location
            } UFunction;
            struct
            {
                uint16 ClassPrivate = 0x8;
                uint16 Next = 0x20;
                uint16 NamePrivate = 0x28;
                uint16 FlagsPrivate = 0x30;
            } FField;
            struct
            {
                uint16 ArrayDim = 0x38;
                uint16 ElementSize = 0x3C;
                uint16 PropertyFlags = 0x40;
                uint16 Offset_Internal = 0x4C;
                uint16 Size = 0x80; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.25+
                uint16 ArrayDim = 0;
                uint16 ElementSize = 0;
                uint16 PropertyFlags = 0;
                uint16 Offset_Internal = 0;
                uint16 Size = 0;
            } UProperty;
        } static profile64;
        static_assert(sizeof(profile64) == sizeof(UE_Offsets));

        // ===============  32bit offsets  =============== //
        struct
        {
            uint16 Stride = 4;
            uint16 FNamePoolBlocks = 0x30; // usually at 0x30 (32bit)
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Number = 8;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16 Name = 0;
            } FNameEntry;
            struct
            {
                uint16 Info = 4;       // Offset to Memory filled with info about type and size of string
                uint16 WideBit = 0;    // Offset to bit which shows if string uses wide characters
                uint16 LenBit = 1;     // Offset to bit which has lenght of string
                uint16 HeaderSize = 6; // Size of FNameEntry header (offset where a string begins)
            } FNameEntry23;
            struct
            {
                uint16 ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16 NumElements = 0xC;
            } TUObjectArray;
            struct
            {
                uint16 Size = 0x10;
            } FUObjectItem;
            struct
            {
                uint16 ObjectFlags = 0x4;
                uint16 InternalIndex = 0x8;
                uint16 ClassPrivate = 0xC;
                uint16 NamePrivate = 0x10;
                uint16 OuterPrivate = 0x1C;
            } UObject;
            struct
            {
                uint16 Next = 0x20; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x2C;
                uint16 Children = 0x30;      // UField*
                uint16 ChildProperties = 0x34; // FField*
                uint16 PropertiesSize = 0x38;
            } UStruct;
            struct
            {
                uint16 Names = 0x30; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x70; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 Func = EFunctionFlags + 0x1C; // ue3-ue4, always +0x1C (32bit) from flags location
            } UFunction;
            struct
            {
                uint16 ClassPrivate = 0x4;
                uint16 Next = 0x10;
                uint16 NamePrivate = 0x14;
                uint16 FlagsPrivate = 0x18;
            } FField;
            struct
            {
                uint16 ArrayDim = 0x24;
                uint16 ElementSize = 0x28;
                uint16 PropertyFlags = 0x30;
                uint16 Offset_Internal = 0x3C;
                uint16 Size = 0x5C; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.25+
                uint16 ArrayDim = 0;
                uint16 ElementSize = 0;
                uint16 PropertyFlags = 0;
                uint16 Offset_Internal = 0;
                uint16 Size = 0;
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