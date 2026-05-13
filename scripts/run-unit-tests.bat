@echo off
setlocal EnableDelayedExpansion

:: Initialize overall exit code to 0 (success)
set EXITCODE=0
:: Forward slash for C program arg
set "TEST_DIR=tests/test-set"

:: Default to Debug, but allow Release as first argument
set "CONFIG=Debug"
if /i "%~1"=="Release" set "CONFIG=Release"
if /i "%~1"=="release" set "CONFIG=Release"

:: Backslash for Win bat file
set "EXE_DIR=build\vs2022\%CONFIG%"

echo Running tests ...
:: List of test executables separated by spaces.
set "testlist=%EXE_DIR%\check_builtin_grammer.exe %EXE_DIR%\check_contentio.exe %EXE_DIR%\check_emptyType.exe %EXE_DIR%\check_exip.exe %EXE_DIR%\check_grammar.exe %EXE_DIR%\check_profile.exe %EXE_DIR%\check_streamIO.exe %EXE_DIR%\check_stringTables.exe %EXE_DIR%\check_strict_grammar.exe %EXE_DIR%\check_xsi_type.exe"

:: Run each test
set TEST_COUNT=0
for %%x in (%testlist%) do (
    set /a TEST_COUNT+=1
    echo [!TEST_COUNT!] Running %%x %TEST_DIR%
    %%x "%TEST_DIR%"
    set "err=!ERRORLEVEL!"
    if !err! neq 0 (
        echo [!TEST_COUNT!] FAILED with error code: !err!
        set "EXITCODE=!err!"
    )
)

:: Report error or success
echo.
if %EXITCODE% neq 0 (
    echo *** One or more of !TEST_COUNT! test suites failed. ***
) else (
    echo All !TEST_COUNT! test suites ran successfully.
)

:: Return without terminating shell
exit /b %EXITCODE%
