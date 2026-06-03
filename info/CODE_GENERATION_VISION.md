# Code Generation from Schema - Vision Document

## Code Generation Pipeline (Simple Steps)

1. **Load schemas** - Load EXI-encoded XSD files into `BinaryBuffer[]` using `loadSchemaFiles()`
2. **Parse and optimize TreeTable AST** - Call `generateOptimizedTreeTable(buffers, ..., &treeT, &treeTCount, &subsTbl, NULL, NULL)` - NULL for schema and loadSchemaHandler means no string table population, just build and link AST
3. **Walk TreeTable** - Extract type definitions, element names, attribute names, namespaces from linked AST
4. **Generate C constants** - Emit static const String declarations for all namespaces and element/attribute names (like exipe example)
5. **Generate C code** - Emit structs, `encode()`/`decode()` functions using reusable type-conversion helpers

**Key:** `generateOptimizedTreeTable()` does all TreeTable generation, include/import resolution, and type hierarchy linking in one call. Generated code uses EXIP's existing `serialize.*Data()` API with simple type conversions where needed.

## Type Conversion Strategy

Generated code uses **normal C types** in structs but needs to convert to EXIP types when calling the serialization API:

```c
// User struct - pure C types
struct Person {
    int age;           // Normal C int
    float height;      // Normal C float  
    bool enabled;      // Standard bool
    char name[101];    // Fixed C string
};

// Generated encode function
errorCode encode_Person(EXIStream* strm, Person* p) {
    // Simple cast for integers
    TRY(serialize.intData(strm, (Integer)p->age));
    
    // Boolean - no conversion needed
    TRY(serialize.booleanData(strm, p->enabled));
    
    // Float requires mantissa/exponent calculation - use helper
    Float f = floatToExipFloat(p->height);
    TRY(serialize.floatData(strm, f));
    
    // String conversion using existing EXIP function
    String s;
    TRY(asciiToString(p->name, &s, &strm->memList, false));
    TRY(serialize.stringData(strm, s));
    
    return EXIP_OK;
}
```

### Schema vs Schemaless Mode Support

exipb supports two generation modes via command-line flag:

**Schema-only mode (default, production/embedded):**
```bash
exipb --mode=schema-only schema.xsd
```
Generated code uses `serialize.intData()`, `serialize.booleanData()`, etc.
User must compile with schema and run in schema mode.
Smallest code size.

**Dual-mode (development/debugging):**
```bash
exipb --mode=dual schema.xsd
```
Generated code works in both schema and schemaless modes.
User compiles EXIP with `EXIP_IMPLICIT_DATA_TYPE_CONVERSION ON`.
`serialize.intData()` automatically converts to string in schemaless mode.
Slightly larger code, maximum flexibility.

**No new API needed** - uses existing `EXIP_IMPLICIT_DATA_TYPE_CONVERSION` flag mechanism.

### Required Type Conversion Helpers

Only these helpers are needed (not in EXIP yet, to be added):

```c
// Float: normal C float → EXIP Float struct (mantissa/exponent)
Float floatToExipFloat(float value);

// Double: normal C double → EXIP Float struct
Float doubleToExipFloat(double value);
```

Everything else is a simple cast or uses existing EXIP functions (`asciiToString`, etc.).

## String Handling Strategy

Generated code uses **clean C strings** (no EXIP types exposed to client code):

### String Type Selection

**Always use fixed-size arrays** - never `char*`:

- **`char[N+1]`** when schema defines `maxLength=N`
  ```c
  <xs:element name="name" type="xs:string">
    <xs:maxLength value="100"/>
  </xs:element>
  
  // Generated:
  struct Person {
      char name[101];  // maxLength + 1 for null terminator
  };
  ```

- **`char[DEFAULT_MAX+1]`** when schema has NO `maxLength` constraint
  ```c
  <xs:element name="description" type="xs:string"/>
  
  // Generated (with --max-string=64):
  struct Person {
      char description[65];  // Configurable default + 1
  };
  ```

**Benefits of fixed arrays:**
- No malloc/free overhead
- No memory leaks possible
- Predictable memory layout
- Safe: compiler enforces bounds
- Stack-friendly for embedded
- Embedded-first design

### exipb Command Line Options

```bash
exipb --max-string=64 schema.xsd    # Default max for unbounded strings (default)
exipb --max-string=128 schema.xsd   # Larger default
exipb --warn-unbounded schema.xsd   # Warn about unbounded strings
```

Example warning output:
```
WARNING: Element 'Person.description' has no maxLength, using default 64
  Consider adding: <xs:maxLength value="256"/>
WARNING: Element 'Address.notes' has no maxLength, using default 64
  Consider adding: <xs:maxLength value="512"/>
```

**Design principle:** Warnings guide schema design but code always works. Users can:
- Add `maxLength` constraints for explicit control (recommended)
- Accept default for rapid prototyping
- Configure default for their domain

### Validation

EXIP library already validates:
- ✅ Integer min/max (minInclusive/maxInclusive)
- ✅ String length (via fixed array size at compile time)
- ❌ minExclusive/maxExclusive (TODO in EXIP)
- ❌ Pattern facets (logged but not enforced)

exipb **relies on EXIP's existing validation** - no duplicate checks in generated code.

## Nested Type Strategy

### Embedded vs Pointer for Complex Types

**Embed by default (strongly preferred):**
```c
struct Person {
    Address address;  // Embedded - single allocation, cache-friendly
};
```

**Benefits:**
- Single malloc for entire struct
- Contiguous memory (cache-efficient)
- Simpler lifecycle (one free)
- No null checks needed
- Predictable `sizeof()`

**Use pointer only when schema requires:**
```c
struct Person {
    Address* altAddress;  // Pointer - only for optional/array elements
};
```

**Pointer required for:**
- `minOccurs="0"` (optional element)
- `maxOccurs="unbounded"` (array of elements)
- Circular schema references (rare)

**Key insight:** Embedded structs with `char*` fields inside are still preferred over pointers. The `char*` fields are internal allocations - embedding the struct itself still wins.

## DateTime Handling Strategy

For `xs:dateTime`, `xs:date`, `xs:time`, `xs:gYearMonth`, and related temporal types, bindings generate a **local DateTime struct** (modeled after EXIP's `EXIPDateTime` but with no EXIP dependency):

```c
// Generated in user's binding header (e.g., person.h)
// User code has ZERO EXIP dependencies
typedef struct {
    struct tm dateTime;      // Standard C time struct from <time.h>
    uint32_t fSecs;          // Fractional seconds (microseconds/nanoseconds)
    int16_t TimeZone;        // TZHours * 64 + TZMinutes (range: -14:00 to +14:00)
    uint8_t presenceMask;    // Flags: TZONE_PRESENCE (0x01), FRACT_PRESENCE (0x02)
} DateTime;  // Local type, not EXIP's EXIPDateTime

struct Person {
    int id;
    char name[256];
    DateTime birthDate;      // User uses DateTime, not EXIPDateTime
};
```

**Why this structure:**
- **Standard C base**: Uses `struct tm` (year, month, day, hour, min, sec) - portable, familiar, works with `<time.h>` functions
- **XML Schema extensions**: Adds fractional seconds and timezone (not in standard C `struct tm`)
- **Optional fields**: `presenceMask` indicates which optional fields are present
  - `0` = neither timezone nor fractional seconds
  - `1` (0x01) = timezone only
  - `2` (0x02) = fractional seconds only
  - `3` (0x03) = both present
- **Compact timezone encoding**: `TZHours * 64 + TZMinutes` fits in 2 bytes, covers XML Schema range
- **Codec control**: Mask tells encoder/decoder whether to read/write optional fields from EXI stream

**No EXIP leakage to user code:**
The generated binding header (`person.h`) contains only the local `DateTime` struct definition. User code never includes EXIP headers or uses EXIP types.

**Internal conversion in generated encode/decode:**
```c
// Inside generated person.c (includes EXIP headers)
errorCode encode(EXIStream* stream, const Person* person) {
    // Convert local DateTime -> EXIP EXIPDateTime
    EXIPDateTime exip_dt;
    exip_dt.dateTime = person->birthDate.dateTime;
    exip_dt.fSecs = person->birthDate.fSecs;
    exip_dt.TimeZone = person->birthDate.TimeZone;
    exip_dt.presenceMask = person->birthDate.presenceMask;

    // Call EXIP (internally)
    TRY(serialize.dateTimeData(stream, exip_dt));
}

errorCode decode(Parser* parser, Person* person) {
    // EXIP callback receives EXIPDateTime
    EXIPDateTime exip_dt = /* from parser callback */;

    // Convert EXIP EXIPDateTime -> local DateTime
    person->birthDate.dateTime = exip_dt.dateTime;
    person->birthDate.fSecs = exip_dt.fSecs;
    person->birthDate.TimeZone = exip_dt.TimeZone;
    person->birthDate.presenceMask = exip_dt.presenceMask;
}
```

**Benefits:**
- User code has **zero EXIP dependencies** - only needs generated .h file
- User doesn't `#include` any EXIP headers
- Binding generator's .c file includes EXIP headers for encode/decode implementation
- Type is identical to `EXIPDateTime`, just locally defined
- Clean separation: user structs vs internal conversion layer

**Higher precision needs:**
If clients need platform-specific high-precision types (`struct timespec` on Unix, `FILETIME` on Windows, `std::chrono` in C++), provide optional conversion helpers but keep binding structs portable with the local `DateTime` type.

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

// Generated structures - clean C types, no EXIP leakage
// String strategy:
//   - char[N+1] when schema defines maxLength=N (efficient, no malloc)
//   - char* when no maxLength constraint (unbounded, malloc'd)
//
// Code generator warnings emitted for:
//   - WARNING: Type 'Address' contains unbounded string 'street', consider adding maxLength
//   - WARNING: Type 'Address' contains unbounded string 'city', consider adding maxLength
//   - WARNING: Type 'Person' embeds 'Address' which contains unbounded strings

struct Address {
    char* street;      // No maxLength in schema -> char* (malloc'd, warning)
    char* city;        // No maxLength -> char* (warning)
    int zipCode;
};

struct Person {
    int id;
    char* name;        // No maxLength -> char* (warning)
    int age;
    bool active;
    Address address;   // Embedded - allowed but warned

    // Dynamic array for unbounded maxOccurs
    char** tags;
    unsigned int tagsCount;

    // Optional attribute
    char* version;     // Warning
};

// Example with constrained strings (preferred for efficiency, no warnings):
// <xs:simpleType name="CountryCode">
//   <xs:restriction base="xs:string">
//     <xs:maxLength value="2"/>
//   </xs:restriction>
// </xs:simpleType>
// Generates: char countryCode[3];  // maxLength+1 for null terminator

// Encode functions (C struct -> EXI)
errorCode encode(EXIStream* stream, const Address* address);
errorCode encode(EXIStream* stream, const Person* person);

// Decode functions (EXI -> C struct)
errorCode decode(Parser* parser, Address* address);
errorCode decode(Parser* parser, Person* person);

// Helper functions
void init(Person* person);    // Calls init() for nested address
void destroy(Person* person); // Free allocated resources
void init(Address* address);

// Full document encode/decode
errorCode encode_person_document(const char* filename, const Person* person,
                                  EXIPSchema* schema);  // NULL for schemaless
errorCode decode_person_document(const char* filename, Person* person,
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

void init(Person* person) {
    memset(person, 0, sizeof(Person));
    person->tags.items = NULL;
    person->tags.count = 0;
    person->tags.capacity = 0;
    init(&person->address);  // Complex types auto-call child init()
}

void init(Address* address) {
    memset(address, 0, sizeof(Address));
}

void destroy(Person* person) {
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
// Encode functions (C struct -> EXI)
//==============================================================================

errorCode encode(EXIStream* stream, const Address* address) {
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
    asciiToString(address->street, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));
    TRY(serialize.endElement(stream));

    // <city>
    qname.localName = &ELEM_CITY;
    TRY(serialize.startElement(stream, qname, &valueType));
    asciiToString(address->city, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));
    TRY(serialize.endElement(stream));

    // <zipCode>
    qname.localName = &ELEM_ZIPCODE;
    TRY(serialize.startElement(stream, qname, &valueType));
    TRY(encodeInt(stream, address->zipCode));  // Auto-switches schema/schemaless
    TRY(serialize.endElement(stream));

    // </address>
    TRY(serialize.endElement(stream));

    return EXIP_OK;
}

errorCode encode(EXIStream* stream, const Person* person) {
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
    asciiToString(person->version, &strVal, &stream->memList, false);
    TRY(serialize.stringData(stream, strVal));

    // <id>
    qname.uri = &NS_PERSON;
    qname.localName = &ELEM_ID;
    TRY(serialize.startElement(stream, qname, &valueType));
    TRY(encodeInt(stream, person->id));
    TRY(serialize.endElement(stream));

    // <name>
    qname.localName = &ELEM_NAME;
    TRY(serialize.startElement(stream, qname, &valueType));
    TRY(encodeString(stream, person->name));
    TRY(serialize.endElement(stream));

    // <age>
    qname.localName = &ELEM_AGE;
    TRY(serialize.startElement(stream, qname, &valueType));
    TRY(encodeInt(stream, person->age));
    TRY(serialize.endElement(stream));

    // <active>
    qname.localName = &ELEM_ACTIVE;
    TRY(serialize.startElement(stream, qname, &valueType));
    TRY(encodeBool(stream, person->active));
    TRY(serialize.endElement(stream));

    // <address> (nested - auto-call child encode)
    TRY(encode(stream, &person->address));

    // <tags> (unbounded - array)
    for(size_t i = 0; i < person->tags.count; i++) {
        qname.localName = &ELEM_TAGS;
        TRY(serialize.startElement(stream, qname, &valueType));
        asciiToString(person->tags.items[i], &strVal, &stream->memList, false);
        TRY(serialize.stringData(stream, strVal));
        TRY(serialize.endElement(stream));
    }

    // </person>
    TRY(serialize.endElement(stream));

    return EXIP_OK;
}

//==============================================================================
// Decode functions (EXI -> C struct)
//==============================================================================

// Decode context for Address
typedef struct {
    Address* target;
    const char* currentElement;
} DecodeContext_Address;

static errorCode decode_Address_startElement(QName qname, void* app_data) {
    DecodeContext_Address* ctx = (DecodeContext_Address*)app_data;
    ctx->currentElement = qname.localName->str;
    return EXIP_OK;
}

static errorCode decode_Address_stringData(const String str_val, void* app_data) {
    DecodeContext_Address* ctx = (DecodeContext_Address*)app_data;

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

static errorCode decode_Address_intData(Integer int_val, void* app_data) {
    DecodeContext_Address* ctx = (DecodeContext_Address*)app_data;

    if(ctx->currentElement && strcmp(ctx->currentElement, "zipCode") == 0) {
        ctx->target->zipCode = (int)int_val;
    }

    return EXIP_OK;
}

errorCode decode(Parser* parser, Address* address) {
    DecodeContext_Address ctx;
    errorCode err;

    ctx.target = address;
    ctx.currentElement = NULL;

    // Set up temporary handlers for this sub-element
    ContentHandler savedHandler = parser->handler;

    parser->handler.startElement = decode_Address_startElement;
    parser->handler.stringData = decode_Address_stringData;
    parser->handler.intData = decode_Address_intData;
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

// Similar implementation for decode(Parser*, Person*)...
// Complex types auto-call child decode() for nested structures

//==============================================================================
// High-level document functions
//==============================================================================

errorCode encode_person_document(const char* filename, const Person* person,
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

    // Encode document
    err = serialize.exiHeader(&stream);
    if(err != EXIP_OK) goto cleanup;

    err = serialize.startDocument(&stream);
    if(err != EXIP_OK) goto cleanup;

    err = encode(&stream, person);
    if(err != EXIP_OK) goto cleanup;

    err = serialize.endDocument(&stream);

cleanup:
    serialize.closeEXIStream(&stream);
    fclose(outfile);
    return err;
}

errorCode decode_person_document(const char* filename, Person* person,
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

    // Decode document
    err = decode(&parser, person);

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
    init(&person);

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

    // Encode to EXI (schema mode)
    EXIPSchema schema;
    loadSchema("person.xsd.exi", &schema);
    err = encode_person_document("person.exi", &person, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Encode error: %d\n", err);
        return 1;
    }

    // Decode from EXI
    Person person2;
    init(&person2);
    err = decode_person_document("person.exi", &person2, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Decode error: %d\n", err);
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
    destroy(&person);
    destroy(&person2);
    destroySchema(&schema);

    return 0;
}
```

## Implementation Plan

### Phase 1: Schema Parser (Reuse Existing!)
- **Reuse EXIP's existing TreeTable parser** from `src/grammarGen/`
- Input: EXI-encoded XSD files (same format as grammar generator)
- Use existing functions:
  - `loadSchemaFiles()` - Load EXI-encoded schemas into BinaryBuffer array
  - `generateTreeTable(buffer, ..., &treeTable, NULL)` - Parse to AST (NULL schema = no string table population)
  - `resolveTypeHierarchy()` - Links types and references
  - Walk TreeTable AST to extract type definitions, elements, attributes
- **Key difference from exipg:** exipg calls `convertTreeTablesToEXIPSchema()` to build runtime grammars; exipb stops after TreeTable and generates C code instead
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
- **Also generate:** Static const String declarations for all namespaces, element names, attribute names (see exipe pattern)

### Phase 3: Encode Code Generator (Encoding: struct → EXI)
- Generate `encode(EXIStream*, const Type* type)` - function overloading by parameter type, lowercase param name
- Pattern: Build QNames from static const Strings, call EXIP serialization API
- **Complex types auto-call child `encode()`** for nested structures
- **Use helper functions:** `encodeInt()`, `encodeBool()`, `encodeFloat()`, `encodeString()` that auto-switch between schema-informed/schema-less modes
- **Helpers are reusable:** Generate helpers in utils for use in both generated AND hand-written code
- **Naming:** Use lowercase type name for parameter (e.g., `Person* person`, `Address* address`)
- **Reference:** See `examples/simpleEncoding/encodeTestEXI.c` for the pattern

### Phase 4: Decode Code Generator (Decoding: EXI → struct)
- Generate `decode(Parser*, Type* type)` - function overloading by parameter type
- **Complex types auto-call child `decode()`** for nested structures
- **Schema-informed mode:** Parser calls typed handlers (`intData`, `booleanData`)
- **Schema-less mode:** Parser calls `stringData` handler, parse strings to typed values
- Set up handler callbacks per type, track current element context
- **Naming:** Use lowercase type name for parameter
- **Reference:** Parser handlers in `examples/simpleDecoding/decodeTestEXI.c`
- **Key difference:** Decode is more complex - string tables get populated dynamically by parser during decode, but encode needs compile-time constants

### Phase 5: Build Integration
- CMake/Makefile integration
- `exipb` command-line tool (bindings generator)
- Watch XSD files and regenerate on changes

## Architecture: exipg vs exipb

**exipg (Grammar Generator):**
```
XSD → BinaryBuffer → generateTreeTable() → TreeTable AST → convertTreeTablesToEXIPSchema() → EXIPSchema (runtime grammars)
                                    ↓ populates string tables
                                 EXIPSchema
```
- Purpose: Generate runtime grammars for encoding/decoding
- Output: Binary grammar file used at runtime
- String tables: Built during TreeTable parsing AND during grammar conversion

**exipb (Binding Generator):**
```
XSD → BinaryBuffer → generateTreeTable(..., NULL) → TreeTable AST → Walk AST → Generate C code
                                                          ↓
                                                   Extract types/names
                                                          ↓
                                      Static const Strings + structs + encode/decode functions
```
- Purpose: Generate compile-time C code for type-safe encoding/decoding
- Output: .h and .c files with structs and functions
- String tables: NOT needed - namespaces/names become static const Strings in generated code
- **Key:** Pass NULL for schema parameter to `generateTreeTable()` - we only need the AST, not string table population

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
