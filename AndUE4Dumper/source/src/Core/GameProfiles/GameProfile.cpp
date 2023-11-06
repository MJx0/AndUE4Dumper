#include "GameProfile.h"

ElfScanner IGameProfile::GetUE4ELF() const
{
    static ElfScanner ue4_elf {};
    if (!ue4_elf.isValid()) {
        ue4_elf = kMgr.getMemElf("libUE4.so");

        if (!ue4_elf.isValid())
        {
            for (auto& it : KittyMemoryEx::getAllMaps(kMgr.processID()))
            {
                if (KittyUtils::string_contains(it.pathname, kMgr.processName()) && KittyUtils::string_endswith(it.pathname, ".apk"))
                    ue4_elf = kMgr.getMemElfInZip(it.pathname, "libUE4.so");

                if (ue4_elf.isValid())
                    break;
            }
        }
    }

    return ue4_elf;
}

uintptr_t IGameProfile::findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result) const
{
    ElfScanner ue4_elf = GetUE4ELF();
    std::vector<KittyMemoryEx::ProcMap> search_segments;
    
    if (map_type == PATTERN_MAP_TYPE::MAP_BASE)
    {
        search_segments.push_back(ue4_elf.baseSegment());
    }
    else if (map_type == PATTERN_MAP_TYPE::MAP_RXP)
    {
        for (auto& it : ue4_elf.segments())
            if (it.is_rx && it.is_private)
                search_segments.push_back(it);
        LOGD("RXP Maps Count: %d", search_segments.size());
    }
    else if (map_type == PATTERN_MAP_TYPE::MAP_RWP)
    {
        for (auto& it : ue4_elf.segments())
            if (it.is_rw && it.is_private)
                search_segments.push_back(it);
        LOGD("RWP Maps Count: %d", search_segments.size());
    }
    else if (map_type == PATTERN_MAP_TYPE::MAP_BSS)
    {
        for (auto& it : ue4_elf.segments())
            if (it.startAddress >= ue4_elf.bss() && it.endAddress <= (ue4_elf.bss() + ue4_elf.bssSize()))
                search_segments.push_back(it);
        LOGD("BSS Maps Count: %d", search_segments.size());
    }
    else return 0;

    uintptr_t insn_address = 0;

    for (auto &it : search_segments)
    {
        if (skip_result > 0)
        {
            auto adr_list = kMgr.memScanner.findIdaPatternAll(it.startAddress, it.endAddress, pattern);
            if (adr_list.size() > skip_result)
            {
                insn_address = adr_list[skip_result];
            }
        }
        else
        {
            insn_address = kMgr.memScanner.findIdaPatternFirst(it.startAddress, it.endAddress, pattern);
        }
        if (insn_address)
            break;
    }
    return (insn_address ? (insn_address + step) : 0);
}