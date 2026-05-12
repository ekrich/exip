# Code Generation from Schema - Vision Document

## Overview

Automatically generate C structures and marshal/unmarshal code from XML Schema (XSD) files, supporting both schema-informed and schemaless EXI processing.

## Goals

1. **Generate C structures** from XSD that mirror the schema
2. **Generate marshal code** (C struct → EXI)
3. **Generate unmarshal code** (EXI → C struct)
4. **Support both modes** - schema-informed and schemaless with same structs
5. **Handle nested structures** automatically
6. **Type-safe** - leverage C compiler for validation

## Example Workflow

### Input: XML Schema (XSD)

```xml
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
           targetNamespace="http://example.com/person"
           xmlns:tns="http://example.com/person">

  <xs:complexType name="Address">
    <xs:sequence>
      <xs:element name="street" type="xs:string"/>
      <xs:element name="city" type="xs:string"/>
      <xs:element name="zipCode" type="xs:int"/>
    </xs:sequence>
  </xs:complexType>

  <xs:complexType name="Person">
    <xs:sequence>
      <xs:element name="id" type="xs:int"/>
      <xs:element name="name" type="xs:string"/>
      <xs:element name="age" type="xs:int"/>
      <xs:element name="active" type="xs:boolean"/>
      <xs:element name="address" type="tns:Address"/>
      <xs:element name="tags" type="xs:string" maxOccurs="unbounded"/>
    </xs:sequence>
    <xs:attribute name="version" type="xs:string"/>
  </xs:complexType>

  <xs:element name="person" type="tns:Person"/>
</xs:schema>
```

### Generated Output: C Header

```c
/* Auto-generated from person.xsd */
#ifndef PERSON_H
#define PERSON_H

#include "EXIParser.h"
#include "EXISerializer.h"
#include <stdbool.h>

// Forward declarations
typedef struct Address Address;
typedef struct Person Person;

// Generated structures
struct Address {
    char street[256];
    char city[256];
    int zipCode;
};

struct Person {
    int id;
    char name[256];
    int age;
    bool active;
    Address address;

    // Dynamic array for unbounded elements
    struct {
        char** items;
        size_t count;
        size_t capacity;
    } tags;

    // Attributes
    char version[64];
};

// Marshal functions (C struct -> EXI)
errorCode marshal_Address(EXIStream* stream, const Address* obj);
errorCode marshal_Person(EXIStream* stream, const Person* obj);

// Unmarshal functions (EXI -> C struct)
errorCode unmarshal_Address(Parser* parser, Address* obj);
errorCode unmarshal_Person(Parser* parser, Person* obj);

// Helper functions
void init_Person(Person* person);
void free_Person(Person* person);
void init_Address(Address* addr);

// Full document marshal/unmarshal
errorCode marshal_person_document(const char* filename, const Person* person,
                                  EXIPSchema* schema);  // NULL for schemaless
errorCode unmarshal_person_document(const char* filename, Person* person,
                                    EXIPSchema* schema);  // NULL for schemaless

#endif /* PERSON_H */
```

### Generated Output: C Implementation

```c
/* Auto-generated from person.xsd */
#include "person.h"
#include <stdlib.h>
#include <string.h>

// Namespace constants
static const String NS_PERSON = {"http://example.com/person", 26};
static const String NS_EMPTY = {NULL, 0};

// Element name constants
static const String ELEM_ADDRESS = {"address", 7};
static const String ELEM_STREET = {"street", 6};
static const String ELEM_CITY = {"city", 4};
static const String ELEM_ZIPCODE = {"zipCode", 7};
static const String ELEM_PERSON = {"person", 6};
static const String ELEM_ID = {"id", 2};
static const String ELEM_NAME = {"name", 4};
static const String ELEM_AGE = {"age", 3};
static const String ELEM_ACTIVE = {"active", 6};
static const String ELEM_TAGS = {"tags", 4};
static const String ATTR_VERSION = {"version", 7};

//==============================================================================
// Initialization functions
//==============================================================================

void init_Person(Person* person) {
    memset(person, 0, sizeof(Person));
    person->tags.items = NULL;
    person->tags.count = 0;
    person->tags.capacity = 0;
    init_Address(&person->address);
}

void init_Address(Address* addr) {
    memset(addr, 0, sizeof(Address));
}

void free_Person(Person* person) {
    // Free dynamic array
    for(size_t i = 0; i < person->tags.count; i++) {
        free(person->tags.items[i]);
    }
    free(person->tags.items);
    person->tags.items = NULL;
    person->tags.count = 0;
    person->tags.capacity = 0;
}

//==============================================================================
// Marshal functions (C struct -> EXI)
//==============================================================================

errorCode marshal_Address(EXIStream* stream, const Address* obj) {
    errorCode err;
    QName qname;
    String strVal;
    EXITypeClass valueType;

    // <address>
    qname.uri = &NS_PERSON;
    qname.localName = &ELEM_ADDRESS;
    qname.prefix = NULL;
    TRY(serialize.startElement(stream, qname, &valueType));

    // <street>
    qname.localName = &ELEM_STREET;
    TRY(serialize.startElement(stream, qname, &valueType));
    asciiToString(obj->street, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));
    TRY(serialize.endElement(stream));

    // <city>
    qname.localName = &ELEM_CITY;
    TRY(serialize.startElement(stream, qname, &valueType));
    asciiToString(obj->city, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));
    TRY(serialize.endElement(stream));

    // <zipCode>
    qname.localName = &ELEM_ZIPCODE;
    TRY(serialize.startElement(stream, qname, &valueType));
    if(stream->schema != NULL) {
        // Schema mode - use typed data
        TRY(serialize.intData(stream, obj->zipCode));
    } else {
        // Schemaless mode - convert to string
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", obj->zipCode);
        asciiToString(buf, &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
    }
    TRY(serialize.endElement(stream));

    // </address>
    TRY(serialize.endElement(stream));

    return EXIP_OK;
}

errorCode marshal_Person(EXIStream* stream, const Person* obj) {
    errorCode err;
    QName qname;
    String strVal;
    EXITypeClass valueType;

    // <person>
    qname.uri = &NS_PERSON;
    qname.localName = &ELEM_PERSON;
    qname.prefix = NULL;
    TRY(serialize.startElement(stream, qname, &valueType));

    // @version attribute
    qname.uri = &NS_EMPTY;
    qname.localName = &ATTR_VERSION;
    TRY(serialize.attribute(stream, qname, true, &valueType));
    asciiToString(obj->version, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));

    // <id>
    qname.uri = &NS_PERSON;
    qname.localName = &ELEM_ID;
    TRY(serialize.startElement(stream, qname, &valueType));
    if(stream->schema != NULL) {
        TRY(serialize.intData(stream, obj->id));
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", obj->id);
        asciiToString(buf, &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
    }
    TRY(serialize.endElement(stream));

    // <name>
    qname.localName = &ELEM_NAME;
    TRY(serialize.startElement(stream, qname, &valueType));
    asciiToString(obj->name, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));
    TRY(serialize.endElement(stream));

    // <age>
    qname.localName = &ELEM_AGE;
    TRY(serialize.startElement(stream, qname, &valueType));
    if(stream->schema != NULL) {
        TRY(serialize.intData(stream, obj->age));
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", obj->age);
        asciiToString(buf, &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
    }
    TRY(serialize.endElement(stream));

    // <active>
    qname.localName = &ELEM_ACTIVE;
    TRY(serialize.startElement(stream, qname, &valueType));
    if(stream->schema != NULL) {
        TRY(serialize.booleanData(stream, obj->active));
    } else {
        asciiToString(obj->active ? "true" : "false", &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
    }
    TRY(serialize.endElement(stream));

    // <address> (nested)
    TRY(marshal_Address(stream, &obj->address));

    // <tags> (unbounded - array)
    for(size_t i = 0; i < obj->tags.count; i++) {
        qname.localName = &ELEM_TAGS;
        TRY(serialize.startElement(stream, qname, &valueType));
        asciiToString(obj->tags.items[i], &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
        TRY(serialize.endElement(stream));
    }

    // </person>
    TRY(serialize.endElement(stream));

    return EXIP_OK;
}

//==============================================================================
// Unmarshal functions (EXI -> C struct)
//==============================================================================

// Unmarshal context for Address
typedef struct {
    Address* target;
    const char* currentElement;
} UnmarshalContext_Address;

static errorCode unmarshal_Address_startElement(QName qname, void* app_data) {
    UnmarshalContext_Address* ctx = (UnmarshalContext_Address*)app_data;
    ctx->currentElement = qname.localName->str;
    return EXIP_OK;
}

static errorCode unmarshal_Address_stringData(const String str_val, void* app_data) {
    UnmarshalContext_Address* ctx = (UnmarshalContext_Address*)app_data;

    if(ctx->currentElement == NULL) return EXIP_OK;

    if(strcmp(ctx->currentElement, "street") == 0) {
        size_t len = str_val.length < 255 ? str_val.length : 255;
        memcpy(ctx->target->street, str_val.str, len);
        ctx->target->street[len] = '\0';
    }
    else if(strcmp(ctx->currentElement, "city") == 0) {
        size_t len = str_val.length < 255 ? str_val.length : 255;
        memcpy(ctx->target->city, str_val.str, len);
        ctx->target->city[len] = '\0';
    }

    return EXIP_OK;
}

static errorCode unmarshal_Address_intData(Integer int_val, void* app_data) {
    UnmarshalContext_Address* ctx = (UnmarshalContext_Address*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "zipCode") == 0) {
        ctx->target->zipCode = (int)int_val;
    }

    return EXIP_OK;
}

errorCode unmarshal_Address(Parser* parser, Address* obj) {
    UnmarshalContext_Address ctx;
    errorCode err;

    ctx.target = obj;
    ctx.currentElement = NULL;

    // Set up temporary handlers for this sub-element
    ContentHandler savedHandler = parser->handler;

    parser->handler.startElement = unmarshal_Address_startElement;
    parser->handler.stringData = unmarshal_Address_stringData;
    parser->handler.intData = unmarshal_Address_intData;
    parser->app_data = &ctx;

    // Parse until we hit </address>
    int depth = 1;
    while(depth > 0 && err == EXIP_OK) {
        err = parse.parseNext(parser);
        // Track depth based on SE/EE events
        // (simplified - real implementation needs proper depth tracking)
    }

    // Restore handlers
    parser->handler = savedHandler;

    return err;
}

// Similar implementation for unmarshal_Person...

//==============================================================================
// High-level document functions
//==============================================================================

errorCode marshal_person_document(const char* filename, const Person* person,
                                  EXIPSchema* schema) {
    errorCode err;
    EXIStream stream;
    BinaryBuffer buffer;
    char buf[4096];
    FILE* outfile;

    // Open output file
    outfile = fopen(filename, "wb");
    if(!outfile) return EXIP_UNEXPECTED_ERROR;

    // Initialize buffer
    buffer.buf = buf;
    buffer.bufLen = sizeof(buf);
    buffer.bufContent = 0;
    buffer.ioStrm.readWriteToStream = (size_t (*)(void*, size_t, void*))fwrite;
    buffer.ioStrm.stream = outfile;

    // Initialize stream
    serialize.initHeader(&stream);
    stream.header.has_cookie = true;
    stream.header.has_options = true;
    if(schema != NULL) {
        SET_STRICT(stream.header.opts.enumOpt);
    }

    err = serialize.initStream(&stream, buffer, schema);
    if(err != EXIP_OK) {
        fclose(outfile);
        return err;
    }

    // Marshal document
    err = serialize.exiHeader(&stream);
    if(err != EXIP_OK) goto cleanup;

    err = serialize.startDocument(&stream);
    if(err != EXIP_OK) goto cleanup;

    err = marshal_Person(&stream, person);
    if(err != EXIP_OK) goto cleanup;

    err = serialize.endDocument(&stream);

cleanup:
    serialize.closeEXIStream(&stream);
    fclose(outfile);
    return err;
}

errorCode unmarshal_person_document(const char* filename, Person* person,
                                    EXIPSchema* schema) {
    errorCode err;
    Parser parser;
    BinaryBuffer buffer;
    char buf[4096];
    FILE* infile;

    // Open input file
    infile = fopen(filename, "rb");
    if(!infile) return EXIP_UNEXPECTED_ERROR;

    // Read file into buffer
    buffer.bufContent = fread(buf, 1, sizeof(buf), infile);
    buffer.buf = buf;
    buffer.bufLen = buffer.bufContent;
    buffer.ioStrm.readWriteToStream = NULL;
    buffer.ioStrm.stream = NULL;
    fclose(infile);

    // Initialize parser
    err = parse.initParser(&parser, buffer, person);
    if(err != EXIP_OK) return err;

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

    // Unmarshal document
    err = unmarshal_Person(&parser, person);

    parse.destroyParser(&parser);
    return err;
}
```

### Usage Example

```c
#include "person.h"
#include <stdio.h>

int main() {
    Person person;
    errorCode err;

    // Initialize
    init_Person(&person);

    // Fill in data
    person.id = 12345;
    strcpy(person.name, "John Doe");
    person.age = 30;
    person.active = true;
    strcpy(person.version, "1.0");

    strcpy(person.address.street, "123 Main St");
    strcpy(person.address.city, "Springfield");
    person.address.zipCode = 12345;

    // Add tags (dynamic array)
    person.tags.count = 2;
    person.tags.capacity = 2;
    person.tags.items = malloc(2 * sizeof(char*));
    person.tags.items[0] = strdup("employee");
    person.tags.items[1] = strdup("manager");

    // Marshal to EXI (schema mode)
    EXIPSchema schema;
    loadSchema("person.xsd.exi", &schema);
    err = marshal_person_document("person.exi", &person, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Marshal error: %d\n", err);
        return 1;
    }

    // Unmarshal from EXI
    Person person2;
    init_Person(&person2);
    err = unmarshal_person_document("person.exi", &person2, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Unmarshal error: %d\n", err);
        return 1;
    }

    // Verify
    printf("ID: %d\n", person2.id);
    printf("Name: %s\n", person2.name);
    printf("Age: %d\n", person2.age);
    printf("Active: %s\n", person2.active ? "true" : "false");
    printf("Address: %s, %s %d\n",
           person2.address.street,
           person2.address.city,
           person2.address.zipCode);

    // Cleanup
    free_Person(&person);
    free_Person(&person2);
    destroySchema(&schema);

    return 0;
}
```

## Implementation Plan

### Phase 1: Schema Parser (Reuse Existing!)
- **Reuse EXIP's existing TreeTable parser** from `src/grammarGen/`
- Input: EXI-encoded XSD files (same format as grammar generator)
- Use existing functions:
  - `generateTreeTable()` - Parses EXI-encoded schema
  - `resolveTypeHierarchy()` - Links types and references
  - Extract type definitions, elements, attributes from `TreeTable`
- **No XML parser needed** - EXIP already has this!

### Phase 2: C Structure Generator
- Generate C struct definitions from schema types
- Handle:
  - Simple types → C primitive types
  - Complex types → C structs
  - Sequences → struct fields
  - Choices → unions
  - Unbounded elements → dynamic arrays
  - Attributes → struct fields

### Phase 3: Marshal Code Generator
- Generate `marshal_TypeName()` functions
- Automatically handle schema vs schemaless mode
- Use typed data calls when schema available
- Convert to strings for schemaless mode

### Phase 4: Unmarshal Code Generator
- Generate `unmarshal_TypeName()` functions
- Generate content handlers for each type
- Handle both typed and string data automatically
- Manage parser state for nested structures

### Phase 5: Build Integration
- CMake/Makefile integration
- `exipgen` command-line tool
- Watch XSD files and regenerate on changes

## Command-Line Tool Design

Tool name: **`exipcodegen`** (EXI Processor Code Generator)

```bash
# Generate code from EXI-encoded schema
exipcodegen --input person.xsd.exi --output person.h person.c

# Options
exipcodegen --input person.xsd.exi \
            --output-dir generated/ \
            --prefix myapp_ \
            --namespace myapp \
            --string-max-length 512 \
            --array-initial-capacity 16

# Generate from multiple EXI-encoded schemas
exipcodegen --input schema1.xsd.exi schema2.xsd.exi \
            --output generated/

# Alternative: Specify schema and let tool encode it
exipcodegen --input-xml person.xsd \
            --xml-schema xml-schema.xsd.exi \
            --output person.h person.c
```

**Input Format**: The tool expects **EXI-encoded schemas** with `Preserve.prefixes` enabled, just like the existing grammar generator.

## Configuration File

```json
{
  "schemas": [
    {
      "input": "person.xsd.exi",
      "namespace": "http://example.com/person",
      "prefix": "person"
    },
    {
      "input": "order.xsd.exi",
      "namespace": "http://example.com/order",
      "prefix": "order"
    }
  ],
  "output": {
    "directory": "generated/",
    "header_extension": ".h",
    "source_extension": ".c"
  },
  "options": {
    "string_max_length": 256,
    "array_initial_capacity": 10,
    "generate_json_converters": false,
    "generate_xml_converters": false
  }
}
```

## Advanced Features

### 1. Choice Handling

```xml
<xs:choice>
  <xs:element name="email" type="xs:string"/>
  <xs:element name="phone" type="xs:string"/>
</xs:choice>
```

Generated:
```c
typedef enum {
    CONTACT_NONE,
    CONTACT_EMAIL,
    CONTACT_PHONE
} ContactType;

typedef struct {
    ContactType type;
    union {
        char email[256];
        char phone[32];
    } data;
} Contact;
```

### 2. Polymorphism (xsi:type)

```c
typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE
} ShapeType;

typedef struct {
    ShapeType type;
    union {
        Circle circle;
        Rectangle rectangle;
    } data;
} Shape;
```

### 3. Validation

```c
// Generated validation functions
errorCode validate_Person(const Person* person) {
    if(person->age < 0 || person->age > 150) {
        return EXIP_INVALID_EXI_INPUT;
    }
    if(strlen(person->name) == 0) {
        return EXIP_INVALID_EXI_INPUT;
    }
    return validate_Address(&person->address);
}
```

### 4. JSON/XML Converters

```c
// Optionally generate converters to other formats
errorCode person_to_json(const Person* person, FILE* output);
errorCode person_from_json(const char* json_str, Person* person);
errorCode person_to_xml(const Person* person, FILE* output);
```

## Benefits

1. **Type Safety** - Compiler catches errors at compile time
2. **No Manual Coding** - Automatically stays in sync with schema
3. **Both Modes** - Works with schema-informed and schemaless
4. **Performance** - Zero-copy where possible, minimal allocations
5. **Memory Management** - Clear ownership, helper functions for cleanup
6. **Maintainability** - Regenerate when schema changes
7. **Documentation** - Generated code is readable

## Type Conversion Strategy

### Generated Structs Use Standard C Types

Generated structs use familiar C types for user-friendliness:
```c
struct Person {
    int id;              // C int, not EXIP Integer
    char name[256];      // Null-terminated C string, not EXIP String
    bool active;         // C bool, not custom type
};
```

**Rationale:**
- Users don't need to understand EXIP internal types
- Works with standard C functions (`printf`, `strcmp`, etc.)
- Fixed buffers are embedded-friendly (no malloc needed)

### Internal Conversion Layer

Generated marshal/unmarshal code handles conversion between C types and EXIP types:

**Schema-informed mode** (typed encoding):
```c
// Marshal: C int → EXIP binary integer
serialize.intData(stream, obj->id);  // Direct, no conversion needed

// Unmarshal: EXIP Integer → C int
int handle_intData(Integer int_val, void* app_data) {
    Person* obj = (Person*)app_data;
    obj->id = (int)int_val;  // Simple cast
}
```

**Schema-less mode** (string encoding):
```c
// Marshal: C int → C string → EXIP String
char buf[32];
snprintf(buf, sizeof(buf), "%d", obj->id);  // C int → C string
asciiToString(buf, &strVal, &stream->memList, false);  // C string → EXIP String
serialize.stringData(stream, strVal);  // Encode EXIP String

// Unmarshal: EXIP String → C string → C int
int handle_stringData(const String value, void* app_data) {
    Person* obj = (Person*)app_data;
    char buf[32];
    size_t len = value.length < 31 ? value.length : 31;
    memcpy(buf, value.str, len);  // EXIP String → C string buffer
    buf[len] = '\0';
    obj->id = atoi(buf);  // C string → C int
}
```

### EXIP String Type

EXIP uses **length-prefixed strings** (not null-terminated):
```c
struct StringType {
    CharType* str;    // Pointer to characters (NOT null-terminated!)
    Index length;     // String length in characters
};
typedef struct StringType String;
```

Generated code must convert between:
- **C strings** (`char[]`, null-terminated) ↔ **EXIP Strings** (`String`, length-prefixed)
- **C types** (`int`, `float`, `bool`) ↔ **C strings** (for schema-less mode)

### Unimplemented Conversion Functions

EXIP has stub functions for typed-to-string conversions (see [UNIMPLEMENTED_FEATURES.md](UNIMPLEMENTED_FEATURES.md)):
- `integerToString()`, `booleanToString()`, `floatToString()`, etc.

These would return **EXIP `String` types**, not C strings. Generated code currently uses `snprintf()` instead, which works but is repetitive. Implementing these functions would clean up generated code but isn't essential.

## Challenges

1. **Complex XSD Features** - Some advanced XSD features are hard to map to C
2. **Memory Management** - Need clear patterns for dynamic arrays, strings
3. **Error Handling** - Need good error messages from generated code
4. **Large Schemas** - Code size could be significant for huge schemas
5. **Circular References** - Need forward declarations and careful ordering
6. **Type Conversions** - Schema-less mode requires string conversions for all typed fields

## Similar Projects

- **protobuf-c** - Protocol Buffers C code generator (good reference)
- **Fast Infoset** - Java binding generator (conceptual reference)
- **OpenC2** - EXI-based C code generation (commercial)

## Next Steps

1. Prototype schema parser using EXIP's TreeTable
2. Build simple struct generator for basic types
3. Add marshal code generation
4. Add unmarshal code generation
5. Test with real-world schemas
6. Add advanced features (choices, unions, etc.)
7. Build command-line tool
8. Write comprehensive documentation

## Related Documents

- [UNIFIED_DECODING_PATTERNS.md](UNIFIED_DECODING_PATTERNS.md) - Manual decoding patterns
- [DECODING_TO_STRUCT.md](DECODING_TO_STRUCT.md) - Hand-written struct decoding
- [ARCHITECTURE.md](ARCHITECTURE.md) - EXIP architecture
- [src/grammarGen/](src/grammarGen/) - Existing schema processing code

---

**Status**: Vision / Planning Phase
**Priority**: High (would significantly improve usability)
**Complexity**: High (3-6 months development)
**Dependencies**: None (uses existing EXIP APIs)
