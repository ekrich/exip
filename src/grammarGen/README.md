# Grammar Generation Module

This module converts schema information to EXIP grammar definitions for schema-informed processing.

## Generated Files

### xmlSchema/staticXmlSchema.c

This file contains the pre-compiled grammar for the W3C XML Schema specification. It enables EXIP to process XSD files using schema-informed mode, which is required for encoding/decoding XML Schema documents.

The grammar includes:
- **XMLSchema.xsd** - The W3C XML Schema specification
- **xml.xsd** - The XML namespace schema (required by XMLSchema.xsd)

**Generation command:**

```bash
exipg -static=10:0:1:5 -pfx=xmlscm_ \
  -schema=src/grammarGen/xmlSchema/XMLSchema-xsd.exi,src/grammarGen/xmlSchema/xml-xsd.exi \
  src/grammarGen/xmlSchema/staticXmlSchema.c
```

**Parameters:**
- `-static=10:0:1:5` - Generate static grammar with deviations (uri:ln:pfx:grammars)
  - 10 additional URI namespaces beyond the schema
  - 0 additional local names per namespace
  - 1 additional prefix per namespace
  - 5 additional built-in grammars
- `-pfx=xmlscm_` - Use "xmlscm_" prefix for generated symbols
- `-schema=` - Paths to XMLSchema and xml schemas (both EXI-encoded, comma-separated)
- Output file: `src/grammarGen/xmlSchema/staticXmlSchema.c`

**Note:** This command should be run from the repository root directory after building the `exipg` tool.

## GRAMMAR_GEN_SCHEMA Flag

When the `GRAMMAR_GEN_SCHEMA` preprocessor flag is defined, the staticXmlSchema.c grammar is compiled into the library, enabling schema-informed processing of XSD files on all platforms.

See [examples/README.md](../../examples/README.md) for build configuration details.
