@echo off
echo pushing dumper binaries...

adb push build/libs/arm64-v8a/UEDump3r_arm64 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_arm64'"

adb push build/libs/armeabi-v7a/UEDump3r_arm /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_arm'"

adb push build/libs/x86/UEDump3r_x86 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_x86'"

adb push build/libs/x86_64/UEDump3r_x86_64 /data/local/tmp
adb shell "su -c 'chmod 755 /data/local/tmp/UEDump3r_x86_64'"

:: test on arm64 device
adb shell "su -c './/data/local/tmp/UEDump3r_arm64 -o /sdcard'"

pause