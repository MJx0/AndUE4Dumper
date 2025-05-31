#!/bin/bash

echo pushing dumper binaries...

adb push libs/arm64-v8a/UEDump3r_arm64 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_arm64'"

adb push libs/armeabi-v7a/UEDump3r_arm /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_arm'"

adb push libs/x86/UEDump3r_x86 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_x86'"

adb push libs/x86_64/UEDump3r_x86_64 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_x86_64'"

:: test on arm64 device
adb shell "su -c './/data/local/tmp/UEDump3r_arm64 -o /sdcard'"