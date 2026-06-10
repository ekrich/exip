# Content I/O Module

This module handles reading and writing XML Information Set content in EXI format, including elements, attributes, and values.

## Generated Files

### staticEXIOptions.c

This file contains pre-compiled EXI grammar for the EXIOptions schema. It is generated using `exipg` and should be regenerated if the EXIOptions schema changes.

**Generation command:**

```bash
exipg -static -pfx=ops_ -ops=0001000 \
  -schema=utils/schemaGen/EXIOptions-xsd.exi \
  src/contentIO/src/staticEXIOptions.c
```

**Parameters:**
- `-static` - Generate static grammar code
- `-pfx=ops_` - Use "ops_" prefix for generated symbols
- `-ops=0001000` - EXI options mask (Preserve.prefixes)
- `-schema=` - Path to EXIOptions schema (EXI-encoded)
- Output file: `src/contentIO/src/staticEXIOptions.c`

**Note:** This command should be run from the repository root directory after building the `exipg` tool.

**Note 2 - Bootstrapping Problem:** The EXIOptions schema must be encoded with **out-of-band options** (`-ops=0001000`) to break a circular dependency:
1. To decode an EXI stream with **in-band** options, you need to decode the options header
2. To decode the options header, you need the EXIOptions grammar/schema
3. But the EXIOptions schema is itself an EXI stream that needs decoding
4. **Solution**: Encode the EXIOptions schema with **out-of-band** options, so the decoder knows the options without having to decode them from the stream first. Once the EXIOptions grammar is compiled into the code, it can then decode other streams that use in-band options.

The mask `0001000` enables only Preserve.prefixes (STRICT=0, SELF_CONTAINED=0, dtd=0, prefixes=1, lexicalValues=0, comments=0, pis=0).
