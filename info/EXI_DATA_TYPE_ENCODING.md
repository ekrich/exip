# EXI Data Type Encoding in EXIP

Summary of how EXIP encodes different data types in EXI format.

## Overview

EXI encoding behavior depends on whether a schema is available:

- **Schema-less mode**: All data encoded as strings, no type conversion
- **Schema-informed mode**: Types converted to efficient binary representations

## String Types

### ASCII Strings
- **Module**: `ASCII_stringManipulate.c`
- **Purpose**: Fast path for ASCII-only content
- **Encoding**: 7-bit or 8-bit per character depending on character set
- **Whitespace**: Preserved as-is in schema-less mode

### Unicode Strings
- Encoded as UTF-8 or UCS (Universal Character Set)
- Supports full Unicode code points
- Uses different code path than ASCII

### Whitespace Normalization (Schema-informed only)
Based on schema's `whiteSpace` facet:
- **preserve**: Keep all whitespace as-is
- **replace**: Replace tabs/newlines/CR with spaces
- **collapse**: Replace + collapse multiple spaces to single space

All numeric types have implicit `whiteSpace="collapse"`.

## Numeric Types

### Integers
- **Encoding**: Variable-length integers (varints)
- **Sign handling**: Unsigned encoding with sign bit for signed types
- **Size**: Efficient for small values (1-8+ bytes depending on magnitude)
- **Schema optimization**: Can use ranges (minInclusive/maxInclusive) for even better compression

Examples:
- `42` → 1-2 bytes
- `1000` → 2 bytes
- `123456789` → 4-5 bytes

### Floats and Doubles
- **Encoding**: IEEE 754 binary format
- **Float**: Always 32 bits (4 bytes)
- **Double**: Always 64 bits (8 bytes)
- **No variable-length encoding** - structure doesn't allow it
- **Scientific notation**: Parsed from XML (e.g., `1.23e5`) but always stored as IEEE 754

| Type | Size | Mantissa Bits | Decimal Digits | Range |
|------|------|---------------|----------------|-------|
| `xs:float` | 4 bytes | 24 | ~7 digits | ±3.4×10³⁸ |
| `xs:double` | 8 bytes | 53 | ~15-17 digits | ±1.7×10³⁰⁸ |

**Important**: These are **approximate** values - precision loss occurs beyond the decimal digit limits.

### Decimals (`xs:decimal`)
- **Encoding**: Two varints (mantissa + exponent)
- **Variable size**: 2-10+ bytes depending on values
- **Representation**: `123.45` → mantissa=12345, exponent=-2
- **No E-notation**: XML Schema doesn't allow scientific notation for decimals

Examples:
- `1.0` → ~2 bytes (mantissa=10, exponent=-1)
- `123.45` → ~3-4 bytes (mantissa=12345, exponent=-2)
- `123456789.123456789` → ~9-10 bytes

**Large Number Example: 5×10⁵⁴**

XML must write out all digits (no E-notation allowed):
```xml
<bigNumber>500000000000000000000000000000000000000000000000000000</bigNumber>
```
That's 55 digits (5 followed by 54 zeros).

EXI encoding:
- Mantissa: `5` → varint (~1 byte)
- Exponent: `54` → varint (~1 byte)
- **Total: ~2 bytes** (vs ~55 bytes as text!)

**Scientific Constant Example: Avogadro's Number (6.02214076×10²³)**

The precise value is `602214076000000000000000` (24 digits).

For `xs:double` with scientific notation:
```xml
<avogadro>6.02214076e23</avogadro>
```
- IEEE 754 double: 8 bytes
- **Precision loss**: Only ~15-17 digits preserved, so digits beyond that are approximated
- Stored approximately as: 6.02214076×10²³ (good to ~16 digits)

For `xs:decimal` (must write without E-notation):
```xml
<avogadro>602214076000000000000000</avogadro>
```
That's 24 digits - all preserved exactly.

EXI encoding:
- Mantissa: `602214076` → varint (~4-5 bytes)
- Exponent: `15` → varint (~1 byte)
- **Total: ~5-6 bytes**
- **No precision loss** - exact representation

**Trade-off**: `xs:double` uses fixed 8 bytes but loses precision beyond ~16 digits. `xs:decimal` uses 5-6 bytes and preserves all 24 digits exactly.

### When to Use Each

| Type | Fixed Size | Best For |
|------|-----------|----------|
| `xs:integer` | Variable | Whole numbers, counters |
| `xs:float` | 4 bytes | Approximate scientific values (±3.4×10³⁸) |
| `xs:double` | 8 bytes | High precision scientific (±1.7×10³⁰⁸) |
| `xs:decimal` | Variable | Financial data, exact decimals, large integers |

## Booleans
- **Encoding**: Single bit
- `false` = 0, `true` = 1

## Date/Time Types
- **Encoding**: Structured integers (year, month, day, hour, minute, second, fractional seconds)
- Each component encoded as varint
- No string representation in binary

## Schema-less vs Schema-informed Behavior

### Example: `<number>  42  </number>`

**Schema-less (built-in grammar):**
1. Encode as string "  42  " (whitespace preserved)
2. No type conversion
3. Size: ~4-5 bytes (string encoding)

**Schema-informed with `<xs:element name="number" type="xs:int"/>`:**
1. Apply `whiteSpace="collapse"`: "  42  " → "42"
2. Parse to integer: 42
3. Encode as varint
4. Size: ~1-2 bytes

### Example: `<temperature>98.6</temperature>`

**Schema-less:**
- String "98.6" (~4 bytes)

**Schema-informed with `type="xs:float"`:**
- Apply whitespace collapse
- Parse to float
- IEEE 754 encoding (4 bytes)

**Schema-informed with `type="xs:decimal"`:**
- Mantissa=986, exponent=-1
- ~2-3 bytes

## String Tables

EXI maintains string tables for:
- Element/attribute names
- Namespace URIs
- String values (in schema-less or when preserve.lexicalValues is set)

Repeated strings are indexed, reducing size for repetitive data.

## Key Points

1. **Schema is crucial for efficiency**: Type information enables compact binary encoding
2. **Whitespace normalization**: Only happens in schema-informed mode based on type
3. **No varint for floats**: IEEE 754 structure prevents variable-length encoding
4. **Decimal is special**: Uses varint mantissa+exponent, can be very compact for simple values
5. **ASCII fast path**: Separate code path for ASCII-only content
6. **Validating parsers do the same**: EXIP's whitespace/type handling matches XML schema validators

## Implementation

- **Grammar generation**: `generateSchemaInformedGrammars()` parses schema and builds grammars
- **String handling**: `ASCII_stringManipulate.c` for ASCII, separate paths for Unicode
- **Type conversion**: Happens during encoding based on grammar-specified types
- **Whitespace facet**: Stored in grammar after PR #69 fix, applied during encoding
