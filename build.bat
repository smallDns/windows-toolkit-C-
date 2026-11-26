@echo off
REM Build com MSVC. Abra o "x64 Native Tools Command Prompt for VS" e rode: build.bat
cl /nologo /O2 /W3 /utf-8 /std:c11 /D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0A00 /Iinclude src\*.c ^
   /Fe:windows-toolkit.exe /Fo:build\ ^
   /link iphlpapi.lib ws2_32.lib psapi.lib powrprof.lib advapi32.lib
