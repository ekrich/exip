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

echo [1] exipd schema example (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%EXAMPLE_DIR%\exipd-test-xsd.exi %EXAMPLE_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [2] exipd schema example (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%EXAMPLE_DIR%\exipd-test-xsd.exi %EXAMPLE_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [3] exipd schema example (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%EXAMPLE_DIR%\exipd-test-schema-xsd.exi %EXAMPLE_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [4] exipd schema example (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%EXAMPLE_DIR%\exipd-test-schema-xsd.exi %EXAMPLE_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [5] exipd schemaless example
%EXE_DIR%\exipd.exe -xml %EXAMPLE_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [6] exipd with exipe-test (schema)
%EXE_DIR%\exipd.exe -xml -schema=examples\simpleEncoding\exipe-test-xsd.exi,examples\simpleEncoding\exipe-test-nested-xsd.exi,examples\simpleEncoding\exipe-test-types-xsd.exi examples\simpleEncoding\exipe-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [7] exipd with exipe-test (schemaless)
%EXE_DIR%\exipd.exe -xml examples\simpleEncoding\exipe-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [8] exipd XML Schema document
%EXE_DIR%\exipd.exe -xml -schema=src\grammarGen\xmlSchema\XMLSchema-xsd.exi,src\grammarGen\xmlSchema\xml-xsd.exi src\grammarGen\xmlSchema\XMLSchema-schema-xsd.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo exipe (encode)
echo.

echo [9] exipe schema example
%EXE_DIR%\exipe.exe -schema=examples\simpleEncoding\exipe-test-xsd.exi,examples\simpleEncoding\exipe-test-types-xsd.exi,examples\simpleEncoding\exipe-test-nested-xsd.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [10] exipe schemaless example
%EXE_DIR%\exipe.exe
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo exipg (grammar generation)
echo.

echo [11] exipg text output for EXIOptions schema
%EXE_DIR%\exipg.exe -text -ops=0001000 -schema=utils\schemaGen\EXIOptions-xsd.exi
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
