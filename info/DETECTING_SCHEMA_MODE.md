# Detecting EXI Schema Mode

This guide explains how to determine whether an EXI document is schema-encoded or schemaless.

## Overview

EXI documents can be encoded in different modes:
- **Schema-informed**: Uses XML Schema for optimized encoding
- **Schemaless**: No schema required, more flexible
- **Built-in types only**: Uses XML Schema built-in types without custom schema
- **Strict mode**: Schema-informed with strict validation

The encoding mode affects how you parse the document and which data handlers are called.

## Method 1: Check EXI Header Options

After parsing the header, inspect the `schemaIDMode` field:

```c
#include "EXIParser.h"

Parser parser;
errorCode err;

// Initialize and parse header
err = parse.initParser(&parser, buffer, app_data);
if(err != EXIP_OK) return err;

err = parse.parseHeader(&parser, false);
if(err != EXIP_OK) return err;

// Check schema mode
switch(parser.strm.header.opts.schemaIDMode) {
    case SCHEMA_ID_ABSENT:
        // Default mode - could be schema or schemaless
        // If you have a schema, use it; otherwise schemaless
        printf("Schema ID absent - mode unclear, check other flags\n");
        break;

    case SCHEMA_ID_SET:
        // Schema ID is specified in the header
        printf("Schema-informed mode\n");
        printf("Schema ID: %.*s\n",
               (int)parser.strm.header.opts.schemaID.length,
               parser.strm.header.opts.schemaID.str);
        // This document expects schema-informed processing
        break;

    case SCHEMA_ID_NIL:
        // Explicitly schemaless
        printf("Schemaless mode (NIL)\n");
        break;

    case SCHEMA_ID_EMPTY:
        // Built-in XML Schema types only (no user-defined schema)
        printf("Built-in types only (EMPTY)\n");
        break;
}
```

## Method 2: Check Strict Mode Flag

If strict mode is enabled, schema-informed processing is required:

```c
err = parse.parseHeader(&parser, false);

if(WITH_STRICT(parser.strm.header.opts.enumOpt)) {
    printf("Strict mode enabled - schema REQUIRED\n");
    // Must use schema-informed processing
    // Schemaless mode is not compatible with strict mode
}
else {
    printf("Not strict mode - schema is optional\n");
}
```

## Method 3: Comprehensive Detection Function

Here's a complete function that determines the encoding mode:

```c
typedef enum {
    EXI_MODE_SCHEMALESS,           // No schema used
    EXI_MODE_SCHEMA_REQUIRED,      // Schema must be provided
    EXI_MODE_SCHEMA_OPTIONAL,      // Can work with or without schema
    EXI_MODE_BUILTIN_TYPES_ONLY    // Only built-in XML Schema types
} EXIEncodingMode;

/**
 * Detect the encoding mode from parsed header
 * Call this after parse.parseHeader()
 */
EXIEncodingMode detectEncodingMode(Parser* parser) {
    EXIOptions* opts = &parser->strm.header.opts;

    // Strict mode REQUIRES schema
    if(WITH_STRICT(opts->enumOpt)) {
        return EXI_MODE_SCHEMA_REQUIRED;
    }

    // Check schemaID mode
    switch(opts->schemaIDMode) {
        case SCHEMA_ID_NIL:
            // Explicitly schemaless
            return EXI_MODE_SCHEMALESS;

        case SCHEMA_ID_EMPTY:
            // Only built-in types, no user schema
            return EXI_MODE_BUILTIN_TYPES_ONLY;

        case SCHEMA_ID_SET:
            // Schema ID provided - schema expected
            return EXI_MODE_SCHEMA_REQUIRED;

        case SCHEMA_ID_ABSENT:
        default:
            // No explicit schema info - can work either way
            return EXI_MODE_SCHEMA_OPTIONAL;
    }
}

/**
 * Get human-readable description of encoding mode
 */
const char* encodingModeToString(EXIEncodingMode mode) {
    switch(mode) {
        case EXI_MODE_SCHEMALESS:
            return "Schemaless";
        case EXI_MODE_SCHEMA_REQUIRED:
            return "Schema Required";
        case EXI_MODE_SCHEMA_OPTIONAL:
            return "Schema Optional";
        case EXI_MODE_BUILTIN_TYPES_ONLY:
            return "Built-in Types Only";
        default:
            return "Unknown";
    }
}
```

## Method 4: Runtime Detection During Parsing

You can also detect the mode by observing which content handlers are called:

```c
typedef struct {
    bool receivedTypedData;   // Received intData, booleanData, floatData, etc.
    bool receivedStringOnly;  // Only received stringData
    int typedDataCount;
    int stringDataCount;
} ModeDetector;

static errorCode detect_intData(Integer int_val, void* app_data) {
    ModeDetector* detector = (ModeDetector*)app_data;
    detector->receivedTypedData = true;
    detector->typedDataCount++;
    return EXIP_OK;
}

static errorCode detect_booleanData(bool bool_val, void* app_data) {
    ModeDetector* detector = (ModeDetector*)app_data;
    detector->receivedTypedData = true;
    detector->typedDataCount++;
    return EXIP_OK;
}

static errorCode detect_floatData(Float fl_val, void* app_data) {
    ModeDetector* detector = (ModeDetector*)app_data;
    detector->receivedTypedData = true;
    detector->typedDataCount++;
    return EXIP_OK;
}

static errorCode detect_stringData(const String str_val, void* app_data) {
    ModeDetector* detector = (ModeDetector*)app_data;
    detector->receivedStringOnly = !detector->receivedTypedData;
    detector->stringDataCount++;
    return EXIP_OK;
}

// After parsing:
// - If receivedTypedData is true → schema-informed mode
// - If only stringData was called → schemaless mode
```

## Complete Usage Example

```c
errorCode parseDocument(BinaryBuffer buffer, EXIPSchema* availableSchema) {
    Parser parser;
    errorCode err;

    // Initialize parser
    err = parse.initParser(&parser, buffer, NULL);
    if(err != EXIP_OK) return err;

    // Parse header
    err = parse.parseHeader(&parser, false);
    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    // Detect encoding mode
    EXIEncodingMode mode = detectEncodingMode(&parser);
    printf("Detected mode: %s\n", encodingModeToString(mode));

    // Handle each mode appropriately
    switch(mode) {
        case EXI_MODE_SCHEMALESS:
            printf("Processing without schema\n");
            err = parse.setSchema(&parser, NULL);
            break;

        case EXI_MODE_SCHEMA_REQUIRED:
            if(availableSchema == NULL) {
                printf("Error: Schema required but not provided\n");
                printf("Schema ID: %.*s\n",
                       (int)parser.strm.header.opts.schemaID.length,
                       parser.strm.header.opts.schemaID.str);
                parse.destroyParser(&parser);
                return EXIP_INVALID_EXIP_CONFIGURATION;
            }
            printf("Processing with schema: %.*s\n",
                   (int)parser.strm.header.opts.schemaID.length,
                   parser.strm.header.opts.schemaID.str);
            err = parse.setSchema(&parser, availableSchema);
            break;

        case EXI_MODE_BUILTIN_TYPES_ONLY:
            printf("Processing with built-in types only\n");
            err = parse.setSchema(&parser, NULL);
            break;

        case EXI_MODE_SCHEMA_OPTIONAL:
            if(availableSchema != NULL) {
                printf("Schema available - using schema-informed processing\n");
                err = parse.setSchema(&parser, availableSchema);
            }
            else {
                printf("No schema available - using schemaless processing\n");
                err = parse.setSchema(&parser, NULL);
            }
            break;
    }

    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    // Set up handlers based on mode
    // (schema mode can use typed handlers, schemaless uses stringData)

    // Parse body
    while(err == EXIP_OK) {
        err = parse.parseNext(&parser);
    }

    // Cleanup
    parse.destroyParser(&parser);

    return (err == EXIP_PARSING_COMPLETE) ? EXIP_OK : err;
}
```

## Loading Schema Based on Schema ID

When `SCHEMA_ID_SET` is detected, you can load the appropriate schema:

```c
EXIPSchema* loadSchemaByID(const String* schemaID) {
    static EXIPSchema schema;
    char idBuffer[256];

    // Convert String to null-terminated
    size_t len = schemaID->length < 255 ? schemaID->length : 255;
    memcpy(idBuffer, schemaID->str, len);
    idBuffer[len] = '\0';

    // Load schema based on ID
    // This could be from file, database, embedded data, etc.
    if(strcmp(idBuffer, "http://example.com/schema-v1") == 0) {
        // Load schema for version 1
        BinaryBuffer schemaBuffer = loadSchemaFile("schema-v1.xsd.exi");
        generateSchemaInformedGrammars(&schemaBuffer, 1,
                                      SCHEMA_FORMAT_XSD_EXI,
                                      NULL, &schema, NULL);
        return &schema;
    }
    else if(strcmp(idBuffer, "http://example.com/schema-v2") == 0) {
        // Load schema for version 2
        BinaryBuffer schemaBuffer = loadSchemaFile("schema-v2.xsd.exi");
        generateSchemaInformedGrammars(&schemaBuffer, 1,
                                      SCHEMA_FORMAT_XSD_EXI,
                                      NULL, &schema, NULL);
        return &schema;
    }

    // Unknown schema ID
    return NULL;
}

// Usage:
if(mode == EXI_MODE_SCHEMA_REQUIRED) {
    EXIPSchema* schema = loadSchemaByID(&parser.strm.header.opts.schemaID);
    if(schema == NULL) {
        fprintf(stderr, "Failed to load schema: %.*s\n",
                (int)parser.strm.header.opts.schemaID.length,
                parser.strm.header.opts.schemaID.str);
        return EXIP_INVALID_EXIP_CONFIGURATION;
    }
    parse.setSchema(&parser, schema);
}
```

## Summary Table

| schemaIDMode | Strict Mode? | Schema Required? | Mode | Data Handlers |
|--------------|--------------|------------------|------|---------------|
| `SCHEMA_ID_SET` | - | Yes | Schema-informed with ID | Typed (int, bool, float, etc.) |
| `SCHEMA_ID_ABSENT` | Yes | Yes | Schema-informed (strict) | Typed (int, bool, float, etc.) |
| `SCHEMA_ID_ABSENT` | No | Optional | Can be either | Depends on schema availability |
| `SCHEMA_ID_NIL` | - | No | Explicitly schemaless | String only |
| `SCHEMA_ID_EMPTY` | - | No | Built-in types only | Typed (built-in types) |

## Key Decision Points

### 1. **Must Use Schema:**
- `SCHEMA_ID_SET` is present
- Strict mode is enabled
- Document expects typed data

### 2. **Cannot Use Schema:**
- `SCHEMA_ID_NIL` is set
- Document explicitly encoded as schemaless

### 3. **Optional Schema:**
- `SCHEMA_ID_ABSENT` and not strict mode
- Use schema if available for better performance
- Fall back to schemaless if schema unavailable

## Handling Content Based on Mode

### Schema Mode Handlers:
```c
// Set up typed handlers
parser.handler.intData = handle_intData;
parser.handler.booleanData = handle_booleanData;
parser.handler.floatData = handle_floatData;
parser.handler.dateTimeData = handle_dateTimeData;
parser.handler.binaryData = handle_binaryData;
parser.handler.decimalData = handle_decimalData;
parser.handler.stringData = handle_stringData;
```

### Schemaless Mode Handlers:
```c
// Only need string handler (all data comes as strings)
parser.handler.stringData = handle_stringData_schemaless;
// Other typed handlers won't be called
```

## Error Handling

```c
EXIEncodingMode mode = detectEncodingMode(&parser);

if(mode == EXI_MODE_SCHEMA_REQUIRED && schema == NULL) {
    fprintf(stderr, "ERROR: Document requires schema but none provided\n");
    fprintf(stderr, "Schema ID: %.*s\n",
            (int)parser.strm.header.opts.schemaID.length,
            parser.strm.header.opts.schemaID.str);
    parse.destroyParser(&parser);
    return EXIP_INVALID_EXIP_CONFIGURATION;
}

if(WITH_STRICT(parser.strm.header.opts.enumOpt) && schema == NULL) {
    fprintf(stderr, "ERROR: Strict mode requires schema\n");
    parse.destroyParser(&parser);
    return EXIP_INVALID_EXIP_CONFIGURATION;
}
```

## Best Practices

1. **Always parse the header first** before making assumptions about encoding mode

2. **Check both schemaIDMode and strict flag** for complete detection

3. **Provide helpful error messages** when schema is required but missing

4. **Support multiple schema versions** by loading based on schema ID

5. **Fall back gracefully** to schemaless mode when schema is optional but unavailable

6. **Set up appropriate handlers** based on detected mode to avoid unexpected callbacks

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall EXIP architecture
- [DECODING_TO_STRUCT.md](DECODING_TO_STRUCT.md) - Decoding EXI to C structures
- [include/procTypes.h](include/procTypes.h) - EXIOptions and SchemaIdMode definitions
- [include/EXIParser.h](include/EXIParser.h) - Parser API reference
