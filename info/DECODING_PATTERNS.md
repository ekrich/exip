# Decoding Patterns for EXIP

This guide shows efficient patterns for decoding EXI streams into C structures.

## Quick Start

**Schema mode (recommended):** Direct typed callbacks - no string conversion needed.
**Schemaless mode:** All data as strings - requires parsing.

## Basic Pattern: Direct Struct Decoding

```c
typedef struct {
    int id;
    bool enabled;
    char name[100];
    float value;
} MyStruct;

typedef struct {
    MyStruct* target;
    const char* currentElement;
} Context;

static errorCode handle_startElement(QName qname, void* app_data) {
    Context* ctx = (Context*)app_data;
    ctx->currentElement = qname.localName->str;
    return EXIP_OK;
}

static errorCode handle_intData(Integer val, void* app_data) {
    Context* ctx = (Context*)app_data;
    if(strcmp(ctx->currentElement, "id") == 0)
        ctx->target->id = (int)val;
    return EXIP_OK;
}

static errorCode handle_stringData(const String val, void* app_data) {
    Context* ctx = (Context*)app_data;
    if(strcmp(ctx->currentElement, "name") == 0) {
        size_t len = val.length < 99 ? val.length : 99;
        memcpy(ctx->target->name, val.str, len);
        ctx->target->name[len] = '\0';
    }
    return EXIP_OK;
}
```

**Problem:** Duplicate logic for schema vs schemaless modes, string comparisons everywhere.

## Unified Pattern: Field-Driven Decoding

Avoid duplication with field tables and type-agnostic conversion:

### 1. Define Field Mapping

```c
typedef enum {
    FIELD_ID,
    FIELD_ENABLED,
    FIELD_NAME,
    FIELD_VALUE,
    FIELD_UNKNOWN
} FieldId;

typedef struct {
    const char* name;
    FieldId id;
    size_t offset;      // offsetof(MyStruct, fieldName)
    int expectedType;   // DATA_TYPE_INT, etc.
} FieldDef;

static const FieldDef FIELDS[] = {
    {"id",      FIELD_ID,      offsetof(MyStruct, id),      DATA_TYPE_INT},
    {"enabled", FIELD_ENABLED, offsetof(MyStruct, enabled), DATA_TYPE_BOOL},
    {"name",    FIELD_NAME,    offsetof(MyStruct, name),    DATA_TYPE_STRING},
    {"value",   FIELD_VALUE,   offsetof(MyStruct, value),   DATA_TYPE_FLOAT},
    {NULL, FIELD_UNKNOWN, 0, 0}
};

static FieldId lookupField(const char* name) {
    for(int i = 0; FIELDS[i].name != NULL; i++) {
        if(strcmp(FIELDS[i].name, name) == 0)
            return FIELDS[i].id;
    }
    return FIELD_UNKNOWN;
}
```

### 2. Unified Data Handler

```c
typedef struct {
    MyStruct* target;
    FieldId currentField;
} UnifiedContext;

static errorCode unified_startElement(QName qname, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    char name[64];
    snprintf(name, 64, "%.*s", (int)qname.localName->length, qname.localName->str);
    ctx->currentField = lookupField(name);
    return EXIP_OK;
}

static errorCode unified_intData(Integer val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    switch(ctx->currentField) {
        case FIELD_ID:
            ctx->target->id = (int)val;
            break;
        default:
            break;
    }
    return EXIP_OK;
}

static errorCode unified_booleanData(bool val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    if(ctx->currentField == FIELD_ENABLED)
        ctx->target->enabled = val;
    return EXIP_OK;
}

static errorCode unified_stringData(const String val, void* app_data) {
    UnifiedContext* ctx = (UnifiedContext*)app_data;
    switch(ctx->currentField) {
        case FIELD_NAME: {
            size_t len = val.length < 99 ? val.length : 99;
            memcpy(ctx->target->name, val.str, len);
            ctx->target->name[len] = '\0';
            break;
        }
        case FIELD_ID:      // Schemaless fallback
            ctx->target->id = atoi(val.str);
            break;
        case FIELD_ENABLED: // Schemaless fallback
            ctx->target->enabled = (val.length > 0 && val.str[0] == 't');
            break;
        default:
            break;
    }
    return EXIP_OK;
}
```

**Benefits:**
- Works in both schema and schemaless modes
- Single code path per field
- Easy to add new fields (add to table + switch case)
- String comparison done once in `startElement`

### 3. Generic Field Setter (Advanced)

For ultimate code reuse, use function pointers and generic setters:

```c
typedef void (*FieldSetter)(void* structPtr, size_t offset, const void* value);

static void setInt(void* ptr, size_t offset, const void* value) {
    *(int*)((char*)ptr + offset) = *(int*)value;
}

static void setBool(void* ptr, size_t offset, const void* value) {
    *(bool*)((char*)ptr + offset) = *(bool*)value;
}

static void setString(void* ptr, size_t offset, const void* value) {
    const String* str = (const String*)value;
    char* dest = (char*)ptr + offset;
    size_t len = str->length < 99 ? str->length : 99;
    memcpy(dest, str->str, len);
    dest[len] = '\0';
}

typedef struct {
    const char* name;
    size_t offset;
    FieldSetter setter;
} GenericField;

static const GenericField GEN_FIELDS[] = {
    {"id",      offsetof(MyStruct, id),      setInt},
    {"enabled", offsetof(MyStruct, enabled), setBool},
    {"name",    offsetof(MyStruct, name),    setString},
    {NULL, 0, NULL}
};

static errorCode generic_intData(Integer val, void* app_data) {
    // Lookup current field, call setter with &val
    // ...
    return EXIP_OK;
}
```

## Hash-Based Field Lookup (Performance)

For structs with many fields (>20), replace linear search with hash map:

```c
#include "hashTable.h"  // EXIP's hash table

static HashTable fieldMap;

void initFieldMap() {
    create_hashtable(&fieldMap);
    for(int i = 0; FIELDS[i].name != NULL; i++) {
        String key;
        asciiToString(FIELDS[i].name, &key, NULL, false);
        hashtable_put(&fieldMap, key, (Index)FIELDS[i].id);
    }
}

static FieldId lookupFieldFast(const String* name) {
    Index id;
    if(hashtable_get(&fieldMap, *name, &id))
        return (FieldId)id;
    return FIELD_UNKNOWN;
}
```

## Memory Management

### Using AllocList

```c
typedef struct {
    MyStruct* target;
    AllocList allocList;
} ManagedContext;

static errorCode managed_stringData(const String val, void* app_data) {
    ManagedContext* ctx = (ManagedContext*)app_data;
    String cloned;
    cloneStringManaged(&val, &cloned, &ctx->allocList);
    // Use cloned.str (stays valid)
    return EXIP_OK;
}

// Cleanup
void cleanup(ManagedContext* ctx) {
    freeAllocList(&ctx->allocList);
}
```

## Helper Functions (exipb binding pattern)

Generated code should use helper functions for common patterns:

```c
// Helper: Decode string to buffer
errorCode decodeStringToBuffer(const String* src, char* dest, size_t destSize) {
    size_t len = src->length < (destSize - 1) ? src->length : (destSize - 1);
    memcpy(dest, src->str, len);
    dest[len] = '\0';
    return EXIP_OK;
}

// Helper: String to int with error checking
errorCode decodeStringToInt(const String* src, int* dest) {
    char buf[32];
    if(src->length >= 32) return EXIP_OUT_OF_BOUND_BUFFER;
    memcpy(buf, src->str, src->length);
    buf[src->length] = '\0';
    *dest = atoi(buf);
    return EXIP_OK;
}

// Usage in generated code
static errorCode handle_stringData(const String val, void* app_data) {
    Context* ctx = (Context*)app_data;
    switch(ctx->currentField) {
        case FIELD_NAME:
            return decodeStringToBuffer(&val, ctx->target->name, 100);
        case FIELD_ID:  // Schemaless
            return decodeStringToInt(&val, &ctx->target->id);
        default:
            return EXIP_OK;
    }
}
```

## Best Practices

1. **Use field enums** instead of string comparisons in data handlers
2. **Single lookup** in `startElement`, use enum in all data handlers
3. **Switch statements** over field enum (compiler optimizes better than ifs)
4. **Handle both modes** in `stringData` for schemaless fallback
5. **Use helper functions** for conversions (exipb will generate these)
6. **AllocList for dynamic strings** - automatic cleanup
7. **Hash maps for large structs** (>20 fields)

## Anti-Patterns to Avoid

❌ String comparison in every data handler
❌ Duplicate schema/schemaless implementations
❌ Manual memory tracking (use AllocList)
❌ Ignoring schemaless mode in stringData
❌ Not validating string lengths before memcpy

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - EXIP architecture overview
- [CODE_GENERATION_VISION.md](CODE_GENERATION_VISION.md) - exipb binding generator
- [include/contentHandler.h](../include/contentHandler.h) - Content handler API
