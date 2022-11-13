#include "Dumper.h"

#include <fmt/core.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "wrappers.h"

#include <KittyMemory/KittyMemory.h>
#include <KittyMemory/KittyScanner.h>
#include <KittyMemory/KittyUtils.h>

#include <Logger.h>

namespace Dumper
{

	namespace JsonGen
	{
		struct IdaFunction
		{
			std::string Parent;
			std::string Name;
			uint64 Address = 0;
		};

		static std::vector<IdaFunction> idaFunctions;

		void to_json(json &j, const IdaFunction &f)
		{
			if (f.Parent.empty() || f.Parent == "None")
				return;
			if (f.Name.empty() || f.Name == "None")
				return;
			if (f.Address == 0)
				return;

			std::string fname = ioutils::replace_specials(f.Parent, '_');
			fname += "$$";
			fname += ioutils::replace_specials(f.Name, '_');

			j = json{{"Name", fname}, {"Address", f.Address}};
		}
	}

	DumpStatus InitProfile(IGameProfile *profile)
	{
		Profile::BaseAddress = profile->GetBaseInfo().map.startAddress;

		UE_Offsets *pOffsets = profile->GetOffsets();
		if (!pOffsets)
			return UE_DS_ERROR_INIT_OFFSETS;

		Profile::offsets = *(UE_Offsets *)pOffsets;

		Profile::isUsingFNamePool = profile->IsUsingFNamePool();

		if (Profile::isUsingFNamePool)
		{
			Profile::NamePoolDataPtr = profile->GetNamesPtr();
			if (Profile::NamePoolDataPtr == 0)
				return UE_DS_ERROR_INIT_NAMEPOOL;
		}
		else
		{
			Profile::GNamesPtr = profile->GetNamesPtr();
			if (Profile::GNamesPtr == 0)
				return UE_DS_ERROR_INIT_GNAMES;
		}

		std::string firstEntry = GetNameByID(0);
		if (*(uint32 *)firstEntry.data() != 'enoN')
		{
			LOGW("Warning, GetNameByID(0) = \"%s\" should be equal to \"None\"", firstEntry.c_str());
		}

		uintptr_t GUObjectsArrayPtr = profile->GetGUObjectArrayPtr();
		if (GUObjectsArrayPtr == 0)
			return UE_DS_ERROR_INIT_GUOBJECTARRAY;

		Profile::ObjObjectsPtr = GUObjectsArrayPtr + Profile::offsets.FUObjectArray.ObjObjects;

		if (!PMemory::vm_rpm_ptr((void *)Profile::ObjObjectsPtr, &Profile::ObjObjects, sizeof(TUObjectArray)))
			return UE_DS_ERROR_INIT_OBJOBJECTS;

		return UE_DS_NONE;
	}

	DumpStatus Dump(DumpArgs *args, IGameProfile *profile)
	{
		LOGI("Reading Target UE4 Library Info...");

		auto ue4Maps = profile->GetMaps();
		if (ue4Maps.empty())
		{
			LOGE("Couldn't find UE4 library in target process maps.");
			return UE_DS_ERROR_LIB_NOT_FOUND;
		}

		auto baseInfo = profile->GetBaseInfo();

		if (baseInfo.map.startAddress == 0)
		{
			LOGE("Couldn't get base info. check dumper and game architecture compatibility.");
			return UE_DS_ERROR_LIB_INVALID_BASE;
		}

		if (!VERIFY_ELF_HEADER(baseInfo.magic))
		{
			auto base_name = baseInfo.map.pathname;
			LOGE("Base of (%s) is not a valid ELF header.", base_name.empty() ? "" : base_name.c_str());
			return UE_DS_ERROR_INVALID_ELF;
		}

		switch (baseInfo.ehdr.e_ident[4]) // EI_CLASS
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
			LOGE("Architecture ( 0x%x ) is not supported for this game.", baseInfo.ehdr.e_machine);
			return UE_DS_ARCH_NOT_SUPPORTED;
		}

		LOGI("e_machine: 0x%x", baseInfo.ehdr.e_machine);
		LOGI("Library: %s", baseInfo.map.pathname.c_str());
		LOGI("BaseAddress: %p", (void *)baseInfo.map.startAddress);
		LOGI("==========================");

		if (args->dump_lib)
		{
			LOGI("Dumping libUE4.so from memory...");
			profile->DumpLibrary();
			LOGI("==========================");
		}

		LOGI("Initializing Game Profile...");

		auto profile_init_status = InitProfile(profile);
		if (profile_init_status != UE_DS_NONE)
			return profile_init_status;

		if (!Profile::isUsingFNamePool)
		{
			LOGI("GNames: %p", (void *)Profile::GNamesPtr);
		}
		else
		{
			LOGI("NamePoolData: %p", (void *)Profile::NamePoolDataPtr);
		}

		LOGI("GUObjectArray: %p", (void *)(Profile::ObjObjectsPtr - Profile::offsets.FUObjectArray.ObjObjects));
		LOGI("ObjObjects: %p", (void *)Profile::ObjObjectsPtr);
		LOGI("ObjObjects Num: %d", Profile::ObjObjects.GetNumElements());
		LOGI("ObjObjects Max: %d", Profile::ObjObjects.GetMaxElements());
		LOGI("==========================");

		LOGI("Dumping, please wait...");

		File objfile;
		if (args->dump_objects)
		{
			std::string objfile_path = args->dump_dir;
			objfile_path += "/objects_dump.txt";
			objfile.open(objfile_path.c_str(), "w");
			if (!objfile)
			{
				LOGE("Couldn't create file [\"%s\"]", objfile_path.c_str());
				return UE_DS_ERROR_IO_OPERATION;
			}
		}

		std::function<void(UE_UObject)> objdump_callback = nullptr;
		if (args->dump_objects && objfile)
		{
			objdump_callback = [&objfile](UE_UObject object)
			{
				fmt::print(objfile, "{}\n", object.GetName());
			};
		}

		if (!args->dump_full && !args->dump_headers && !args->gen_functions_script)
		{
			if (objdump_callback)
			{
				Profile::ObjObjects.ForEachObject(objdump_callback);
			}
			return UE_DS_SUCCESS;
		}

		std::unordered_map<uint8 *, std::vector<UE_UObject>> packages;
		std::function<void(UE_UObject)> callback;
		callback = [&objdump_callback, &packages](UE_UObject object)
		{
			if (object.IsA<UE_UFunction>() || object.IsA<UE_UStruct>() || object.IsA<UE_UEnum>())
			{
				auto packageObj = object.GetPackageObject();
				packages[packageObj].push_back(object);
			}
			if (objdump_callback)
				objdump_callback(object);
		};

		Profile::ObjObjects.ForEachObject(callback);

		if (!packages.size())
		{
			LOGI("Error: Packages are empty.");
			return UE_DS_ERROR_EMPTY_PACKAGES;
		}

		int packages_saved = 0;
		std::string packages_unsaved{};

		int classes_saved = 0;
		int structs_saved = 0;
		int enums_saved = 0;

		static bool processInternal_once = false;

		for (UE_UPackage package : packages)
		{
			package.Process();
			if (package.Save(args->dump_full ? args->dump_dir.c_str() : nullptr, args->dump_headers ? args->dump_headers_dir.c_str() : nullptr))
			{
				packages_saved++;
				classes_saved += package.Classes.size();
				structs_saved += package.Structures.size();
				enums_saved += package.Enums.size();

				for (const auto &cls : package.Classes)
				{
					if (!cls.Functions.size())
						continue;

					for (const auto &func : cls.Functions)
					{
						// UObject::ProcessInternal for blueprint functions
						if (!processInternal_once && (func.EFlags & FUNC_BlueprintEvent))
						{
							JsonGen::idaFunctions.push_back({"UObject", "ProcessInternal", func.Func - Profile::BaseAddress});
							processInternal_once = true;
						}

						if (func.EFlags & FUNC_Native)
						{
							std::string execFuncName = "exec";
							execFuncName += func.Name;
							JsonGen::idaFunctions.push_back({cls.Name, execFuncName, func.Func - Profile::BaseAddress});
						}
					}
				}

				for (const auto &st : package.Structures)
				{
					if (!st.Functions.size())
						continue;

					for (const auto &func : st.Functions)
					{
						if (func.EFlags & FUNC_Native)
						{
							std::string execFuncName = "exec";
							execFuncName += func.Name;
							JsonGen::idaFunctions.push_back({st.Name, execFuncName, func.Func - Profile::BaseAddress});
						}
					}
				}
			}
			else
			{
				packages_unsaved += (package.GetObject().GetName() + ", ");
			}
		}

		LOGI("Saved packages: %d", packages_saved);
		LOGI("Saved classes: %d", classes_saved);
		LOGI("Saved structs: %d", structs_saved);
		LOGI("Saved enums: %d", enums_saved);
		LOGI("==========================");

		if (packages_unsaved.size())
		{
			packages_unsaved.erase(packages_unsaved.size() - 2);
			LOGW("Unsaved packages: [ %s ]", packages_unsaved.c_str());
			LOGI("==========================");
		}

		if (args->gen_functions_script && JsonGen::idaFunctions.size())
		{
			LOGI("Generating json...");
			LOGI("Functions: %zu", JsonGen::idaFunctions.size());
			json js;
			for (auto &idf : JsonGen::idaFunctions)
			{
				js["Functions"].push_back(idf);
			}

			std::string jsfile_path = args->dump_dir;
			jsfile_path += "/script.json";
			File jsfile(jsfile_path.c_str(), "w");
			if (!jsfile)
			{
				LOGE("Couldn't create file [\"%s\"]", jsfile_path.c_str());
				return UE_DS_ERROR_IO_OPERATION;
			}

			fmt::print(jsfile, "{}", js.dump(4));
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

}