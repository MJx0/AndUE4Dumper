#include <cerrno>
#include <chrono>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "Utils/Logger.h"
#include "Utils/ioutils.h"

#include "Core/Dumper.h"

#include "Core/GameProfiles/ARK.h"
#include "Core/GameProfiles/ArenaBreakout.h"
#include "Core/GameProfiles/DBD.h"
#include "Core/GameProfiles/Distyle.h"
#include "Core/GameProfiles/Farlight.h"
#include "Core/GameProfiles/MortalK.h"
#include "Core/GameProfiles/PES.h"
#include "Core/GameProfiles/PUBGM.h"
#include "Core/GameProfiles/Torchlight.h"

// increase if needed
#define WAIT_TIME_SEC 20

IGameProfile *UE_Games[] = {
    new PESProfile(),
    new DistyleProfile(),
    new MortalKProfile(),
    new ArkProfile(),
    new DBDProfile(),
    new PUBGMProfile(),
    new FarlightProfile(),
    new TorchlightProfile(),
    new ArenaBreakoutProfile(),
};

int UE_GamesCount = (sizeof(UE_Games) / sizeof(IGameProfile *));

void dump_thread(bool bDumpLib);

extern "C" void callMe(bool bDumpLib)
{
    LOGI("Starting UEDump3r thread...");
    std::thread(dump_thread, bDumpLib).detach();
}

__attribute__((constructor)) void onload()
{
    LOGI("~: UEDump3r loaded :~");
    // callMe(true); // dump lib from memory
    callMe(false);
}

void dump_thread(bool bDumpLib)
{
    LOGI("Dump will start after %d seconds.", WAIT_TIME_SEC);
    sleep(WAIT_TIME_SEC);
    LOGI("==========================");

    std::string sGamePackage = getprogname();
    pid_t gamePID = getpid();

    // dumping at external app data folder to avoid external storage permission
    std::string sOutDirectory = KittyUtils::getExternalStorage();
    sOutDirectory += "/Android/data/";
    sOutDirectory += sGamePackage;
    sOutDirectory += "/files";

    LOGI("Game: %s", sGamePackage.c_str());
    LOGI("Process ID: %d", gamePID);
    LOGI("Output directory: %s", sOutDirectory.c_str());
    LOGI("Dump Library: %s", bDumpLib ? "true" : "false");
    LOGI("==========================");

    std::string sDumpDir = sOutDirectory + "/UEDump3r";
    std::string sDumpGameDir = sDumpDir + "/" + sGamePackage;
    ioutils::delete_directory(sDumpGameDir);

    if (ioutils::mkdir_recursive(sDumpGameDir, 0777) == -1)
    {
        int err = errno;
        LOGE("Couldn't create Output Directory [\"%s\"] error=%d | %s.", sDumpDir.c_str(), err, strerror(err));
        return;
    }

    if (!kMgr.initialize(gamePID, EK_MEM_OP_SYSCALL, false) && !kMgr.initialize(gamePID, EK_MEM_OP_IO, false))
    {
        LOGE("Failed to initialize KittyMemoryMgr.");
        return;
    }

    auto dmpStart = std::chrono::steady_clock::now();

    Dumper::DumpStatus dumpStatus = Dumper::UE_DS_NONE;
    std::unordered_map<std::string, BufferFmt> buffersMap;
    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (sGamePackage == pkg)
            {
                if (bDumpLib)
                {
                    auto ue_elf = it->GetUnrealEngineELF();
                    if (!ue_elf.isValid())
                    {
                        LOGE("Couldn't find a valid UE ELF in target process maps.");
                        return;
                    }

                    LOGI("Dumping libUE.so from memory...");
                    std::string libdumpPath = KittyUtils::String::Fmt("%s/libUE_%p-%p.so", sDumpGameDir.c_str(), ue_elf.base(), ue_elf.end());
                    LOGI("Dumping lib: %s.", kMgr.dumpMemELF(ue_elf.base(), libdumpPath) ? "success" : "failed");
                    LOGI("==========================");
                }

                dumpStatus = Dumper::InitUEVars(it);
                if (dumpStatus == Dumper::UE_DS_NONE)
                {
                    dumpStatus = Dumper::Dump(&buffersMap);
                }
                goto done;
            }
        }
    }
done:

    if (dumpStatus == Dumper::UE_DS_NONE)
    {
        LOGE("Game is not supported. check AppID.");
        return;
    }

    std::string status_str = Dumper::DumpStatusToStr(dumpStatus);

    if (buffersMap.empty())
    {
        LOGE("Dump Failed, Error <Buffers empty>");
        LOGE("Dump Status <%s>", status_str.c_str());
        return;
    }

    LOGI("Saving Files...");

    for (const auto &it : buffersMap)
    {
        if (!it.first.empty())
        {
            std::string path = KittyUtils::String::Fmt("%s/%s", sDumpGameDir.c_str(), it.first.c_str());
            it.second.writeBufferToFile(path);
        }
    }

    auto dmpEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float, std::milli> dmpDurationMS = (dmpEnd - dmpStart);

    LOGI("Dump Status: %s", status_str.c_str());
    LOGI("Dump Duration: %.2fms", dmpDurationMS.count());
    LOGI("Dump Location: %s", sDumpGameDir.c_str());
}