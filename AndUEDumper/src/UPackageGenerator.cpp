#include "UPackageGenerator.hpp"

#include "UE/UEMemory.hpp"
using namespace UEMemory;

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
        }  // this shouldn't be zero

        auto type = prop->GetType();
        m->Type = type.second;
        m->Name = prop->GetName();
        m->Offset = prop->GetOffset();

        if (m->Offset > offset)
        {
            UE_UPackage::FillPadding(object, s.Members, offset, bitOffset, m->Offset);
        }
        if (type.first == UEPropertyType::BoolProperty && *(uint32_t *)type.second.data() != 'loob')
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

    uint64_t nameSize = GetPtrAlignedOf(UEWrappers::GetUEVars()->GetOffsets()->FName.Size);
    uint64_t pairSize = nameSize + sizeof(int64_t);

    auto names = object.GetNames();
    uint64_t max = 0;

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

        e.Members.emplace_back(str, value);
    }

    // enum values should be in ascending order
    auto isUninitializedEnum = [](Enum &e) -> bool
    {
        if (e.Members.size() > 1)
        {
            for (size_t i = 1; i < e.Members.size(); ++i)
            {
                if (e.Members[i].second <= e.Members[i - 1].second)
                    return true;
            }
        }
        return false;
    };

    if (isUninitializedEnum(e))
    {
        max = e.Members.size();
        for (size_t i = 0; i < e.Members.size(); ++i)
        {
            e.Members[i].second = i;
        }
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
                    pBufFmt->append(", {}", m.extra);
                }
            }
        }
        if (s.Functions.size())
        {
            if (s.Members.size())
                pBufFmt->append("\n");

            for (auto &f : s.Functions)
            {
                void *funcOffset = f.Func ? (void *)(f.Func - UEWrappers::GetUEVars()->GetBaseAddress()) : nullptr;
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
            pBufFmt->append("\n\t{} = {},", m.first, m.second);
        }

        auto &m = e.Members.at(lastIdx);
        pBufFmt->append("\n\t{} = {}", m.first, m.second);

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

    pBufFmt->append("// Package: {}\n// Enums: {}\n// Structs: {}\n// Classes: {}\n\n",
                    GetObject().GetName(), Enums.size(), Structures.size(), Classes.size());

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
