
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <thread>
#include <cerrno>
#include <map>

#include <Logger.h>

#include "KittyCmdln.h"

#include "Core/Dumper.h"

#include "Core/GameProfiles/Games/PES.h"
#include "Core/GameProfiles/Games/ARK.h"
#include "Core/GameProfiles/Games/Apex.h"
#include "Core/GameProfiles/Games/DBD.h"
#include "Core/GameProfiles/Games/PUBGM.h"
#include "Core/GameProfiles/Games/Distyle.h"
#include "Core/GameProfiles/Games/MortalK.h"

IGameProfile *UE_Games[]{
    new PESProfile(),
    new DistyleProfile(),
    new MortalKProfile(),
    new ArkProfile(),
    new DBDProfile(),
    new ApexProfile(),
    new PUBGMProfile(),
};

size_t UE_GamesCount = (sizeof(UE_Games) / sizeof(IGameProfile *));

bool bNeededHelp = false;

Dumper::DumpArgs dArgs = {"", "", false, false, false, false, false};

int main(int argc, char **args)
{
    KittyCmdln cmdline(argc, args);

    cmdline.setUsage("Usage: ./UE4Dump3r [-h] [-o] [-p] [ options ]");

    cmdline.addCmd("-h", "--help", "show available arguments", false, [&cmdline]()
                   { std::cout << cmdline.toString() << std::endl; bNeededHelp = true; });

    char tmpOutDir[512] = {0}, tmpGamePkg[512] = {0};
    cmdline.addScanf("-o", "--output", "specify output directory path.", true, "%s", tmpOutDir);
    cmdline.addScanf("-p", "--package", "specify game ID in advance.", false, "%s", tmpGamePkg);

    // options
    cmdline.addFlag("lib", "", "dump UE4 library from memory.", false, &dArgs.dump_lib);
    cmdline.addFlag("full", "", "generate all-in-one sdk dump.", false, &dArgs.dump_full);
    cmdline.addFlag("headers", "", "generate header files dump.", false, &dArgs.dump_headers);
    cmdline.addFlag("objects", "", "generate \"ObjObjects\" dump.", false, &dArgs.dump_objects);
    cmdline.addFlag("script", "", "generate json file of game functions.", false, &dArgs.gen_functions_script);

    cmdline.parseArgs();

    if (bNeededHelp)
    {
        return 0;
    }

    if (!cmdline.requiredCmdsCheck())
    {
        LOGE("Required arguments needed. see -h.");
        return 1;
    }

    std::string gOutDirectory = tmpOutDir;
    std::string gGamePackage = tmpGamePkg;

    if (gOutDirectory.empty())
    {
        LOGE("Output directory path is not specified.");
        return 1;
    }

    if (gGamePackage.empty())
    {
        std::cout << "Choose from the available games:" << std::endl;
        int gameIndex = 1;
        std::map<int, std::pair<int, int>> gameIndexMap;
        for (size_t i = 0; i < UE_GamesCount; i++)
        {
            const auto &appIDs = UE_Games[i]->GetAppIDs();
            for (size_t j = 0; j < appIDs.size(); j++)
            {
                std::cout << "\t" << gameIndex << " : " << UE_Games[i]->GetAppName() << " | " << appIDs[j].c_str() << std::endl;
                gameIndexMap[gameIndex] = {i, j};
                gameIndex++;
            }
        }

        std::cout << "Game number: ";
        int gameNumber = 0;
        scanf("%d", &gameNumber);
        if (gameIndexMap.count(gameNumber) <= 0)
        {
            LOGE("Game number is not available.");
            return 1;
        }

        gGamePackage = UE_Games[gameIndexMap[gameNumber].first]->GetAppIDs()[gameIndexMap[gameNumber].second];
    }

    pid_t gamePID = findProcID(gGamePackage);
    if (gamePID < 1)
    {
        LOGE("Couldn't find \"%s\" in the running processes list.", gGamePackage.c_str());
        return 1;
    }

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
    dArgs.dump_dir += "/UE4Dump3r";

    errno = 0;
    if (mkdir(dArgs.dump_dir.c_str(), 0777) == -1)
    {
        int err = errno;
        if (err != EEXIST) // 17
        {
            LOGE("Couldn't create output directory [\"%s\"] error=%d | %s.", dArgs.dump_dir.c_str(), err, strerror(err));
            return 1;
        }
    }

    dArgs.dump_dir += "/";
    dArgs.dump_dir += gGamePackage;
    dArgs.dump_headers_dir = dArgs.dump_dir + "/Headers";

    ioutils::delete_directory(dArgs.dump_dir.c_str());

    errno = 0;
    if (mkdir(dArgs.dump_dir.c_str(), 0777) == -1)
    {
        int err = errno;
        LOGE("Couldn't create output directory [\"%s\"] error=%d | %s.", dArgs.dump_dir.c_str(), err, strerror(err));
        return 1;
    }

    if (dArgs.dump_headers)
    {
        errno = 0;
        if (mkdir(dArgs.dump_headers_dir.c_str(), 0777) == -1)
        {
            int err = errno;
            LOGE("Couldn't create output directory [\"%s\"] error=%d | %s.", dArgs.dump_headers_dir.c_str(), err, strerror(err));
            return 1;
        }
    }

    PMemory::Initialize(gamePID, gGamePackage, dArgs.dump_dir);

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

    return 0;
}