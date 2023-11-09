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

bool IGameProfile::isEmulator()  const
{ 
    return !KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm/nb/").empty() || !KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm64/nb/").empty();
}

uintptr_t IGameProfile::findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result) const
{
    ElfScanner ue4_elf = GetUE4ELF();
    std::vector<KittyMemoryEx::ProcMap> search_segments;
    bool hasBSS = ue4_elf.bss() > 0;

    if (map_type == PATTERN_MAP_TYPE::BSS)
    {
        if (!hasBSS)
            return 0;

        for (auto& it : ue4_elf.segments())
            if (it.startAddress >= ue4_elf.bss() && it.endAddress <= (ue4_elf.bss() + ue4_elf.bssSize()))
                search_segments.push_back(it);
    }
    else
    {
        for (auto& it : ue4_elf.segments())
        {
            if (!it.readable || !it.is_private)
                continue;

            if (map_type == PATTERN_MAP_TYPE::ANY_X && !it.executable)
                continue;
            else if (map_type == PATTERN_MAP_TYPE::ANY_W && !it.writeable)
                continue;

            if (hasBSS && it.startAddress >= ue4_elf.bss() && it.endAddress <= (ue4_elf.bss() + ue4_elf.bssSize()))
                continue;

            search_segments.push_back(it);
        }
    }

    LOGD("search_segments count = %p", (void*)search_segments.size());

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