#include "GameProfile.h"

#include <utfcpp/unchecked.h>

ElfScanner IGameProfile::GetUnrealEngineELF() const
{
    static const int nUELibNames = 2;
    static const char *cUELibNames[]{"libUE4.so", "libUnreal.so"};

    static ElfScanner ue_elf{};
    if (ue_elf.isValid()) return ue_elf;

    for (int n = 0; n < nUELibNames; n++)
    {
        ue_elf = kMgr.getMemElf(cUELibNames[n]);
        if (ue_elf.isValid()) return ue_elf;

        for (auto &it : KittyMemoryEx::getAllMaps(kMgr.processID()))
        {
            if (KittyUtils::String::Contains(it.pathname, kMgr.processName()) && KittyUtils::String::EndsWith(it.pathname, ".apk"))
            {
                ue_elf = kMgr.getMemElfInZip(it.pathname, cUELibNames[n]);
                if (ue_elf.isValid()) return ue_elf;
            }
        }
    }

    return ue_elf;
}

bool IGameProfile::isEmulator() const
{
    if (!KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm/nb/").empty() || !KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm64/nb/").empty())
        return true;

    for (auto &it : GetUnrealEngineELF().segments())
        if (it.executable) return false;

    return true;
}

uintptr_t IGameProfile::findIdaPattern(PATTERN_MAP_TYPE map_type, const std::string &pattern, const int step, uint32_t skip_result) const
{
    ElfScanner ue_elf = GetUnrealEngineELF();
    std::vector<KittyMemoryEx::ProcMap> search_segments;
    bool hasBSS = ue_elf.bss() > 0;

    if (map_type == PATTERN_MAP_TYPE::BSS)
    {
        if (!hasBSS)
            return 0;

        for (auto &it : ue_elf.segments())
            if (it.startAddress >= ue_elf.bss() && it.endAddress <= (ue_elf.bss() + ue_elf.bssSize()))
                search_segments.push_back(it);
    }
    else
    {
        for (auto &it : ue_elf.segments())
        {
            if (!it.readable || !it.is_private)
                continue;

            if (map_type == PATTERN_MAP_TYPE::ANY_X && !it.executable)
                continue;
            else if (map_type == PATTERN_MAP_TYPE::ANY_W && !it.writeable)
                continue;

            if (hasBSS && it.startAddress >= ue_elf.bss() && it.endAddress <= (ue_elf.bss() + ue_elf.bssSize()))
                continue;

            search_segments.push_back(it);
        }
    }

    LOGD("search_segments count = %p", (void *)search_segments.size());

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

uint8_t *IGameProfile::GetNameEntry_Internal(int32_t id) const
{
    if (id < 0) return nullptr;

    // don't forget static
    static uintptr_t namesPtr = GetNamesPtr();
    if (namesPtr == 0) return nullptr;

    if (!IsUsingFNamePool())
    {
        const int32_t ElementsPerChunk = 16384;
        const int32_t ChunkIndex = id / ElementsPerChunk;
        const int32_t WithinChunkIndex = id % ElementsPerChunk;

        // FNameEntry**
        uint8_t *FNameEntryArray = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + ChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryArray) return nullptr;

        // FNameEntry*
        return vm_rpm_ptr<uint8_t *>(FNameEntryArray + WithinChunkIndex * sizeof(uintptr_t));
    }

    uint32_t block_offset = ((id >> 16) * sizeof(void *));
    uint32_t chunck_offset = ((id & 0xffff) * GetOffsets()->Stride);

    uint8_t *chunck = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + GetOffsets()->FNamePoolBlocks + block_offset));
    if (!chunck) return nullptr;

    return (chunck + chunck_offset);
}

std::string IGameProfile::GetNameEntryString_Internal(uint8_t *entry) const
{
    if (!entry) return "";

    UE_Offsets *offsets = GetOffsets();

    uint8_t *pStr = nullptr;
    // don't care for now
    // bool isWide = false;
    size_t strLen = 0;
    int strNumber = 0;

    if (!IsUsingFNamePool())
    {
        int32_t name_index = 0;
        if (!vm_rpm_ptr(entry, &name_index, sizeof(int32_t))) return "";

        pStr = entry + offsets->FNameEntry.Name;
        // isWide = (name_index & 1);
        strLen = offsets->FNameMaxSize;
    }
    else
    {
        uint16_t header = 0;
        if (!vm_rpm_ptr(entry + offsets->FNameEntry23.Header, &header, sizeof(int16_t)))
            return "";

        if (isUsingOutlineNumberName() && offsets->FNameEntry23.GetLength(header) == 0)
        {
            const int32_t stringOff = offsets->FNameEntry23.Header + sizeof(int16_t);
            const int32_t entryIdOff = stringOff + ((stringOff == 6) * 2);
            const int32_t nextEntryId = vm_rpm_ptr<int32_t>(entry + entryIdOff);
            if (nextEntryId <= 0) return "";

            strNumber = vm_rpm_ptr<int32_t>(entry + entryIdOff + sizeof(int32_t));
            entry = GetNameEntry_Internal(nextEntryId);
            if (!vm_rpm_ptr(entry + offsets->FNameEntry23.Header, &header, sizeof(int16_t)))
                return "";
        }

        strLen = std::min<size_t>(offsets->FNameEntry23.GetLength(header), offsets->FNameMaxSize);
        if (strLen <= 0) return "";

        // isWide = offsets->FNameEntry23.GetIsWide(header);
        pStr = entry + offsets->FNameEntry23.Header + sizeof(int16_t);
    }

    std::string result = vm_rpm_str(pStr, strLen);

    if (strNumber > 0)
        result += '_' + std::to_string(strNumber - 1);

    return result;
}

std::string IGameProfile::GetNameByID_Internal(int32_t id) const
{
    return GetNameEntryString_Internal(GetNameEntry_Internal(id));
}

std::string IGameProfile::GetNameByID(int32_t id) const
{
    static std::unordered_map<int32_t, std::string> namesCachedMap;
    if (namesCachedMap.count(id) > 0)
        return namesCachedMap[id];

    std::string name = GetNameByID_Internal(id);
    if (!name.empty())
    {
        namesCachedMap[id] = name;
    }
    return name;
}