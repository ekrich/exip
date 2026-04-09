#!/usr/bin/env bash
# Test exipg
set -eux

./build/vs2022/Debug/exipg.exe -text -schema="examples/simpleEncoding/exipe-test-xsd.exi,examples/simpleEncoding/exipe-test-types-xsd.exi,examples/simpleEncoding/exipe-test-nested-xsd.exi"
