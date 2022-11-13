#pragma once

#include "../GameProfile.h"

// Apex Legends
// UE 4.25+ ??

class ApexProfile : public IGameProfile
{
public:
    ApexProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetBaseInfo().ehdr.e_machine;
        // arm & arm64
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "Apex Legends";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.ea.gp.apexlegendsmobilefps" };
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        //return GetBaseInfo().map.startAddress + 0x000000;

        auto e_machine = GetBaseInfo().ehdr.e_machine;
        // arm pattern
        if (e_machine == EM_ARM)
        {
            std::string hex = "031191E7003000E3FF3F40E3";
            std::string mask(hex.length() / 2, 'x');
            int step = -0x18;

            uintptr_t insn_address = findPattern(PATTERN_MAP_TYPE::MAP_RXP, hex, mask, step);
            if (insn_address == 0)
            {
                LOGE("GUObjectArray pattern failed.");
                return 0;
            }

            uintptr_t PC = insn_address + 8, PC_ldr = 0, R1 = 0, R2 = 4;

            PMemory::vm_rpm_ptr((void *)(insn_address), &PC_ldr, sizeof(uintptr_t));
            PC_ldr = KittyArm::decode_ldr_literal(PC_ldr);

            PMemory::vm_rpm_ptr((void *)(PC + PC_ldr), &R1, sizeof(uintptr_t));

            return (PC + R1 + R2);
        }
        // arm64 pattern
        else if (e_machine == EM_AARCH64)
        {

            std::string hex = "08 04 40 B9 00 00 00 34 09 00 40 B9 E0 03 1F 2A";
            std::string mask = "xxxx??xxxxxxxxxx";
            int step = 0x24;

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

            if (KittyArm64::is_insn_ldst_uimm(add_insn))
                KittyArm64::decode_ldrstr_uimm(add_insn, &add_imm12);
            else
                add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

            if (add_imm12 == 0)
                return 0;

            return ((page_off + adrp_pc_rel + add_imm12) - GetOffsets()->FUObjectArray.ObjObjects);
        }

        return 0;
    }

    uintptr_t GetNamesPtr() const override
    {
        // return GetBaseInfo().map.startAddress + 0x000000;

        auto e_machine = GetBaseInfo().ehdr.e_machine;
        // arm pattern
        if (e_machine == EM_ARM)
        {
            std::string hex = "08 00 9F E5 00 00 8F E0 30 00 80 E2 1E FF 2F E1 00 00 00 00 00 00 2D";
            std::string mask = "xxxxxxxxxxxxxxxx??????x";
            
            uintptr_t insn_address = findPattern(PATTERN_MAP_TYPE::MAP_RXP, hex, mask, 0);
            if (insn_address == 0)
            {
                LOGE("NamePoolData pattern failed.");
                return 0;
            }

            uintptr_t PC = insn_address + 8, PC_ldr = 0, R1 = 0, R2 = 4;

            PMemory::vm_rpm_ptr((void *)(insn_address), &PC_ldr, sizeof(uintptr_t));
            PC_ldr = KittyArm::decode_ldr_literal(PC_ldr);

            PMemory::vm_rpm_ptr((void *)(PC + PC_ldr), &R1, sizeof(uintptr_t));

            return (PC + R1 + R2);
        }
        // arm64 pattern
        else if (e_machine == EM_AARCH64)
        {
            std::string hex = "C8 00 00 37 00 00 00 B0 00 00 00 91 00 00 00 97 28 00 80 52";
            std::string mask = "xxxx???xxx?x???xxxxx";
            int step = 4;

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

            if (KittyArm64::is_insn_ldst_uimm(add_insn))
                KittyArm64::decode_ldrstr_uimm(add_insn, &add_imm12);
            else
                add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

            if (add_imm12 == 0)
                return 0;

            return (page_off + adrp_pc_rel + add_imm12);
        }

        return 0;
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
                uint16 ObjObjects = 0x18;
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
                uint16 Children = 0x48;      // UField*
                uint16 ChildProperties = 0x50; // FField*
                uint16 PropertiesSize = 0x58;
            } UStruct;
            struct
            {
                uint16 Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0xC0; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location.
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
                uint16 ArrayDim = 0x34; // sizeof(FField)
                uint16 ElementSize = 0x38;
                uint16 PropertyFlags = 0x40;
                uint16 Offset_Internal = 0x4C;
                uint16 Size = 0x78; // sizeof(FProperty)
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
            uint16 Stride = 2;             // alignof(FNameEntry)
            uint16 FNamePoolBlocks = 0x30; // usually at 0x30 (32bit)
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
                uint16 ObjObjects = 0x14;
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
                uint16 OuterPrivate = 0x18;
            } UObject;
            struct
            {
                uint16 Next = 0x1C; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x28;
                uint16 Children = 0x2C;      // UField*
                uint16 ChildProperties = 0x30; // FField*
                uint16 PropertiesSize = 0x34;
            } UStruct;
            struct
            {
                uint16 Names = 0x2C; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x7C; // sizeof(UStruct)
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
                uint16 ArrayDim = 0x20;
                uint16 ElementSize = 0x24;
                uint16 PropertyFlags = 0x28;
                uint16 Offset_Internal = 0x34;
                uint16 Size = 0x50; // sizeof(FProperty)
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