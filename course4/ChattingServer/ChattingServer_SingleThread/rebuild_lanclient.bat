@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
if %ERRORLEVEL% neq 0 (
    echo VCVARS FAILED
    exit /b 1
)
cd /d "D:\프로카데미\Academy\course4\ChattingServer\ChattingServer_SingleThread"
cl.exe /c /ZI /JMC /nologo /W3 /WX- /sdl /Od /D _DEBUG /D _CONSOLE /D _UNICODE /D UNICODE /EHsc /RTC1 /MDd /GS /std:c++17 /permissive- /Fo"Chatting.88a0eac5\x64\Debug\CLanClient.obj" /Fd"Chatting.88a0eac5\x64\Debug\vc143.pdb" /TP /FC /utf-8 CLanClient.cpp
if %ERRORLEVEL% neq 0 (
    echo COMPILE FAILED
    exit /b 2
)
echo COMPILE_SUCCESS
link.exe /OUT:"x64\Debug\ChattingServer_SingleThread.exe" /NOLOGO kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /DEBUG /PDB:"x64\Debug\ChattingServer_SingleThread.pdb" /SUBSYSTEM:CONSOLE /MACHINE:X64 Chatting.88a0eac5\x64\Debug\*.obj
if %ERRORLEVEL% neq 0 (
    echo LINK FAILED
    exit /b 3
)
echo LINK_SUCCESS
