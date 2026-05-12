# Unimplemented Features

Features that return `EXIP_NOT_IMPLEMENTED_YET` or are incomplete.

## HIGH Priority - Blocks Common Schemas

**Wildcards** - [treeTableToGrammars.c](../src/grammarGen/src/treeTableToGrammars.c)
- `xs:any` and `xs:anyAttribute` - Code exists but crashes on schemas with wildcards
- Critical for schema evolution and extensibility patterns
- See [WILDCARD_CRASH_ISSUE.md](../WILDCARD_CRASH_ISSUE.md)

## MEDIUM Priority - Useful Features

**Non-string enumerations** - [treeTableToGrammars.c:1946-1976](../src/grammarGen/src/treeTableToGrammars.c)
- Typed enums: boolean, integer, decimal, float, dateTime
- String enums work; missing typed enums require runtime conversions

**Pattern facet** - [treeTableToGrammars.c:1778](../src/grammarGen/src/treeTableToGrammars.c)
- Regex patterns logged but not enforced
- Could enable restricted character set encoding (better compression)

**Schema imports/includes** - [treeTableBuild.c:168](../src/grammarGen/src/treeTableBuild.c)
- `xs:import` and `xs:include` not processed automatically
- Workaround: compile each schema to EXI, pass all on command line (up to 10 schemas): `exipg -schema=main.xsd.exi,imported1.xsd.exi,imported2.xsd.exi`

## LOW Priority - Rarely Needed

**`xs:all` compositor** - [treeTableToGrammars.c:1539](../src/grammarGen/src/treeTableToGrammars.c)
- Unordered child elements (N! grammar permutations)
- Rarely used; workaround: `xs:sequence` with `minOccurs="0"`

**Anonymous simple type in restriction** - [treeTableToGrammars.c:1699](../src/grammarGen/src/treeTableToGrammars.c)
- Restriction without base attribute
- "Very weird use case... does not bring useful features"

## Optional Features (Conditionally Compiled)

**Type-to-string conversions** - [ASCII_stringManipulate.c:250-270](../src/common/src/ASCII_stringManipulate.c)
- `integerToString()`, `booleanToString()`, `floatToString()`, `decimalToString()`, `dateTimeToString()`
- Only for `EXIP_IMPLICIT_DATA_TYPE_CONVERSION` mode
- Needed for JAXB-style code generation with schema-less fallback

## Runtime Edge Cases

Various runtime encoding/decoding code paths return `EXIP_NOT_IMPLEMENTED_YET`. These are scattered throughout:
- [EXISerializer.c](../src/contentIO/src/EXISerializer.c) - Self-contained elements, DTD preservation, various value types
- [bodyEncode.c](../src/contentIO/src/bodyEncode.c) / [bodyDecode.c](../src/contentIO/src/bodyDecode.c) - Edge cases in encoding/decoding
- [headerEncode.c](../src/contentIO/src/headerEncode.c) - Preserve options encoding

Search for `EXIP_NOT_IMPLEMENTED_YET` in these files to identify specific cases.
