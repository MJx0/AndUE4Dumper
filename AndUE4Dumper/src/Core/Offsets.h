#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

struct UE_Offsets
{
    UE_Offsets()
    {
        memset(this, 0, sizeof(UE_Offsets));
    }
    
    uint16_t Stride = 0;
    uint16_t FNamePoolBlocks = 0;
    uint16_t FNameMaxSize = 0;
    struct
    {
        uint16_t Number = 0;
    } FName;
    struct
    {
        uint16_t Name = 0;
    } FNameEntry;
    struct
    {
        uint16_t Header = 0;
        std::function<bool(uint16_t)> GetIsWide = nullptr;
        std::function<size_t(uint16_t)> GetLength = nullptr;
    } FNameEntry23;
    struct
    {
        uint16_t ObjObjects = 0;
    } FUObjectArray;
    struct
    {
        uint16_t Objects = 0;
        uint16_t NumElements = 0;
    } TUObjectArray;
    struct
    {
        uint16_t Object = 0;
        uint16_t Size = 0;
    } FUObjectItem;
    struct
    {
        uint16_t ObjectFlags = 0;
        uint16_t InternalIndex = 0;
        uint16_t ClassPrivate = 0;
        uint16_t NamePrivate = 0;
        uint16_t OuterPrivate = 0;
    } UObject;
    struct
    {
        uint16_t Next = 0;
    } UField;
    struct
    {
        uint16_t SuperStruct = 0;
        uint16_t Children = 0;
        uint16_t ChildProperties = 0;
        uint16_t PropertiesSize = 0;
    } UStruct;
    struct
    {
        uint16_t Names = 0;
    } UEnum;
    struct
    {
        uint16_t EFunctionFlags = 0;
        uint16_t NumParams = 0;
        uint16_t ParamSize = 0;
        uint16_t Func = 0;
    } UFunction;
    struct
    {
        uint16_t ClassPrivate = 0;
        uint16_t Next = 0;
        uint16_t NamePrivate = 0;
        uint16_t FlagsPrivate = 0;
    } FField;
    struct
    {
        uint16_t ArrayDim = 0;
        uint16_t ElementSize = 0;
        uint16_t PropertyFlags = 0;
        uint16_t Offset_Internal = 0;
        uint16_t Size = 0;
    } FProperty;
    struct
    {
        uint16_t ArrayDim = 0;
        uint16_t ElementSize = 0;
        uint16_t PropertyFlags = 0;
        uint16_t Offset_Internal = 0;
        uint16_t Size = 0;
    } UProperty;
};
