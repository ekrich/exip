# EXIP Examples

This directory contains example programs demonstrating EXI encoding and decoding. See the scripts to run individual example programs.

## Running Examples

```bash
scripts/run-examples
scripts/run-examples.bat
```

## Example Files

### Decoding Examples (simpleDecoding/)

**Schema Files:**
- `exipd-test.xsd` - Source XML Schema document **xml** (1182 bytes)
- `exipd-test-xsd.exi` - Schema encoded in **schema-less exi** (354 bytes)
- `exipd-test-schema-xsd.exi` - Schema encoded in **schema-informed exi** using XML Schema grammar (217 bytes, **39% smaller**)

**Data Files:**
- `exipd-test.exi` - Test data encoded without schema
- `exipd-test-schema.exi` - Test data encoded with schema

### Encoding Examples (simpleEncoding/)

**Schema Files:**
- `exipe-test.xsd` - Main test schema
- `exipe-test-types.xsd` - Type definitions (imported by exipe-test.xsd)
- `exipe-test-nested.xsd` - Nested schema (imported by exipe-test.xsd)
- `exipe-test-xsd.exi`, `exipe-test-types-xsd.exi`, `exipe-test-nested-xsd.exi` - EXI-encoded schemas

**Important:** The `exipe` example encodes a **fixed, hardcoded XML document** defined in its C source code (`encodeTestEXI.c`). **It cannot encode arbitrary XML input.** The hardcoded document conforms to the `exipe-test.xsd` schema described above.

**Data Files:**

The `exipe` tool generates test data files saved to `../simpleDecoding/` for round-trip testing:
- `exipe-test.exi` - Test data encoded in **schema-less mode**
- `exipe-test-schema.exi` - Test data encoded in **schema-informed mode**

These files are used by `exipd` for decoding tests and sizing comparisons.

## Schema-Informed vs Schema-Less Encoding

### Creating exipd-test-xsd.exi (Schema-Less Mode)

1. Start with: `exipd-test.xsd`
2. Encode in **schema-less mode** with Preserve.prefixes
3. No schema grammar needed - uses built-in EXI rules
4. Results in: `exipd-test-xsd.exi`

**Characteristics:**
- No schema required during encoding
- Uses built-in EXI grammar rules
- Larger file size but more flexible
- Can be decoded without providing a schema

### Creating exipd-test-schema-xsd.exi (Schema Mode)

1. Start with: `exipd-test.xsd`
2. Since exipd-test.xsd is an XML Schema document, it follows the W3C XML Schema specification
3. Provide the XML Schema grammar to the encoder: `XMLSchema.xsd` + `xml.xsd` (both encoded as .exi files)
4. The encoder uses that grammar to compress the file
5. Results in: `exipd-test-schema-xsd.exi`

**Characteristics:**
- Requires schema grammar during encoding
- More efficient compression
- XMLSchema.xsd depends on xml.xsd, so both must be provided as exi (src/grammarGen/xmlSchema)
- Requires the same schema grammar for decoding

## GRAMMAR_GEN_SCHEMA Support

To encode/decode XML Schema files in schema mode, EXIP includes built-in support for the W3C XML Schema specification:

**Linux/macOS** (build/gcc/Makefile):
```makefile
CFLAGS += -DGRAMMAR_GEN_SCHEMA
LIB_SOURCES += staticXmlSchema.c
```

**Windows/VS2022** (build/vs2022/exip.vcxproj):
```xml
<PreprocessorDefinitions>GRAMMAR_GEN_SCHEMA;...</PreprocessorDefinitions>
<ClCompile Include="..\..\src\grammarGen\xmlSchema\staticXmlSchema.c" />
```

The `staticXmlSchema.c` file contains the pre-compiled grammar for both:
- **XMLSchema.xsd** - The W3C XML Schema specification
- **xml.xsd** - The XML namespace schema (required by XMLSchema.xsd)

This allows EXIP to efficiently encode/decode XSD files using schema-informed mode.
