#include "memory.h"

KittyMemoryMgr kMgr{};

bool vm_rpm_ptr(void *address, void *result, size_t len)
{
    return kMgr.readMem(uintptr_t(address), result, len) == len;
}

std::string vm_rpm_str(void *address, size_t max_len)
{
    std::vector<char> chars(max_len, '\0');
    if (!vm_rpm_ptr(address, chars.data(), max_len))
        return "";

    std::string str = "";
    for (size_t i = 0; i < chars.size(); i++)
    {
        if (chars[i] == '\0')
            break;
        
        str.push_back(chars[i]);
    }

    chars.clear();
    chars.shrink_to_fit();

    if ((int)str[0] == 0 && str.size() == 1)
        return "";

    return str;
}

std::wstring vm_rpm_strw(void *address, size_t max_len)
{
    std::vector<wchar_t> chars(max_len, '\0');
    if (!vm_rpm_ptr(address, chars.data(), max_len*2))
        return L"";

    std::wstring str = L"";
    for (size_t i = 0; i < chars.size(); i++)
    {
        if (chars[i] == L'\0')
            break;
        
        str.push_back(chars[i]);
    }

    chars.clear();
    chars.shrink_to_fit();

    if ((int)str[0] == 0 && str.size() == 1)
        return L"";

    return str;
}