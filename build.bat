@echo off

set Argument=%1

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7
set CommonCompilerFlags=-D_CRT_SECURE_NO_WARNINGS %CommonCompilerFlags%
set CommonLinkerFlags= -incremental:no -opt:ref 

IF NOT EXIST build mkdir build
pushd build

cl %CommonCompilerFlags% -Fe:compiler ..\*.cpp /link %CommonLinkerFlags%
set LastError=%ERRORLEVEL%
popd

IF NOT %LastError%==0 GOTO :end

IF %Argument%.==. (
    ECHO "Missing file name."
) ELSE (
    build\compiler.exe %Argument%

    IF NOT EXIST result mkdir result
    pushd result
    cl %CommonCompilerFlags% -Fe:result ..\result.c /link %CommonLinkerFlags%
    set LastError2=%ERRORLEVEL%
    popd
)

:end
