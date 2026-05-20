@echo off
setlocal EnableDelayedExpansion

:: Run examples
:: Run from exip directory: scripts\run-examples.bat

:: Initialize overall exit code to 0 (success)
set EXITCODE=0

:: Default to Debug, but allow Release as first argument
set "CONFIG=Debug"
if /i "%~1"=="Release" set "CONFIG=Release"
if /i "%~1"=="release" set "CONFIG=Release"

:: Windows uses backslash
set "EXE_DIR=build\vs2022\%CONFIG%"
set "EXAMPLE_DIR=examples\simpleDecoding"

echo Running examples...

:: Check if executables exist
if not exist "%EXE_DIR%\exipd.exe" (
    echo Error: %EXE_DIR%\exipd.exe not found. Please run build first. >&2
    exit /b 1
)

echo.
echo exipd (decode)
echo.

echo [1] Decoding with schema (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%EXAMPLE_DIR%\exipd-test-xsd.exi %EXAMPLE_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo [2] Decoding with schema (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%EXAMPLE_DIR%\exipd-test-xsd.exi %EXAMPLE_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo [3] Decoding with XML Schema grammar (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%EXAMPLE_DIR%\exipd-test-schema-xsd.exi %EXAMPLE_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo [4] Decoding with XML Schema grammar (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%EXAMPLE_DIR%\exipd-test-schema-xsd.exi %EXAMPLE_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo exipe (encode)
echo.

echo [5] Encoding with multiple schemas
%EXE_DIR%\exipe.exe -schema=%EXAMPLE_DIR%\exipe-test-xsd.exi,%EXAMPLE_DIR%\exipe-test-types-xsd.exi,%EXAMPLE_DIR%\exipe-test-nested-xsd.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

:: Report error or success
echo.
if %EXITCODE% neq 0 (
    echo *** One or more examples failed. ***
) else (
    echo Done. All examples ran successfully.
)

:: Return without terminating shell
exit /b %EXITCODE%
