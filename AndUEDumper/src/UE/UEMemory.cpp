#include "UEMemory.hpp"

namespace UEMemory
{
    KittyMemoryMgr kMgr{};
    KittyPtrValidator PtrValidator;

    bool vm_rpm_ptr(const void *address, void *result, size_t len)
    {
        if (!PtrValidator.isPtrReadable(address))
            return false;

        return kMgr.readMem(uintptr_t(address), result, len) == len;
    }

    std::string vm_rpm_str(const void *address, size_t max_len)
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

    std::wstring vm_rpm_strw(const void *address, size_t max_len)
    {
        std::vector<wchar_t> chars(max_len, '\0');
        if (!vm_rpm_ptr(address, chars.data(), max_len * 2))
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

    uintptr_t FindAlignedPointerRefrence(uintptr_t start, size_t range, uintptr_t ptr)
    {
        if (start == 0 || start != GetPtrAlignedOf(start))
            return 0;

        if (range < sizeof(void *) || range != GetPtrAlignedOf(range))
            return 0;

        for (size_t i = 0; (i + sizeof(void *)) <= range; i += sizeof(void *))
        {
            uintptr_t val = vm_rpm_ptr<uintptr_t>((void *)(start + i));
            if (val == ptr) return (start + i);
        }
        return 0;
    }

    namespace Arm64
    {
        uintptr_t Decode_ADRP_ADD(uintptr_t adrp_address, uint32_t add_offset)
        {
            if (adrp_address == 0) return 0;

            const uintptr_t page_off = kINSN_PAGE_OFFSET(adrp_address);

            int64_t adrp_pc_rel = 0;
            int32_t add_imm12 = 0;

            uint32_t adrp_insn = vm_rpm_ptr<uint32_t>((void *)(adrp_address));
            uint32_t add_insn = vm_rpm_ptr<uint32_t>((void *)(adrp_address + add_offset));
            if (adrp_insn == 0 || add_insn == 0)
                return 0;

            if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
                return 0;

            add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

            return (page_off + adrp_pc_rel + add_imm12);
        }

        uintptr_t Decode_ADRP_LDR(uintptr_t adrp_address, uint32_t ldr_offset)
        {
            if (adrp_address == 0) return 0;

            const uintptr_t page_off = kINSN_PAGE_OFFSET(adrp_address);

            int64_t adrp_pc_rel = 0;
            int32_t ldr_imm12 = 0;

            uint32_t adrp_insn = vm_rpm_ptr<uint32_t>((void *)(adrp_address));
            uint32_t ldr_insn = vm_rpm_ptr<uint32_t>((void *)(adrp_address + ldr_offset));
            if (adrp_insn == 0 || ldr_insn == 0)
                return 0;

            if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
                return 0;

            if (!KittyArm64::decode_ldrstr_uimm(ldr_insn, &ldr_imm12))
                return 0;

            return (page_off + adrp_pc_rel + ldr_imm12);
        }
    }  // namespace Arm64

}  // namespace UEMemory

namespace IOUtils
{
    std::string get_filename(const std::string &filePath)
    {
        std::string filename;
        const size_t last_slash_idx = filePath.find_last_of("/\\");
        if (std::string::npos != last_slash_idx)
        {
            filename = filePath.substr(last_slash_idx + 1);
        }
        return filename;
    }

    std::string get_file_directory(const std::string &filePath)
    {
        std::string directory;
        const size_t last_slash_idx = filePath.find_last_of("/\\");
        if (std::string::npos != last_slash_idx)
        {
            directory = filePath.substr(0, last_slash_idx);
        }
        return directory;
    }

    std::string get_file_extension(const std::string &filePath)
    {
        std::string ext;
        const size_t last_slash_idx = filePath.find_last_of(".");
        if (std::string::npos != last_slash_idx)
        {
            ext = filePath.substr(last_slash_idx + 1);
        }
        return ext;
    }

    bool file_path_contains(const std::string &filePath, const std::string &subPath)
    {
        return (!filePath.empty() && (filePath.find(subPath) != std::string::npos));
    }

    std::string remove_specials(std::string s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s.erase(s.begin() + i);
            --i;
        }
        return s;
    }

    std::string replace_specials(std::string s, char c)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s[i] = c;
        }
        return s;
    }

    void delete_directory(const std::string &directory)
    {
        DIR *dp;
        struct dirent *rd;
        char buff[512] = {0};

        dp = opendir(directory.c_str());
        if (!dp)
            return;

        while ((rd = readdir(dp)) != NULL)
        {
            if (!strcmp(rd->d_name, ".") || !strcmp(rd->d_name, ".."))
                continue;

            sprintf(buff, "%s/%s", directory.c_str(), rd->d_name);
            if (path_is_directory(buff))
                delete_directory(buff);
            else
                unlink(buff);
        }

        closedir(dp);
        rmdir(directory.c_str());
    }

    int path_is_directory(const std::string &path)
    {
        struct stat st;

        if (stat(path.c_str(), &st))
            return 0;

        return S_ISDIR(st.st_mode);
    }

    int mkdir_recursive(const std::string &dirPath, mode_t mode)
    {
        std::string tmp = dirPath;
        for (char *p = strchr(tmp.data() + 1, '/'); p; p = strchr(p + 1, '/'))
        {
            *p = '\0';
            errno = 0;
            if (mkdir(tmp.c_str(), mode) == -1)
            {
                if (errno != EEXIST)
                {
                    *p = '/';
                    return -1;
                }
            }
            *p = '/';
        }
        errno = 0;
        return mkdir(tmp.c_str(), mode);
    }
}  // namespace IOUtils
