#include <thread>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <string>
#include <cerrno>

#include <Logger.h>

#include "Core/ioutils.h"
#include "Core/Dumper.h"

#include "Core/GameProfiles/Games/PES.h"
#include "Core/GameProfiles/Games/ARK.h"
#include "Core/GameProfiles/Games/DBD.h"
#include "Core/GameProfiles/Games/PUBGM.h"
#include "Core/GameProfiles/Games/Distyle.h"
#include "Core/GameProfiles/Games/MortalK.h"
#include "Core/GameProfiles/Games/Farlight.h"
#include "Core/GameProfiles/Games/Torchlight.h"

// increase if needed
#define WAIT_TIME_SEC 20

IGameProfile *UE_Games[] =
{
    new PESProfile(),
    new DistyleProfile(),
    new MortalKProfile(),
    new ArkProfile(),
    new DBDProfile(),
    new PUBGMProfile(),
    new FarlightProfile(),
    new TorchlightProfile()
};

int UE_GamesCount = (sizeof(UE_Games)/sizeof(IGameProfile*));

void dump_thread(bool bDumpLib);

extern "C" void callMe(bool bDumpLib)
{
    LOGI("Starting UE4Dump3r thread...");
    std::thread(dump_thread, bDumpLib).detach();
}

__attribute__((constructor)) void onload()
{
    LOGI("~: UE4Dump3r loaded :~");
    //callMe(true); // dump lib from memory
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

    std::string sDumpDir = sOutDirectory + "/UE4Dump3r";
    std::string sDumpHeadersDir = sDumpDir + "/Headers";
    ioutils::delete_directory(sDumpDir.c_str());

    if (ioutils::mkdir_recursive(sDumpHeadersDir, 0777) == -1)
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

    Dumper::DumpStatus dumpStatus = Dumper::UE_DS_NONE;
    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (sGamePackage == pkg)
            {
                dumpStatus = Dumper::Dump(sDumpDir, sDumpHeadersDir, bDumpLib, it);
                goto done;
            }
        }
    }
done:

    if (dumpStatus == Dumper::UE_DS_NONE)
    {
        LOGE("Game is not supported. check AppID.");
    }
    else
    {
        std::string status_str = Dumper::DumpStatusToStr(dumpStatus);
        LOGI("Dump Status: %s", status_str.c_str());
    }

    LOGI("Dump Location: %s", sDumpDir.c_str());
}