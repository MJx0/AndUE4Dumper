#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Utils/BufferFmt.hpp"
#include "Utils/ProgressUtils.hpp"

#include "UE/UEWrappers.hpp"

class UE_UPackage
{
private:
    struct Member
    {
        std::string Type;
        std::string Name;
        std::string extra;  // extra comment
        uint32_t Offset = 0;
        uint32_t Size = 0;
    };
    struct Function
    {
        std::string Name;
        std::string FullName;
        std::string CppName;
        std::string Params;
        uint32_t EFlags = 0;
        std::string Flags;
        int8_t NumParams = 0;
        int16_t ParamSize = 0;
        uintptr_t Func = 0;
    };
    struct Struct
    {
        std::string Name;
        std::string FullName;
        std::string CppName;
        uint32_t Inherited = 0;
        uint32_t Size = 0;
        std::vector<Member> Members;
        std::vector<Function> Functions;
    };
    struct Enum
    {
        std::string FullName;
        std::string CppName;
        std::vector<std::pair<std::string, uint64_t>> Members;
    };

private:
    std::pair<uint8_t *const, std::vector<UE_UObject>> *Package;

public:
    std::vector<Struct> Classes;
    std::vector<Struct> Structures;
    std::vector<Enum> Enums;

private:
    static void GenerateFunction(const UE_UFunction &fn, Function *out);
    static void GenerateStruct(const UE_UStruct &object, std::vector<Struct> &arr);
    static void GenerateEnum(const UE_UEnum &object, std::vector<Enum> &arr);

    static void GenerateBitPadding(std::vector<Member> &members, uint32_t offset, uint8_t bitOffset, uint8_t size);
    static void GeneratePadding(std::vector<Member> &members, uint32_t offset, uint32_t size);
    static void FillPadding(const UE_UStruct &object, std::vector<Member> &members, uint32_t &offset, uint8_t &bitOffset, uint32_t end);

    static void AppendStructsToBuffer(std::vector<Struct> &arr, class BufferFmt *bufFmt);
    static void AppendEnumsToBuffer(std::vector<Enum> &arr, class BufferFmt *bufFmt);

public:
    UE_UPackage(std::pair<uint8_t *const, std::vector<UE_UObject>> &package) : Package(&package) {};
    inline UE_UObject GetObject() const { return UE_UObject(Package->first); }
    void Process();
    bool AppendToBuffer(class BufferFmt *bufferFmt);
};
