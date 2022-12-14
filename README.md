# Android Unreal Engine 4 Dumper / UE4 Dumper

Generate sdk and functions script for unreal engine 4 games on android.

The dumper is based on [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)
project.

## Features

* Supports ARM64, ARM and x86
* Can be compiled as executable for external and as library for internal use
* Dump UE4 classes, structs, enums and functions
* Generate function names json script to use with IDA & Ghidra etc
* Symbol and pattern scanning to find GUObjectArray, GNames and NamePoolData addresses automatically
* Dump UE4 library from memory

<br />

## Currently Supported Games

* Dead by Daylight Mobile (64bit only)
* Apex Legends Mobile (32bit & 64bit)
* PUBG Mobile (32bit & 64bit)
* ARK Survival (32bit & 64bit)
* Mortal Kombat (32bit & 64bit)
* eFootBall 2023 (64bit only)
* Distyle (64bit only)

<br />

## Library Usage

Simply load or inject the library with whichever method and let it do its thing.
run logcat with tag filter "UE4Dump3r" for dump logs.
The dump output will be at the game's external data folder (/sdcard/Android/data/< game >/files) to avoid external storage permission.

<br />

## Executable Usage

You will have to push the dumper in an executable directory like /data/local/tmp then give it execute permission. Its recommended to have adb, you can check [push](AndUE4Dumper/push.bat) script for this.
Use the compatible dumper, if game is 64bit use arm64, if 32bit then use arm or x86 version.
run executable in shell ./UE4Dump3r, notice the -p argument to specify game package is optional. the dumper will display list of available game if not set.

```
Usage: ./UE4Dump3r [-h] [-o] [-p] [ options ]

Required arguments:
   -o, --output        specify output directory path.

Optional arguments:
   -h, --help          show available arguments
   -p, --package       specify game ID in advance.
   lib                 dump UE4 library from memory.
   full                generate all-in-one sdk dump.
   headers             generate header files dump.
   objects             generate "ObjObjects" dump.
   script              generate json file of game functions.
```

Example to generate full dump with headers and functions scripts:

```
./UE4Dump3r_arm64 -o /sdcard/Download full headers script
Choose from the available games:
        1 : eFootball 2023 | jp.konami.pesam
        2 : Distyle | com.lilithgames.xgame.gp
        3 : Mortal Kombat | com.wb.goog.mkx
        4 : Ark Survival | com.studiowildcard.wardrumstudios.ark
        5 : Dead by Daylight | com.bhvr.deadbydaylight
        6 : Apex Legends | com.ea.gp.apexlegendsmobilefps
        7 : PUBG | com.tencent.ig
        8 : PUBG | com.rekoo.pubgm
        9 : PUBG | com.pubg.imobile
        10 : PUBG | com.pubg.krmobile
        11 : PUBG | com.vng.pubgmobile
Game number: 1
```

<br />

## Output Files

##### headers

* C++ headers that you can use in your source, however the headers might not compile directly without a change

##### FullDump.hpp

* An all-in-one dump file

##### objects_dump.txt

* ObjObjects dump

##### script.json

* If you are familiar with Il2cppDumper script.json, this is similar. It contains a json array of function names and addresses
<br />
<br />

## Adding a new game to the Dumper

Follow the prototype in [GameProfiles](AndUE4Dumper/source/src/Core/GameProfiles)
<br />
You can also use the provided patterns to find GUObjectArray, GNames or NamePoolData.

<br />

## How to compile

You need to have make installed on your OS and an ndk (I used ndk v25), then simply run [build](AndUE4Dumper/build.bat) script. make sure to set NDK_Home in [Makefile](AndUE4Dumper/Makefile).
<br />
<br />

## Credits & Thanks

* [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)

* [Il2cppDumper](https://github.com/Perfare/Il2CppDumper/blob/master/README.md)
