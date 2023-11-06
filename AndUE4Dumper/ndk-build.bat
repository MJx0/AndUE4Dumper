@echo off
if exist "build" rmdir "build" /S /Q
md build 2> nul
make
move libs build/libs
move obj build/obj
pause