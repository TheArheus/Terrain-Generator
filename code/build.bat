@echo off
if not defined DevEnvDir (
    call vcvarsall x64
)

set LIB_VCPKG="F:\Env\vcpkg\installed\x64-windows\lib"
set INC_VCPKG="F:\Env\vcpkg\installed\x64-windows\include"

set CompileFlags=-MTd -nologo -fp:fast -EHa -Od -WX- -W4 -Oi -GR- -Gm- -GS -wd4505 -wd4100 -wd4201 -FC -Z7 -I %INC_VCPKG%
set LinkFlags=-opt:ref -incremental:no /SUBSYSTEM:console /NODEFAULTLIB:no /LIBPATH:%LIB_VCPKG%

if not exist ..\build mkdir ..\build
pushd ..\build
cl %CompileFlags% SDL2main.lib SDL2.lib msvcrt.lib libcmtd.lib ..\code\PTerr.cpp /link %LinkFlags%
popd
