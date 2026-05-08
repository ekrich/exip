# EXI Schema Grammar Generation

## Overview
This document explains how EXIP generates compiled grammar code from XML schemas, with a focus on how the built-in EXI options schema (`staticEXIOptions.c`) is created.

## What is `ops_schema`?

Before diving into generation, it's important to understand what we're creating:

**`ops_schema`** is a **global variable** of type `EXIPSchema` that contains the complete compiled EXI Options schema. It's defined in the auto-generated file `staticEXIOptions.c`.

### Structure

From [staticEXIOptions.c:2798](../src/contentIO/src/staticEXIOptions.c#L2798):

```c
CONST EXIPSchema ops_schema =
{
    {0},                                              // Version/flags
    {{sizeof(UriEntry), 5, 5}, ops_uriEntry, 5},     // URI table (5 URIs)
    {ops_docGrammarRule, 100663296, 2},              // Document grammar
    {{sizeof(SimpleType), 67, 67}, ops_simpleTypes, 67},  // 67 types
    {{sizeof(EXIGrammar), 75, 75}, ops_grammarTable, 75}, // 75 grammars
    75,                                               // Grammar count
    {{sizeof(EnumDefinition), 0, 0}, NULL, 0}        // Enumerations
};
```

### What It Contains

The `ops_schema` variable references arrays defined earlier in the same file:
- **String arrays**: `ops_LN_4_20[]` = "header", `ops_LN_4_33[]` = "strict", etc.
- **`ops_uriEntry[5]`**: Array of URI entries including `http://www.w3.org/2009/exi`
- **`ops_simpleTypes[67]`**: Array of type definitions
- **`ops_grammarTable[75]`**: Array of grammar objects (one per element/type)
- **`ops_docGrammarRule`**: Document-level grammar rules

### How It's Used

The encoder and decoder take a pointer to this schema:

**Encoder** ([headerEncode.c:87](../src/contentIO/src/headerEncode.c#L87)):
```c
options_strm.schema = (EXIPSchema*) &ops_schema;
```

**Decoder** ([headerDecode.c:167](../src/contentIO/src/headerDecode.c#L167)):
```c
TRY_CATCH(setSchema(&optionsParser, (EXIPSchema*) &ops_schema), ...);
```

With this schema loaded, the encoder/decoder can:
- Look up element names by index
- Follow grammar production rules
- Encode/decode the options document without transmitting element names

## The `exipg` Tool

EXIP includes a grammar generator tool called `exipg` that converts XML schemas into compiled C code containing:
- Pre-populated string tables (URIs, element names, attribute names)
- Grammar production rules
- Type definitions
- All data structures needed for schema-informed EXI processing

### Tool Location
- Source: [utils/schemaHandling/exipg.c](../utils/schemaHandling/exipg.c)
- Built executable (Linux/macOS): `bin/utils/exipg`
- Built executable (Windows): `build\vs2022\Debug\exipg.exe` or `build\vs2022\Release\exipg.exe`

### Basic Usage
```bash
exipg [options] -schema=<xsd_in> [grammar_out]
```

## How the EXI Options Schema is Generated

### Source Files
The W3C EXI specification defines a standard schema for EXI options:

1. **XML Schema Definition**: [utils/schemaHandling/EXIOptions.xsd](../utils/schemaHandling/EXIOptions.xsd)
   - Standard W3C XML Schema
   - Defines elements: `<header>`, `<strict>`, `<common>`, `<lesscommon>`, `<uncommon>`, etc.
   - Namespace: `http://www.w3.org/2009/exi`

2. **EXI-Encoded Schema**: [utils/schemaHandling/EXIOptions-xsd.exi](../utils/schemaHandling/EXIOptions-xsd.exi)
   - The `EXIOptions.xsd` file encoded in EXI format (using EXIficient or similar tool)
   - More compact than XML
   - Input format for `exipg` (uses `SCHEMA_FORMAT_XSD_EXI`)

### Generation Command

From [.launches/exipg EXIOptions.launch:31](../.launches/exipg EXIOptions.launch#L31):

```bash
exipg -static -pfx=ops_ -ops=0001000 \
  -schema=utils/schemaHandling/EXIOptions-xsd.exi \
  src/contentIO/src/staticEXIOptions.c
```

#### Command Parameters:
- **`-static`**: Generate static C code (not dynamic runtime loading)
- **`-pfx=ops_`**: Prefix all symbols with `ops_` (e.g., `ops_URI_4`, `ops_LN_4_20`)
- **`-ops=0001000`**: Options mask for schema processing (see format below)
- **`-schema=...`**: Input EXI-encoded schema file
- **Output**: `src/contentIO/src/staticEXIOptions.c`

#### Options Mask Format (`-ops`):

The `-ops` parameter uses a 7-bit mask to set EXI processing options. Each position represents a specific option:

| Position | Option | Description | Example (`-ops=0001000`) |
|----------|--------|-------------|--------------------------|
| 1 | STRICT | Schema-strict encoding mode | 0 (Off) |
| 2 | SELF_CONTAINED | Self-contained elements | 0 (Off) |
| 3 | dtd | Preserve DTD | 0 (Off) |
| 4 | prefixes | Preserve prefixes | **1 (On)** ← Preserve.prefixes enabled |
| 5 | lexicalValues | Preserve lexical values | 0 (Off) |
| 6 | comments | Preserve comments | 0 (Off) |
| 7 | pis | Preserve processing instructions | 0 (Off) |

**Example**: `-ops=0001000` enables only the `prefixes` preservation option (bit 4), which is necessary when generating schema grammars that use namespace prefixes.

### Generated Output

The generated [staticEXIOptions.c](../src/contentIO/src/staticEXIOptions.c) contains:

#### 1. String Definitions (Lines 12-109)
```c
CONST CharType ops_URI_4[] = {0x68, 0x74, 0x74, 0x70, ...};
/* http://www.w3.org/2009/exi */

CONST CharType ops_LN_4_20[] = {0x68, 0x65, 0x61, 0x64, 0x65, 0x72};
/* header */

CONST CharType ops_LN_4_33[] = {0x73, 0x74, 0x72, 0x69, 0x63, 0x74};
/* strict */

CONST CharType ops_LN_4_6[] = {0x63, 0x6f, 0x6d, 0x6d, 0x6f, 0x6e};
/* common */
```

#### 2. String Tables
- URI tables with prefixes
- Local name tables
- Links between URIs and local names

#### 3. Grammar Definitions
- Document grammar
- Element-specific grammars
- Type grammars
- Production rules for each grammar state

#### 4. Complete Schema Structure (Line 2798)
```c
CONST EXIPSchema ops_schema = {
    // URI table
    // Grammar table
    // Simple type table
    // etc.
};
```

## The Process Flow

```
┌─────────────────────────────┐
│ EXIOptions.xsd (XML)        │ W3C Standard Schema
└──────────┬──────────────────┘
           │ (encode with EXI)
           ↓
┌─────────────────────────────┐
│ EXIOptions-xsd.exi          │ EXI-Encoded Schema
└──────────┬──────────────────┘
           │ (process with exipg)
           ↓
┌─────────────────────────────┐
│ staticEXIOptions.c          │ Auto-Generated C Code
│ - String tables             │ Line 1: AUTO-GENERATED comment
│ - Grammar productions       │
│ - Schema structure          │
└──────────┬──────────────────┘
           │ (compile with EXIP)
           ↓
┌─────────────────────────────┐
│ ops_schema (built-in)       │ Compiled into every EXIP binary
└──────────┬──────────────────┘
           │ (used at runtime)
           ↓
┌─────────────────────────────┐
│ Encode/Decode EXI Options   │ In actual EXI streams
└─────────────────────────────┘
```

## Why EXI-Encode the Schema?

### Advantages of `SCHEMA_FORMAT_XSD_EXI`:

1. **Efficiency**: The schema itself is more compact
2. **Consistency**: Uses the same parser code path as other schemas
3. **Self-hosting**: EXI can process its own schemas
4. **Standard format**: Primary supported format in EXIP

### Schema Format Support

From [grammarGenerator.h:32](../include/grammarGenerator.h#L32):

```c
enum SchemaFormat
{
    SCHEMA_FORMAT_XSD_EXI   = 0,  // Primary format (supported)
    SCHEMA_FORMAT_XSD_XML   = 1,  // Not fully implemented
    SCHEMA_FORMAT_DTD       = 2,  // Not implemented
    SCHEMA_FORMAT_RELAX_NG  = 3   // Not implemented
};
```

## How It Works Internally

### 1. Schema Loading
The `exipg` tool calls [generateSchemaInformedGrammars()](../include/grammarGenerator.h#L64):

```c
errorCode generateSchemaInformedGrammars(
    BinaryBuffer* buffers,      // EXI-encoded schema files
    unsigned int bufCount,       // Number of schema files
    SchemaFormat schemaFormat,   // SCHEMA_FORMAT_XSD_EXI
    EXIOptions* opt,            // Processing options
    EXIPSchema* schema,         // Output schema structure
    loadSchemaHandler handler   // For imports/includes
);
```

### 2. Grammar Generation
From [exipg.c:340](../utils/schemaHandling/exipg.c#L340):

```c
tmp_err_code = generateSchemaInformedGrammars(
    buffer,
    schemaFilesCount,
    SCHEMA_FORMAT_XSD_EXI,  // Input format
    opt,                     // Options mask
    schema,                  // Output
    NULL                     // No import handler
);
```

### 3. Code Generation
The tool then serializes the in-memory schema structure to C source code:
- Writes string arrays for all URIs and names
- Writes grammar rule arrays
- Writes type definition arrays
- Writes the master `ops_schema` structure

## Using Custom Schemas

You can generate grammar code for your own schemas:

```bash
# Generate static C code for mySchema.exi
exipg -static -pfx=my_ \
  -schema=mySchema.exi \
  mySchemaGrammar.c

# Generate EXIP binary format
exipg -exip \
  -schema=mySchema.exi \
  mySchema.exip

# Generate human-readable text
exipg -text \
  -schema=mySchema.exi \
  mySchema.txt
```

## Interoperability

### All EXI Implementations Must Have This Schema

The EXI options schema is **part of the W3C EXI specification**, not specific to EXIP:

- **Standard schema**: Defined in EXI spec
- **Same elements**: All implementations use `<header>`, `<strict>`, etc.
- **Interoperability**: EXIP can decode options from EXIficient, OpenEXI, etc.
- **Each implementation compiles it**: But the source schema is the same

### Why Pre-compile?

Instead of loading the schema at runtime:

1. **Performance**: No parsing overhead at startup
2. **Size**: Smaller binary than including XML schema
3. **Simplicity**: Schema is always available, no file I/O
4. **Embedded systems**: Critical for resource-constrained environments

## Regenerating staticEXIOptions.c

If you need to regenerate the options schema (e.g., after modifying the XSD):

1. **Encode the XSD in EXI** (if modified):
   ```bash
   # Use an EXI tool to encode EXIOptions.xsd
   # Output: EXIOptions-xsd.exi
   ```

2. **Run exipg**:
   ```bash
   bin/utils/exipg -static -pfx=ops_ -ops=0001000 \
     -schema=utils/schemaHandling/EXIOptions-xsd.exi \
     src/contentIO/src/staticEXIOptions.c
   ```

3. **Rebuild EXIP**:
   ```bash
   cd build/gcc
   make TARGET=pc clean all utils
   ```

   This rebuilds the library and utilities (including `exipg`) with the updated `staticEXIOptions.c`.

## Key Concepts

### Schema-Informed Encoding
- Element names are indices into grammar productions
- No strings transmitted for schema-defined elements
- Extremely compact encoding

### Built-in vs Runtime Schemas
- **Built-in** (`ops_schema`): Compiled into binary
- **Runtime**: Loaded via `generateSchemaInformedGrammars()` at runtime
- Both use the same grammar structures and encoding

### The Circular Beauty
The EXI options schema is:
1. Defined in the EXI spec (W3C standard)
2. Encoded using EXI (self-hosting)
3. Processed by EXIP's grammar generator
4. Compiled into EXIP
5. Used to encode/decode EXI options
6. Which control how EXI processes documents!

## Related Files
- [exipg.c](../utils/schemaHandling/exipg.c) - Grammar generator tool
- [grammarGenerator.h](../include/grammarGenerator.h) - API for schema processing
- [staticEXIOptions.c](../src/contentIO/src/staticEXIOptions.c) - Generated options schema
- [EXIOptions.xsd](../utils/schemaHandling/EXIOptions.xsd) - Source XML schema
- [EXIOptions-xsd.exi](../utils/schemaHandling/EXIOptions-xsd.exi) - EXI-encoded schema
