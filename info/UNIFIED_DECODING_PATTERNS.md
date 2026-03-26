# Unified Decoding Patterns

This guide shows how to decode EXI documents to C structures with **minimal code duplication** between schema mode and schemaless mode.

## The Problem

Without careful design, you end up with:
- Duplicate logic for schema vs schemaless modes
- Ugly branching everywhere: `if(schemaMode) ... else ...`
- Hard to maintain when adding new fields
- Copy-paste errors between modes

## The Solution

Use a **unified data handling layer** that:
1. Accepts data in any format (typed or string)
2. Converts to target type automatically
3. Single code path for each field
4. Easy to extend with new fields

---

## Strategy 1: Unified Handler with Type Conversion

**Best for:** Small to medium structs (< 20 fields)

### Core Concept

Create a single data receiver that handles all types, with automatic conversion:

```c
#include "EXIParser.h"
#include <stdlib.h>
#include <string.h>

// Unified data representation
typedef union {
    Integer intVal;
    bool boolVal;
    Float floatVal;
    String stringVal;
    const char* binaryVal;
    EXIPDateTime dateTimeVal;
} DataValue;

typedef enum {
    DATA_TYPE_INT,
    DATA_TYPE_BOOL,
    DATA_TYPE_FLOAT,
    DATA_TYPE_STRING,
    DATA_TYPE_BINARY,
    DATA_TYPE_DATETIME
} DataType;

// Application context
typedef struct {
    void* targetStruct;
    const char* currentElement;
    const char* currentAttribute;
    bool inAttribute;
} UnifiedContext;

// Helper: Convert string to integer
static int stringToInt(const String* str) {
    char buf[32];
    size_t len = str->length < 31 ? str->length : 31;
    memcpy(buf, str->str, len);
    buf[len] = '\0';
    return atoi(buf);
}

// Helper: Convert string to boolean
static bool stringToBool(const String* str) {
    return (str->length >= 4 && strncmp(str->str, "true", 4) == 0) ||
           (str->length >= 1 && str->str[0] == '1');
}

// Helper: Convert string to float
static float stringToFloat(const String* str) {
    char buf[32];
    size_t len = str->length < 31 ? str->length : 31;
    memcpy(buf, str->str, len);
    buf[len] = '\0';
    return (float)atof(buf);
}

// Helper: Convert EXI Float to C float
static float exiFloatToFloat(Float exiFloat) {
    return (float)exiFloat.mantissa * powf(10.0f, (float)exiFloat.exponent);
}

// Single unified data receiver - handles BOTH schema and schemaless modes
errorCode receiveData(const char* elementName, DataType type,
                     DataValue value, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    MyStruct* target = (MyStruct*)ctx->targetStruct;

    if(elementName == NULL) return EXIP_OK;

    // Handle each field with automatic type conversion
    if(strcmp(elementName, "id") == 0) {
        switch(type) {
            case DATA_TYPE_INT:
                target->id = (int)value.intVal;
                break;
            case DATA_TYPE_STRING:
                target->id = stringToInt(&value.stringVal);
                break;
            default:
                return EXIP_INVALID_EXI_INPUT;
        }
    }
    else if(strcmp(elementName, "enabled") == 0) {
        switch(type) {
            case DATA_TYPE_BOOL:
                target->enabled = value.boolVal;
                break;
            case DATA_TYPE_STRING:
                target->enabled = stringToBool(&value.stringVal);
                break;
            default:
                return EXIP_INVALID_EXI_INPUT;
        }
    }
    else if(strcmp(elementName, "name") == 0) {
        if(type == DATA_TYPE_STRING) {
            size_t len = value.stringVal.length < 99 ?
                        value.stringVal.length : 99;
            memcpy(target->name, value.stringVal.str, len);
            target->name[len] = '\0';
        }
    }
    else if(strcmp(elementName, "value") == 0) {
        switch(type) {
            case DATA_TYPE_FLOAT:
                target->value = exiFloatToFloat(value.floatVal);
                break;
            case DATA_TYPE_STRING:
                target->value = stringToFloat(&value.stringVal);
                break;
            default:
                return EXIP_INVALID_EXI_INPUT;
        }
    }

    return EXIP_OK;
}

// Thin wrapper handlers - just forward to unified receiver
static errorCode handle_startElement(QName qname, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    ctx->currentElement = qname.localName->str;
    ctx->inAttribute = false;
    return EXIP_OK;
}

static errorCode handle_attribute(QName qname, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    ctx->currentAttribute = qname.localName->str;
    ctx->inAttribute = true;
    return EXIP_OK;
}

static errorCode handle_intData(Integer int_val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    DataValue val = { .intVal = int_val };
    const char* name = ctx->inAttribute ? ctx->currentAttribute : ctx->currentElement;
    return receiveData(name, DATA_TYPE_INT, val, app_data);
}

static errorCode handle_booleanData(bool bool_val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    DataValue val = { .boolVal = bool_val };
    const char* name = ctx->inAttribute ? ctx->currentAttribute : ctx->currentElement;
    return receiveData(name, DATA_TYPE_BOOL, val, app_data);
}

static errorCode handle_floatData(Float float_val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    DataValue val = { .floatVal = float_val };
    const char* name = ctx->inAttribute ? ctx->currentAttribute : ctx->currentElement;
    return receiveData(name, DATA_TYPE_FLOAT, val, app_data);
}

static errorCode handle_stringData(const String str_val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    DataValue val = { .stringVal = str_val };
    const char* name = ctx->inAttribute ? ctx->currentAttribute : ctx->currentElement;
    return receiveData(name, DATA_TYPE_STRING, val, app_data);
}

// Setup function - works for BOTH modes
void setupUnifiedHandlers(Parser* parser, UnifiedContext* ctx) {
    parser->handler.startElement = handle_startElement;
    parser->handler.attribute = handle_attribute;
    parser->handler.intData = handle_intData;
    parser->handler.booleanData = handle_booleanData;
    parser->handler.floatData = handle_floatData;
    parser->handler.stringData = handle_stringData;
    // Add other typed handlers as needed
}
```

### Usage Example

```c
typedef struct {
    int id;
    bool enabled;
    char name[100];
    float value;
} MyStruct;

errorCode parseToStructUnified(BinaryBuffer buffer, EXIPSchema* schema, MyStruct* output) {
    Parser parser;
    UnifiedContext ctx;
    errorCode err;

    // Initialize context
    ctx.targetStruct = output;
    ctx.currentElement = NULL;
    ctx.currentAttribute = NULL;
    ctx.inAttribute = false;

    // Initialize parser
    err = parse.initParser(&parser, buffer, &ctx);
    if(err != EXIP_OK) return err;

    // Setup unified handlers (same for schema and schemaless!)
    setupUnifiedHandlers(&parser, &ctx);

    // Parse header and set schema (NULL for schemaless)
    err = parse.parseHeader(&parser, false);
    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    err = parse.setSchema(&parser, schema);
    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    // Parse body
    while(err == EXIP_OK) {
        err = parse.parseNext(&parser);
    }

    parse.destroyParser(&parser);
    return (err == EXIP_PARSING_COMPLETE) ? EXIP_OK : err;
}
```

**Benefits:**
- ✅ Same code for schema and schemaless modes
- ✅ Easy to add new fields (one place)
- ✅ Type conversion in one place per field
- ✅ Simple and readable

---

## Strategy 2: Function Pointer Table

**Best for:** Medium to large structs (20-100 fields)

### Core Concept

Use a table-driven approach with converter functions:

```c
// Generic converter function type
typedef errorCode (*FieldConverter)(DataValue src, DataType srcType, void* dest);

// Converter for integer fields
errorCode convertToInt(DataValue src, DataType srcType, void* dest) {
    int* target = (int*)dest;
    switch(srcType) {
        case DATA_TYPE_INT:
            *target = (int)src.intVal;
            break;
        case DATA_TYPE_STRING:
            *target = stringToInt(&src.stringVal);
            break;
        default:
            return EXIP_INVALID_EXI_INPUT;
    }
    return EXIP_OK;
}

// Converter for boolean fields
errorCode convertToBool(DataValue src, DataType srcType, void* dest) {
    bool* target = (bool*)dest;
    switch(srcType) {
        case DATA_TYPE_BOOL:
            *target = src.boolVal;
            break;
        case DATA_TYPE_STRING:
            *target = stringToBool(&src.stringVal);
            break;
        default:
            return EXIP_INVALID_EXI_INPUT;
    }
    return EXIP_OK;
}

// Converter for float fields
errorCode convertToFloat(DataValue src, DataType srcType, void* dest) {
    float* target = (float*)dest;
    switch(srcType) {
        case DATA_TYPE_FLOAT:
            *target = exiFloatToFloat(src.floatVal);
            break;
        case DATA_TYPE_STRING:
            *target = stringToFloat(&src.stringVal);
            break;
        default:
            return EXIP_INVALID_EXI_INPUT;
    }
    return EXIP_OK;
}

// Converter for string fields
errorCode convertToString(DataValue src, DataType srcType, void* dest) {
    char* target = (char*)dest;
    if(srcType != DATA_TYPE_STRING) {
        return EXIP_INVALID_EXI_INPUT;
    }
    size_t len = src.stringVal.length < 99 ? src.stringVal.length : 99;
    memcpy(target, src.stringVal.str, len);
    target[len] = '\0';
    return EXIP_OK;
}

// Field mapping table
typedef struct {
    const char* elementName;
    void* targetField;
    FieldConverter converter;
} FieldMapping;

// Define mappings once (macro helps avoid errors)
#define STRUCT_OFFSET(type, field) ((void*)&((type*)0)->field)

FieldMapping* createFieldMappings(MyStruct* target) {
    static FieldMapping mappings[] = {
        {"id",      NULL, convertToInt},
        {"enabled", NULL, convertToBool},
        {"name",    NULL, convertToString},
        {"value",   NULL, convertToFloat},
        {NULL, NULL, NULL}  // Sentinel
    };

    // Calculate actual addresses
    mappings[0].targetField = &target->id;
    mappings[1].targetField = &target->enabled;
    mappings[2].targetField = target->name;
    mappings[3].targetField = &target->value;

    return mappings;
}

// Single generic receiver using table
errorCode receiveDataTable(const char* elementName, DataType type,
                           DataValue value, FieldMapping* mappings) {
    if(elementName == NULL) return EXIP_OK;

    for(FieldMapping* field = mappings; field->elementName != NULL; field++) {
        if(strcmp(elementName, field->elementName) == 0) {
            return field->converter(value, type, field->targetField);
        }
    }

    // Unknown field - ignore or log warning
    return EXIP_OK;
}

// Modified wrapper handlers
static errorCode handle_intData_table(Integer int_val, void* app_data) {
    TableContext* ctx = (TableContext*)app_data;
    DataValue val = { .intVal = int_val };
    const char* name = ctx->inAttribute ? ctx->currentAttribute : ctx->currentElement;
    return receiveDataTable(name, DATA_TYPE_INT, val, ctx->mappings);
}

// Similar for other handlers...
```

**Benefits:**
- ✅ Very maintainable - add field = one line in table
- ✅ Generic converters can be reused
- ✅ Easy to log/debug (table is inspectable)
- ✅ Can generate from schema at build time

---

## Strategy 3: Macro-Based Code Generation

**Best for:** Large structs (100+ fields) or code generation tools

### Core Concept

Define structure once, generate all code automatically:

```c
// Define your struct mapping once
#define FIELD_MAPPINGS(X) \
    X(id,      int,    INT) \
    X(enabled, bool,   BOOL) \
    X(name,    string, STRING) \
    X(value,   float,  FLOAT)

// Generate converter functions automatically
#define GENERATE_CONVERTER(name, ctype, dtype) \
    errorCode convert_##name(DataValue src, DataType srcType, MyStruct* target) { \
        return convertTo##dtype(src, srcType, &target->name); \
    }

FIELD_MAPPINGS(GENERATE_CONVERTER)
#undef GENERATE_CONVERTER

// Generate dispatch table automatically
#define GENERATE_TABLE_ENTRY(name, ctype, dtype) {#name, convert_##name},

typedef struct {
    const char* fieldName;
    errorCode (*converter)(DataValue, DataType, MyStruct*);
} FieldEntry;

FieldEntry fieldTable[] = {
    FIELD_MAPPINGS(GENERATE_TABLE_ENTRY)
    {NULL, NULL}
};

#undef GENERATE_TABLE_ENTRY

// Single dispatcher
errorCode receiveDataGenerated(const char* elementName, DataType type,
                               DataValue value, MyStruct* target) {
    for(int i = 0; fieldTable[i].fieldName != NULL; i++) {
        if(strcmp(elementName, fieldTable[i].fieldName) == 0) {
            return fieldTable[i].converter(value, type, target);
        }
    }
    return EXIP_OK;
}
```

**Benefits:**
- ✅ DRY (Don't Repeat Yourself) - define structure once
- ✅ Type-safe (compiler checks)
- ✅ Can generate from XSD at build time
- ✅ Zero runtime overhead

---

## Strategy 4: Hash Table for Fast Lookup

**Best for:** Very large structs (1000+ fields) with performance requirements

```c
#include "hashtable.h"  // EXIP's built-in hash table

typedef struct {
    FieldMapping* mappings;
    struct hashtable* lookupTable;
} FastLookupContext;

// Build hash table once
FastLookupContext* createFastLookup(FieldMapping* mappings) {
    FastLookupContext* ctx = malloc(sizeof(FastLookupContext));
    ctx->mappings = mappings;

    // Create hash table
    ctx->lookupTable = create_hashtable(100, djbHash, stringEqual);

    // Populate with field mappings
    for(int i = 0; mappings[i].elementName != NULL; i++) {
        String key;
        key.str = (char*)mappings[i].elementName;
        key.length = strlen(mappings[i].elementName);
        hashtable_insert(ctx->lookupTable, key, i);
    }

    return ctx;
}

// Fast O(1) lookup
errorCode receiveDataFast(const char* elementName, DataType type,
                         DataValue value, FastLookupContext* ctx) {
    if(elementName == NULL) return EXIP_OK;

    String key;
    key.str = (char*)elementName;
    key.length = strlen(elementName);

    Index idx = hashtable_search(ctx->lookupTable, key);
    if(idx == INDEX_MAX) {
        return EXIP_OK;  // Unknown field
    }

    FieldMapping* field = &ctx->mappings[idx];
    return field->converter(value, type, field->targetField);
}
```

---

## Comparison Matrix

| Strategy | Code Size | Maintainability | Performance | Best For |
|----------|-----------|-----------------|-------------|----------|
| **Strategy 1: Unified Handler** | Small | Good | Good | Small structs (< 20 fields) |
| **Strategy 2: Function Table** | Medium | Excellent | Good | Medium structs (20-100 fields) |
| **Strategy 3: Macro Generation** | Small | Excellent | Excellent | Large structs, code generation |
| **Strategy 4: Hash Table** | Medium | Good | Excellent | Very large structs (1000+ fields) |

---

## Complete Working Example (Strategy 1)

```c
#include "EXIParser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    int id;
    bool enabled;
    char name[100];
    float value;
} MyStruct;

// ... (Include all helper functions from Strategy 1 above) ...

int main() {
    MyStruct myData = {0};
    BinaryBuffer buffer;
    EXIPSchema* schema = NULL;  // NULL for schemaless

    // Load EXI file
    FILE* f = fopen("data.exi", "rb");
    if(!f) {
        fprintf(stderr, "Cannot open file\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    buffer.bufLen = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer.buf = malloc(buffer.bufLen);
    fread(buffer.buf, 1, buffer.bufLen, f);
    fclose(f);

    buffer.bufContent = buffer.bufLen;
    buffer.ioStrm.readWriteToStream = NULL;
    buffer.ioStrm.stream = NULL;

    // Parse (works for BOTH schema and schemaless!)
    errorCode err = parseToStructUnified(buffer, schema, &myData);

    if(err == EXIP_OK) {
        printf("Success!\n");
        printf("  id: %d\n", myData.id);
        printf("  enabled: %s\n", myData.enabled ? "true" : "false");
        printf("  name: %s\n", myData.name);
        printf("  value: %f\n", myData.value);
    } else {
        printf("Parse error: %d\n", err);
    }

    free(buffer.buf);
    return 0;
}
```

---

## Best Practices

### 1. Choose the Right Strategy

- **Small projects**: Strategy 1 (Unified Handler)
- **Medium projects**: Strategy 2 (Function Table)
- **Large/Generated**: Strategy 3 (Macro Generation)
- **Performance critical**: Strategy 4 (Hash Table)

### 2. Centralize Type Conversion

Put all string-to-type conversions in helper functions:

```c
// Good: Reusable helpers
int stringToInt(const String* str);
bool stringToBool(const String* str);
float stringToFloat(const String* str);

// Bad: Inline conversions everywhere
```

### 3. Handle Unknown Fields Gracefully

```c
if(fieldNotFound) {
    // Option 1: Ignore silently
    return EXIP_OK;

    // Option 2: Log warning
    fprintf(stderr, "Unknown field: %s\n", elementName);
    return EXIP_OK;

    // Option 3: Strict mode - error
    return EXIP_INVALID_EXI_INPUT;
}
```

### 4. Validate Converted Data

```c
errorCode convertToInt(DataValue src, DataType srcType, void* dest) {
    int* target = (int*)dest;
    int value;

    if(srcType == DATA_TYPE_INT) {
        value = (int)src.intVal;
    } else {
        value = stringToInt(&src.stringVal);
    }

    // Validate range
    if(value < MIN_VALID_ID || value > MAX_VALID_ID) {
        return EXIP_INVALID_EXI_INPUT;
    }

    *target = value;
    return EXIP_OK;
}
```

### 5. Use Const for Element Names

```c
// Good: String literals have static lifetime
const char* currentElement = qname.localName->str;

// Also good: Copy to buffer if needed later
char elementBuf[100];
strncpy(elementBuf, qname.localName->str, qname.localName->length);
elementBuf[qname.localName->length] = '\0';
```

---

## Testing Both Modes

```c
void testBothModes() {
    MyStruct schemaResult = {0};
    MyStruct schemalessResult = {0};

    // Test with schema
    EXIPSchema schema;
    loadSchema(&schema);
    parseToStructUnified(buffer, &schema, &schemaResult);

    // Test schemaless
    parseToStructUnified(buffer, NULL, &schemalessResult);

    // Results should be identical!
    assert(schemaResult.id == schemalessResult.id);
    assert(schemaResult.enabled == schemalessResult.enabled);
    assert(strcmp(schemaResult.name, schemalessResult.name) == 0);
    assert(fabs(schemaResult.value - schemalessResult.value) < 0.001f);
}
```

---

## See Also

- [DECODING_TO_STRUCT.md](DECODING_TO_STRUCT.md) - Basic decoding patterns
- [DETECTING_SCHEMA_MODE.md](DETECTING_SCHEMA_MODE.md) - Detecting encoding mode
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall EXIP architecture
- [include/EXIParser.h](include/EXIParser.h) - Parser API reference
