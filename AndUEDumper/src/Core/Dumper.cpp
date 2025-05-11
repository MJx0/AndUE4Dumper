#include "Dumper.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "wrappers.h"

#include "../Utils/Logger.h"
#include "../Utils/ioutils.h"
#include "../Utils/memory.h"

namespace Dumper
{

    namespace jf_ns
    {
        struct JsonFunction
        {
            std::string Parent;
            std::string Name;
            uint64_t Address = 0;
        };
        static std::vector<JsonFunction> jsonFunctions;

        void to_json(json &j, const JsonFunction &jf)
        {
            if (jf.Parent.empty() || jf.Parent == "None" || jf.Parent == "null")
                return;
            if (jf.Name.empty() || jf.Name == "None" || jf.Name == "null")
                return;
            if (jf.Address == 0)
                return;

            std::string fname = ioutils::replace_specials(jf.Parent, '_');
            fname += "$$";
            fname += ioutils::replace_specials(jf.Name, '_');

            j = json{{"Name", fname}, {"Address", (jf.Address - UEVars::BaseAddress)}};
        }
    } // namespace jf_ns

    DumpStatus InitUEVars(IGameProfile *profile)
    {
        LOGI("Reading Target UE Library Info...");

        auto ue_elf = profile->GetUnrealEngineELF();
        if (!ue_elf.isValid())
        {
            LOGE("Couldn't find a valid UE ELF in target process maps.");
            return UE_DS_ERROR_LIB_NOT_FOUND;
        }

        switch (ue_elf.header().e_ident[4]) // EI_CLASS
        {
        case 1: // ELFCLASS32
            if (sizeof(void *) != 4)
            {
                LOGE("Dumper is 64bit while target process is 32bit. Please use the correct architecture.");
                return UE_DS_ERROR_ARCH_MISMATCH;
            }
            LOGI("ei_class: 32");
            break;

        case 2: // ELFCLASS64
            if (sizeof(void *) != 8)
            {
                LOGE("Dumper is 32bit while target process is 64bit. Please use the correct architecture.");
                return UE_DS_ERROR_ARCH_MISMATCH;
            }
            LOGI("ei_class: 64");
            break;

        default:
            break;
        }

        if (!profile->ArchSupprted())
        {
            LOGE("Architecture ( 0x%x ) is not supported for this game.", ue_elf.header().e_machine);
            return UE_DS_ARCH_NOT_SUPPORTED;
        }

        LOGI("e_machine: 0x%x", ue_elf.header().e_machine);
        LOGI("Library: %s", ue_elf.filePath().c_str());
        LOGI("BaseAddress: %p", (void *)(ue_elf.base()));
        LOGI("BaseMap: %s", ue_elf.baseSegment().toString().c_str());
        LOGI("==========================");

        LOGI("Initializing Game Profile...");

        UEVars::Profile = profile;

        UEVars::BaseAddress = profile->GetUnrealEngineELF().base();

        UE_Offsets *pOffsets = profile->GetOffsets();
        if (!pOffsets)
            return UE_DS_ERROR_INIT_OFFSETS;

        UEVars::Offsets = *(UE_Offsets *)pOffsets;

        UEVars::isUsingFNamePool = profile->IsUsingFNamePool();
        UEVars::isUsingOutlineNumberName = profile->isUsingOutlineNumberName();

        if (UEVars::isUsingFNamePool)
        {
            UEVars::NamePoolDataPtr = profile->GetNamesPtr();
            LOGI("NamePoolData: %p", (void *)UEVars::NamePoolDataPtr);
            if (UEVars::NamePoolDataPtr == 0)
                return UE_DS_ERROR_INIT_NAMEPOOL;
        }
        else
        {
            UEVars::GNamesPtr = profile->GetNamesPtr();
            LOGI("GNames: %p", (void *)UEVars::GNamesPtr);
            if (UEVars::GNamesPtr == 0)
                return UE_DS_ERROR_INIT_GNAMES;
        }

        std::string firstEntry = profile->GetNameByID(0);
        if (*(uint32_t *)firstEntry.data() != 'enoN')
        {
            LOGW("Warning, GetNameByID(0) = \"%s\" should be equal to \"None\"", firstEntry.c_str());
        }

        uintptr_t GUObjectsArrayPtr = profile->GetGUObjectArrayPtr();
        LOGI("GUObjectArray: %p", (void *)GUObjectsArrayPtr);
        if (GUObjectsArrayPtr == 0)
            return UE_DS_ERROR_INIT_GUOBJECTARRAY;

        UEVars::ObjObjectsPtr = GUObjectsArrayPtr + UEVars::Offsets.FUObjectArray.ObjObjects;
        LOGI("ObjObjects: %p", (void *)UEVars::ObjObjectsPtr);

        if (!kMgr.readMem(UEVars::ObjObjectsPtr, &UEVars::ObjObjects, sizeof(UE_UObjectArray)))
            return UE_DS_ERROR_INIT_OBJOBJECTS;

        LOGI("ObjObjects Num: %d", UEVars::ObjObjects.GetNumElements());
        LOGI("==========================");

        return UE_DS_NONE;
    }

    DumpStatus Dump(std::unordered_map<std::string, BufferFmt> *outBuffersMap)
    {
        IGameProfile *profile = UEVars::Profile;
        auto ue_elf = profile->GetUnrealEngineELF();

        outBuffersMap->insert({"Logs.txt", BufferFmt()});
        BufferFmt &logsBufferFmt = outBuffersMap->at("Logs.txt");

        logsBufferFmt.append("e_machine: 0x{:X}\n", ue_elf.header().e_machine);
        logsBufferFmt.append("Library: {}\n", ue_elf.filePath().c_str());
        logsBufferFmt.append("BaseAddress: 0x{:X}\n", ue_elf.base());
        logsBufferFmt.append("BaseMap: {}\n", ue_elf.baseSegment().toString());
        logsBufferFmt.append("==========================\n");

        if (!UEVars::isUsingFNamePool)
        {
            logsBufferFmt.append("GNames: [<Base> + 0x{:X] = 0x{:X}\n",
                                 UEVars::GNamesPtr - UEVars::BaseAddress, UEVars::GNamesPtr);
        }
        else
        {
            logsBufferFmt.append("FNamePool: [<Base> + 0x{:X}] = 0x{:X}\n",
                                 UEVars::NamePoolDataPtr - UEVars::BaseAddress, UEVars::NamePoolDataPtr);
        }

        logsBufferFmt.append("Test dumping first 5 name entries\n");
        for (int i = 0; i < 5; i++)
        {
            logsBufferFmt.append("GetNameByID({}): {}\n", i, profile->GetNameByID(i));
        }
        logsBufferFmt.append("==========================\n");

        logsBufferFmt.append("ObjObjects: [<Base> + 0x{:X}] = 0x{:X}\n", UEVars::ObjObjectsPtr - UEVars::BaseAddress, UEVars::ObjObjectsPtr);
        logsBufferFmt.append("ObjObjects Num: {}\n", UEVars::ObjObjects.GetNumElements());
        logsBufferFmt.append("Test Dumping First 5 Name Entries\n");
        for (int i = 0; i < 5; i++)
        {
            UE_UObject obj = UEVars::ObjObjects.GetObjectPtr(i);
            logsBufferFmt.append("GetObjectPtr({}): {}\n", i, obj.GetName());
        }
        logsBufferFmt.append("==========================\n");

        LOGI("Dumping, please wait...");
        logsBufferFmt.append("Dumping, please wait...\n");

        outBuffersMap->insert({"Objects.txt", BufferFmt()});
        BufferFmt &objsBufferFmt = outBuffersMap->at("Objects.txt");

        std::vector<std::pair<uint8_t *const, std::vector<UE_UObject>>> packages;
        std::function<bool(UE_UObject)> callback;
        callback = [&objsBufferFmt, &packages](UE_UObject object)
        {
            if (object.IsA<UE_UFunction>() || object.IsA<UE_UStruct>() || object.IsA<UE_UEnum>())
            {
                bool found = false;
                auto packageObj = object.GetPackageObject();
                for (auto &pkg : packages)
                {
                    if (pkg.first == packageObj)
                    {
                        pkg.second.push_back(object);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    packages.push_back(std::make_pair(packageObj, std::vector<UE_UObject>(1, object)));
                }
            }

            objsBufferFmt.append("[{:010}]: {}\n", object.GetIndex(), object.GetFullName());

            return false;
        };

        UEVars::ObjObjects.ForEachObject(callback);

        if (!packages.size())
        {
            LOGI("Error: Packages are empty.");
            logsBufferFmt.append("Error: Packages are empty.\n");
            return UE_DS_ERROR_EMPTY_PACKAGES;
        }

        int packages_saved = 0;
        std::string packages_unsaved{};

        int classes_saved = 0;
        int structs_saved = 0;
        int enums_saved = 0;

        static bool processInternal_once = false;

        outBuffersMap->insert({"AIOHeader.hpp", BufferFmt()});
        BufferFmt &aioBufferFmt = outBuffersMap->at("AIOHeader.hpp");

        for (UE_UPackage package : packages)
        {
            package.Process();
            if (!package.AppendToBuffer(&aioBufferFmt))
            {
                packages_unsaved += "\t";
                packages_unsaved += (package.GetObject().GetName() + ",\n");
                continue;
            }

            packages_saved++;
            classes_saved += package.Classes.size();
            structs_saved += package.Structures.size();
            enums_saved += package.Enums.size();

            for (const auto &cls : package.Classes)
            {
                for (const auto &func : cls.Functions)
                {
                    // UObject::ProcessInternal for blueprint functions
                    if (!processInternal_once && (func.EFlags & FUNC_BlueprintEvent) && func.Func)
                    {
                        jf_ns::jsonFunctions.push_back({"UObject", "ProcessInternal", func.Func});
                        processInternal_once = true;
                    }

                    if ((func.EFlags & FUNC_Native) && func.Func)
                    {
                        std::string execFuncName = "exec";
                        execFuncName += func.Name;
                        jf_ns::jsonFunctions.push_back({cls.Name, execFuncName, func.Func});
                    }
                }
            }

            for (const auto &st : package.Structures)
            {
                for (const auto &func : st.Functions)
                {
                    if ((func.EFlags & FUNC_Native) && func.Func)
                    {
                        std::string execFuncName = "exec";
                        execFuncName += func.Name;
                        jf_ns::jsonFunctions.push_back({st.Name, execFuncName, func.Func});
                    }
                }
            }
        }

        LOGI("Saved packages: %d", packages_saved);
        LOGI("Saved classes: %d", classes_saved);
        LOGI("Saved structs: %d", structs_saved);
        LOGI("Saved enums: %d", enums_saved);
        LOGI("==========================");

        logsBufferFmt.append("Saved packages: {}\nSaved classes: {}\nSaved structs: {}\nSaved enums: {}\n", packages_saved, classes_saved, structs_saved, enums_saved);

        if (packages_unsaved.size())
        {
            logsBufferFmt.append("Unsaved packages: [\n{}\n]\n", packages_unsaved);

            packages_unsaved.erase(packages_unsaved.size() - 2);
            LOGW("Unsaved packages: [ %s ]", packages_unsaved.c_str());
            LOGI("==========================");
        }

        if (jf_ns::jsonFunctions.size())
        {
            outBuffersMap->insert({"script.json", BufferFmt()});
            BufferFmt &scriptBufferFmt = outBuffersMap->at("script.json");

            logsBufferFmt.append("Generating json script...\nFunctions: {}\n", jf_ns::jsonFunctions.size());

            json js;
            for (const auto &jf : jf_ns::jsonFunctions)
            {
                js["Functions"].push_back(jf);
            }

            scriptBufferFmt.append("{}", js.dump(4));
        }

        return UE_DS_SUCCESS;
    }

    std::string DumpStatusToStr(DumpStatus ds)
    {
        switch (ds)
        {
        case UE_DS_NONE:
            return "DS_NONE";
        case UE_DS_SUCCESS:
            return "DS_SUCCESS";
        case UE_DS_ERROR_INVALID_ELF:
            return "DS_ERROR_INVALID_ELF";
        case UE_DS_ARCH_NOT_SUPPORTED:
            return "UDS_ARCH_NOT_SUPPORTED";
        case UE_DS_ERROR_ARCH_MISMATCH:
            return "DS_ERROR_ARCH_MISMATCH";
        case UE_DS_ERROR_LIB_INVALID_BASE:
            return "DS_ERROR_LIB_INVALID_BASE";
        case UE_DS_ERROR_LIB_NOT_FOUND:
            return "DS_ERROR_LIB_NOT_FOUND";
        case UE_DS_ERROR_IO_OPERATION:
            return "DS_ERROR_IO_OPERATION";
        case UE_DS_ERROR_INIT_GNAMES:
            return "DS_ERROR_INIT_GNAMES";
        case UE_DS_ERROR_INIT_NAMEPOOL:
            return "DS_ERROR_INIT_NAMEPOOL";
        case UE_DS_ERROR_INIT_GUOBJECTARRAY:
            return "DS_ERROR_INIT_GUOBJECTARRAY";
        case UE_DS_ERROR_INIT_OBJOBJECTS:
            return "DS_ERROR_INIT_OBJOBJECTS";
        case UE_DS_ERROR_INIT_OFFSETS:
            return "DS_ERROR_INIT_OFFSETS";
        case UE_DS_ERROR_EMPTY_PACKAGES:
            return "DS_ERROR_EMPTY_PACKAGES";
        default:
            break;
        }
        return "UNKNOWN";
    }

} // namespace Dumper