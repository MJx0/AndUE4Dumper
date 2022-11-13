#pragma once

#include "../GameProfile.h"

// DeadByDaylight
// UE 4.25

class DBDProfile : public IGameProfile
{
public:
    DBDProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetBaseInfo().ehdr.e_machine;
        // only arm64
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Dead by Daylight";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.bhvr.deadbydaylight" };
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        std::string hex = "08 00 00 91 E1 03 00 AA E0 03 08 AA E2 03 1F 2A";
        std::string mask = "x??xxx?xxxxxxxxx";
        int step = -4;

        uintptr_t insn_address = findPattern(PATTERN_MAP_TYPE::MAP_RXP, hex, mask, step);
        if (insn_address == 0)
        {
            LOGE("GUObjectArray pattern failed.");
            return 0;
        }

        int64_t adrp_pc_rel = 0;
        int32_t add_imm12 = 0;

        uintptr_t page_off = INSN_PAGE_OFFSET(insn_address);

        uint32_t adrp_insn = 0, add_insn = 0;
        PMemory::vm_rpm_ptr((void *)(insn_address), &adrp_insn, sizeof(uint32_t));
        PMemory::vm_rpm_ptr((void *)(insn_address + sizeof(uint32_t)), &add_insn, sizeof(uint32_t));

        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);
        if (add_imm12 == 0)
            return 0;

        return (page_off + adrp_pc_rel + add_imm12);

        //return GetBaseInfo().map.startAddress + 0x0000000;
    }

    uintptr_t GetNamesPtr() const override
    {
        std::string hex = "F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 00 00 00 00 A8 02 00 39";
        std::string mask = "xxxxxxxxxx?x????xx?x";
        int step = 0x24;

        uintptr_t insn_address = findPattern(PATTERN_MAP_TYPE::MAP_RXP, hex, mask, step);
        if (insn_address == 0)
        {
            LOGE("NamePoolData pattern failed.");
            return 0;
        }

        int64_t adrp_pc_rel = 0;
        int32_t add_imm12 = 0;

        uintptr_t page_off = INSN_PAGE_OFFSET(insn_address);

        uint32_t adrp_insn = 0, add_insn = 0;
        PMemory::vm_rpm_ptr((void *)(insn_address), &adrp_insn, sizeof(uint32_t));
        PMemory::vm_rpm_ptr((void *)(insn_address + sizeof(uint32_t)), &add_insn, sizeof(uint32_t));

        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);
        if (add_imm12 == 0)
            return 0;

        return (page_off + adrp_pc_rel + add_imm12);

        //return GetBaseInfo().map.startAddress + 0x0000000;
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
        } static profile;
        static_assert(sizeof(profile) == sizeof(UE_Offsets));

        return (UE_Offsets *)&profile;
    }
};