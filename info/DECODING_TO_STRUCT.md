# Decoding EXI to C Structures

This guide shows how to decode EXI streams directly into C structures using EXIP.

## Schema Mode vs Schemaless Mode

### Schema Mode (Recommended)

With schema-informed grammars, EXIP calls typed handlers directly:
- `intData()` for integers
- `booleanData()` for booleans
- `floatData()` for floats
- `stringData()` for strings
- etc.

**Advantages:**
- No string parsing overhead
- Type safety from schema
- Direct assignment to struct fields
- Validation included

### Schemaless Mode

Without a schema, all data comes through `stringData()` as strings.

**Advantages:**
- No schema required
- Flexible for dynamic content

**Trade-offs:**
- Manual string-to-type conversion required
- No automatic validation
- Slight performance overhead

## Complete Example: Decoding to Struct

### Define Your Structure

```c
typedef struct {
    int id;
    bool enabled;
    char name[100];
    float value;
} MyStruct;
```

### Schema Mode Implementation

```c
#include "EXIParser.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    bool enabled;
    char name[100];
    float value;
} MyStruct;

// Application data to track decoding state
typedef struct {
    MyStruct* targetStruct;
    const char* currentElement;
    const char* currentAttribute;
} DecodingContext;

// Track which element we're decoding
static errorCode handle_startElement(QName qname, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;
    ctx->currentElement = qname.localName->str;
    return EXIP_OK;
}

static errorCode handle_endElement(void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;
    ctx->currentElement = NULL;
    return EXIP_OK;
}

static errorCode handle_attribute(QName qname, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;
    ctx->currentAttribute = qname.localName->str;
    return EXIP_OK;
}

// Handle integer data (schema mode)
static errorCode handle_intData(Integer int_val, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "id") == 0) {
        ctx->targetStruct->id = (int)int_val;
    }
    else if(ctx->currentAttribute && strcmp(ctx->currentAttribute, "id") == 0) {
        ctx->targetStruct->id = (int)int_val;
    }

    return EXIP_OK;
}

// Handle boolean data (schema mode)
static errorCode handle_booleanData(bool bool_val, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "enabled") == 0) {
        ctx->targetStruct->enabled = bool_val;
    }

    return EXIP_OK;
}

// Handle string data
static errorCode handle_stringData(const String str_val, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "name") == 0) {
        // Copy string to struct field
        size_t len = str_val.length < 99 ? str_val.length : 99;
        memcpy(ctx->targetStruct->name, str_val.str, len);
        ctx->targetStruct->name[len] = '\0';
    }

    return EXIP_OK;
}

// Handle float data (schema mode)
static errorCode handle_floatData(Float fl_val, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "value") == 0) {
        // Convert EXI Float (mantissa/exponent) to C float
        ctx->targetStruct->value = (float)fl_val.mantissa *
                                   powf(10.0f, (float)fl_val.exponent);
    }

    return EXIP_OK;
}

// Main parsing function
errorCode parseToStruct(BinaryBuffer buffer, EXIPSchema* schema, MyStruct* output) {
    Parser parser;
    DecodingContext ctx;
    errorCode err;

    // Initialize context
    ctx.targetStruct = output;
    ctx.currentElement = NULL;
    ctx.currentAttribute = NULL;

    // Initialize parser
    err = parse.initParser(&parser, buffer, &ctx);
    if(err != EXIP_OK) return err;

    // Set up handlers
    parser.handler.startElement = handle_startElement;
    parser.handler.endElement = handle_endElement;
    parser.handler.attribute = handle_attribute;
    parser.handler.intData = handle_intData;
    parser.handler.booleanData = handle_booleanData;
    parser.handler.stringData = handle_stringData;
    parser.handler.floatData = handle_floatData;

    // Parse header
    err = parse.parseHeader(&parser, false);
    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    // Set schema
    err = parse.setSchema(&parser, schema);
    if(err != EXIP_OK) {
        parse.destroyParser(&parser);
        return err;
    }

    // Parse body
    while(err == EXIP_OK) {
        err = parse.parseNext(&parser);
    }

    // Cleanup
    parse.destroyParser(&parser);

    return (err == EXIP_PARSING_COMPLETE) ? EXIP_OK : err;
}
```

### Schemaless Mode Implementation

In schemaless mode, all data comes as strings, so you need to convert:

```c
// Helper: Convert string to null-terminated buffer
static void stringToBuffer(const String* str, char* buf, size_t bufSize) {
    size_t len = str->length < (bufSize - 1) ? str->length : (bufSize - 1);
    memcpy(buf, str->str, len);
    buf[len] = '\0';
}

// Handle all data as strings in schemaless mode
static errorCode handle_stringData_schemaless(const String str_val, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;
    char buf[100];

    if(ctx->currentElement == NULL) {
        return EXIP_OK;  // Ignore text between elements
    }

    if(strcmp(ctx->currentElement, "id") == 0) {
        // Convert string to int
        stringToBuffer(&str_val, buf, sizeof(buf));
        ctx->targetStruct->id = atoi(buf);
    }
    else if(strcmp(ctx->currentElement, "enabled") == 0) {
        // Convert string to bool
        ctx->targetStruct->enabled = (str_val.length >= 4 &&
                                     strncmp(str_val.str, "true", 4) == 0);
    }
    else if(strcmp(ctx->currentElement, "name") == 0) {
        // Copy string directly
        size_t len = str_val.length < 99 ? str_val.length : 99;
        memcpy(ctx->targetStruct->name, str_val.str, len);
        ctx->targetStruct->name[len] = '\0';
    }
    else if(strcmp(ctx->currentElement, "value") == 0) {
        // Convert string to float
        stringToBuffer(&str_val, buf, sizeof(buf));
        ctx->targetStruct->value = (float)atof(buf);
    }

    return EXIP_OK;
}

// Similarly handle attributes in schemaless mode
static errorCode handle_attribute_schemaless(QName qname, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;
    ctx->currentAttribute = qname.localName->str;
    // Next stringData() call will have the attribute value
    return EXIP_OK;
}
```

## Using QNames for Element Identification

The `QName` structure provides namespace and local name:

```c
typedef struct QName {
    const String* uri;        // Namespace URI
    const String* localName;  // Element/attribute local name
    const String* prefix;     // Namespace prefix (if preserved)
} QName;
```

### Namespace-aware matching:

```c
static errorCode handle_startElement(QName qname, void* app_data) {
    DecodingContext* ctx = (DecodingContext*)app_data;

    // Match by namespace and local name
    if(stringEqual(qname.uri, &MY_NAMESPACE) &&
       stringEqual(qname.localName, &ELEM_CONFIG)) {
        ctx->inConfigElement = true;
    }

    // Or just use local name if namespaces don't matter
    ctx->currentElement = qname.localName->str;

    return EXIP_OK;
}
```

## Handling Nested Structures

For nested structures, maintain a stack or state machine:

```c
typedef struct {
    char model[50];
    int year;
} Car;

typedef struct {
    char name[100];
    int age;
    Car car;  // Nested structure
} Person;

typedef enum {
    STATE_NONE,
    STATE_IN_PERSON,
    STATE_IN_CAR
} ParsingState;

typedef struct {
    Person* targetStruct;
    ParsingState state;
    const char* currentElement;
} NestedContext;

static errorCode handle_startElement_nested(QName qname, void* app_data) {
    NestedContext* ctx = (NestedContext*)app_data;

    if(strcmp(qname.localName->str, "person") == 0) {
        ctx->state = STATE_IN_PERSON;
    }
    else if(strcmp(qname.localName->str, "car") == 0) {
        ctx->state = STATE_IN_CAR;
    }

    ctx->currentElement = qname.localName->str;
    return EXIP_OK;
}

static errorCode handle_stringData_nested(const String str_val, void* app_data) {
    NestedContext* ctx = (NestedContext*)app_data;

    if(ctx->state == STATE_IN_PERSON) {
        if(strcmp(ctx->currentElement, "name") == 0) {
            size_t len = str_val.length < 99 ? str_val.length : 99;
            memcpy(ctx->targetStruct->name, str_val.str, len);
            ctx->targetStruct->name[len] = '\0';
        }
    }
    else if(ctx->state == STATE_IN_CAR) {
        if(strcmp(ctx->currentElement, "model") == 0) {
            size_t len = str_val.length < 49 ? str_val.length : 49;
            memcpy(ctx->targetStruct->car.model, str_val.str, len);
            ctx->targetStruct->car.model[len] = '\0';
        }
    }

    return EXIP_OK;
}

static errorCode handle_intData_nested(Integer int_val, void* app_data) {
    NestedContext* ctx = (NestedContext*)app_data;

    if(ctx->state == STATE_IN_PERSON &&
       strcmp(ctx->currentElement, "age") == 0) {
        ctx->targetStruct->age = (int)int_val;
    }
    else if(ctx->state == STATE_IN_CAR &&
            strcmp(ctx->currentElement, "year") == 0) {
        ctx->targetStruct->car.year = (int)int_val;
    }

    return EXIP_OK;
}

static errorCode handle_endElement_nested(void* app_data) {
    NestedContext* ctx = (NestedContext*)app_data;

    if(ctx->state == STATE_IN_CAR) {
        ctx->state = STATE_IN_PERSON;  // Exit car, back to person
    }
    else if(ctx->state == STATE_IN_PERSON) {
        ctx->state = STATE_NONE;  // Exit person
    }

    ctx->currentElement = NULL;
    return EXIP_OK;
}
```

## Handling Arrays/Lists

For repeated elements (arrays), use dynamic allocation or fixed-size arrays:

```c
#define MAX_ITEMS 100

typedef struct {
    int values[MAX_ITEMS];
    size_t count;
} IntArray;

typedef struct {
    IntArray* targetArray;
    bool inItem;
} ArrayContext;

static errorCode handle_startElement_array(QName qname, void* app_data) {
    ArrayContext* ctx = (ArrayContext*)app_data;

    if(strcmp(qname.localName->str, "item") == 0) {
        ctx->inItem = true;
    }

    return EXIP_OK;
}

static errorCode handle_intData_array(Integer int_val, void* app_data) {
    ArrayContext* ctx = (ArrayContext*)app_data;

    if(ctx->inItem && ctx->targetArray->count < MAX_ITEMS) {
        ctx->targetArray->values[ctx->targetArray->count++] = (int)int_val;
    }

    return EXIP_OK;
}

static errorCode handle_endElement_array(void* app_data) {
    ArrayContext* ctx = (ArrayContext*)app_data;
    ctx->inItem = false;
    return EXIP_OK;
}
```

## Best Practices

1. **Use Schema Mode When Possible**
   - Better performance (no string parsing)
   - Type safety from schema
   - Automatic validation

2. **Track State Carefully**
   - Use context structure to maintain parsing state
   - Clear state in `endElement` handlers
   - Use enums for state machines in complex documents

3. **Handle Namespaces**
   - Match both URI and local name for robustness
   - Use namespace-aware comparison when needed

4. **Validate Input**
   - Check buffer sizes before copying strings
   - Validate converted values (range checks, etc.)
   - Handle missing optional elements

5. **Error Handling**
   - Return appropriate error codes from handlers
   - Use `EXIP_HANDLER_STOP` to abort parsing on fatal errors
   - Clean up resources in error paths

## Complete Usage Example

```c
int main() {
    MyStruct myData = {0};
    BinaryBuffer buffer;
    EXIPSchema schema;
    errorCode err;

    // Load EXI file into buffer
    FILE* f = fopen("data.exi", "rb");
    fseek(f, 0, SEEK_END);
    buffer.bufLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer.buf = malloc(buffer.bufLen);
    fread(buffer.buf, 1, buffer.bufLen, f);
    fclose(f);
    buffer.bufContent = buffer.bufLen;
    buffer.ioStrm.readWriteToStream = NULL;
    buffer.ioStrm.stream = NULL;

    // Load schema (optional, pass NULL for schemaless)
    // ... load schema into 'schema' ...

    // Parse EXI into struct
    err = parseToStruct(buffer, &schema, &myData);

    if(err == EXIP_OK) {
        printf("Parsed successfully:\n");
        printf("  id: %d\n", myData.id);
        printf("  enabled: %s\n", myData.enabled ? "true" : "false");
        printf("  name: %s\n", myData.name);
        printf("  value: %f\n", myData.value);
    }
    else {
        printf("Parsing failed: error %d\n", err);
    }

    free(buffer.buf);
    return 0;
}
```

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall EXIP architecture
- [examples/simpleDecoding/decodeTestEXI.c](examples/simpleDecoding/decodeTestEXI.c) - Complete decoding example
- [include/EXIParser.h](include/EXIParser.h) - Parser API reference
- [include/contentHandler.h](include/contentHandler.h) - Content handler callbacks
