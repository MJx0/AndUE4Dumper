
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "Utils/KittyCmdln.h"
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

size_t UE_GamesCount = (sizeof(UE_Games) / sizeof(IGameProfile *));

bool bNeededHelp = false;

int main(int argc, char **args)
{
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    setbuf(stdin, nullptr);

    KittyCmdln cmdline(argc, args);

    cmdline.setUsage("Usage: ./UEDump3r [-h] [-o] [ options ]");

    cmdline.addCmd("-h", "--help", "show available arguments", false, [&cmdline]()
                   { std::cout << cmdline.toString() << std::endl; bNeededHelp = true; });

    char sOutDir[0xff] = {0};
    cmdline.addScanf("-o", "--output", "specify output directory path.", true, "%s", sOutDir);

    // optional
    char sGamePkg[0xff] = {0};
    cmdline.addScanf("-p", "--package", "specify game package ID in advance.", false, "%s", sGamePkg);

    // options
    bool bDumpLib = false;
    cmdline.addFlag("-dump_lib", "", "dump UE library from memory.", false, &bDumpLib);

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

    std::string sOutDirectory = sOutDir, sGamePackage = sGamePkg;
    if (sOutDirectory.empty())
    {
        LOGE("Output directory path is not specified.");
        return 1;
    }

    if (sGamePackage.empty())
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

        sGamePackage = UE_Games[gameIndexMap[gameNumber].first]->GetAppIDs()[gameIndexMap[gameNumber].second];
    }

    pid_t gamePID = KittyMemoryEx::getProcessID(sGamePackage);
    if (gamePID < 1)
    {
        LOGE("Couldn't find \"%s\" in the running processes list.", sGamePackage.c_str());
        return 1;
    }

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
        return 1;
    }

    if (!kMgr.initialize(gamePID, EK_MEM_OP_SYSCALL, false) && !kMgr.initialize(gamePID, EK_MEM_OP_IO, false))
    {
        LOGE("Failed to initialize KittyMemoryMgr.");
        return 1;
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
                        return 1;
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
        return 1;
    }

    std::string status_str = Dumper::DumpStatusToStr(dumpStatus);

    if (buffersMap.empty())
    {
        LOGE("Dump Failed, Error <Buffers empty>");
        LOGE("Dump Status <%s>", status_str.c_str());
        return 1;
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

    return 0;
}