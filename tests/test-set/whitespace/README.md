# XML Schema whiteSpace Facet Test

## Overview

Tests EXIP's handling of the XML Schema `whiteSpace` facet, which controls how whitespace characters are normalized in string values.

## XML Schema whiteSpace Facet Specification

### Reference
- **W3C XML Schema Part 2: Datatypes Second Edition**
- Section 4.3.6: whiteSpace
- URL: https://www.w3.org/TR/xmlschema-2/#rf-whiteSpace

### Valid Values

| Value | Description | Behavior |
|-------|-------------|----------|
| `preserve` | Preserve all whitespace | No normalization; all whitespace characters (space, tab, newline, carriage return) are preserved as-is |
| `replace` | Replace whitespace | Each tab (U+0009), newline (U+000A), and carriage return (U+000D) is replaced with a space (U+0020) |
| `collapse` | Collapse whitespace | After replacing as above, sequences of spaces are collapsed to a single space, and leading/trailing spaces are removed |

### Default Values by Type

The default `whiteSpace` value depends on the base type:

| Base Type | Default whiteSpace | Rationale |
|-----------|-------------------|-----------|
| `xs:string` | `preserve` | Strings should keep all whitespace by default |
| `xs:normalizedString` | `replace` | Normalized strings replace control chars with spaces |
| `xs:token` | `collapse` | Tokens are typically whitespace-trimmed identifiers |
| `xs:Name`, `xs:NCName`, `xs:ID`, etc. | `collapse` | Derived from `xs:token`, inherit collapse |
| Numeric types (`xs:integer`, `xs:decimal`, etc.) | `collapse` | Numbers don't have meaningful whitespace |
| `xs:boolean` | `collapse` | Boolean values are tokens (`true`/`false`) |
| `xs:date`, `xs:time`, `xs:dateTime` | `collapse` | Date/time formats don't preserve whitespace |

### Inheritance

When you define a restriction on a type:
- The `whiteSpace` facet can be **restricted further** but **not relaxed**
- Valid transitions:
  - `preserve` → `replace` ✓
  - `preserve` → `collapse` ✓
  - `replace` → `collapse` ✓
  - `collapse` → `replace` ✗ (invalid - relaxes restriction)
  - `replace` → `preserve` ✗ (invalid - relaxes restriction)

## Test Files

### whitespace-test.xsd

XML Schema demonstrating all three `whiteSpace` values:

```xml
<xs:simpleType name="PreservedString">
  <xs:restriction base="xs:string">
    <xs:whiteSpace value="preserve"/>
  </xs:restriction>
</xs:simpleType>
```

This schema defines three custom string types, each with explicit `whiteSpace` facets, to test that EXIP correctly:
1. Parses the facet from the schema
2. Stores the value in the grammar
3. Doesn't crash with `EXIP_NOT_IMPLEMENTED_YET`

### whitespace-test-xsd.exi

EXI-encoded version of the schema for use with `generateSchemaInformedGrammars()`.

**To generate using EXIficient GUI:**
```bash
java -jar exificient-gui-jar-with-dependencies.jar
```

**Required settings:**
- ☑ **Use Schema: None** (schema-less mode)
- ☑ **Preserve Namespace Declarations** (enables Preserve.prefixes)
- ☑ **Include options** (embeds options in EXI header - no out-of-band needed)
- Input: `whitespace-test.xsd`
- Output: `whitespace-test-xsd.exi`

**Why these settings:**
- Schema-less encoding prevents circularity (encoding a schema with itself)
- Preserve.prefixes required for correct QName handling in schema processing
- Including options in header avoids need for `-ops=` argument with exipg

## Expected Test Behavior

### Before Fix
```c
tmp_err_code = generateSchemaInformedGrammars(&buffer, 1, SCHEMA_FORMAT_XSD_EXI,
                                              NULL, &schema, NULL);
// Returns: EXIP_NOT_IMPLEMENTED_YET (when encountering ELEMENT_WHITE_SPACE)
```

### After Fix
```c
tmp_err_code = generateSchemaInformedGrammars(&buffer, 1, SCHEMA_FORMAT_XSD_EXI,
                                              NULL, &schema, NULL);
// Returns: EXIP_OK
// Schema contains types with whitespaceValue set to:
//   - WHITESPACE_PRESERVE (for PreservedString)
//   - WHITESPACE_REPLACE  (for ReplacedString)
//   - WHITESPACE_COLLAPSE (for CollapsedString)
```

## Implementation Notes

### Why Test All Three Values?

Testing all three values ensures:
1. **Correct parsing** - EXIP reads "preserve", "replace", "collapse" strings correctly
2. **Correct mapping** - Each string maps to the right enum value
3. **No hardcoded assumptions** - The code doesn't assume only one value works

### Why Explicit Facets?

The test uses **explicit** `<xs:whiteSpace value="..."/>` declarations rather than relying on defaults because:
- **Triggers the code path** - Default values don't create `ELEMENT_WHITE_SPACE` nodes in the schema tree
- **Tests the parsing** - We need to verify the string-to-enum conversion works
- **Clear intent** - Makes it obvious what's being tested

## Test Coverage

### 1. Explicit whiteSpace Facets (tests parsing logic)
- ✅ All 3 values: `preserve`, `replace`, `collapse`
- ✅ On `xs:string` (default preserve)
- ✅ On `xs:normalizedString` (default replace)
- ✅ On `xs:token` (default collapse)

### 2. Default Inheritance (tests that omitted facets work)
- ✅ `xs:string` → inherits `preserve`
- ✅ `xs:normalizedString` → inherits `replace`
- ✅ `xs:token` → inherits `collapse`

### 3. Built-in Type Inheritance
- ✅ `xs:Name`, `xs:NCName` (from token → collapse)
- ✅ `xs:integer` (collapse)
- ✅ `xs:boolean` (collapse)
- ✅ `xs:date` (collapse)

This tests both paths through the code:
1. **Parsing path**: When `ELEMENT_WHITE_SPACE` is encountered in schema
2. **Inheritance path**: When types inherit from their base

## Related Code

- `src/grammarGen/include/treeTableSchema.h:85` - `ELEMENT_WHITE_SPACE` enum definition
- `src/grammarGen/src/treeTableToGrammars.c:1780` - whiteSpace facet handling
- `include/procTypes.h` - `WhiteSpaceValue` enum (after implementation)

## References

- [W3C XML Schema Part 2: whiteSpace](https://www.w3.org/TR/xmlschema-2/#rf-whiteSpace)
- [W3C EXI Format Specification](https://www.w3.org/TR/exi/)
