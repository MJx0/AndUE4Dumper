#pragma once

#include "../GameProfile.h"

// PUBGM
// UE 4.18

class PUBGMProfile : public IGameProfile
{
public:
    PUBGMProfile() = default;

    virtual bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        // arm & arm64
        return e_machine == EM_AARCH64 || e_machine == EM_ARM;
    }

    std::string GetAppName() const override
    {
        return "PUBG";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {
            "com.tencent.ig",
            "com.rekoo.pubgm",
            "com.pubg.imobile",
            "com.pubg.krmobile",
            "com.vng.pubgmobile",
        };

        // chinese version doesn't have GNames encrypted but FNameEntry* is encrypted
        // (game ver 1.20.13 arm64)
        // decrypt FNameEntry* -> sub_5158B18(__int64 in, __int64 *out)
        // GNames = 0x74F0480 -> name_index = 8 -> Name = 0xC
        // GUObjectArray = 0xB491A20 -> ObjObjects = 0xB0 -> NumElements = 0x38
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
        auto e_machine = GetUnrealEngineELF().header().e_machine;
        // arm patterns
        if (e_machine == EM_ARM)
        {
            PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
            std::string ida_pattern = "BC109FE501108FE0082091E5";
            uintptr_t insn_address = findIdaPattern(map_type, ida_pattern, 0);
            if (insn_address != 0)
            {
                uintptr_t PC = insn_address + 8, PC_ldr = 0, R1 = 0, R2 = 4;

                kMgr.readMem((insn_address), &PC_ldr, sizeof(uintptr_t));
                PC_ldr = KittyArm::decode_ldr_literal(PC_ldr);

                kMgr.readMem((PC + PC_ldr), &R1, sizeof(uintptr_t));

                return (PC + R1 + R2);
            }

            // alternative .bss pattern
            insn_address = findIdaPattern(PATTERN_MAP_TYPE::BSS, "0100??0100F049020000000000", -2);
            if (insn_address == 0)
            {
                LOGE("GUObjectArray pattern failed.");
                return 0;
            }
            return insn_address;
        }
        // arm64 patterns
        else if (e_machine == EM_AARCH64)
        {
            PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
            std::string ida_pattern = "12 40 B9 ? 3E 40 B9 ? ? ? 6B ? ? ? 54 ? ? ? ? ? ? ? 91";
            int step = 0xF;

            uintptr_t insn_address = findIdaPattern(map_type, ida_pattern, step);
            if(insn_address == 0)
            {
                LOGE("GUObjectArray pattern failed.");
                return 0;
            }

            int64_t adrp_pc_rel = 0;
            int32_t add_imm12 = 0;

            uintptr_t page_off = INSN_PAGE_OFFSET(insn_address);

            uint32_t adrp_insn = 0, add_insn = 0;
            kMgr.readMem((insn_address), &adrp_insn, sizeof(uint32_t));
            kMgr.readMem((insn_address + sizeof(uint32_t)), &add_insn, sizeof(uint32_t));

            if (adrp_insn == 0 || add_insn == 0)
                return 0;

            if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
                return 0;

            add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

            return (page_off + adrp_pc_rel + add_imm12);
        }

        return 0;
    }

    uintptr_t GetNamesPtr() const override
    {
        uintptr_t enc_names = 0;

        auto e_machine = GetUnrealEngineELF().header().e_machine;
        // arm patterns
        if (e_machine == EM_ARM)
        {
            PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
            std::string ida_pattern = "E0019FE500008FE0307090E5";
            uintptr_t insn_address = findIdaPattern(map_type, ida_pattern, 0);
            if (insn_address != 0)
            {
                uintptr_t PC = insn_address + 0x8, PC_ldr = 0, R1 = 0, R2 = 0x30;

                kMgr.readMem((insn_address), &PC_ldr, sizeof(uintptr_t));
                PC_ldr = KittyArm::decode_ldr_literal(PC_ldr);

                kMgr.readMem((PC + PC_ldr), &R1, sizeof(uintptr_t));

                enc_names = (PC + R1 + R2 + 4);
            }
            else
            {
                // alternative .bss pattern
                ida_pattern = "00E432D8B00D4F891FB77ECFACA24AFD362843C6E1534D2CA2868E6CA38CBD1764";
                int step = -0xF;
                int skip = 1;
                enc_names = findIdaPattern(PATTERN_MAP_TYPE::BSS, ida_pattern, step, skip);
            }
        }
        // arm64 patterns
        else if (e_machine == EM_AARCH64)
        {
            PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
            std::string ida_pattern = "81 80 52 ? ? ? ? ? 03 1F 2A";
            int step = 0x17;

            uintptr_t insn_address = findIdaPattern(map_type, ida_pattern, step);
            if (insn_address != 0)
            {
                int64_t adrp_pc_rel = 0;
                int32_t add_imm12 = 0, ldrb_imm12 = 0;

                uintptr_t page_off = INSN_PAGE_OFFSET(insn_address);

                uint32_t adrp_insn = 0, add_insn = 0, ldrb_insn = 0;
                kMgr.readMem((insn_address), &adrp_insn, sizeof(uint32_t));
                kMgr.readMem((insn_address + 4), &add_insn, sizeof(uint32_t));
                kMgr.readMem((insn_address + 8), &ldrb_insn, sizeof(uint32_t));

                if (adrp_insn == 0 || add_insn == 0 || ldrb_insn == 0)
                    return 0;

                if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
                    return 0;

                add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

                if (!KittyArm64::decode_ldrstr_uimm(ldrb_insn, &ldrb_imm12))
                    return 0;

                enc_names = (page_off + adrp_pc_rel + add_imm12 + ldrb_imm12 - 4);
            }
        }

        if (enc_names == 0)
        {
            LOGE("GNames pattern failed.");
            return 0;
        }

        // getting randomized GNames ptr
        int32_t in;
        uintptr_t out[16];

        kMgr.readMem(enc_names, &in, sizeof(int32_t));
        in = (in - 100) / 3;
        
        kMgr.readMem(enc_names + 8, &out[in - 1], sizeof(uintptr_t));
        while (in - 2 >= 0)
        {
            kMgr.readMem(out[in - 1], &out[in - 2], sizeof(uintptr_t));
            --in;
        }

        uintptr_t ret = 0;
        kMgr.readMem(out[0], &ret, sizeof(uintptr_t));
        return ret;
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
                uint16_t Name = 0xC;
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
                uint16_t Size = 0x70; // sizeof(UProperty)
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
                uint16_t NumParams = EFunctionFlags + 0x4;
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
                uint16_t Offset_Internal = 0x34;
                uint16_t Size = 0x50; // sizeof(UProperty)
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