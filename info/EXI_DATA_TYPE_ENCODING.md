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
- **Default Encoding**: Variable-length integers (varints)
- **Sign handling**: Sign bit + unsigned varint with offset for negatives
- **Size**: Efficient for small values (1-8+ bytes depending on magnitude)

**Signed Integer Encoding** (see [streamEncode.c:139-159](../src/streamIO/src/streamEncode.c#L139-L159)):

1. Write sign bit: `0` for positive/zero, `1` for negative
2. **Positive** `n`: encode varint(n)
3. **Negative** `-n`: encode varint(n - 1)

**Why the offset?** This handles `INT_MIN` safely:
- Without offset: negating `INT_MIN` (-2⁶³) would overflow since `abs(INT_MIN) > INT_MAX`
- With offset: `INT_MIN + 1` can be safely negated to `INT_MAX`

**Examples**:
- `5`: sign=0, varint(5)
- `-5`: sign=1, varint(4) → `-5` becomes `5-1=4`
- `-1`: sign=1, varint(0) → `-1` becomes `1-1=0`
- `INT_MIN`: sign=1, varint(INT_MAX)

**Decoding** (see [streamDecode.c:148-167](../src/streamIO/src/streamDecode.c#L148-L167)):
- Read sign bit
- Read unsigned varint → `val`
- If sign=0: result = `val`
- If sign=1: result = `-(val + 1)`

**Schema Optimization - Bounded Ranges**:

When schema defines `minInclusive`/`maxInclusive` facets, EXI uses fixed n-bit encoding instead of varint:

**Algorithm**:
1. Subtract minimum: `encoded_value = actual_value - min`
2. Calculate range: `range = max - min`
3. Determine bits needed: `n = ceil(log2(range + 1))`
4. Encode as n-bit unsigned integer

**Examples**:

`<xs:simpleType><xs:restriction base="xs:int"><xs:minInclusive value="0"/><xs:maxInclusive value="100"/></xs:restriction></xs:simpleType>`
- Range: 0-100 (101 values)
- Bits needed: 7 bits (2⁷ = 128 ≥ 101)
- Value 55: `55 - 0 = 55` → encoded in 7 bits
- **Always 7 bits, no varint overhead**

`<xs:simpleType><xs:restriction base="xs:int"><xs:minInclusive value="-50"/><xs:maxInclusive value="50"/></xs:restriction></xs:simpleType>`
- Range: -50 to 50 (101 values)
- Bits needed: 7 bits
- Value -50: `-50 - (-50) = 0` → encoded in 7 bits
- Value 0: `0 - (-50) = 50` → encoded in 7 bits
- Value 50: `50 - (-50) = 100` → encoded in 7 bits
- **No sign bit needed** - offset converts to unsigned range 0-100

**Benefits**:
- Much more compact than varint for bounded ranges
- Eliminates sign bit overhead for ranges spanning negative/positive
- Predictable bit width simplifies parsing

**Varint Examples** (unbounded or very large ranges):
- `42` → 1-2 bytes
- `1000` → 2 bytes
- `123456789` → 4-5 bytes

### Floats and Doubles (`xs:float`, `xs:double`)
- **Encoding**: Two varints (mantissa + exponent), same as decimals
- **Variable size**: 2-10+ bytes depending on values
- **Representation**: `45.4` → mantissa=454, exponent=-1
- **Base-10**: Unlike IEEE 754, EXI uses decimal (base-10) representation
- **Scientific notation**: Supported (e.g., `1.23e5`)

| Type | Mantissa Range | Typical Size | Precision |
|------|---------------|--------------|-----------|
| `xs:float` | 32-bit signed | 2-6 bytes | ~7 decimal digits |
| `xs:double` | 64-bit signed | 2-10 bytes | ~15-17 decimal digits |

**Important**:
- EXI does NOT use IEEE 754 binary format for encoding
- Values are converted to decimal mantissa/exponent pairs
- Each component (mantissa, exponent) is encoded as a varint
- More compact than fixed-size IEEE 754 for many values

Examples:
- `45.4` → mantissa=454, exponent=-1 → ~2-3 bytes
- `1.23e5` → mantissa=123, exponent=3 → ~2-3 bytes
- `0.0001` → mantissa=1, exponent=-4 → ~2 bytes

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
EXI encoding (6.02214076 × 10²³ = 602214076 × 10¹⁵):
- Mantissa: `602214076` → varint (~4-5 bytes)
- Exponent: `15` → varint (~1 byte)
- **Total: ~5-6 bytes**
- **Precision**: Depends on how many digits preserved in mantissa (typically ~15-17 for double)

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

**Note**: In EXI wire format, `xs:double` and `xs:decimal` use the same mantissa/exponent encoding. The difference is semantic - `xs:double` implies approximation with limited precision, while `xs:decimal` implies exact representation.

### When to Use Each

| Type | Size | Best For |
|------|------|----------|
| `xs:integer` | Variable | Whole numbers, counters |
| `xs:float` | Variable | Approximate scientific values with ~7 digit precision |
| `xs:double` | Variable | High precision scientific with ~15-17 digit precision |
| `xs:decimal` | Variable | Financial data, exact decimals, no precision loss |

**Note**: All floating-point types (`xs:float`, `xs:double`, `xs:decimal`) use the same mantissa/exponent encoding in EXI. The difference is in precision limits and semantic meaning in XML Schema, not in the wire format.

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
