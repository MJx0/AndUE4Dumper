#include "wrappers.h"

#include <hash/hash.h>

#include "../Utils/BufferFmt.h"
#include "../Utils/ioutils.h"
#include "../Utils/memory.h"

#include "GameProfile.h"
#include "UE_Offsets.h"

#include <utfcpp/unchecked.h>

namespace UEVars
{
    uintptr_t BaseAddress = 0;

    bool isUsingFNamePool = false;
    bool isUsingOutlineNumberName = false;

    uintptr_t NamePoolDataPtr = 0;
    uintptr_t GNamesPtr = 0;
    uintptr_t ObjObjectsPtr = 0;

    UE_UObjectArray ObjObjects{};

    UE_Offsets Offsets{};

    IGameProfile *Profile = nullptr;
} // namespace UEVars

std::string FString::ToString() const
{
    if (!IsValid()) return "";

    std::wstring wstr = ToWString();
    if (wstr.empty()) return "";

    std::string result;
    utf8::unchecked::utf16to8(wstr.begin(), wstr.end(), std::back_inserter(result));
    return result;
}

int32_t UE_UObjectArray::GetNumElements() const
{
    if (UEVars::ObjObjectsPtr == 0)
        return 0;

    return vm_rpm_ptr<int32_t>((void *)(UEVars::ObjObjectsPtr + UEVars::Offsets.TUObjectArray.NumElements));
}

uint8_t *UE_UObjectArray::GetObjectPtr(int32_t id) const
{
    if (id < 0 || id >= GetNumElements() || !Objects)
        return nullptr;

    if (!UEVars::isUsingFNamePool)
    {
        return vm_rpm_ptr<uint8_t *>((void *)((uintptr_t)Objects + (id * UEVars::Offsets.FUObjectItem.Size) + UEVars::Offsets.FUObjectItem.Object));
    }

    const int32_t NumElementsPerChunk = 64 * 1024;
    const int32_t chunkIndex = id / NumElementsPerChunk;
    const int32_t withinChunkIndex = id % NumElementsPerChunk;

    // if (chunkIndex >= NumChunks) return nullptr;

    uint8_t *chunk = vm_rpm_ptr<uint8_t *>(Objects + chunkIndex);
    if (!chunk)
        return nullptr;

    return vm_rpm_ptr<uint8_t *>(chunk + (withinChunkIndex * UEVars::Offsets.FUObjectItem.Size) + UEVars::Offsets.FUObjectItem.Object);
}

void UE_UObjectArray::ForEachObject(const std::function<bool(uint8_t *)> &callback) const
{
    for (int32_t i = 0; i < GetNumElements(); i++)
    {
        uint8_t *object = GetObjectPtr(i);
        if (!object) continue;

        if (callback(object)) return;
    }
}

void UE_UObjectArray::ForEachObjectOfClass(const UE_UClass &cmp, const std::function<bool(uint8_t *)> &callback) const
{
    for (int32_t i = 0; i < GetNumElements(); i++)
    {
        UE_UObject object = GetObjectPtr(i);
        if (object && object.IsA(cmp) && object.GetName().find("_Default") == std::string::npos)
        {
            if (callback(object)) return;
        }
    }
}

bool UE_UObjectArray::IsObject(const UE_UObject &address) const
{
    for (int32_t i = 0; i < GetNumElements(); i++)
    {
        UE_UObject object = GetObjectPtr(i);
        if (address == object) return true;
    }
    return false;
}

int UE_FName::GetNumber() const
{
    if (!object || UEVars::isUsingOutlineNumberName)
        return 0;

    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.FName.Number);
}

std::string UE_FName::GetName() const
{
    if (!object) return "";

    int32_t index = 0;
    if (!vm_rpm_ptr(object, &index, sizeof(int32_t)) && index < 0)
        return "";

    std::string name = UEVars::Profile->GetNameByID(index);
    if (name.empty()) return "";

    if (!UEVars::isUsingOutlineNumberName)
    {
        int32_t number = GetNumber();
        if (number > 0)
        {
            name += '_' + std::to_string(number - 1);
        }
    }

    auto pos = name.rfind('/');
    if (pos != std::string::npos)
    {
        name = name.substr(pos + 1);
    }

    return name;
}

int32_t UE_UObject::GetIndex() const
{
    if (!object) return -1;

    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.UObject.InternalIndex);
}

UE_UClass UE_UObject::GetClass() const
{
    if (!object) return nullptr;

    return vm_rpm_ptr<UE_UClass>(object + UEVars::Offsets.UObject.ClassPrivate);
}

UE_UObject UE_UObject::GetOuter() const
{
    if (!object) return nullptr;

    return vm_rpm_ptr<UE_UObject>(object + UEVars::Offsets.UObject.OuterPrivate);
}

UE_UObject UE_UObject::GetPackageObject() const
{
    if (!object) return nullptr;

    UE_UObject package(nullptr);
    for (auto outer = GetOuter(); outer; outer = outer.GetOuter())
    {
        package = outer;
    }
    return package;
}

std::string UE_UObject::GetName() const
{
    if (!object) return "";

    auto fname = UE_FName(object + UEVars::Offsets.UObject.NamePrivate);
    return fname.GetName();
}

std::string UE_UObject::GetFullName() const
{
    if (!object) return "";

    std::string temp;
    for (auto outer = GetOuter(); outer; outer = outer.GetOuter())
    {
        temp = outer.GetName() + "." + temp;
    }
    UE_UClass objectClass = GetClass();
    std::string name = objectClass.GetName() + " " + temp + GetName();
    return name;
}

std::string UE_UObject::GetCppName() const
{
    if (!object) return "";

    std::string name;
    if (IsA<UE_UClass>())
    {
        for (auto c = Cast<UE_UStruct>(); c; c = c.GetSuper())
        {
            if (c == UE_AActor::StaticClass())
            {
                name = "A";
                break;
            }
            else if (c == UE_UObject::StaticClass())
            {
                name = "U";
                break;
            }
            else if (c == UE_UInterface::StaticClass())
            {
                name = "I";
                break;
            }
        }
    }
    else
    {
        name = "F";
    }

    name += GetName();
    return name;
}

bool UE_UObject::IsA(UE_UClass cmp) const
{
    if (!object) return false;

    for (auto super = GetClass(); super; super = super.GetSuper().Cast<UE_UClass>())
    {
        if (super == cmp)
        {
            return true;
        }
    }

    return false;
}

UE_UClass UE_UObject::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Object");
    return obj;
}

UE_UClass UE_UInterface::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Interface");
    return obj;
}

UE_UClass UE_AActor::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class Engine.Actor");
    return obj;
}

UE_UField UE_UField::GetNext() const
{
    if (!object)
        return nullptr;

    return vm_rpm_ptr<UE_UField>(object + UEVars::Offsets.UField.Next);
}

UE_UClass UE_UField::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Field");
    return obj;
}

std::string IUProperty::GetName() const
{
    return ((UE_UProperty *)(this->prop))->GetName();
}

int32_t IUProperty::GetArrayDim() const
{
    return ((UE_UProperty *)(this->prop))->GetArrayDim();
}

int32_t IUProperty::GetSize() const
{
    return ((UE_UProperty *)(this->prop))->GetSize();
}

int32_t IUProperty::GetOffset() const
{
    return ((UE_UProperty *)(this->prop))->GetOffset();
}

uint64_t IUProperty::GetPropertyFlags() const
{
    return ((UE_UProperty *)(this->prop))->GetPropertyFlags();
}

std::pair<PropertyType, std::string> IUProperty::GetType() const
{
    return ((UE_UProperty *)(this->prop))->GetType();
}

uint8_t IUProperty::GetFieldMask() const
{
    return ((UE_UBoolProperty *)(this->prop))->GetFieldMask();
}

int32_t UE_UProperty::GetArrayDim() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.UProperty.ArrayDim);
}

int32_t UE_UProperty::GetSize() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.UProperty.ElementSize);
}

int32_t UE_UProperty::GetOffset() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.UProperty.Offset_Internal);
}

uint64_t UE_UProperty::GetPropertyFlags() const
{
    return vm_rpm_ptr<uint64_t>(object + UEVars::Offsets.UProperty.PropertyFlags);
}

std::pair<PropertyType, std::string> UE_UProperty::GetType() const
{
    if (IsA<UE_UDoubleProperty>())
    {
        return {PropertyType::DoubleProperty, Cast<UE_UDoubleProperty>().GetTypeStr()};
    }
    if (IsA<UE_UFloatProperty>())
    {
        return {PropertyType::FloatProperty, Cast<UE_UFloatProperty>().GetTypeStr()};
    }
    if (IsA<UE_UIntProperty>())
    {
        return {PropertyType::IntProperty, Cast<UE_UIntProperty>().GetTypeStr()};
    }
    if (IsA<UE_UInt16Property>())
    {
        return {PropertyType::Int16Property, Cast<UE_UInt16Property>().GetTypeStr()};
    }
    if (IsA<UE_UInt32Property>())
    {
        return {PropertyType::Int32Property, Cast<UE_UInt32Property>().GetTypeStr()};
    }
    if (IsA<UE_UInt64Property>())
    {
        return {PropertyType::Int64Property, Cast<UE_UInt64Property>().GetTypeStr()};
    }
    if (IsA<UE_UInt8Property>())
    {
        return {PropertyType::Int8Property, Cast<UE_UInt8Property>().GetTypeStr()};
    }
    if (IsA<UE_UUInt16Property>())
    {
        return {PropertyType::UInt16Property, Cast<UE_UUInt16Property>().GetTypeStr()};
    }
    if (IsA<UE_UUInt32Property>())
    {
        return {PropertyType::UInt32Property, Cast<UE_UUInt32Property>().GetTypeStr()};
    }
    if (IsA<UE_UUInt64Property>())
    {
        return {PropertyType::UInt64Property, Cast<UE_UUInt64Property>().GetTypeStr()};
    }
    if (IsA<UE_UTextProperty>())
    {
        return {PropertyType::TextProperty, Cast<UE_UTextProperty>().GetTypeStr()};
    }
    if (IsA<UE_UStrProperty>())
    {
        return {PropertyType::TextProperty, Cast<UE_UStrProperty>().GetTypeStr()};
    }
    if (IsA<UE_UClassProperty>())
    {
        return {PropertyType::ClassProperty, Cast<UE_UClassProperty>().GetTypeStr()};
    }
    if (IsA<UE_UStructProperty>())
    {
        return {PropertyType::StructProperty, Cast<UE_UStructProperty>().GetTypeStr()};
    }
    if (IsA<UE_UNameProperty>())
    {
        return {PropertyType::NameProperty, Cast<UE_UNameProperty>().GetTypeStr()};
    }
    if (IsA<UE_UBoolProperty>())
    {
        return {PropertyType::BoolProperty, Cast<UE_UBoolProperty>().GetTypeStr()};
    }
    if (IsA<UE_UByteProperty>())
    {
        return {PropertyType::ByteProperty, Cast<UE_UByteProperty>().GetTypeStr()};
    }
    if (IsA<UE_UArrayProperty>())
    {
        return {PropertyType::ArrayProperty, Cast<UE_UArrayProperty>().GetTypeStr()};
    }
    if (IsA<UE_UEnumProperty>())
    {
        return {PropertyType::EnumProperty, Cast<UE_UEnumProperty>().GetTypeStr()};
    }
    if (IsA<UE_USetProperty>())
    {
        return {PropertyType::SetProperty, Cast<UE_USetProperty>().GetTypeStr()};
    }
    if (IsA<UE_UMapProperty>())
    {
        return {PropertyType::MapProperty, Cast<UE_UMapProperty>().GetTypeStr()};
    }
    if (IsA<UE_UInterfaceProperty>())
    {
        return {PropertyType::InterfaceProperty, Cast<UE_UInterfaceProperty>().GetTypeStr()};
    }
    if (IsA<UE_UMulticastDelegateProperty>())
    {
        return {PropertyType::MulticastDelegateProperty, Cast<UE_UMulticastDelegateProperty>().GetTypeStr()};
    }
    if (IsA<UE_UWeakObjectProperty>())
    {
        return {PropertyType::WeakObjectProperty, Cast<UE_UWeakObjectProperty>().GetTypeStr()};
    }
    if (IsA<UE_ULazyObjectProperty>())
    {
        return {PropertyType::LazyObjectProperty, Cast<UE_ULazyObjectProperty>().GetTypeStr()};
    }
    if (IsA<UE_UObjectProperty>())
    {
        return {PropertyType::ObjectProperty, Cast<UE_UObjectProperty>().GetTypeStr()};
    }
    if (IsA<UE_UObjectPropertyBase>())
    {
        return {PropertyType::ObjectProperty, Cast<UE_UObjectPropertyBase>().GetTypeStr()};
    }
    return {PropertyType::Unknown, GetClass().GetName()};
}

IUProperty UE_UProperty::GetInterface() const { return IUProperty(this); }

UE_UClass UE_UProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Property");
    return obj;
}

UE_UStruct UE_UStruct::GetSuper() const
{
    return vm_rpm_ptr<UE_UStruct>(object + UEVars::Offsets.UStruct.SuperStruct);
}

UE_FField UE_UStruct::GetChildProperties() const
{
    if (UEVars::Offsets.UStruct.ChildProperties > 0)
    {
        return vm_rpm_ptr<UE_FField>(object + UEVars::Offsets.UStruct.ChildProperties);
    }
    return {};
}

UE_UField UE_UStruct::GetChildren() const
{
    if (UEVars::Offsets.UStruct.Children > 0)
    {
        return vm_rpm_ptr<UE_UField>(object + UEVars::Offsets.UStruct.Children);
    }
    return {};
}

int32_t UE_UStruct::GetSize() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.UStruct.PropertiesSize);
}

UE_UClass UE_UStruct::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Struct");
    return obj;
}

UE_FField UE_UStruct::FindChildProp(const std::string &name) const
{
    for (auto prop = GetChildProperties(); prop; prop = prop.GetNext())
    {
        if (prop.GetName() == name)
            return prop;
    }
    return {};
}

UE_UField UE_UStruct::FindChild(const std::string &name) const
{
    for (auto prop = GetChildren(); prop; prop = prop.GetNext())
    {
        if (prop.GetName() == name)
            return prop;
    }
    return {};
}

uintptr_t UE_UFunction::GetFunc() const
{
    return vm_rpm_ptr<uintptr_t>(object + UEVars::Offsets.UFunction.Func);
}

int8_t UE_UFunction::GetNumParams() const
{
    return vm_rpm_ptr<int8_t>(object + UEVars::Offsets.UFunction.NumParams);
}

int16_t UE_UFunction::GetParamSize() const
{
    return vm_rpm_ptr<int16_t>(object + UEVars::Offsets.UFunction.ParamSize);
}

uint32_t UE_UFunction::GetFunctionEFlags() const
{
    return vm_rpm_ptr<uint32_t>(object + UEVars::Offsets.UFunction.EFunctionFlags);
}

std::string UE_UFunction::GetFunctionFlags() const
{
    auto flags = GetFunctionEFlags();
    std::string result;
    if (flags == FUNC_None)
    {
        result = "None";
    }
    else
    {
        if (flags & FUNC_Final)
        {
            result += "Final|";
        }
        if (flags & FUNC_RequiredAPI)
        {
            result += "RequiredAPI|";
        }
        if (flags & FUNC_BlueprintAuthorityOnly)
        {
            result += "BlueprintAuthorityOnly|";
        }
        if (flags & FUNC_BlueprintCosmetic)
        {
            result += "BlueprintCosmetic|";
        }
        if (flags & FUNC_Net)
        {
            result += "Net|";
        }
        if (flags & FUNC_NetReliable)
        {
            result += "NetReliable";
        }
        if (flags & FUNC_NetRequest)
        {
            result += "NetRequest|";
        }
        if (flags & FUNC_Exec)
        {
            result += "Exec|";
        }
        if (flags & FUNC_Native)
        {
            result += "Native|";
        }
        if (flags & FUNC_Event)
        {
            result += "Event|";
        }
        if (flags & FUNC_NetResponse)
        {
            result += "NetResponse|";
        }
        if (flags & FUNC_Static)
        {
            result += "Static|";
        }
        if (flags & FUNC_NetMulticast)
        {
            result += "NetMulticast|";
        }
        if (flags & FUNC_UbergraphFunction)
        {
            result += "UbergraphFunction|";
        }
        if (flags & FUNC_MulticastDelegate)
        {
            result += "MulticastDelegate|";
        }
        if (flags & FUNC_Public)
        {
            result += "Public|";
        }
        if (flags & FUNC_Private)
        {
            result += "Private|";
        }
        if (flags & FUNC_Protected)
        {
            result += "Protected|";
        }
        if (flags & FUNC_Delegate)
        {
            result += "Delegate|";
        }
        if (flags & FUNC_NetServer)
        {
            result += "NetServer|";
        }
        if (flags & FUNC_HasOutParms)
        {
            result += "HasOutParms|";
        }
        if (flags & FUNC_HasDefaults)
        {
            result += "HasDefaults|";
        }
        if (flags & FUNC_NetClient)
        {
            result += "NetClient|";
        }
        if (flags & FUNC_DLLImport)
        {
            result += "DLLImport|";
        }
        if (flags & FUNC_BlueprintCallable)
        {
            result += "BlueprintCallable|";
        }
        if (flags & FUNC_BlueprintEvent)
        {
            result += "BlueprintEvent|";
        }
        if (flags & FUNC_BlueprintPure)
        {
            result += "BlueprintPure|";
        }
        if (flags & FUNC_EditorOnly)
        {
            result += "EditorOnly|";
        }
        if (flags & FUNC_Const)
        {
            result += "Const|";
        }
        if (flags & FUNC_NetValidate)
        {
            result += "NetValidate|";
        }
        if (result.size())
        {
            result.erase(result.size() - 1);
        }
    }
    return result;
}

UE_UClass UE_UFunction::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Function");
    return obj;
}

UE_UClass UE_UScriptStruct::StaticClass()
{
    static UE_UClass obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ScriptStruct");
    return obj;
}

UE_UClass UE_UClass::StaticClass()
{
    static UE_UClass obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Class");
    return obj;
}

TArray<uint8_t> UE_UEnum::GetNames() const
{
    return vm_rpm_ptr<TArray<uint8_t>>(object + UEVars::Offsets.UEnum.Names);
}

std::string UE_UEnum::GetName() const
{
    std::string name = UE_UField::GetName();
    if (!name.empty() && name[0] != 'E')
        return "E" + name;
    return name;
}

UE_UClass UE_UEnum::StaticClass()
{
    static UE_UClass obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Enum");
    return obj;
}

std::string UE_UDoubleProperty::GetTypeStr() const { return "double"; }

UE_UClass UE_UDoubleProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.DoubleProperty");
    return obj;
}

UE_UStruct UE_UStructProperty::GetStruct() const
{
    return vm_rpm_ptr<UE_UStruct>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UStructProperty::GetTypeStr() const
{
    return "struct " + GetStruct().GetCppName();
}

UE_UClass UE_UStructProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.StructProperty");
    return obj;
}

std::string UE_UNameProperty::GetTypeStr() const { return "struct FName"; }

UE_UClass UE_UNameProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.NameProperty");
    return obj;
}

UE_UClass UE_UObjectPropertyBase::GetPropertyClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UObjectPropertyBase::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_UClass UE_UObjectPropertyBase::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ObjectPropertyBase");
    return obj;
}

UE_UClass UE_UObjectProperty::GetPropertyClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UObjectProperty::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_UClass UE_UObjectProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ObjectProperty");
    return obj;
}

UE_UProperty UE_UArrayProperty::GetInner() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UArrayProperty::GetTypeStr() const
{
    return "struct TArray<" + GetInner().GetType().second + ">";
}

UE_UClass UE_UArrayProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ArrayProperty");
    return obj;
}

UE_UEnum UE_UByteProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UEnum>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UByteProperty::GetTypeStr() const
{
    auto e = GetEnum();
    if (e)
        return "enum class " + e.GetName();
    return "uint8_t";
}

UE_UClass UE_UByteProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ByteProperty");
    return obj;
}

uint8_t UE_UBoolProperty::GetFieldSize() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.UProperty.Size);
}

uint8_t UE_UBoolProperty::GetByteOffset() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.UProperty.Size + 1);
}

uint8_t UE_UBoolProperty::GetByteMask() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.UProperty.Size + 2);
}

uint8_t UE_UBoolProperty::GetFieldMask() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.UProperty.Size + 3);
}

std::string UE_UBoolProperty::GetTypeStr() const
{
    if (GetFieldMask() == 0xFF)
    {
        return "bool";
    }
    return "uint8_t";
}

UE_UClass UE_UBoolProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.BoolProperty");
    return obj;
}

std::string UE_UFloatProperty::GetTypeStr() const { return "float"; }

UE_UClass UE_UFloatProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.FloatProperty");
    return obj;
}

std::string UE_UIntProperty::GetTypeStr() const { return "int"; }

UE_UClass UE_UIntProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.IntProperty");
    return obj;
}

std::string UE_UInt16Property::GetTypeStr() const { return "int16_t"; }

UE_UClass UE_UInt16Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Int16Property");
    return obj;
}

std::string UE_UInt64Property::GetTypeStr() const { return "int64_t"; }

UE_UClass UE_UInt64Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Int64Property");
    return obj;
}

std::string UE_UInt8Property::GetTypeStr() const { return "uint8_t"; }

UE_UClass UE_UInt8Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Int8Property");
    return obj;
}

std::string UE_UUInt16Property::GetTypeStr() const { return "uint16_t"; }

UE_UClass UE_UUInt16Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.UInt16Property");
    return obj;
}

std::string UE_UUInt32Property::GetTypeStr() const { return "uint32_t"; }

UE_UClass UE_UUInt32Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.UInt32Property");
    return obj;
}

std::string UE_UInt32Property::GetTypeStr() const { return "int32_t"; }

UE_UClass UE_UInt32Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.Int32Property");
    return obj;
}

std::string UE_UUInt64Property::GetTypeStr() const { return "uint64_t"; }

UE_UClass UE_UUInt64Property::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.UInt64Property");
    return obj;
}

std::string UE_UTextProperty::GetTypeStr() const { return "struct FText"; }

UE_UClass UE_UTextProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.TextProperty");
    return obj;
}

std::string UE_UStrProperty::GetTypeStr() const { return "struct FString"; }

UE_UClass UE_UStrProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.StrProperty");
    return obj;
}

UE_UProperty UE_UEnumProperty::GetUnderlayingProperty() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size);
}

UE_UEnum UE_UEnumProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UEnum>(object + UEVars::Offsets.UProperty.Size + sizeof(void *));
}

std::string UE_UEnumProperty::GetTypeStr() const
{
    if (GetEnum())
        return "enum class " + GetEnum().GetName();

    return GetUnderlayingProperty().GetType().second;
}

UE_UClass UE_UEnumProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.EnumProperty");
    return obj;
}

UE_UClass UE_UClassProperty::GetMetaClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + UEVars::Offsets.UProperty.Size + sizeof(void *));
}

std::string UE_UClassProperty::GetTypeStr() const
{
    return "struct " + GetMetaClass().GetCppName() + "*";
}

UE_UClass UE_UClassProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.ClassProperty");
    return obj;
}

std::string UE_USoftClassProperty::GetTypeStr() const
{
    auto className = GetMetaClass() ? GetMetaClass().GetCppName() : GetPropertyClass().GetCppName();
    return "struct TSoftClassPtr<struct " + className + "*>";
}

UE_UProperty UE_USetProperty::GetElementProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_USetProperty::GetTypeStr() const
{
    return "struct TSet<" + GetElementProp().GetType().second + ">";
}

UE_UClass UE_USetProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.SetProperty");
    return obj;
}

UE_UProperty UE_UMapProperty::GetKeyProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size);
}

UE_UProperty UE_UMapProperty::GetValueProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size + sizeof(void *));
}

std::string UE_UMapProperty::GetTypeStr() const
{
    return "struct TMap<" + GetKeyProp().GetType().second + ", " + GetValueProp().GetType().second + ">";
}

UE_UClass UE_UMapProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.MapProperty");
    return obj;
}

UE_UProperty UE_UInterfaceProperty::GetInterfaceClass() const
{
    return vm_rpm_ptr<UE_UProperty>(object + UEVars::Offsets.UProperty.Size);
}

std::string UE_UInterfaceProperty::GetTypeStr() const
{
    return "struct TScriptInterface<" + GetInterfaceClass().GetType().second + ">";
}

UE_UClass UE_UInterfaceProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.InterfaceProperty");
    return obj;
}

std::string UE_UMulticastDelegateProperty::GetTypeStr() const
{
    return "struct FScriptMulticastDelegate";
}

UE_UClass UE_UMulticastDelegateProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.MulticastDelegateProperty");
    return obj;
}

std::string UE_UWeakObjectProperty::GetTypeStr() const
{
    return "struct TWeakObjectPtr<" + this->Cast<UE_UStructProperty>().GetTypeStr() + ">";
}

UE_UClass UE_UWeakObjectProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.WeakObjectProperty");
    return obj;
}

std::string UE_ULazyObjectProperty::GetTypeStr() const
{
    return "struct TLazyObjectPtr<" + this->Cast<UE_UStructProperty>().GetTypeStr() + ">";
}

UE_UClass UE_ULazyObjectProperty::StaticClass()
{
    static auto obj = UEVars::ObjObjects.FindObject<UE_UClass>("Class CoreUObject.LazyObjectProperty");
    return obj;
}

std::string UE_FFieldClass::GetName() const
{
    auto name = UE_FName(object);
    return name.GetName();
}

UE_FField UE_FField::GetNext() const
{
    return vm_rpm_ptr<UE_FField>(object + UEVars::Offsets.FField.Next);
}

std::string UE_FField::GetName() const
{
    auto name = UE_FName(object + UEVars::Offsets.FField.NamePrivate);
    return name.GetName();
}

UE_FFieldClass UE_FField::GetClass() const
{
    return vm_rpm_ptr<UE_FFieldClass>(object + UEVars::Offsets.FField.ClassPrivate);
}

std::string IFProperty::GetName() const
{
    return ((UE_FProperty *)prop)->GetName();
}

int32_t IFProperty::GetArrayDim() const
{
    return ((UE_FProperty *)prop)->GetArrayDim();
}

int32_t IFProperty::GetSize() const { return ((UE_FProperty *)prop)->GetSize(); }

int32_t IFProperty::GetOffset() const
{
    return ((UE_FProperty *)prop)->GetOffset();
}

uint64_t IFProperty::GetPropertyFlags() const
{
    return ((UE_FProperty *)prop)->GetPropertyFlags();
}

std::pair<PropertyType, std::string> IFProperty::GetType() const
{
    return ((UE_FProperty *)prop)->GetType();
}

uint8_t IFProperty::GetFieldMask() const
{
    return ((UE_FBoolProperty *)prop)->GetFieldMask();
}

int32_t UE_FProperty::GetArrayDim() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.FProperty.ArrayDim);
}

int32_t UE_FProperty::GetSize() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.FProperty.ElementSize);
}

int32_t UE_FProperty::GetOffset() const
{
    return vm_rpm_ptr<int32_t>(object + UEVars::Offsets.FProperty.Offset_Internal);
}

uint64_t UE_FProperty::GetPropertyFlags() const
{
    return vm_rpm_ptr<uint64_t>(object + UEVars::Offsets.FProperty.PropertyFlags);
}

PropTypeInfo UE_FProperty::GetType() const
{
    auto objectClass = GetClass();
    PropTypeInfo type = {PropertyType::Unknown, objectClass.GetName()};

    auto &str = type.second;
    auto hash = Hash(str.c_str(), str.size());
    switch (hash)
    {
    case HASH("StructProperty"):
    {
        auto obj = this->Cast<UE_FStructProperty>();
        type = {PropertyType::StructProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("ObjectProperty"):
    {
        auto obj = this->Cast<UE_FObjectPropertyBase>();
        type = {PropertyType::ObjectProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("SoftObjectProperty"):
    {
        auto obj = this->Cast<UE_FObjectPropertyBase>();
        type = {PropertyType::SoftObjectProperty, "struct TSoftObjectPtr<" + obj.GetPropertyClass().GetCppName() + ">"};
        break;
    }
    case HASH("FloatProperty"):
    {
        type = {PropertyType::FloatProperty, "float"};
        break;
    }
    case HASH("ByteProperty"):
    {
        auto obj = this->Cast<UE_FByteProperty>();
        type = {PropertyType::ByteProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("BoolProperty"):
    {
        auto obj = this->Cast<UE_FBoolProperty>();
        type = {PropertyType::BoolProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("IntProperty"):
    {
        type = {PropertyType::IntProperty, "int32_t"};
        break;
    }
    case HASH("Int8Property"):
    {
        type = {PropertyType::Int8Property, "int8_t"};
        break;
    }
    case HASH("Int16Property"):
    {
        type = {PropertyType::Int16Property, "int16_t"};
        break;
    }
    case HASH("Int64Property"):
    {
        type = {PropertyType::Int64Property, "int64_t"};
        break;
    }
    case HASH("UInt16Property"):
    {
        type = {PropertyType::UInt16Property, "uint16_t"};
        break;
    }
    case HASH("Int32Property"):
    {
        type = {PropertyType::Int32Property, "int32_t"};
        break;
    }
    case HASH("UInt32Property"):
    {
        type = {PropertyType::UInt32Property, "uint32_t"};
        break;
    }
    case HASH("UInt64Property"):
    {
        type = {PropertyType::UInt64Property, "uint64_t"};
        break;
    }
    case HASH("NameProperty"):
    {
        type = {PropertyType::NameProperty, "struct FName"};
        break;
    }
    case HASH("DelegateProperty"):
    {
        type = {PropertyType::DelegateProperty, "struct FDelegate"};
        break;
    }
    case HASH("SetProperty"):
    {
        auto obj = this->Cast<UE_FSetProperty>();
        type = {PropertyType::SetProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("ArrayProperty"):
    {
        auto obj = this->Cast<UE_FArrayProperty>();
        type = {PropertyType::ArrayProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("WeakObjectProperty"):
    {
        auto obj = this->Cast<UE_FStructProperty>();
        type = {PropertyType::WeakObjectProperty, "struct TWeakObjectPtr<" + obj.GetTypeStr() + ">"};
        break;
    }
    case HASH("LazyObjectProperty"):
    {
        auto obj = this->Cast<UE_FStructProperty>();
        type = {PropertyType::LazyObjectProperty, "struct TLazyObjectPtr<" + obj.GetTypeStr() + ">"};
        break;
    }
    case HASH("StrProperty"):
    {
        type = {PropertyType::StrProperty, "struct FString"};
        break;
    }
    case HASH("TextProperty"):
    {
        type = {PropertyType::TextProperty, "struct FText"};
        break;
    }
    case HASH("MulticastSparseDelegateProperty"):
    {
        type = {PropertyType::MulticastSparseDelegateProperty, "struct FMulticastSparseDelegate"};
        break;
    }
    case HASH("EnumProperty"):
    {
        auto obj = this->Cast<UE_FEnumProperty>();
        type = {PropertyType::EnumProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("DoubleProperty"):
    {
        type = {PropertyType::DoubleProperty, "double"};
        break;
    }
    case HASH("MulticastDelegateProperty"):
    {
        type = {PropertyType::MulticastDelegateProperty, "FMulticastDelegate"};
        break;
    }
    case HASH("ClassProperty"):
    {
        auto obj = this->Cast<UE_FClassProperty>();
        type = {PropertyType::ClassProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("MulticastInlineDelegateProperty"):
    {
        type = {PropertyType::MulticastDelegateProperty, "struct FMulticastInlineDelegate"};
        break;
    }
    case HASH("MapProperty"):
    {
        auto obj = this->Cast<UE_FMapProperty>();
        type = {PropertyType::MapProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("InterfaceProperty"):
    {
        auto obj = this->Cast<UE_FInterfaceProperty>();
        type = {PropertyType::InterfaceProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("FieldPathProperty"):
    {
        auto obj = this->Cast<UE_FFieldPathProperty>();
        type = {PropertyType::FieldPathProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("SoftClassProperty"):
    {
        auto obj = this->Cast<UE_FSoftClassProperty>();
        type = {PropertyType::SoftClassProperty, obj.GetTypeStr()};
        break;
    }
    }

    return type;
}

IFProperty UE_FProperty::GetInterface() const { return IFProperty(this); }

uint32_t UE_FProperty::FindSubFPropertyBaseOffset() const
{
    uint32_t offset = 0;
    uintptr_t temp = 0;
    if (vm_rpm_ptr(object + UEVars::Offsets.FProperty.Size, &temp, sizeof(uintptr_t)) && temp > 0xFFFF)
    {
        offset = UEVars::Offsets.FProperty.Size;
    }
    else if (vm_rpm_ptr(object + UEVars::Offsets.FProperty.Size + sizeof(void *), &temp, sizeof(uintptr_t)) && temp > 0xFFFF)
    {
        offset = UEVars::Offsets.FProperty.Size + sizeof(void *);
    }
    return offset;
}

UE_UStruct UE_FStructProperty::GetStruct() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_UStruct>(object + offset) : UE_UStruct();
}

std::string UE_FStructProperty::GetTypeStr() const
{
    return "struct " + GetStruct().GetCppName();
}

UE_UClass UE_FObjectPropertyBase::GetPropertyClass() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_UClass>(object + offset) : UE_UClass();
}

std::string UE_FObjectPropertyBase::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_FProperty UE_FArrayProperty::GetInner() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_FProperty>(object + offset) : UE_FProperty();
}

std::string UE_FArrayProperty::GetTypeStr() const
{
    return "struct TArray<" + GetInner().GetType().second + ">";
}

UE_UEnum UE_FByteProperty::GetEnum() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_UEnum>(object + offset) : UE_UEnum();
}

std::string UE_FByteProperty::GetTypeStr() const
{
    auto e = GetEnum();
    if (e)
        return "enum class " + e.GetName();
    return "uint8_t";
}

uint8_t UE_FBoolProperty::GetFieldSize() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.FProperty.Size);
}

uint8_t UE_FBoolProperty::GetByteOffset() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.FProperty.Size + 1);
}

uint8_t UE_FBoolProperty::GetByteMask() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.FProperty.Size + 2);
}

uint8_t UE_FBoolProperty::GetFieldMask() const
{
    return vm_rpm_ptr<uint8_t>(object + UEVars::Offsets.FProperty.Size + 3);
}

std::string UE_FBoolProperty::GetTypeStr() const
{
    if (GetFieldMask() == 0xFF)
    {
        return "bool";
    }
    return "uint8_t";
}

UE_FProperty UE_FEnumProperty::GetUnderlayingProperty() const
{
    static uint32_t off = 0;
    if (off == 0)
    {
        auto p = vm_rpm_ptr<UE_FProperty>(object + UEVars::Offsets.FProperty.Size);
        if (p && p.GetName() == "UnderlyingType")
        {
            off = UEVars::Offsets.FProperty.Size;
        }
        else
        {
            p = vm_rpm_ptr<UE_FProperty>(object + UEVars::Offsets.FProperty.Size - sizeof(void *));
            if (p && p.GetName() == "UnderlyingType")
            {
                off = UEVars::Offsets.FProperty.Size + sizeof(void *);
            }
        }
        return off == 0 ? nullptr : p;
    }

    return vm_rpm_ptr<UE_FProperty>(object + off);
}

UE_UEnum UE_FEnumProperty::GetEnum() const
{
    static uint32_t off = 0;
    if (off == 0)
    {
        auto e = vm_rpm_ptr<UE_UEnum>(object + UEVars::Offsets.FProperty.Size + sizeof(void *));
        if (e && e.IsA<UE_UEnum>())
        {
            off = UEVars::Offsets.FProperty.Size + sizeof(void *);
        }
        else
        {
            e = vm_rpm_ptr<UE_UEnum>(object + UEVars::Offsets.FProperty.Size);
            if (e && e.IsA<UE_UEnum>())
            {
                off = UEVars::Offsets.FProperty.Size + (sizeof(void *) * 2);
            }
        }
        return off == 0 ? nullptr : e;
    }

    return vm_rpm_ptr<UE_UEnum>(object + off);
}

std::string UE_FEnumProperty::GetTypeStr() const
{
    if (GetEnum())
        return "enum class " + GetEnum().GetName();

    return GetUnderlayingProperty().GetType().second;
}

UE_UClass UE_FClassProperty::GetMetaClass() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_UClass>(object + offset + sizeof(void *)) : UE_UClass();
}

std::string UE_FClassProperty::GetTypeStr() const
{
    return "struct " + GetMetaClass().GetCppName() + "*";
}

std::string UE_FSoftClassProperty::GetTypeStr() const
{
    auto className = GetMetaClass() ? GetMetaClass().GetCppName() : GetPropertyClass().GetCppName();
    return "struct TSoftClassPtr<struct " + className + "*>";
}

UE_FProperty UE_FSetProperty::GetElementProp() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_FProperty>(object + offset) : UE_FProperty();
}

std::string UE_FSetProperty::GetTypeStr() const
{
    return "struct TSet<" + GetElementProp().GetType().second + ">";
}

UE_FProperty UE_FMapProperty::GetKeyProp() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_FProperty>(object + offset) : UE_FProperty();
}

UE_FProperty UE_FMapProperty::GetValueProp() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_FProperty>(object + offset + sizeof(void *)) : UE_FProperty();
}

std::string UE_FMapProperty::GetTypeStr() const
{
    return "struct TMap<" + GetKeyProp().GetType().second + ", " + GetValueProp().GetType().second + ">";
}

UE_UClass UE_FInterfaceProperty::GetInterfaceClass() const
{
    uint32_t offset = FindSubFPropertyBaseOffset();
    return offset ? vm_rpm_ptr<UE_UClass>(object + offset) : UE_UClass();
}

std::string UE_FInterfaceProperty::GetTypeStr() const
{
    return "struct TScriptInterface<I" + GetInterfaceClass().GetName() + ">";
}

UE_FName UE_FFieldPathProperty::GetPropertyName() const
{
    return vm_rpm_ptr<UE_FName>(object + UEVars::Offsets.FProperty.Size);
}

std::string UE_FFieldPathProperty::GetTypeStr() const
{
    return "struct TFieldPath<F" + GetPropertyName().GetName() + ">";
}

void UE_UPackage::GenerateBitPadding(std::vector<Member> &members, uint32_t offset, uint8_t bitOffset, uint8_t size)
{
    Member padding;
    padding.Type = "uint8_t";
    padding.Name = fmt::format("BitPad_0x{:X}_{} : {}", offset, bitOffset, size);
    padding.Offset = offset;
    padding.Size = 1;
    members.push_back(padding);
}

void UE_UPackage::GeneratePadding(std::vector<Member> &members, uint32_t offset, uint32_t size)
{
    Member padding;
    padding.Type = "uint8_t";
    padding.Name = fmt::format("Pad_0x{:X}[0x{:X}]", offset, size);
    padding.Offset = offset;
    padding.Size = size;
    members.push_back(padding);
}

void UE_UPackage::FillPadding(const UE_UStruct &object, std::vector<Member> &members, uint32_t &offset, uint8_t &bitOffset, uint32_t end)
{
    (void)object;

    if (bitOffset && bitOffset < 8)
    {
        UE_UPackage::GenerateBitPadding(members, offset, bitOffset, 8 - bitOffset);
        bitOffset = 0;
        offset++;
    }

    if (offset != end)
    {
        GeneratePadding(members, offset, end - offset);
        offset = end;
    }
}

void UE_UPackage::GenerateFunction(const UE_UFunction &fn, Function *out)
{
    out->Name = fn.GetName();
    out->FullName = fn.GetFullName();
    out->EFlags = fn.GetFunctionEFlags();
    out->Flags = fn.GetFunctionFlags();
    out->NumParams = fn.GetNumParams();
    out->ParamSize = fn.GetParamSize();
    out->Func = fn.GetFunc();

    auto generateParam = [&](IProperty *prop)
    {
        auto flags = prop->GetPropertyFlags();

        // if property has 'ReturnParm' flag
        if (flags & CPF_ReturnParm)
        {
            out->CppName = prop->GetType().second + " " + fn.GetName();
        }
        // if property has 'Parm' flag
        else if (flags & CPF_Parm)
        {
            if (prop->GetArrayDim() > 1)
            {
                out->Params += fmt::format("{}* {}, ", prop->GetType().second, prop->GetName());
            }
            else
            {
                if (flags & CPF_OutParm)
                {
                    out->Params += fmt::format("{}& {}, ", prop->GetType().second, prop->GetName());
                }
                else
                {
                    out->Params += fmt::format("{} {}, ", prop->GetType().second, prop->GetName());
                }
            }
        }
    };

    for (auto prop = fn.GetChildProperties().Cast<UE_FProperty>(); prop; prop = prop.GetNext().Cast<UE_FProperty>())
    {
        auto propInterface = prop.GetInterface();
        generateParam(&propInterface);
    }
    for (auto prop = fn.GetChildren().Cast<UE_UProperty>(); prop; prop = prop.GetNext().Cast<UE_UProperty>())
    {
        auto propInterface = prop.GetInterface();
        generateParam(&propInterface);
    }
    if (out->Params.size())
    {
        out->Params.erase(out->Params.size() - 2);
    }

    if (out->CppName.size() == 0)
    {
        out->CppName = "void " + fn.GetName();
    }
}

void UE_UPackage::GenerateStruct(const UE_UStruct &object, std::vector<Struct> &arr)
{
    Struct s;
    s.Size = object.GetSize();
    if (s.Size == 0)
    {
        return;
    }
    s.Inherited = 0;
    s.Name = object.GetName();
    s.FullName = object.GetFullName();

    s.CppName = "struct ";
    s.CppName += object.GetCppName();

    auto super = object.GetSuper();
    if (super)
    {
        s.CppName += " : ";
        s.CppName += super.GetCppName();
        s.Inherited = super.GetSize();
    }

    uint32_t offset = s.Inherited;
    uint8_t bitOffset = 0;

    auto generateMember = [&](IProperty *prop, Member *m)
    {
        auto arrDim = prop->GetArrayDim();
        m->Size = prop->GetSize() * arrDim;
        if (m->Size == 0)
        {
            return;
        } // this shouldn't be zero

        auto type = prop->GetType();
        m->Type = type.second;
        m->Name = prop->GetName();
        m->Offset = prop->GetOffset();

        if (m->Offset > offset)
        {
            UE_UPackage::FillPadding(object, s.Members, offset, bitOffset, m->Offset);
        }
        if (type.first == PropertyType::BoolProperty && *(uint32_t *)type.second.data() != 'loob')
        {
            auto boolProp = prop;
            auto mask = boolProp->GetFieldMask();
            uint8_t zeros = 0, ones = 0;
            while (mask & ~1)
            {
                mask >>= 1;
                zeros++;
            }
            while (mask & 1)
            {
                mask >>= 1;
                ones++;
            }
            if (zeros > bitOffset)
            {
                UE_UPackage::GenerateBitPadding(s.Members, offset, bitOffset, zeros - bitOffset);
                bitOffset = zeros;
            }
            m->Name += fmt::format(" : {}", ones);
            bitOffset += ones;

            if (bitOffset == 8)
            {
                offset++;
                bitOffset = 0;
            }

            m->extra = fmt::format("Mask(0x{:X})", boolProp->GetFieldMask());
        }
        else
        {
            if (arrDim > 1)
            {
                m->Name += fmt::format("[0x{:X}]", arrDim);
            }

            offset += m->Size;
        }
    };

    for (auto prop = object.GetChildProperties().Cast<UE_FProperty>(); prop; prop = prop.GetNext().Cast<UE_FProperty>())
    {
        Member m;
        auto propInterface = prop.GetInterface();
        generateMember(&propInterface, &m);
        s.Members.push_back(m);
    }

    for (auto child = object.GetChildren(); child; child = child.GetNext())
    {
        if (child.IsA<UE_UFunction>())
        {
            auto fn = child.Cast<UE_UFunction>();
            Function f;
            GenerateFunction(fn, &f);
            s.Functions.push_back(f);
        }
        else if (child.IsA<UE_UProperty>())
        {
            auto prop = child.Cast<UE_UProperty>();
            Member m;
            auto propInterface = prop.GetInterface();
            generateMember(&propInterface, &m);
            s.Members.push_back(m);
        }
    }

    if (s.Size > offset)
    {
        UE_UPackage::FillPadding(object, s.Members, offset, bitOffset, s.Size);
    }

    arr.push_back(s);
}

void UE_UPackage::GenerateEnum(const UE_UEnum &object, std::vector<Enum> &arr)
{
    Enum e;
    e.FullName = object.GetFullName();

    auto names = object.GetNames();

    uint64_t max = 0;

    // size of FName struct
    auto numOff = UEVars::Offsets.FName.Number == 0 ? 4 : UEVars::Offsets.FName.Number;
    uint64_t nameSize = ((numOff + 4) + 7) & ~(7);

    uint64_t pairSize = nameSize + 8;

    for (int32_t i = 0; i < names.Num(); i++)
    {
        auto pair = names.GetData() + i * pairSize;
        auto name = UE_FName(pair);
        auto str = name.GetName();
        auto pos = str.find_last_of(':');
        if (pos != std::string::npos)
            str = str.substr(pos + 1);

        auto value = vm_rpm_ptr<uint64_t>(pair + nameSize);
        if (value > max)
            max = value;

        str.append(" = ").append(fmt::format("{}", value));
        e.Members.push_back(str);
    }

    const char *type = nullptr;

    if (max > GetMaxOfType<uint32_t>())
        type = " : uint64_t";
    else if (max > GetMaxOfType<uint16_t>())
        type = " : uint32_t";
    else if (max > GetMaxOfType<uint8_t>())
        type = " : uint16_t";
    else
        type = " : uint8_t";

    e.CppName = "enum class " + object.GetName() + type;

    if (e.Members.size())
    {
        arr.push_back(e);
    }
}

void UE_UPackage::AppendStructsToBuffer(std::vector<Struct> &arr, BufferFmt *pBufFmt)
{
    for (auto &s : arr)
    {
        pBufFmt->append("// Object: {}\n// Size: 0x{:X} (Inherited: 0x{:X})\n{}\n{{",
                        s.FullName, s.Size, s.Inherited, s.CppName);

        if (s.Members.size())
        {
            for (auto &m : s.Members)
            {
                pBufFmt->append("\n\t{} {}; // 0x{:X}(0x{:X})", m.Type, m.Name, m.Offset, m.Size);
                if (!m.extra.empty())
                {
                    pBufFmt->append(" ( {} )", m.extra);
                }
            }
        }
        if (s.Functions.size())
        {
            if (s.Members.size())
                pBufFmt->append("\n");

            for (auto &f : s.Functions)
            {
                void *funcOffset = f.Func ? (void *)(f.Func - UEVars::BaseAddress) : nullptr;
                pBufFmt->append("\n\n\t// Object: {}\n\t// Flags: [{}]\n\t// Offset: {}\n\t// Params: [ Num({}) Size(0x{:X}) ]\n\t{}({});", f.FullName, f.Flags, funcOffset, f.NumParams, f.ParamSize, f.CppName, f.Params);
            }
        }
        pBufFmt->append("\n}};\n\n");
    }
}

void UE_UPackage::AppendEnumsToBuffer(std::vector<Enum> &arr, BufferFmt *pBufFmt)
{
    for (auto &e : arr)
    {
        pBufFmt->append("// Object: {}\n{}\n{{", e.FullName, e.CppName);

        size_t lastIdx = e.Members.size() - 1;
        for (size_t i = 0; i < lastIdx; i++)
        {
            auto &m = e.Members.at(i);
            pBufFmt->append("\n\t{},", m);
        }

        auto &m = e.Members.at(lastIdx);
        pBufFmt->append("\n\t{}", m);

        pBufFmt->append("\n}};\n\n");
    }
}

void UE_UPackage::Process()
{
    auto &objects = Package->second;
    for (auto &object : objects)
    {
        if (object.IsA<UE_UClass>())
        {
            GenerateStruct(object.Cast<UE_UStruct>(), Classes);
        }
        else if (object.IsA<UE_UScriptStruct>())
        {
            GenerateStruct(object.Cast<UE_UStruct>(), Structures);
        }
        else if (object.IsA<UE_UEnum>())
        {
            GenerateEnum(object.Cast<UE_UEnum>(), Enums);
        }
    }
}

bool UE_UPackage::AppendToBuffer(BufferFmt *pBufFmt)
{
    if (!pBufFmt)
        return false;

    if (!Classes.size() && !Structures.size() && !Enums.size())
        return false;

    // make safe to use as a file name
    std::string packageName = ioutils::replace_specials(GetObject().GetName(), '_');

    pBufFmt->append("#pragma once\n\n#include <cstdio>\n#include <string>\n#include <cstdint>\n\n");

    pBufFmt->append("// Package: {}\n// Enums: {}\n// Structs: {}\n// Classes: {}\n\n",
                    packageName, Enums.size(), Structures.size(), Classes.size());

    if (Enums.size())
    {
        UE_UPackage::AppendEnumsToBuffer(Enums, pBufFmt);
    }

    if (Structures.size())
    {
        UE_UPackage::AppendStructsToBuffer(Structures, pBufFmt);
    }

    if (Classes.size())
    {
        UE_UPackage::AppendStructsToBuffer(Classes, pBufFmt);
    }

    return true;
}