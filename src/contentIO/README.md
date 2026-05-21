# Content I/O Module

This module handles reading and writing XML Information Set content in EXI format, including elements, attributes, and values.

## Generated Files

### staticEXIOptions.c

This file contains pre-compiled EXI grammar for the EXIOptions schema. It is generated using `exipg` and should be regenerated if the EXIOptions schema changes.

**Generation command:**

```bash
exipg -static -pfx=ops_ -ops=0001000 \
  -schema=utils/schemaHandling/EXIOptions-xsd.exi \
  src/contentIO/src/staticEXIOptions.c
```

**Parameters:**
- `-static` - Generate static grammar code
- `-pfx=ops_` - Use "ops_" prefix for generated symbols
- `-ops=0001000` - EXI options mask (Preserve.prefixes)
- `-schema=` - Path to EXIOptions schema (EXI-encoded)
- Output file: `src/contentIO/src/staticEXIOptions.c`

**Note:** This command should be run from the repository root directory after building the `exipg` tool.
