#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <string>
#include <cerrno>

#include <Logger.h>

#include "Core/Dumper.h"

#include "Core/GameProfiles/Games/PES.h"
#include "Core/GameProfiles/Games/ARK.h"
#include "Core/GameProfiles/Games/Apex.h"
#include "Core/GameProfiles/Games/DBD.h"
#include "Core/GameProfiles/Games/PUBGM.h"
#include "Core/GameProfiles/Games/Distyle.h"
#include "Core/GameProfiles/Games/MortalK.h"

// increase if needed
#define WAIT_TIME_SEC 20

using Dumper::DumpArgs;

IGameProfile *UE_Games[]
{
    new PESProfile(),
    new DistyleProfile(),
    new MortalKProfile(),
    new ArkProfile(),
    new DBDProfile(),
    new ApexProfile(),
    new PUBGMProfile(),
};

int UE_GamesCount = (sizeof(UE_Games)/sizeof(IGameProfile*));

// defaults { dump_lib=false, full_dump=true, dump_headers=true, dump_objects=true, gen_script=true }
DumpArgs dumperArgs = { "", "", false, true, true, true, true };

void *dump_thread(void *);

extern "C" void callMe(DumpArgs *dArgs)
{
    LOGI("Starting UE4Dump3r thread...");
    pthread_t ptid;
    pthread_create(&ptid, nullptr, dump_thread, dArgs);
}

__attribute__((constructor)) void onload()
{
    LOGI("~: UE4Dump3r loaded :~");
    callMe(&dumperArgs);
}

void *dump_thread(void *args)
{
    DumpArgs dArgs = *reinterpret_cast<DumpArgs *>(args);

    LOGI("Dump will start after %d seconds.", WAIT_TIME_SEC);
    sleep(WAIT_TIME_SEC);
    LOGI("==========================");

    std::string gGamePackage = getCurrentProcName();
    pid_t gamePID = getpid();

    // dumping at external app data folder to avoid external storage permission
    std::string gOutDirectory = getenv("EXTERNAL_STORAGE");
    gOutDirectory += "/Android/data/";
    gOutDirectory += gGamePackage;
    gOutDirectory += "/files/UE4Dumper";

    LOGI("Game: %s", gGamePackage.c_str());
    LOGI("Process ID: %d", gamePID);
    LOGI("Output directory: %s", gOutDirectory.c_str());
    LOGI("Dump Library: %s", dArgs.dump_lib ? "true" : "false");
    LOGI("Full: %s", dArgs.dump_full ? "true" : "false");
    LOGI("Headers: %s", dArgs.dump_headers ? "true" : "false");
    LOGI("Objects: %s", dArgs.dump_objects ? "true" : "false");
    LOGI("Script: %s", dArgs.gen_functions_script ? "true" : "false");
    LOGI("==========================");

    dArgs.dump_dir = gOutDirectory;
    dArgs.dump_headers_dir = dArgs.dump_dir + "/Headers";

    ioutils::delete_directory(dArgs.dump_dir.c_str());

    errno = 0;
    if (mkdir(dArgs.dump_dir.c_str(), 0777) == -1)
    {
        int err = errno;
        LOGE("Couldn't create Output Directory [\"%s\"] error=%d | %s.", dArgs.dump_dir.c_str(), err, strerror(err));
        return nullptr;
    }

    if (dArgs.dump_headers)
    {
        errno = 0;
        if (mkdir(dArgs.dump_headers_dir.c_str(), 0777) == -1)
        {
            int err = errno;
            LOGE("Couldn't create Output Directory [\"%s\"] error=%d | %s.", dArgs.dump_headers_dir.c_str(), err, strerror(err));
            return nullptr;
        }
    }

    PMemory::Initialize(gamePID, gGamePackage, gOutDirectory);

    Dumper::DumpStatus dumpStatus = Dumper::UE_DS_NONE;
    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (gGamePackage == pkg)
            {
                dumpStatus = Dumper::Dump(&dArgs, it);
                break;
            }
        }
    }

    if (dumpStatus == Dumper::UE_DS_NONE)
    {
        LOGE("Game is not supported. check AppID.");
    }
    else
    {
        std::string status_str = Dumper::DumpStatusToStr(dumpStatus);
        LOGI("Dump Status: %s", status_str.c_str());
    }

    LOGI("Dump Location: %s", dArgs.dump_dir.c_str());

    return nullptr;
}