#include "GameProfile.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unordered_map>

#include <hash/hash.h>

BaseInfo IGameProfile::GetBaseInfo() const
{
    static BaseInfo baseInfo;
    static bool once = false;
    if (!once)
    {
        auto ue4maps = GetMaps();
        for (auto &it : ue4maps)
        {
            if (!it.is_private || !it.readable || it.writeable)
                continue;

            char magic[4] = {0};
            PMemory::vm_rpm_ptr((void *)(it.startAddress), magic, sizeof(magic));
            if (VERIFY_ELF_HEADER(magic))
            {
                PMemory::vm_rpm_ptr((void *)(it.startAddress), &baseInfo.ehdr, sizeof(baseInfo.ehdr));
                baseInfo.map = it;
                // just in case if game has two headers r--p / r-xp
                // do not break;
            }
        }
        once = true;
    }
    return baseInfo;
}

std::pair<uintptr_t, uintptr_t> IGameProfile::GetRange() const
{
    static std::pair<uintptr_t, uintptr_t> libRange;
    static bool once = false;
    if (!once)
    {
        auto fromAddress = GetBaseInfo().map.startAddress;
        auto toAddress = GetMaps().back().endAddress;

#ifdef _EXECUTABLE
        auto allMaps = KittyMemory::getAllMapsEx();
#else
        auto allMaps = KittyMemory::getAllMaps();
#endif
        // check .bss
        for (auto &it : allMaps)
        {
            if (it.startAddress == toAddress)
            {
                if (it.is_private && it.is_rw && (it.pathname.empty() || it.pathname == "[anon:.bss]"))
                    toAddress = it.endAddress;
                else
                    break;
            }
        }

        libRange = {fromAddress, toAddress};
        once = true;
    }
    return libRange;
}

void IGameProfile::DumpLibrary() const
{
    auto libRange = GetRange();
    PMemory::dumpMaps();
    PMemory::dumpMemory(libRange.first, libRange.second);
}

uintptr_t IGameProfile::findSymbol(const std::string &symbol) const
{
    // refs https://gist.github.com/resilar/24bb92087aaec5649c9a2afc0b4350c8

    static std::unordered_map<uint64_t, uintptr_t> _cachedSymbols;

    uint64_t symbolHash = Hash(symbol.c_str(), symbol.length());

    if (_cachedSymbols.size() > 0)
    {
        if (_cachedSymbols.count(symbolHash) > 0)
        {
            return _cachedSymbols[symbolHash];
        }
        return 0;
    }

    LOGI("findSymbol: Initializing symbols cache...");

    if (!VERIFY_ELF_HEADER(GetBaseInfo().magic))
    {
        LOGI("findSymbol: bad elf.");
        return 0;
    }

    auto ehdr = GetBaseInfo().ehdr;
    uintptr_t base = GetBaseInfo().map.startAddress;

    // read all program headers
    std::vector<char> phdr_buff(ehdr.e_phnum * ehdr.e_phentsize);
    PMemory::vm_rpm_ptr((void *)(base + ehdr.e_phoff), phdr_buff.data(), ehdr.e_phnum * ehdr.e_phentsize);

    int loads = 0;
    size_t strtab = 0, strsz = 0, symtab = 0, syment = 0;
    for (ElfW(Half) i = 0; i < ehdr.e_phnum; i++)
    {
        ElfW(Phdr) phdr = {};
        memcpy(&phdr, phdr_buff.data() + (i * ehdr.e_phentsize), ehdr.e_phentsize);

        if (phdr.p_type == PT_LOAD)
        {
            if (ehdr.e_type == ET_EXEC)
            {
                if (phdr.p_vaddr - phdr.p_offset < base)
                {
                    LOGE("findSymbol: This is not the lowest base of the ELF");
                    return 0;
                }
            }
            loads++;
        }
        else if (phdr.p_type == PT_DYNAMIC)
        {
            uintptr_t dyn_addr = ((ehdr.e_type != ET_EXEC) ? base + phdr.p_vaddr : phdr.p_vaddr);

            std::vector<ElfW(Dyn)> dyn_buff(phdr.p_memsz / sizeof(ElfW(Dyn)));
            if (!PMemory::vm_rpm_ptr((void *)dyn_addr, dyn_buff.data(), phdr.p_memsz))
            {
                LOGE("findSymbol: failed to read dynamic.");
                return 0;
            }

            for (auto &dyn : dyn_buff)
            {
                switch (dyn.d_tag)
                {
                case DT_STRTAB:
                    strtab = dyn.d_un.d_ptr;
                    break;
                case DT_SYMTAB:
                    symtab = dyn.d_un.d_ptr;
                    break;
                case DT_STRSZ:
                    strsz = dyn.d_un.d_ptr;
                    break;
                case DT_SYMENT:
                    syment = dyn.d_un.d_ptr;
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Check that we have all program headers required for dynamic linking
    if (!loads || !strtab || !strsz || !symtab || !syment)
    {
        LOGE("findSymbol: failed to require all program headers for dynamic linking.");
        return 0;
    }

    // String table (immediately) follows the symbol table
    if (!(symtab < strtab))
    {
        LOGE("findSymbol: String table doesn't follow the symbol table.");
        return 0;
    }

    const char *strtab_p = (char *)strtab;
    if (strtab < base)
        strtab_p += base;

    uintptr_t sym_addr = symtab + syment;
    if (symtab < base)
        sym_addr += base;

    // symbol & string tables

    std::vector<ElfW(Sym)> symtab_buff((strtab - symtab) / sizeof(ElfW(Sym)));
    PMemory::vm_rpm_ptr((void *)sym_addr, symtab_buff.data(), (strtab - symtab));

    for (auto &sym : symtab_buff)
    {
        if (sym.st_name > strsz)
            break;

        std::string sym_str = PMemory::vm_rpm_str((char *)(strtab_p + sym.st_name), 1024);
        if (!sym_str.empty())
        {
            _cachedSymbols[Hash(sym_str.c_str(), sym_str.length())] = ((ehdr.e_type != ET_EXEC) ? base + sym.st_value : sym.st_value);
        }
    }

    if (_cachedSymbols.count(symbolHash) > 0)
    {
        return _cachedSymbols[symbolHash];
    }

    return 0;
}

uintptr_t IGameProfile::findPattern(PATTERN_MAP_TYPE map_type, const std::string &hex, const std::string &mask, const int step, uint32_t skip_result) const
{
    MapInfoArray mapsArray;
    switch (map_type)
    {
    case PATTERN_MAP_TYPE::MAP_RXP:
        mapsArray = GetMappedRXPs();
        LOGD("RXP Maps Count: %d", mapsArray.size());
        break;
    case PATTERN_MAP_TYPE::MAP_RWP:
        mapsArray = GetMappedRWPs();
        LOGD("RWP Maps Count: %d", mapsArray.size());
        break;
    case PATTERN_MAP_TYPE::MAP_BSS:
        mapsArray = GetMappedBSSs();
        LOGD("BSS Maps Count: %d", mapsArray.size());
        break;

    default:
        return 0;
    }

    uintptr_t insn_address = 0;

#ifdef _EXECUTABLE
    uintptr_t local_base = 0, remote_base = 0;
    for (auto &it : mapsArray)
    {
        local_base = it.local_ptr;
        remote_base = it.remote_map.startAddress;
        if (skip_result > 0)
        {
            auto adr_list = KittyScanner::findHexAll(it.local_ptr, it.local_ptr + it.remote_map.length, hex, mask);
            if (adr_list.size() > skip_result)
            {
                insn_address = adr_list[skip_result];
            }
        }
        else
        {
            insn_address = KittyScanner::findHexFirst(it.local_ptr, it.local_ptr + it.remote_map.length, hex, mask);
        }
        if (insn_address)
            break;
    }
    return ((insn_address != 0) ? (remote_base + ((insn_address + step) - local_base)) : 0);
#else
    for (auto &it : mapsArray)
    {
        if (skip_result > 0)
        {
            auto adr_list = KittyScanner::findHexAll(it.startAddress, it.endAddress, hex, mask);
            if (adr_list.size() > skip_result)
            {
                insn_address = adr_list[skip_result];
            }
        }
        else
        {
            insn_address = KittyScanner::findHexFirst(it.startAddress, it.endAddress, hex, mask);
        }
        if (insn_address)
            break;
    }
    return (insn_address ? (insn_address + step) : 0);
#endif
}

MapInfoArray IGameProfile::GetMappedRXPs() const
{
    static MapInfoArray mapInfoArray{};
    static bool once = false;
    if (!once)
    {
        auto maps = GetMaps();
#ifdef _EXECUTABLE
        for (auto &it : maps)
        {
            if (!it.is_private || !it.is_rx)
                continue;

            void *mapped = mmap(NULL, it.length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (!mapped)
            {
                LOGW("Couldn't allocate r-xp map at %p-%p into local memory.", (void *)it.startAddress, (void *)it.endAddress);
                continue;
            }

            if (!PMemory::vm_rpm_ptr((void *)it.startAddress, mapped, it.length))
            {
                LOGW("Couldn't copy r-xp map at %p-%p.", (void *)it.startAddress, (void *)it.endAddress);
                munmap(mapped, it.length);
                continue;
            }
            mapInfoArray.push_back({(uintptr_t)mapped, it});
        }
#else
        for (auto &it : maps)
        {
            if (it.is_private && it.is_rx)
            {
                mapInfoArray.push_back(it);
            }
        }
#endif
        once = true;
    }
    return mapInfoArray;
}

MapInfoArray IGameProfile::GetMappedRWPs() const
{
    static MapInfoArray mapInfoArray{};
    static bool once = false;
    if (!once)
    {
        auto maps = GetMaps();
#ifdef _EXECUTABLE
        for (auto &it : maps)
        {
            if (!it.is_private || !it.is_rw || it.inode != GetBaseInfo().map.inode)
                continue;

            void *mapped = mmap(NULL, it.length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (!mapped)
            {
                LOGW("Couldn't allocate rw-p map at %p-%p into local memory.", (void *)it.startAddress, (void *)it.endAddress);
                continue;
            }

            if (!PMemory::vm_rpm_ptr((void *)it.startAddress, mapped, it.length))
            {
                LOGW("Couldn't copy rw-p map at %p-%p.", (void *)it.startAddress, (void *)it.endAddress);
                munmap(mapped, it.length);
                continue;
            }
            mapInfoArray.push_back({(uintptr_t)mapped, it});
        }
#else
        for (auto &it : maps)
        {
            if (it.is_private && it.is_rw && it.inode == GetBaseInfo().map.inode)
            {
                mapInfoArray.push_back(it);
            }
        }
#endif
        once = true;
    }
    return mapInfoArray;
}

MapInfoArray IGameProfile::GetMappedBSSs() const
{
    static MapInfoArray mapInfoArray{};
    static bool once = false;
    if (!once)
    {
        auto libRange = GetRange();
        std::vector<ProcMap> bssMaps;
#ifdef _EXECUTABLE
        auto allMaps = KittyMemory::getAllMapsEx();
#else
        auto allMaps = KittyMemory::getAllMaps();
#endif
        for (auto &it : allMaps)
        {
            if (it.startAddress >= libRange.first && it.endAddress <= libRange.second)
            {
                if (it.is_private && it.is_rw && (it.pathname.empty() || it.pathname == "[anon:.bss]"))
                    bssMaps.push_back(it);
            }
            else if (it.endAddress > libRange.second)
                break;
        }

#ifdef _EXECUTABLE
        for (auto &it : bssMaps)
        {
            void *mapped = mmap(NULL, it.length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (!mapped)
            {
                LOGW("Couldn't allocate .bss map at %p-%p into local memory.", (void *)it.startAddress, (void *)it.endAddress);
                continue;
            }

            if (!PMemory::vm_rpm_ptr((void *)it.startAddress, mapped, it.length))
            {
                LOGW("Couldn't copy .bss map at %p-%p.", (void *)it.startAddress, (void *)it.endAddress);
                munmap(mapped, it.length);
                continue;
            }
            mapInfoArray.push_back({(uintptr_t)mapped, it});
        }
#else
        mapInfoArray = bssMaps;
#endif
        once = true;
    }
    return mapInfoArray;
}