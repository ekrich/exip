# Unimplemented Features

Features that return `EXIP_NOT_IMPLEMENTED_YET` or are incomplete.

## HIGH Priority - Blocks Common Schemas

**Wildcards** - [treeTableToGrammars.c](../src/grammarGen/src/treeTableToGrammars.c)
- `xs:any` and `xs:anyAttribute` - Code exists but crashes on schemas with wildcards
- Critical for schema evolution and extensibility patterns
- See [WILDCARD_CRASH_ISSUE.md](../WILDCARD_CRASH_ISSUE.md)

**Whitespace normalization not generated or applied** - [treeTableToGrammars.c](../src/grammarGen/src/treeTableToGrammars.c)
- Grammar generation doesn't populate `SimpleType.whiteSpace` field correctly
- Static grammars have missing 5th field: [staticXmlSchema.c:15363](../src/grammarGen/xmlSchema/staticXmlSchema.c), [staticEXIOptions.c:2729](../src/contentIO/src/staticEXIOptions.c)
- Encode/decode paths check the field but it's always `WHITESPACE_PRESERVE` (0)
- **Root cause:** Grammar generator doesn't extract whitespace facet from XSD
- **Impact:** Schema-informed encoding/decoding doesn't apply xs:whiteSpace normalization rules
- **Example:** xs:token should collapse whitespace, but EXIP preserves it
- **Fix needed:**
  1. Extract whitespace facet in grammar generation ([treeTableToGrammars.c:1785-1786](../src/grammarGen/src/treeTableToGrammars.c))
  2. Regenerate static grammars with correct values
  3. Verify normalization applied correctly in encode/decode ([bodyEncode.c:42](../src/contentIO/src/bodyEncode.c), [bodyDecode.c:1256](../src/contentIO/src/bodyDecode.c))

## MEDIUM Priority - Useful Features

**Binary data API signature** - [EXISerializer.h](../include/EXISerializer.h)
- `serialize.binaryData()` takes `const char* octets` - should be `const uint8_t*`
- Binary data is semantically unsigned bytes, not characters
- Current signature causes pointer-sign warnings in generated binding code
- Similar issue in Java: `byte` is signed but universally used for binary data
- **Impact:** Generated code needs casts: `serialize.binaryData(strm, (const char*)data, len)`
- **Fix:** Change signature to match semantics, update all call sites

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
