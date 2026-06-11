#!/usr/bin/env bash
# Regenerate static grammar files
# Works on Windows (Git Bash) and Unix
set -eux

# Detect platform
if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then
    # Windows (Git Bash) - Debug build only
    EXIPG="./build/vs2022/Debug/exipg.exe"
else
    # Linux/macOS/WSL
    EXIPG="./bin/exipg"
fi

# Regenerate staticEXIOptions.c
$EXIPG -static -pfx=ops_ -ops=0001000 \
  -schema=utils/schemaGen/EXIOptions-xsd.exi \
  src/contentIO/src/staticEXIOptions.c

# Regenerate staticXmlSchema.c
$EXIPG -static=10:0:1:5 -pfx=xmlscm_ \
  -schema=src/grammarGen/xmlSchema/XMLSchema-xsd.exi,src/grammarGen/xmlSchema/xml-xsd.exi \
  src/grammarGen/xmlSchema/staticXmlSchema.c
