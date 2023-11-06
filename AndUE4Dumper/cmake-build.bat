@ECHO OFF

SET "NDK=%NDK_HOME%"
SET "ABI=arm64-v8a"
SET "HOST=windows-x86_64"
SET "CMAKE=cmake"
SET "MAKE=make"

SET "BUILD_PATH=cmake_build/%ABI%"

CMAKE -S. -B%BUILD_PATH% -G "Unix Makefiles" -Wno-unused-command-line-argument ^
-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_TOOLCHAIN_FILE=%NDK%/build/cmake/android.toolchain.cmake ^
-DCMAKE_C_COMPILER=%NDK_HOME%\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe ^
-DCMAKE_CXX_COMPILER=%NDK_HOME%\toolchains\llvm\prebuilt\windows-x86_64\bin\clang++.exe ^
-DANDROID_NDK=%NDK% ^
-DANDROID_ABI=%ABI% ^
-DANDROID_NATIVE_API_LEVEL=21

MAKE -C%BUILD_PATH% -j16

PAUSE
