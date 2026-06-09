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
set "DECODING_DIR=examples\simpleDecoding"
set "ENCODING_DIR=examples\simpleEncoding"

echo Running examples...

:: Check if executables exist
if not exist "%EXE_DIR%\exipd.exe" (
    echo Error: %EXE_DIR%\exipd.exe not found. Please run build first. >&2
    exit /b 1
)

echo.
echo exipg (grammar generation)
echo.

echo [1] exipg text output for EXIOptions schema
%EXE_DIR%\exipg.exe -text -ops=0001000 -schema=utils\schemaGen\EXIOptions-xsd.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo exipe (encode)
echo.

echo [2] exipe schema-informed example
%EXE_DIR%\exipe.exe -schema=%ENCODING_DIR%\exipe-test-xsd.exi,%ENCODING_DIR%\exipe-test-types-xsd.exi,%ENCODING_DIR%\exipe-test-nested-xsd.exi %DECODING_DIR%\exipe-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [3] exipe schemaless example
%EXE_DIR%\exipe.exe %DECODING_DIR%\exipe-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo.
echo exipd (decode)
echo.

echo [4] exipd schema example (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%DECODING_DIR%\exipd-test-xsd.exi %DECODING_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [5] exipd schema example (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%DECODING_DIR%\exipd-test-xsd.exi %DECODING_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [6] exipd schema example (XML output)
%EXE_DIR%\exipd.exe -xml -schema=%DECODING_DIR%\exipd-test-schema-xsd.exi %DECODING_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [7] exipd schema example (EXI output)
%EXE_DIR%\exipd.exe -exi -schema=%DECODING_DIR%\exipd-test-schema-xsd.exi %DECODING_DIR%\exipd-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [8] exipd XML Schema document
%EXE_DIR%\exipd.exe -xml -schema=src\grammarGen\xmlSchema\XMLSchema-xsd.exi,src\grammarGen\xmlSchema\xml-xsd.exi src\grammarGen\xmlSchema\XMLSchema-schema-xsd.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [9] exipd schemaless example
%EXE_DIR%\exipd.exe -xml %DECODING_DIR%\exipd-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [10] exipd with exipe-test (schema-informed roundtrip)
%EXE_DIR%\exipd.exe -xml -schema=%ENCODING_DIR%\exipe-test-xsd.exi,%ENCODING_DIR%\exipe-test-nested-xsd.exi,%ENCODING_DIR%\exipe-test-types-xsd.exi %DECODING_DIR%\exipe-test-schema.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

echo [11] exipd with exipe-test (schemaless roundtrip)
%EXE_DIR%\exipd.exe -xml %DECODING_DIR%\exipe-test.exi
if !ERRORLEVEL! neq 0 set "EXITCODE=!ERRORLEVEL!"

:: Check if .exi files changed (only if git available and files tracked)
git --version >nul 2>&1
if !ERRORLEVEL! equ 0 (
    git ls-files --error-unmatch %DECODING_DIR%\exipe-test-schema.exi >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        git diff --quiet %DECODING_DIR%\exipe-test-schema.exi
        if !ERRORLEVEL! neq 0 (
            echo WARNING: exipe-test-schema.exi changed after encoding
            set "EXITCODE=1"
        )
    )
    git ls-files --error-unmatch %DECODING_DIR%\exipe-test.exi >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        git diff --quiet %DECODING_DIR%\exipe-test.exi
        if !ERRORLEVEL! neq 0 (
            echo WARNING: exipe-test.exi changed after encoding
            set "EXITCODE=1"
        )
    )
)

:: Report error or success
echo.
if %EXITCODE% neq 0 (
    echo *** One or more examples failed. ***
) else (
    echo Done. All examples ran successfully.
)

:: Return without terminating shell
exit /b %EXITCODE%
