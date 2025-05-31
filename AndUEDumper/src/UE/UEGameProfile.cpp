#include "UEGameProfile.hpp"

#include "UEMemory.hpp"
#include "UEWrappers.hpp"

using namespace UEMemory;

UEVarsInitStatus IGameProfile::InitUEVars()
{
    auto ue_elf = GetUnrealEngineELF();
    if (!ue_elf.isValid())
    {
        LOGE("Couldn't find a valid UE ELF in target process maps.");
        return UEVarsInitStatus::ERROR_LIB_NOT_FOUND;
    }

    switch (ue_elf.header().e_ident[4])  // EI_CLASS
    {
    case 1:  // ELFCLASS32
        if (sizeof(void *) != 4)
        {
            LOGE("Dumper is 64bit while target process is 32bit. Please use the correct architecture.");
            return UEVarsInitStatus::ERROR_ARCH_MISMATCH;
        }
        break;

    case 2:  // ELFCLASS64
        if (sizeof(void *) != 8)
        {
            LOGE("Dumper is 32bit while target process is 64bit. Please use the correct architecture.");
            return UEVarsInitStatus::ERROR_ARCH_MISMATCH;
        }
        break;

    default:
        break;
    }

    if (!ArchSupprted())
    {
        LOGE("Architecture ( 0x%x ) is not supported for this game.", ue_elf.header().e_machine);
        return UEVarsInitStatus::ARCH_NOT_SUPPORTED;
    }

    PtrValidator.setPID(kMgr.processID());
    PtrValidator.setUseCache(true);
    PtrValidator.refreshRegionCache();
    if (PtrValidator.regions().empty())
        return UEVarsInitStatus::ERROR_INIT_PTR_VALIDATOR;

    _UEVars.BaseAddress = ue_elf.base();

    UE_Offsets *pOffsets = GetOffsets();
    if (!pOffsets)
        return UEVarsInitStatus::ERROR_INIT_OFFSETS;

    _UEVars.Offsets = pOffsets;

    _UEVars.NamesPtr = GetNamesPtr();
    if (IsUsingFNamePool())
    {
        if (!PtrValidator.isPtrReadable(_UEVars.NamesPtr))
            return UEVarsInitStatus::ERROR_INIT_NAMEPOOL;
    }
    else
    {
        if (!PtrValidator.isPtrReadable(_UEVars.NamesPtr))
            return UEVarsInitStatus::ERROR_INIT_GNAMES;
    }

    _UEVars.pGetNameByID = [this](int32_t id) -> std::string
    {
        return GetNameByID(id);
    };

    _UEVars.GUObjectsArrayPtr = GetGUObjectArrayPtr();
    if (!PtrValidator.isPtrReadable(_UEVars.GUObjectsArrayPtr))
        return UEVarsInitStatus::ERROR_INIT_GUOBJECTARRAY;

    _UEVars.ObjObjectsPtr = _UEVars.GUObjectsArrayPtr + pOffsets->FUObjectArray.ObjObjects;

    if (!vm_rpm_ptr((void *)(_UEVars.ObjObjectsPtr + pOffsets->TUObjectArray.Objects),
                    &_UEVars.ObjObjects_Objects, sizeof(uintptr_t)))
        return UEVarsInitStatus::ERROR_INIT_OBJOBJECTS;

    UEWrappers::Init(GetUEVars());

    return UEVarsInitStatus::SUCCESS;
}

uint8_t *IGameProfile::GetNameEntry(int32_t id) const
{
    if (id < 0)
        return nullptr;

    uintptr_t namesPtr = _UEVars.GetNamesPtr();
    if (namesPtr == 0)
        return nullptr;

    if (!IsUsingFNamePool())
    {
        const int32_t ElementsPerChunk = 16384;
        const int32_t ChunkIndex = id / ElementsPerChunk;
        const int32_t WithinChunkIndex = id % ElementsPerChunk;

        // FNameEntry**
        uint8_t *FNameEntryArray = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + ChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryArray)
            return nullptr;

        // FNameEntry*
        return vm_rpm_ptr<uint8_t *>(FNameEntryArray + WithinChunkIndex * sizeof(uintptr_t));
    }

    uintptr_t blockBit = GetOffsets()->FNamePool.BlocksBit;
    uintptr_t blocks = GetOffsets()->FNamePool.BlocksOff;
    uintptr_t chunckMask = (1 << blockBit) - 1;
    uintptr_t stride = GetOffsets()->FNamePool.Stride;

    uintptr_t block_offset = ((id >> blockBit) * sizeof(void *));
    uintptr_t chunck_offset = ((id & chunckMask) * stride);

    uint8_t *chunck = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + blocks + block_offset));
    if (!chunck)
        return nullptr;

    return (chunck + chunck_offset);
}

std::string IGameProfile::GetNameEntryString(uint8_t *entry) const
{
    if (!entry)
        return "";

    UE_Offsets *offsets = GetOffsets();

    uint8_t *pStr = nullptr;
    // don't care for now
    // bool isWide = false;
    size_t strLen = 0;
    int strNumber = 0;

    if (!IsUsingFNamePool())
    {
        int32_t name_index = 0;
        if (!vm_rpm_ptr(entry + offsets->FNameEntry.Index, &name_index,
                        sizeof(int32_t)))
            return "";

        pStr = entry + offsets->FNameEntry.Name;
        // isWide = offsets->FNameEntry.GetIsWide(name_index)
        strLen = kMAX_UENAME_BUFFER;
    }
    else
    {
        uint16_t header = 0;
        if (!vm_rpm_ptr(entry + offsets->FNamePoolEntry.Header, &header,
                        sizeof(int16_t)))
            return "";

        if (isUsingOutlineNumberName() &&
            offsets->FNamePoolEntry.GetLength(header) == 0)
        {
            const uintptr_t stringOff =
                offsets->FNamePoolEntry.Header + sizeof(int16_t);
            const uintptr_t entryIdOff = stringOff + ((stringOff == 6) * 2);
            const int32_t nextEntryId = vm_rpm_ptr<int32_t>(entry + entryIdOff);
            if (nextEntryId <= 0)
                return "";

            strNumber = vm_rpm_ptr<int32_t>(entry + entryIdOff + sizeof(int32_t));
            entry = GetNameEntry(nextEntryId);
            if (!vm_rpm_ptr(entry + offsets->FNamePoolEntry.Header, &header,
                            sizeof(int16_t)))
                return "";
        }

        strLen = std::min<size_t>(offsets->FNamePoolEntry.GetLength(header), kMAX_UENAME_BUFFER);
        if (strLen <= 0)
            return "";

        // isWide = offsets->FNamePoolEntry.GetIsWide(header);
        pStr = entry + offsets->FNamePoolEntry.Header + sizeof(int16_t);
    }

    std::string result = vm_rpm_str(pStr, strLen);

    if (strNumber > 0)
        result += '_' + std::to_string(strNumber - 1);

    return result;
}

std::string IGameProfile::GetNameByID(int32_t id) const
{
    return GetNameEntryString(GetNameEntry(id));
}

ElfScanner IGameProfile::GetUnrealEngineELF() const
{
    static const std::vector<std::string> cUELibNames = {"libUE4.so",
                                                         "libUnreal.so"};

    static ElfScanner ue_elf{};
    if (ue_elf.isValid())
        return ue_elf;

    for (const auto &lib : cUELibNames)
    {
        ue_elf = kMgr.getMemElf(lib);
        if (ue_elf.isValid())
            return ue_elf;

        for (auto &it : KittyMemoryEx::getAllMaps(kMgr.processID()))
        {
            if (KittyUtils::String::Contains(it.pathname, kMgr.processName()) &&
                KittyUtils::String::EndsWith(it.pathname, ".apk"))
            {
                ue_elf = kMgr.getMemElfInZip(it.pathname, lib);
                if (ue_elf.isValid())
                    return ue_elf;
            }
        }
    }

    return ue_elf;
}

bool IGameProfile::isEmulator() const
{
    if (!KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm/nb/").empty() ||
        !KittyMemoryEx::getMapsContain(kMgr.processID(), "/arm64/nb/").empty())
        return true;

    for (auto &it : GetUnrealEngineELF().segments())
        if (it.executable)
            return false;

    return true;
}

uintptr_t IGameProfile::findIdaPattern(PATTERN_MAP_TYPE map_type,
                                       const std::string &pattern,
                                       const int step,
                                       uint32_t skip_result) const
{
    ElfScanner ue_elf = GetUnrealEngineELF();
    std::vector<KittyMemoryEx::ProcMap> search_segments;
    bool hasBSS = ue_elf.bss() > 0;

    if (map_type == PATTERN_MAP_TYPE::BSS)
    {
        if (!hasBSS)
            return 0;

        for (auto &it : ue_elf.segments())
            if (it.startAddress >= ue_elf.bss() &&
                it.endAddress <= (ue_elf.bss() + ue_elf.bssSize()))
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

            if (hasBSS && it.startAddress >= ue_elf.bss() &&
                it.endAddress <= (ue_elf.bss() + ue_elf.bssSize()))
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
            auto adr_list = kMgr.memScanner.findIdaPatternAll(it.startAddress,
                                                              it.endAddress, pattern);
            if (adr_list.size() > skip_result)
            {
                insn_address = adr_list[skip_result];
            }
        }
        else
        {
            insn_address = kMgr.memScanner.findIdaPatternFirst(
                it.startAddress, it.endAddress, pattern);
        }
        if (insn_address)
            break;
    }
    return (insn_address ? (insn_address + step) : 0);
}