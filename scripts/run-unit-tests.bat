@echo off
setlocal EnableDelayedExpansion

:: Initialize overall exit code to 0 (success)
set EXITCODE=0
:: Forward slash for C program arg
set "TEST_DIR=tests/test-set"
:: Backslash for Win bat file
set "EXE_DIR=build\vs2022\Debug"

echo Running tests ...
:: List of test executables separated by spaces.
set "testlist=%EXE_DIR%\check_builtin_grammer.exe %EXE_DIR%\check_contentio.exe %EXE_DIR%\check_emptyType.exe %EXE_DIR%\check_exip.exe %EXE_DIR%\check_grammar.exe %EXE_DIR%\check_profile.exe %EXE_DIR%\check_streamIO.exe %EXE_DIR%\check_stringTables.exe %EXE_DIR%\check_xsi_type.exe"

:: Run each test
for %%x in (%testlist%) do (
    echo Running %%x %TEST_DIR%
    %%x "%TEST_DIR%"
    set "err=!ERRORLEVEL!"
    if !err! neq 0 (
        echo %%x test failed with error code: !err!
        set "EXITCODE=!err!"
    )
)

:: Report error or success
if %EXITCODE% neq 0 (
    echo *** One or more tests failed. ***
) else (
    echo All tests ran successfully.
)

:: Return without terminating shell
exit /b %EXITCODE%