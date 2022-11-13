#pragma once

#include <types.h>

struct UE_Offsets
{
    uint16 Stride = 0;
    uint16 FNamePoolBlocks = 0;
    uint16 FNameMaxSize = 0;
    struct
    {
        uint16 Number = 0;
    } FName;
    struct
    {
        uint16 Name = 0;
    } FNameEntry;
    struct
    {
        uint16 Info = 0;
        uint16 WideBit = 0;
        uint16 LenBit = 0;
        uint16 HeaderSize = 0;
    } FNameEntry23;
    struct
    {
        uint16 ObjObjects = 0;
    } FUObjectArray;
    struct
    {
        uint16 NumElements = 0;
    } TUObjectArray;
    struct
    {
        uint16 Size = 0;
    } FUObjectItem;
    struct
    {
        uint16 ObjectFlags = 0;
        uint16 InternalIndex = 0;
        uint16 ClassPrivate = 0;
        uint16 NamePrivate = 0;
        uint16 OuterPrivate = 0;
    } UObject;
    struct
    {
        uint16 Next = 0;
    } UField;
    struct
    {
        uint16 SuperStruct = 0;
        uint16 Children = 0;
        uint16 ChildProperties = 0;
        uint16 PropertiesSize = 0;
    } UStruct;
    struct
    {
        uint16 Names = 0;
    } UEnum;
    struct
    {
        uint16 EFunctionFlags = 0;
        uint16 NumParams = 0;
        uint16 ParamSize = 0;
        uint16 Func = 0;
    } UFunction;
    struct
    {
        uint16 ClassPrivate = 0;
        uint16 Next = 0;
        uint16 NamePrivate = 0;
        uint16 FlagsPrivate = 0;
    } FField;
    struct
    {
        uint16 ArrayDim = 0;
        uint16 ElementSize = 0;
        uint16 PropertyFlags = 0;
        uint16 Offset_Internal = 0;
        uint16 Size = 0;
    } FProperty;
    struct
    {
        uint16 ArrayDim = 0;
        uint16 ElementSize = 0;
        uint16 PropertyFlags = 0;
        uint16 Offset_Internal = 0;
        uint16 Size = 0;
    } UProperty;
};