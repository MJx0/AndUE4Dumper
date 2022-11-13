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
        auto e_machine = GetBaseInfo().ehdr.e_machine;
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
        // GFNameTableForDebuggerVisualizers_MT = &GNames
        uintptr_t name_table_p = findSymbol("GFNameTableForDebuggerVisualizers_MT");
        if (name_table_p == 0)
        {
            LOGE("Failed to find GFNameTableForDebuggerVisualizers_MT symbol.");
            return 0;
        }
        return PMemory::vm_rpm_ptr<uintptr_t>((void *)name_table_p);

        // return GetBaseInfo().map.startAddress + 0x0000000;
    }

    UE_Offsets *GetOffsets() const override
    {
        // ===============  64bit offsets  =============== //
        struct
        {
            uint16 Stride = 0;          // not needed in versions older than UE4.23
            uint16 FNamePoolBlocks = 0; // not needed in versions older than UE4.23
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Number = 4;
            } FName;
            struct
            {
                uint16 Name = 0x10;
            } FNameEntry;
            struct
            { // not needed in versions older than UE4.23
                uint16 Info = 0;
                uint16 WideBit = 0;
                uint16 LenBit = 0;
                uint16 HeaderSize = 0;
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
                uint16 SuperStruct = 0x30; // sizeof(UField)
                uint16 Children = 0x38;    // UField*
                uint16 ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16 PropertiesSize = 0x40;
            } UStruct;
            struct
            {
                uint16 Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x88; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x6;
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
                uint16 Offset_Internal = 0x50;
                uint16 Size = 0x78; // sizeof(FProperty)
            } UProperty;
        } static profile64;
        static_assert(sizeof(profile64) == sizeof(UE_Offsets));

        // ===============  32bit offsets  =============== //
        struct
        {
            uint16 Stride = 0;          // not needed in versions older than UE4.23
            uint16 FNamePoolBlocks = 0; // not needed in versions older than UE4.23
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Number = 4;
            } FName;
            struct
            {
                uint16 Name = 0x8;
            } FNameEntry;
            struct
            { // not needed in versions older than UE4.23
                uint16 Info = 0;
                uint16 WideBit = 0;
                uint16 LenBit = 0;
                uint16 HeaderSize = 0;
            } FNameEntry23;
            struct
            {
                uint16 ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16 NumElements = 0x8;
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
                uint16 OuterPrivate = 0x18;
            } UObject;
            struct
            {
                uint16 Next = 0x1C; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x20; // sizeof(UField)
                uint16 Children = 0x24;    // UField*
                uint16 ChildProperties = 0;  // not needed in versions older than UE4.25
                uint16 PropertiesSize = 0x28;
            } UStruct;
            struct
            {
                uint16 Names = 0x2C; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x58; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x6;
                uint16 ParamSize = NumParams + 0x2;
                uint16 Func = EFunctionFlags + 0x1C; // +0x1C (32bit) from flags location.
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
                uint16 ArrayDim = 0x20; // sizeof(UField)
                uint16 ElementSize = 0x24;
                uint16 PropertyFlags = 0x28;
                uint16 Offset_Internal = 0x40;
                uint16 Size = 0x58; // sizeof(UProperty)
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