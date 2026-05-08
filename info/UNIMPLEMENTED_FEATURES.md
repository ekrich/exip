# Unimplemented Features in EXIP

## Overview

This document tracks features that return `EXIP_NOT_IMPLEMENTED_YET` in the EXIP codebase.

## Schema Grammar Generation

Located in [src/grammarGen/src/treeTableToGrammars.c](../src/grammarGen/src/treeTableToGrammars.c):

- **Line 1539**: `xs:all` compositor - allows unordered child elements
- **Line 1699**: Anonymous simple type in restriction - "very weird use case" per code comment
- **Line 1778**: **Pattern facet** (regex) - currently logged but not enforced
- **Line 1783**: **whiteSpace facet** - **IN PROGRESS (PR #2)** - controls string normalization
- **Lines 1906-1925**: Enumerations for non-string types (boolean, date/time, decimal, float, integer)

Located in [src/grammarGen/src/treeTableBuild.c](../src/grammarGen/src/treeTableBuild.c):

- **Line 168**: Schema imports/includes - multi-file schemas not supported

Located in [src/grammarGen/src/genUtils.c](../src/grammarGen/src/genUtils.c):

- **Lines 361, 648**: Various utility functions

## String Conversion (Optional Feature)

Located in [src/common/src/ASCII_stringManipulate.c](../src/common/src/ASCII_stringManipulate.c):

Only used when `EXIP_IMPLICIT_DATA_TYPE_CONVERSION` is enabled:

- **Line 250**: `integerToString()`
- **Line 255**: `booleanToString()`
- **Line 260**: `floatToString()`
- **Line 265**: `decimalToString()`
- **Line 270**: `dateTimeToString()`

**Note**: These functions would be essential for code generation (JAXB-style) that supports both schema-informed and schema-less modes. When encoding schema-less EXI from typed C structs, typed fields (int, float, bool) need conversion to strings since schema-less only supports string values. Currently users must use `sprintf()` manually (see exipd example).

## Runtime Encoding/Decoding

Located in [src/contentIO/src/EXISerializer.c](../src/contentIO/src/EXISerializer.c):

- Self-contained elements (lines 154, 334, 474)
- DTD preservation (lines 965, 1026)
- Various value type encodings (lines 1092-1228)

Located in [src/contentIO/src/bodyEncode.c](../src/contentIO/src/bodyEncode.c):

- Various encoding edge cases (lines 327-618)

Located in [src/contentIO/src/bodyDecode.c](../src/contentIO/src/bodyDecode.c):

- Various decoding edge cases (lines 274-1072)

Located in [src/contentIO/src/headerEncode.c](../src/contentIO/src/headerEncode.c):

- **Line 240**: Preserve options encoding

## Schema Handling Utilities

Located in [utils/schemaHandling/createGrammars.c](../utils/schemaHandling/createGrammars.c):

- **Lines 208, 213**: Utility functions
