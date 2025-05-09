#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Utils/BufferFmt.h"
#include "GameProfile.h"

namespace Dumper
{

    enum DumpStatus : uint8_t
    {
        UE_DS_NONE = 0,
        UE_DS_SUCCESS = 1,
        UE_DS_ERROR_INVALID_ELF = 2,
        UE_DS_ARCH_NOT_SUPPORTED = 3,
        UE_DS_ERROR_ARCH_MISMATCH = 4,
        UE_DS_ERROR_LIB_INVALID_BASE = 5,
        UE_DS_ERROR_LIB_NOT_FOUND = 6,
        UE_DS_ERROR_IO_OPERATION = 7,
        UE_DS_ERROR_INIT_GNAMES = 8,
        UE_DS_ERROR_INIT_NAMEPOOL = 9,
        UE_DS_ERROR_INIT_GUOBJECTARRAY = 10,
        UE_DS_ERROR_INIT_OBJOBJECTS = 11,
        UE_DS_ERROR_INIT_OFFSETS = 12,
        UE_DS_ERROR_EMPTY_PACKAGES = 13,
    };

    DumpStatus InitUEVars(IGameProfile *profile);

    DumpStatus Dump(std::unordered_map<std::string, BufferFmt> *outBuffersMap);

    std::string DumpStatusToStr(DumpStatus ds);

} // namespace Dumper