#!/usr/bin/env bash
# Test exipg (Git Bash on Windows or Linux/macOS)
# set -eux

# Detect platform
if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]]; then
    # Windows (Git Bash)
    EXIPG="./build/vs2022/Debug/exipg.exe"
else
    # Linux/macOS/WSL
    EXIPG="./bin/exipg"
fi

$EXIPG -text -schema="examples/simpleEncoding/exipe-test-xsd.exi,examples/simpleEncoding/exipe-test-types-xsd.exi,examples/simpleEncoding/exipe-test-nested-xsd.exi"
$EXIPG -text -pfx -schema="examples/simpleEncoding/exipe-test-xsd.exi,examples/simpleEncoding/exipe-test-types-xsd.exi,examples/simpleEncoding/exipe-test-nested-xsd.exi"
$EXIPG -text -ops -schema="examples/simpleEncoding/exipe-test-xsd.exi,examples/simpleEncoding/exipe-test-types-xsd.exi,examples/simpleEncoding/exipe-test-nested-xsd.exi"
