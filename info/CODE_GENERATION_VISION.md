# Code Generation from Schema - Vision Document

## Introduction

**The Problem:** Working with EXI streams requires tedious, error-prone manual coding using EXIP's low-level `serialize.*()` and `parse.*()` APIs. Developers must carefully track grammar states, handle type conversions, manage memory, and implement validation by hand. This makes EXI adoption harder than it should be.

**The Solution:** `exipb` (EXI Binding Generator) is a code generator that automatically creates type-safe C structs and encode/decode functions from XSD schemas. Write your data model in XSD once, then use clean, idiomatic C code with compile-time safety and automatic validation.

**Killer Feature: Schema-Enforced Validation.** Unlike Protocol Buffers (no constraints) or hand-written parsers (manual validation), `exipb` bakes XSD constraints directly into generated code. String length limits, numeric ranges, array bounds, and required fields are enforced automatically at encode/decode time—defense-in-depth for embedded systems where malformed input can cause crashes or security vulnerabilities.

**Current Status:** `exipb` is currently a stub/concept. This document describes the target architecture and implementation approach. The EXIP library already provides the foundation (schema parser, EXI encoder/decoder); `exipb` will generate the binding layer on top.

**What Developers Get:**
```c
// Generated from XSD schema automatically
Person person = create_Person("Alice", 30, create_Address("123 Main St", "Boston", 02101));

// Encode to EXI stream (validation automatic)
errorCode err = encode_Person(&stream, &person);

// Decode from EXI stream (validation automatic)
Person received;
err = decode_Person(&parser, &received);
```

Clean, functional API. No manual EXI calls. Type safety enforced by the compiler. Validation enforced by the schema.

---

## Type System: XSD to C Mappings

This section describes how XSD schema constructs map to idiomatic C code patterns.

### Basic Type Mapping

XSD built-in types map to C primitive types with optional compile-time bounds:

**Integers:**
```xml
<xs:element name="age" type="xs:int"/>  <!-- 32-bit signed -->
<xs:element name="count">
  <xs:simpleType>
    <xs:restriction base="xs:int">
      <xs:minInclusive value="0"/>
      <xs:maxInclusive value="100"/>
    </xs:restriction>
  </xs:simpleType>
</xs:element>
```

```c
// Generated C
int32_t age;           // Unrestricted int
int32_t count;         // 0-100 range enforced at validate/encode/decode

// Validation (if EXIP_VALIDATION_MODE >= 2)
if (count < 0 || count > 100) return EXIP_INVALID_EXI_INPUT;
```

**Strings:**
```xml
<xs:element name="name">
  <xs:simpleType>
    <xs:restriction base="xs:string">
      <xs:maxLength value="64"/>
    </xs:restriction>
  </xs:simpleType>
</xs:element>
```

```c
// Generated C - fixed-size array (embedded-friendly)
char name[65];  // maxLength=64 + null terminator

// Validation (if EXIP_VALIDATION_MODE >= 3)
if (strlen(name) > 64) return EXIP_INVALID_EXI_INPUT;
```

**Unbounded strings (no maxLength specified):**
- `exipb` defaults to `char[256]` (255 chars + null terminator)
- Warns during code generation: "No maxLength for element 'name', defaulting to 255"
- Embedded systems should always specify maxLength in schemas for predictable memory usage

**Type mapping table:**
| XSD Type | C Type | Notes |
|----------|--------|-------|
| `xs:int` | `int32_t` | 32-bit signed |
| `xs:long` | `int64_t` | 64-bit signed |
| `xs:short` | `int16_t` | 16-bit signed |
| `xs:byte` | `int8_t` | 8-bit signed |
| `xs:unsignedInt` | `uint32_t` | 32-bit unsigned |
| `xs:boolean` | `bool` (C99) | true/false |
| `xs:float` | `float` | 32-bit IEEE 754 |
| `xs:double` | `double` | 64-bit IEEE 754 |
| `xs:string` (bounded) | `char[N+1]` | Fixed array, N from maxLength |
| `xs:string` (unbounded) | `char[256]` | Default 255+null, warns during codegen |
| `xs:dateTime` | `int64_t` | Unix epoch milliseconds |

### Sequences and Structs

XSD complex types with sequences become C structs:

```xml
<xs:complexType name="Person">
  <xs:sequence>
    <xs:element name="name" type="xs:string"/>
    <xs:element name="age" type="xs:int"/>
  </xs:sequence>
</xs:complexType>
```

```c
// Generated C struct
typedef struct {
    char name[256];  // Default maxLength=255 if not specified in schema
    int32_t age;
} Person;

// Generated constructor
Person create_Person(const char* name, int32_t age) {
    Person p = {0};
    strncpy(p.name, name, sizeof(p.name) - 1);
    p.age = age;
    return p;
}
```

**Nested sequences** (structs containing structs):
```xml
<xs:complexType name="Employee">
  <xs:sequence>
    <xs:element name="id" type="xs:int"/>
    <xs:element name="person" type="Person"/>
  </xs:sequence>
</xs:complexType>
```

```c
typedef struct {
    int32_t id;
    Person person;  // Nested struct by value
} Employee;

// Constructor composes nested types
Employee create_Employee(int32_t id, Person person) {
    Employee e = {0};
    e.id = id;
    e.person = person;  // Copy by value
    return e;
}
```

### Optional Fields (minOccurs="0")

**For strings:** Use empty string check (idiomatic C, no wrapper needed):

```xml
<xs:element name="email" type="xs:string" minOccurs="0"/>
```

```c
typedef struct {
    char email[256];  // Empty string (email[0] == '\0') means not present
    char name[64];    // Required field
} Contact;

// Usage - idiomatic C null check
if (contact.email[0] != '\0') {
    printf("Email: %s\n", contact.email);
}
```

**For non-strings:** Generate wrapper structs with `.exists` and `.value` fields:

```xml
<xs:element name="age" type="xs:int" minOccurs="0"/>
<xs:element name="score" type="xs:int" minOccurs="0"/>
```

```c
// Generated wrapper types (reused across schema)
typedef struct {
    int32_t value;
    bool exists;
} OptionalInt32;

typedef struct {
    float value;
    bool exists;
} OptionalFloat;

// Person struct uses wrappers for optional fields
typedef struct {
    char name[64];        // Required - plain field
    OptionalInt32 age;    // Optional - wrapped
    OptionalInt32 score;  // Optional - wrapped
} Person;

// Constructor helpers
Person create_Person_with_age(const char* name, int32_t age) {
    Person p = {0};
    strncpy(p.name, name, sizeof(p.name) - 1);
    p.age.value = age;
    p.age.exists = true;
    p.score.exists = false;  // Not provided
    return p;
}

// Usage - uniform pattern for all optional fields
if (person.age.exists) {
    printf("Age: %d\n", person.age.value);
}

if (person.score.exists) {
    printf("Score: %d\n", person.score.value);
}
```

**Benefits:**
- ✅ **Uniform access**: Always `field.exists` and `field.value`
- ✅ **Type-safe**: Compiler prevents using value without checking exists
- ✅ **Self-documenting**: Type signature shows optionality
- ✅ **Familiar**: Similar to Rust's `Option<T>` or Haskell's `Maybe`

**Note:** Strings use implicit presence (empty string) to avoid the extra indirection and keep string handling idiomatic.

**Optional constructor helpers** (generated for cleaner function calls):

```c
// Generated helper macros for constructing Optional values
#define SOME(T, val) ((Optional##T){.exists = true, .value = (val)})
#define NONE(T) ((Optional##T){.exists = false})

// Or as inline functions (type-safe alternative to macros)
static inline OptionalInt32 some_int32(int32_t value) {
    return (OptionalInt32){.exists = true, .value = value};
}

static inline OptionalInt32 none_int32(void) {
    return (OptionalInt32){.exists = false};
}

// Constructor using Optional parameters (cleaner than pointer-or-NULL)
Person create_Person_full(const char* name, OptionalInt32 age, OptionalInt32 score) {
    Person p = {0};
    strncpy(p.name, name, sizeof(p.name) - 1);
    p.age = age;
    p.score = score;
    return p;
}

// Usage - explicit and readable
Person p1 = create_Person_full("Alice", SOME(Int32, 30), NONE(Int32));
Person p2 = create_Person_full("Bob", some_int32(25), some_int32(95));

// Compare to old pointer-or-NULL pattern (awkward):
// Person p = create_person("Alice", &age, NULL);  // Address-of, NULL magic
```

### Arrays (maxOccurs > 1)

Repeated elements become arrays with count fields:

```xml
<xs:element name="tags" type="xs:string" minOccurs="0" maxOccurs="10"/>
```

```c
// Fixed-size array (when maxOccurs is bounded)
typedef struct {
    char tags[10][64];  // Array of strings
    size_t tagsCount;   // How many are valid
} TaggedItem;
```

```xml
<xs:element name="values" type="xs:int" maxOccurs="unbounded"/>
```

```c
// Dynamic array (when maxOccurs="unbounded")
typedef struct {
    int32_t* values;
    size_t valuesCount;
    size_t valuesCapacity;
} DataSet;

// Requires cleanup function
void destroy_DataSet(DataSet* ds) {
    free(ds->values);
    ds->values = NULL;
    ds->valuesCount = 0;
    ds->valuesCapacity = 0;
}
```

### XSD Choice → Pattern Matching Without Duplication

**Schema:**
```xml
<xs:complexType name="Content">
  <xs:choice>
    <xs:element name="text" type="xs:string"/>
    <xs:element name="image" type="ImageType"/>
    <xs:element name="video" type="VideoType"/>
  </xs:choice>
</xs:complexType>
```

**Generated C:**
```c
typedef enum {
    CONTENT_NONE,
    CONTENT_TEXT,
    CONTENT_IMAGE,
    CONTENT_VIDEO
} ContentType;

typedef struct {
    ContentType type;
    union {              // C11 anonymous union
        char text[256];
        ImageType image;
        VideoType video;
    };
} Content;
```

**Processing - One Pattern Match, Zero Duplication:**
```c
switch(message.content.type) {
    case CONTENT_TEXT:
        process_text(message.content.text);
        break;
    case CONTENT_IMAGE:
        process_image(&message.content.image);
        break;
    case CONTENT_VIDEO:
        process_video(&message.content.video);
        break;
}
```

### XSD Extension → Type-Safe Polymorphism with Discriminated Unions

C doesn't have true inheritance, but we can achieve **type-safe polymorphism** through **discriminated unions (tagged unions)**. This is the most **idiomatic C approach** for handling type hierarchies, used extensively in compilers, parsers, network protocols, and system software.

**Why discriminated unions are the standard C pattern:**
- ✅ **Explicit and clear** - type tag makes runtime type visible
- ✅ **Type-safe** - compiler warns on missing switch cases
- ✅ **No pointer casting** - no unsafe casts or assumptions about memory layout
- ✅ **Single allocation** - entire object fits on stack
- ✅ **Common in production C** - compilers (AST nodes), parsers (tokens), protocols (packets)
- ✅ **Embedded-friendly** - fixed size, no hidden overhead
- ✅ **Exhaustive checking** - compiler enforces handling all variants

#### Pattern 1: Concrete Extension (Non-Abstract Base)

**Schema:**
```xml
<xs:complexType name="Person">
  <xs:sequence>
    <xs:element name="name" type="xs:string"/>
    <xs:element name="age" type="xs:int"/>
  </xs:sequence>
</xs:complexType>

<xs:complexType name="Employee">
  <xs:complexContent>
    <xs:extension base="Person">
      <xs:sequence>
        <xs:element name="employeeId" type="xs:int"/>
      </xs:sequence>
    </xs:extension>
  </xs:complexContent>
</xs:complexType>
```

**Generated C (Discriminated Union):**
```c
// Discriminator enum - explicit runtime type
typedef enum {
    PERSON_TYPE_BASE,      // Just Person
    PERSON_TYPE_EMPLOYEE   // Employee extension
} PersonType;

// Generated struct with discriminator + union
typedef struct {
    PersonType type;

    // Base fields (common to all variants)
    char name[256];
    int age;

    // Extended fields (variant-specific)
    union {
        struct {
            int employeeId;
        } employee;
        // Future extensions go here
    };
} Person;

// Type-safe constructors
Person create_person(const char* name, int age) {
    Person p = {0};
    p.type = PERSON_TYPE_BASE;
    strncpy(p.name, name, 255);
    p.age = age;
    return p;
}

Person create_employee(const char* name, int age, int id) {
    Person p = {0};
    p.type = PERSON_TYPE_EMPLOYEE;
    strncpy(p.name, name, 255);
    p.age = age;
    p.employee.employeeId = id;
    return p;
}
```

**Type-safe usage (compiler enforces checking):**
```c
void print_person(Person* p) {
    // Common fields available on all variants
    printf("Name: %s, Age: %d\n", p->name, p->age);

    // Type-specific fields require explicit check
    switch (p->type) {
        case PERSON_TYPE_BASE:
            // Just a person, no extra fields
            break;
        case PERSON_TYPE_EMPLOYEE:
            printf("Employee ID: %d\n", p->employee.employeeId);
            break;
    }
}

// Usage
Person person = create_person("Bob", 25);
Person employee = create_employee("Alice", 30, 12345);

print_person(&person);
print_person(&employee);
```

**Benefits over embedded base approach:**
- ✅ **No pointer casting** - access fields directly, no `(Person*)` casts
- ✅ **Explicit type checking** - must check discriminator before accessing variant fields
- ✅ **Single struct** - simpler API, one type name instead of Base/Derived
- ✅ **Compiler warnings** - missing switch cases caught at compile time
- ✅ **Standard C idiom** - immediately recognizable pattern

**Embedded-friendly:** Fixed layout, single allocation, stack-allocatable.

#### Pattern 2: Abstract Base Type (Polymorphic Element)

**Schema:**
```xml
<xs:complexType name="Shape" abstract="true">
  <xs:sequence>
    <xs:element name="color" type="xs:string"/>
    <xs:element name="id" type="xs:int"/>
  </xs:sequence>
</xs:complexType>

<xs:complexType name="Circle">
  <xs:complexContent>
    <xs:extension base="Shape">
      <xs:sequence>
        <xs:element name="radius" type="xs:double"/>
      </xs:sequence>
    </xs:extension>
  </xs:complexContent>
</xs:complexType>

<xs:complexType name="Rectangle">
  <xs:complexContent>
    <xs:extension base="Shape">
      <xs:sequence>
        <xs:element name="width" type="xs:double"/>
        <xs:element name="height" type="xs:double"/>
      </xs:sequence>
    </xs:extension>
  </xs:complexContent>
</xs:complexType>

<xs:element name="shape" type="Shape"/>  <!-- Polymorphic -->
```

**Generated C (Discriminated Union - Idiomatic):**
```c
// Discriminator enum for runtime type identification
typedef enum {
    SHAPE_TYPE_CIRCLE,
    SHAPE_TYPE_RECTANGLE
} ShapeType;

// Single struct with discriminator + union
typedef struct {
    ShapeType type;      // Runtime type tag

    // Base fields (common to all shape variants)
    char color[64];
    int id;

    // Variant-specific fields in union
    union {
        struct {
            double radius;
        } circle;

        struct {
            double width;
            double height;
        } rectangle;
    };
} Shape;
```

**Type-safe constructors (impossible to create invalid state):**
```c
Shape create_circle(const char* color, int id, double radius) {
    Shape shape = {0};
    shape.type = SHAPE_TYPE_CIRCLE;
    strncpy(shape.color, color, 63);
    shape.id = id;
    shape.circle.radius = radius;
    return shape;
}

Shape create_rectangle(const char* color, int id, double w, double h) {
    Shape shape = {0};
    shape.type = SHAPE_TYPE_RECTANGLE;
    strncpy(shape.color, color, 63);
    shape.id = id;
    shape.rectangle.width = w;
    shape.rectangle.height = h;
    return shape;
}
```

**Pattern matching (type-specific processing):**
```c
void process_shape(Shape* shape) {
    // Access common fields directly - no casting needed
    printf("Shape #%d, color: %s\n", shape->id, shape->color);

    // Type-specific logic with explicit discriminator check
    switch (shape->type) {
        case SHAPE_TYPE_CIRCLE:
            printf("  Circle with radius %.2f\n", shape->circle.radius);
            double area = 3.14159 * shape->circle.radius * shape->circle.radius;
            printf("  Area: %.2f\n", area);
            break;

        case SHAPE_TYPE_RECTANGLE:
            printf("  Rectangle %.2f x %.2f\n",
                   shape->rectangle.width,
                   shape->rectangle.height);
            double area = shape->rectangle.width * shape->rectangle.height;
            printf("  Area: %.2f\n", area);
            break;
    }
}

// Compiler warns if a case is missing - exhaustive checking!
```

**Usage example:**
```c
// Create shapes
Shape circle = create_circle("red", 1, 5.0);
Shape rect = create_rectangle("blue", 2, 10.0, 20.0);

// Process them
process_shape(&circle);
process_shape(&rect);

// Array of polymorphic shapes - single type!
Shape shapes[] = {
    create_circle("green", 3, 3.0),
    create_rectangle("yellow", 4, 8.0, 6.0)
};

for (size_t i = 0; i < 2; i++) {
    process_shape(&shapes[i]);
}
```

**Type safety guarantees:**
- ✅ **Explicit type tag** - `shape->type` makes runtime type visible and required
- ✅ **Exhaustive switch checking** - compiler warns if case is missing (-Wswitch)
- ✅ **No unsafe casts** - access fields directly without pointer casting
- ✅ **Single struct type** - simpler API, no base/derived type juggling
- ✅ **Compiler-enforced checking** - must check discriminator to access variant fields
- ✅ **No vtables, no function pointers** - zero runtime overhead
- ✅ **Familiar pattern** - used in compilers, parsers, protocols throughout C codebases

**Embedded-friendly aspects:**
- ✅ **No malloc required** - stack or static allocation
- ✅ **Fixed size known at compile time** - `sizeof(Shape)` is constant
- ✅ **Single allocation** - no separate allocations for base/derived
- ✅ **Cache-friendly** - contiguous memory layout
- ✅ **Zero runtime dispatch overhead** - direct field access
- ✅ **Works in bare-metal environments** - no hidden dependencies

**Real-world usage examples:**

This pattern appears throughout production C code:

```c
// Compiler AST nodes
typedef enum { NODE_IF, NODE_WHILE, NODE_CALL } NodeType;
typedef struct {
    NodeType type;
    union {
        struct { Expr* cond; Stmt* body; } if_stmt;
        struct { Expr* cond; Stmt* body; } while_stmt;
        struct { char* func; Expr** args; } call;
    };
} ASTNode;

// Network protocol messages
typedef enum { MSG_CONNECT, MSG_DATA, MSG_CLOSE } MsgType;
typedef struct {
    MsgType type;
    union {
        struct { char* hostname; uint16_t port; } connect;
        struct { uint8_t* bytes; size_t len; } data;
        struct { uint32_t reason; } close;
    };
} Message;
```

**Why discriminated unions beat embedded base:**

| Aspect | Discriminated Union | Embedded Base |
|--------|-------------------|--------------|
| **Type checking** | Explicit tag, compiler enforces | Implicit via pointer cast, runtime only |
| **Field access** | Direct: `shape->color` | Via cast or `.base`: `circle.base.color` |
| **Pointer casting** | Not needed | Required for polymorphism |
| **API simplicity** | Single type name | Multiple types (Base, Derived1, Derived2...) |
| **Switch exhaustiveness** | Compiler warns missing cases | N/A |
| **Common in C** | Very common (compilers, parsers) | Less common (GTK, kernel) |
| **Learning curve** | Immediately clear | Requires understanding memory layout |

**TreeTable structure:**
```
[complexType] name="Shape" abstract="true"
  [sequence]
    [element] name="color" type="string"
    [element] name="id" type="int"

[complexType] name="Circle"
  [extension] base="Shape"
    supertype.entry → points to Shape ← KEY!
    [sequence]
      [element] name="radius" type="double"

[complexType] name="Rectangle"
  [extension] base="Shape"
    supertype.entry → points to Shape ← KEY!
    [sequence]
      [element] name="width" type="double"
      [element] name="height" type="double"

[element] name="shape" type="Shape"
  (Links to abstract Shape - code gen must find all subtypes)
```

**Code generation strategy:**
1. Walk TreeTable, find all types with `abstract="true"`
2. Find all types with `extension base="AbstractType"`
3. Generate discriminator enum from subtype names
4. Generate single struct with:
5. Generate wrapper struct with discriminator + union for polymorphic elements
6. Generate type-safe constructor per subtype
7. Generate helper for upcast to base type

**XSD's inheritance + EXI's grammar + generated C pattern = type-safe polymorphism without runtime overhead.**

### Functional Composition

The constructor pattern enables **functional composition** - build complex structures from simple parts:

```c
// Polymorphic composition
ShapeElement circle = create_circle("red", 1, 5.0);
ShapeElement rect = create_rectangle("blue", 2, 10.0, 20.0);

// Array of polymorphic shapes
ShapeElement shapes[] = {circle, rect};
for (size_t i = 0; i < 2; i++) {
    process_shape(&shapes[i]);
}

// Bottom-up composition with nested types
DateTime timestamp = create_timestamp(2026, 6, 3, 14, 30, 0);
LoginEvent login = create_login_event(timestamp, 1, "user@example.com", "192.168.1.1");
Event event = create_event(EVENT_LOGIN, login);

// Or all at once
Event event = create_event(
    EVENT_LOGIN,
    create_login_event(
        create_timestamp(2026, 6, 3, 14, 30, 0),
        1,
        "user@example.com",
        "192.168.1.1"
    )
);
```

**Benefits:**
- Pure functions - no hidden state
- Testable - construct test data easily
- Reusable - call constructors with different parameters
- Transparent - all data visible in struct
- Type-safe - compiler catches everything

Compare to JAXB (Java):
```java
ObjectFactory factory = new ObjectFactory();
JAXBElement<Event> element = factory.createEvent();
Event event = element.getValue();
event.setTimestamp(/* XMLGregorianCalendar confusion */);
// ... endless setter calls, opaque structure, can't see the message shape
```

### The Vision

Combine:
1. **XSD's rich type system** - for proper domain modeling
2. **EXI's binary efficiency** - for compact wire format
3. **Functional C API** - for composable, type-safe code
4. **Pattern matching** - for duplication-free processing

Result: Clean, type-safe C code for domains that need:
- Rich schemas (finance, healthcare, legal, aerospace)
- Type hierarchies and polymorphism
- Validation and constraints
- Both binary efficiency AND human-readable XML fallback

The composable constructor pattern + XSD's type system creates idiomatic, functional C code.

## Generated Binding API Architecture

### High-Level Design

The binding generator (`exipb`) produces type-safe encoding/decoding functions that work with EXI streams:

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│                                                                 │
│  User Code:  Person person = create_Person(...);                │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────▼─────────┐
                    │ encode_Person()  │  User API (generated)
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │  bind_Person()   │  Binding layer (generated)
                    │   + validate     │  (may inline later)
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │ serialize.*()    │  EXIP core API
                    └────────┬─────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                    EXI Stream / Buffer                          │
│                                                                 │
│             BinaryBuffer with EXI encoded data                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────▼─────────┐
                    │    Transport     │  MQTT, TCP, HTTP, CAN,
                    │   (any method)   │  file, shared memory...
                    └────────┬─────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                    EXI Stream / Buffer                          │
│                                                                 │
│             BinaryBuffer with EXI encoded data                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                    ┌────────▼─────────┐
                    │   parse.*()      │  EXIP core API
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │ unbind_Person()  │  Binding layer (generated)
                    │   + validate     │  (may inline later)
                    └────────┬─────────┘
                             │
                    ┌────────▼─────────┐
                    │ decode_Person()  │  User API (generated)
                    └────────┬─────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                      Application Layer                          │
│                                                                 │
│  User Code:  printf("Name: %s, Age: %d", person.name, ...);     │
└─────────────────────────────────────────────────────────────────┘
```

### Generated Function Signatures

**User-facing API:**
```c
// Validation - user can call directly for pre-flight checks
errorCode validate_Person(const Person* person);

// Encoding - user calls after composing struct with create_*()
errorCode encode_Person(EXIStream* strm, const Person* person);

// Decoding - user calls to parse stream into struct
errorCode decode_Person(Parser* parser, Person* person);
```

**Internal implementation (not exposed to user):**
```c
// Validation layer - can be ifdef'd out with EXIP_VALIDATION_MODE
// Recursive: validates nested types (Address, etc.)
static errorCode validate_Person(const Person* person);

// Binding layer - validates then encodes to EXI stream
// Calls validate_Person(), then serialize.* functions
// May be inlined into encode_Person() in future (kept separate for now)
static errorCode bind_Person(EXIStream* strm, const Person* person);

// Unbinding layer - decodes from EXI stream into struct
// Calls parse.* functions, populates struct fields directly
// For nested types, calls their unbind_*() recursively
static errorCode unbind_Person(Parser* parser, Person* person);
```

### Design Principles

**1. Three-layer architecture**
- **User API**: `create_*()`, `validate_*()`, `encode_*()`, `decode_*()` - clean interface
- **Binding layer**: `bind_*()`, `unbind_*()` - validation + EXI stream operations (may be simplified later)
- **EXIP core**: `serialize.*()`, `parse.*()` - existing low-level API

**2. User owns memory**
- Structs allocated by user (stack or heap)
- Streams/buffers provided by user
- Suitable for embedded systems

**3. Type safety**
- Compiler enforces correct struct usage
- Constructor functions (`create_*`) ensure invariants
- Generated code handles EXIP type conversions

**4. Transport-agnostic**
- User controls stream setup and transport
- Generated encode/decode work with any EXI stream source
- MQTT, TCP, UDP, HTTP, WebSocket, serial, CAN bus, file, shared memory
- Same generated code works with any transport

### Integration Example

```c
// Create struct using constructor (bottom-up composition)
Address addr = create_Address("123 Main St", "Springfield", 12345);
Person person = create_Person(1, "Alice", 30, true, addr);

// User sets up EXI stream and buffer
BinaryBuffer buffer;
EXIStream strm;
char exiBuf[1024];
buffer.buf = exiBuf;
buffer.bufLen = sizeof(exiBuf);
buffer.bufContent = 0;

EXIOptions opts;
// Initialize stream for encoding
serialize.initStream(&strm, buffer, NULL);  // NULL for schemaless, or pass schema

// Encode struct to stream (user API)
errorCode err = encode_Person(&strm, &person);
if (err != EXIP_OK) {
    fprintf(stderr, "Encode error: %d\n", err);
    return;
}

// Close stream and get encoded data
serialize.closeEXIStream(&strm);
size_t exi_len = buffer.bufContent;

// Send via ANY transport - user controls this
mqtt_publish("sensors/person", exiBuf, exi_len);     // MQTT
send(sockfd, exiBuf, exi_len, 0);                    // TCP
can_send(0x123, exiBuf, exi_len);                    // CAN bus
fwrite(exiBuf, 1, exi_len, file);                    // File

// ---- Receiving side ----

// Receive from transport - user controls this
uint8_t recvBuf[1024];
size_t recv_len = mqtt_receive("sensors/person", recvBuf, sizeof(recvBuf));

// Set up parser with received buffer
BinaryBuffer recvBuffer;
recvBuffer.buf = recvBuf;
recvBuffer.bufLen = recv_len;
recvBuffer.bufContent = recv_len;

Parser parser;
parse.initParser(&parser, recvBuffer, NULL);

// Parse header
parse.parseHeader(&parser, false);

// Decode from stream (user API)
Person person2;
err = decode_Person(&parser, &person2);
if (err != EXIP_OK) {
    fprintf(stderr, "Decode error: %d\n", err);
    return;
}

// Use decoded data
printf("Received: %s, age %d\n", person2.name, person2.age);

// Cleanup
parse.destroyParser(&parser);
```

### Internal Implementation

Generated functions internally use:
- **Three-layer architecture** - User API → Binding layer → EXIP core
- **bindapi.h helpers** - `bindInt()`, `bindFloat()`, `bindString()`, etc.
- **Mode awareness** - Automatically handles schema vs schemaless encoding
- **Memory model** - Caller-provided buffers, no dynamic allocation
- **Validation** - Optional compile-time controlled via `EXIPB_VALIDATION_STRICT`

### Schema as Field Mapping Metadata

**Critical insight:** The schema parameter isn't just for validation - it's the **field mapping table** that guides both encoding modes.

#### Schema Mode (Direct Type Mapping)
```c
// Schema tells EXIP: "next is 'age' element, type xs:int"
// Binding knows: "'age' → person.age field (int type)"
static errorCode unbind_Person(Parser* parser, Person* person) {
    // Schema-informed parse: EXIP calls intData handler
    // Binding knows which field based on schema grammar state
    person->age = received_int;  // Direct assignment
}
```

#### Schemaless Mode (String Conversion)
```c
// No schema grammar, everything is strings
// Element name: "age", value: "30" (as string)
static errorCode unbind_Person(Parser* parser, Person* person) {
    // Schema METADATA tells binding: field "age" is xs:int type
    // Binding converts: string "30" → int 30

    if (strcmp(element_name, "age") == 0) {
        // Schema says this field is int, so convert from string
        person->age = atoi(string_value);  // String → int conversion
    }
}
```

#### Schema Provides Field Mapping Table

The schema gives the binding essential metadata:

| Field Name | C Type | Schema Type | Struct Offset | Conversion Needed |
|------------|--------|-------------|---------------|-------------------|
| "name"     | char[] | xs:string   | 0             | No (string→string) |
| "age"      | int    | xs:int      | 65            | Yes (string→int in schemaless) |
| "active"   | bool   | xs:boolean  | 69            | Yes (string→bool in schemaless) |
| "score"    | float  | xs:float    | 70            | Yes (string→float in schemaless) |

**The binding uses schema for:**
1. **Field discovery** - What fields exist, in what order
2. **Type mapping** - C type vs XSD type
3. **Conversion routing** - Schema mode (direct) vs Schemaless mode (parse string)
4. **Struct layout** - Where to put data (field offsets)
5. **Grammar enforcement** - Element order, occurrence constraints (via EXIP)

**This is why bindings are schema-source agnostic but schema-dependent:**
- Works with static compiled schemas (`extern const EXIPSchema`)
- Works with runtime loaded schemas (from `.xsd.exi` file)
- But MUST have schema metadata to know field mapping and types
- Schema reuse eliminates duplicating grammar logic in generated code

## Code Generation Pipeline (Simple Steps)

1. **Load schemas** - Load EXI-encoded XSD files into `BinaryBuffer[]` using `loadSchemaFiles()`
2. **Parse and optimize TreeTable AST** - Call `generateOptimizedTreeTable(buffers, ..., &treeT, &treeTCount, &subsTbl, NULL, NULL)` - NULL for schema and loadSchemaHandler means no string table population, just build and link AST
3. **Walk TreeTable** - Extract type definitions, element names, attribute names, namespaces from linked AST
4. **Generate C constants** - Emit static const String declarations for all namespaces and element/attribute names (like exipe example)
5. **Generate C code** - Emit structs, `encode()`/`decode()` functions using reusable type-conversion helpers

**Key:** `generateOptimizedTreeTable()` does all TreeTable generation, include/import resolution, and type hierarchy linking in one call. Generated code uses EXIP's existing `serialize.*Data()` API with simple type conversions where needed.


Generated bindings follow EXIP's ownership model: **caller owns pointer data**.

**Design principle:**
- Constructor functions accept pointers and store them directly
- Constructors do NOT malloc/copy pointer data
- Destroy functions do NOT free pointer data passed to constructors
- Caller is responsible for managing lifetime of all pointer data

**Rationale:**
- Matches EXIP's API design (user owns buffers, EXIP owns internal DynArrays)
- Supports embedded/real-time systems that avoid malloc entirely
- Caller controls allocation strategy (stack, static, heap, memory pool)
- No ownership flags needed - simple, clear contract

**Example:**
```c
// Constructor stores pointer, does not copy
TypesTest create_types_test(uint8_t* binaryData, size_t binaryLen, EnumType* greeting) {
    return (TypesTest){
        .binaryTest = binaryData,        // Store pointer directly
        .binaryTestLen = binaryLen,
        .enumTest = greeting ? *greeting : 0
    };
}

// Destroy does not free user's pointer
void destroy_types_test(TypesTest* data) {
    // Only frees memory allocated by the struct itself (if any)
    // Caller must free binaryData if they malloc'd it
}

// Usage - caller controls allocation
uint8_t staticBuffer[] = {0x01, 0x02, 0x03};
TypesTest t1 = create_types_test(staticBuffer, sizeof(staticBuffer), NULL);  // Static
// No free needed - staticBuffer lives until end of scope

uint8_t* heapBuffer = malloc(100);
TypesTest t2 = create_types_test(heapBuffer, 100, NULL);  // Heap
free(heapBuffer);  // Caller frees, not destroy_types_test()
```

This pattern supports:
- **Static allocation**: `uint8_t buffer[100]; create_xxx(buffer, 100)`
- **Stack allocation**: `uint8_t buffer[100]; /* local variable */`
- **Heap allocation**: `uint8_t* buf = malloc(100); /* caller frees */`
- **Memory pools**: Custom allocators for real-time systems

Alternative design (struct owns data) would require malloc in constructors, breaking embedded/real-time use cases.

**Generate `create_XXX()` when a struct has ANY of:**

1. **Optional fields** - Needs to set `bool hasXXX` presence flags
   ```c
   TypesTest create_types_test(int8_t byte, DateTime dt, EnumType* enumVal) {
       return (TypesTest){
           .byteTest = byte,
           .dateTimeTest = dt,
           .enumTest = enumVal ? *enumVal : HELLO,
           .hasEnumTest = (enumVal != NULL)  // Auto-calculate flag
       };
   }
   ```

2. **Pointer + length pairs** - Binary data, arrays, enum lookups

   **Binary data (runtime):**
   ```c
   Message create_message(uint8_t* data, size_t len) {
       return (Message){
           .binaryData = data,
           .binaryDataLen = len,
           .hasBinaryData = (data != NULL && len > 0)
       };
   }
   ```

   **Dynamic arrays (runtime):**
   ```c
   struct Person {
       char** tags;           // Array of strings
       size_t tagsCount;      // How many elements
   };
   ```

   **Enum lookup tables (compile-time):**
   ```c
   // Generated for XSD enumerations - avoids strlen() at runtime
   static const struct {
       const char* str;
       size_t len;
   } enum_type_strings[] = {
       [HELLO] = {"hello", 5},
       [HI]    = {"hi", 2},
       [HEY]   = {"hey", 3},
       [HEJ]   = {"hej", 3}
   };

   // Usage: directly map to EXIP String without strlen()
   errorCode encode_enum(EXIStream* strm, EnumType value) {
       String exipStr = {
           .str = (CharType*)enum_type_strings[value].str,
           .length = enum_type_strings[value].len  // Pre-calculated!
       };
       return serialize.stringData(strm, exipStr);
   }
   ```

   **Summary:**
   | Type | Storage | Why Pointer+Length Needed |
   |------|---------|---------------------------|
   | **Binary data** | `uint8_t* + size_t` | Not null-terminated, can contain `0x00` bytes |
   | **Arrays** | `Type* + size_t count` | Can't determine element count from pointer |
   | **Enum lookup** | `const char* + size_t` (static) | EXIP needs length, avoid `strlen()` overhead |
   | **Strings** | `char*` (usually no length) | Null-terminated, `strlen()` works |

3. **Tagged unions (choice types)** - Ensures discriminator and union field match
   ```c
   // WITHOUT constructor - easy to mismatch
   BasicType bt = {
       .choiceType = BASIC_TYPE_BOOL,  // Oops!
       .choice.intValue = 123          // Set wrong union field
   };

   // WITH constructor - impossible to mismatch
   BasicType create_basic_type_int(int value, int id) {
       return (BasicType){
           .choiceType = BASIC_TYPE_INT,    // Guaranteed correct
           .choice.intValue = value,         // Matches discriminator
           .id = id
       };
   }

   BasicType create_basic_type_bool(bool value, int id) {
       return (BasicType){
           .choiceType = BASIC_TYPE_BOOL,
           .choice.boolValue = value,
           .id = id
       };
   }
   ```

4. **Calculated fields** - Presence masks, checksums, derived values
   ```c
   DateTime create_datetime(int year, int month, int day,
                            int hour, int min, int sec,
                            int16_t* timezone, uint32_t* fSecs) {
       uint8_t mask = 0;
       if (timezone) mask |= TZONE_PRESENCE;
       if (fSecs) mask |= FRACT_PRESENCE;

       return (DateTime){
           .dateTime = {
               .tm_year = year - 1900,
               .tm_mon = month - 1,
               .tm_mday = day,
               .tm_hour = hour,
               .tm_min = min,
               .tm_sec = sec
           },
           .TimeZone = timezone ? *timezone : 0,
           .fSecs = fSecs ? *fSecs : 0,
           .presenceMask = mask  // Auto-calculated
       };
   }
   ```

5. **Multiple nested structs** - More than 5 fields (convenience)
   ```c
   // Constructor simplifies initialization of complex structures
   MultipleXSDsTest create_test_data(
       EXIPEncoder encoder,
       const char* description,
       TestSetup setup,
       BasicType typeTest,
       TypesTest* extendedTypeTest
   );
   ```

**Skip `create_XXX()` for simple structs with:**
- Only primitive fields (int, bool, float)
- No optional elements
- No dynamic data
- ≤ 3-4 fields

Users can initialize these directly with **C99 designated initializers**:

```c
// Simple structs - no constructor needed
TestSetup setup = {
    .content = "test content",
    .goal = "validate encoding"
};

// Tagged unions with NO additional fields - still generate constructors
// Even without the `id` attribute, wrappers ensure discriminator/union match
```

**Key Principle:** Constructors add value when they **enforce invariants** (discriminator matches union field), **calculate derived data** (flags, lengths, masks), or **hide complexity** (many fields, nested initialization). For simple structs, direct initialization is clearer.

#### Tagged Union Constructor Pattern

For XSD `<xs:choice>` types mapped to tagged unions, **always generate per-choice constructors** even if there are no additional attributes. The constructor ensures the discriminator enum and union field are always synchronized.

```c
// Schema: choice without attributes
typedef enum {
    MESSAGE_TYPE_NONE,
    MESSAGE_TYPE_TEXT,
    MESSAGE_TYPE_IMAGE
} MessageTypeChoice;

typedef struct {
    MessageTypeChoice choiceType;
    union {
        char text[256];
        ImageData image;
    } choice;
} MessageType;

// Generate one constructor per choice option
MessageType create_message_type_text(const char* text) {
    MessageType mt = {0};
    mt.choiceType = MESSAGE_TYPE_TEXT;
    strncpy(mt.choice.text, text, 255);
    mt.choice.text[255] = '\0';
    return mt;
}

MessageType create_message_type_image(ImageData image) {
    return (MessageType){
        .choiceType = MESSAGE_TYPE_IMAGE,
        .choice.image = image
    };
}
```

**Why:** Without constructors, users must manually set both `.choiceType` and `.choice.xxx`, risking mismatches that C won't catch at compile time. The constructor makes this impossible.

### Schema vs Schemaless Mode Support

exipb generates code using the **bindapi** which provides automatic mode switching and dual-layer validation.

#### Dual-Layer Architecture

**Layer 1: bindapi (Application/Routing Layer)**
- Location: `include/bindapi.h`, `src/common/src/bindapi.c`
- Responsibility: Checks `strm->schema` upfront and routes to appropriate encoding
- Schema mode: Native C types → EXIP types → `serialize.intData()` etc.
- Schemaless mode: Native C types → string conversion → `serialize.stringData()`

**Layer 2: EXISerializer (Grammar/Validation Layer)**
- Location: `src/contentIO/src/bodyEncode.c`, `EXISerializer.c`
- Responsibility: Validates type matches grammar state (`exiType`)
- Catches mismatches: called `intData()` but grammar expects string
- Strict mode: Returns `EXIP_INVALID_EXI_INPUT` on mismatch (catches bugs)

**Example flow:**
```c
// Generated binding code
serializeIntValue(strm, person->age);

// bindapi layer checks schema:
if (strm->schema != NULL) {
    serialize.intData(strm, (Integer)age);  // → bodyEncode.c
} else {
    // Convert to string, call serialize.stringData()
}

// bodyEncode.c validates:
exiType = GET_EXI_TYPE(...);  // From grammar state
if (exiType == VALUE_TYPE_STRING) {
    // Mismatch! We're encoding int but grammar expects string
    #if EXIP_IMPLICIT_DATA_TYPE_CONVERSION
        // Try to recover (lenient)
    #else
        return EXIP_INVALID_EXI_INPUT;  // Fail fast in strict mode
    #endif
}
```

**Benefits of dual-layer approach:**
1. **Early routing** - bindapi decides schema vs schemaless before grammar state
2. **Late validation** - EXISerializer catches type/grammar mismatches
3. **Strict mode safety** - Detects encoding bugs (grammar state inconsistency)
4. **Clean separation** - Application logic (bindapi) vs encoding logic (serializer)

#### Generation Modes

**Schema-only mode (default, production/embedded):**
```bash
exipb --mode=schema-only schema.xsd
```
Generated code uses bindapi functions: `serializeIntValue()`, `serializeFloatValue()`, etc.
User must compile with schema and run in schema mode.
Smallest code size.

**Dual-mode (development/debugging):**
```bash
exipb --mode=dual schema.xsd
```
Generated code works in both schema and schemaless modes.
bindapi automatically routes based on `strm->schema` presence.
Slightly larger code, maximum flexibility.

**Note on EXIP_IMPLICIT_DATA_TYPE_CONVERSION:**
- This flag enables Layer 2 (serializer-level) fallback conversion
- **bindapi makes it unnecessary** for generated code - routing happens at Layer 1
- Still useful for: `xs:any` wildcards, dynamic content, hand-written serializers
- Currently unimplemented (returns `EXIP_NOT_IMPLEMENTED_YET`)
- See [ARCHITECTURE.md](ARCHITECTURE.md) for details

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

**Current EXIP validation** (in schema-informed mode):
- ✅ Integer min/max bounds (minInclusive/maxInclusive) - for range encoding optimization
- ✅ String length (via fixed array size at compile time in generated code)
- ✅ Grammar state validation - catches type mismatches in strict mode
- ❌ minExclusive/maxExclusive (TODO in EXIP)
- ❌ Pattern facets (logged but not enforced)
- ❌ Enumeration values (used for encoding, not validated)
- ❌ totalDigits/fractionDigits (not validated)

exipb **relies on EXIP's existing validation** - no duplicate checks in generated code.

#### Super Strict Mode (Future Feature)

**Concept**: Optional runtime facet validation during encoding/decoding.

**Configuration**:
```c
// Compile-time flag
#define EXIP_SUPER_STRICT_VALIDATION ON

// Or runtime option
strm->header.opts.superStrictValidation = true;
```

**What it would validate**:
- ✅ All integer bounds (minInclusive, maxInclusive, minExclusive, maxExclusive)
- ✅ String patterns (regex validation via facet.pattern)
- ✅ Enumeration constraints (value in allowed set)
- ✅ Decimal precision (totalDigits, fractionDigits)
- ✅ String length constraints (length, minLength, maxLength)
- ✅ List item count constraints (for xs:list types)

**Use cases**:
- **Development**: Catch data errors early before they become encoded stream corruption
- **Compliance**: Ensure outgoing EXI streams strictly follow schema contracts
- **Security**: Prevent malformed/malicious data from being encoded
- **Testing**: Validate test data generator output

**Trade-offs**:
- **Performance**: Validation overhead on every encode/decode call
- **Code size**: Regex engine, bounds checking, enumeration lookups
- **Memory**: Pattern compilation, enumeration sets

**Implementation approach**:
1. Add validation hooks in `bodyEncode.c` after type routing
2. Check facets from `strm->schema->simpleTypeTable`
3. Return descriptive errors: `EXIP_FACET_VIOLATION` with facet name
4. `#if EXIP_SUPER_STRICT_VALIDATION` guards for compile-time exclusion
5. Runtime opt-in via `EXIOptions` for runtime control

**Example error**:
```c
// Encoding person->age = 150, but schema says maxInclusive=120
errorCode = serializeIntValue(strm, person->age);
// Returns: EXIP_FACET_VIOLATION
// Error detail: "Value 150 exceeds maxInclusive=120 for element 'age'"
```

**Note**: Current strict mode validates grammar state (Layer 2), super strict would add facet validation (data constraints).

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
errorCode encode_person(EXIStream* stream, const Person* person) {
    // Convert local DateTime -> EXIP EXIPDateTime
    EXIPDateTime exip_dt;
    exip_dt.dateTime = person->birthDate.dateTime;
    exip_dt.fSecs = person->birthDate.fSecs;
    exip_dt.TimeZone = person->birthDate.TimeZone;
    exip_dt.presenceMask = person->birthDate.presenceMask;

    // Call EXIP (internally)
    TRY(serialize.dateTimeData(stream, exip_dt));
}

errorCode decode_person(Parser* parser, Person* person) {
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


```c
#include "person.h"
#include <stdio.h>

int main() {
    Person p;
    errorCode err;

    // Initialize (OOP vtable style)
    person.init(&p);

    // Fill in data
    p.id = 12345;
    strcpy(p.name, "John Doe");
    p.age = 30;
    p.active = true;
    strcpy(p.version, "1.0");

    strcpy(p.address.street, "123 Main St");
    strcpy(p.address.city, "Springfield");
    p.address.zipCode = 12345;

    // Add tags (dynamic array)
    p.tags.count = 2;
    p.tags.capacity = 2;
    p.tags.items = malloc(2 * sizeof(char*));
    p.tags.items[0] = strdup("employee");
    p.tags.items[1] = strdup("manager");

    // Encode to EXI using vtable (schema mode)
    EXIPSchema schema;
    loadSchema("person.xsd.exi", &schema);
    err = person.encodeDocument("person.exi", &p, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Encode error: %d\n", err);
        return 1;
    }

    // Decode from EXI using vtable
    Person p2;
    person.init(&p2);
    err = person.decodeDocument("person.exi", &p2, &schema);
    if(err != EXIP_OK) {
        fprintf(stderr, "Decode error: %d\n", err);
        return 1;
    }

    // Verify
    printf("ID: %d\n", p2.id);
    printf("Name: %s\n", p2.name);
    printf("Age: %d\n", p2.age);

    // Cleanup (vtable style)
    person.destroy(&p);
    person.destroy(&p2);
    return 0;
}
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
- Generate `encode_<typename>(EXIStream*, const Type* type)` - function name includes type, lowercase param name
- Pattern: Build QNames from static const Strings, call EXIP serialization API
- **Complex types auto-call child `encode_<typename>()`** for nested structures
- **Use helper functions:** `encodeInt()`, `encodeBool()`, `encodeFloat()`, `encodeString()` that auto-switch between schema-informed/schema-less modes
- **Helpers are reusable:** Generate helpers in utils for use in both generated AND hand-written code
- **Naming:** Use lowercase type name for parameter (e.g., `Person* person`, `Address* address`)
- **Reference:** See `examples/simpleEncoding/encodeTestEXI.c` for the pattern

### Phase 4: Decode Code Generator (Decoding: EXI → struct)
- Generate `decode_<typename>(Parser*, Type* type)` - function name includes type, lowercase param name
- **Complex types auto-call child `decode_<typename>()`** for nested structures
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

Tool name: **`exipb`** (EXI Processor Binding Generator)

```bash
# Generate code from EXI-encoded schema
exipb --input person.xsd.exi --output person.h person.c

# Options
exipb --input person.xsd.exi \
            --output-dir generated/ \
            --prefix myapp_ \
            --namespace myapp \
            --string-max-length 512 \
            --array-initial-capacity 16

# Generate from multiple EXI-encoded schemas
exipb --input schema1.xsd.exi schema2.xsd.exi \
            --output generated/

# Alternative: Specify schema and let tool encode it
exipb --input-xml person.xsd \
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

**Schema:**
```xml
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">

  <!-- Abstract base type -->
  <xs:complexType name="Shape" abstract="true">
    <xs:sequence>
      <xs:element name="color" type="xs:string"/>
    </xs:sequence>
  </xs:complexType>

  <!-- Circle extends Shape -->
  <xs:complexType name="Circle">
    <xs:complexContent>
      <xs:extension base="Shape">
        <xs:sequence>
          <xs:element name="radius" type="xs:double"/>
        </xs:sequence>
      </xs:extension>
    </xs:complexContent>
  </xs:complexType>

  <!-- Rectangle extends Shape -->
  <xs:complexType name="Rectangle">
    <xs:complexContent>
      <xs:extension base="Shape">
        <xs:sequence>
          <xs:element name="width" type="xs:double"/>
          <xs:element name="height" type="xs:double"/>
        </xs:sequence>
      </xs:extension>
    </xs:complexContent>
  </xs:complexType>

  <!-- Element that can hold any Shape subtype -->
  <xs:element name="shape" type="Shape"/>

</xs:schema>
```

**Example XML instance:**
```xml
<shape xsi:type="Circle" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <color>red</color>
  <radius>5.0</radius>
</shape>
```

**Generated C:**
```c
typedef struct {
    char color[64];
    double radius;
} Circle;

typedef struct {
    char color[64];
    double width;
    double height;
} Rectangle;

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE
} ShapeType;

typedef struct {
    ShapeType type;
    union {              // C11 anonymous union
        Circle circle;
        Rectangle rectangle;
    };
} Shape;

// Constructor functions
Shape create_circle(const char* color, double radius) {
    Shape shape;
    shape.type = SHAPE_CIRCLE;
    strncpy(shape.circle.color, color, 64);
    shape.circle.radius = radius;
    return shape;
}

Shape create_rectangle(const char* color, double width, double height) {
    Shape shape;
    shape.type = SHAPE_RECTANGLE;
    strncpy(shape.rectangle.color, color, 64);
    shape.rectangle.width = width;
    shape.rectangle.height = height;
    return shape;
}

// Usage
void process_shape(Shape* shape) {
    switch(shape->type) {
        case SHAPE_CIRCLE:
            printf("Circle: color=%s, radius=%.2f\n",
                   shape->circle.color,
                   shape->circle.radius);
            break;
        case SHAPE_RECTANGLE:
            printf("Rectangle: color=%s, %.2fx%.2f\n",
                   shape->rectangle.color,
                   shape->rectangle.width,
                   shape->rectangle.height);
            break;
    }
}
        case SHAPE_RECTANGLE:
            printf("Rectangle: color=%s, %.2fx%.2f\n",
                   shape->data.rectangle.color,
                   shape->data.rectangle.width,
                   shape->data.rectangle.height);
            break;
    }
}
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

Generated bind/unbind code handles conversion between C types and EXIP types:

**Schema-informed mode** (typed encoding):
```c
// Bind: C int → EXIP binary integer
serialize.intData(stream, obj->id);  // Direct, no conversion needed

// Unbind: EXIP Integer → C int
int handle_intData(Integer int_val, void* app_data) {
    Person* obj = (Person*)app_data;
    obj->id = (int)int_val;  // Simple cast
}
```

**Schema-less mode** (string encoding):
```c
// Bind: C int → C string → EXIP String
char buf[32];
snprintf(buf, sizeof(buf), "%d", obj->id);  // C int → C string
asciiToString(buf, &strVal, &stream->memList, false);  // C string → EXIP String
serialize.stringData(stream, strVal);  // Encode EXIP String

// Unbind: EXIP String → C string → C int
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

## Proof-of-Concept: Hand-Coded Prototype

The EXIP repository already contains a **working prototype** that demonstrates the target code generation approach:

**Location:** `examples/simpleEncoding/`
- **[exipe_data.h](../examples/simpleEncoding/exipe_data.h)** - Clean C struct definitions
- **[exipe_data.c](../examples/simpleEncoding/exipe_data.c)** - Constructor functions
- **[encodeTestEXI.c](../examples/simpleEncoding/encodeTestEXI.c)** - Manual encoding (what `exipb` would generate)

**Schemas:** Based on `exipe-test.xsd`, `exipe-test-nested.xsd`, `exipe-test-types.xsd`

### What the Prototype Demonstrates

**exipe_data.h shows target struct layout:**
```c
// Fixed-size arrays for bounded strings
typedef struct {
    char content[256];       // Text content
    char version[32];        // @version attribute
    int8_t testByte;         // @testByte attribute (optional)
    bool hasTestByte;        // Presence flag
} EXIPEncoder;

// Discriminated union for xs:choice
typedef enum {
    BASIC_TYPE_NONE,
    BASIC_TYPE_BOOL,
    BASIC_TYPE_INT
} BasicTypeChoice;

typedef struct {
    BasicTypeChoice choiceType;
    union {
        bool boolValue;
        int intValue;
    } choice;
    int id;  // Attribute
} BasicType;

// Nested structs
typedef struct {
    EXIPEncoder encoder;        // Nested complex type
    char description[256];
    TestSetup testSetup;        // Another nested type
    BasicType typeTest;         // Choice type
    TypesTest extendedTypeTest; // Optional nested type
    bool hasExtendedTypeTest;   // Presence flag
} MultipleXSDsTest;
```

**exipe_data.c shows constructor pattern:**
```c
// Bottom-up construction
TypesTest create_types_test(uint8_t* binaryData, size_t binaryLen, EnumType* greeting) {
    return (TypesTest){
        .byteTest = 11,
        .dateTimeTest = { /* ... */ },
        .binaryTest = binaryData,
        .binaryTestLen = (binaryData != NULL) ? binaryLen : 0,
        .enumTest = greeting ? *greeting : 0,
        .hasEnumTest = (greeting != NULL)
    };
}

// Compose parent from child structs
MultipleXSDsTest create_test_data(TypesTest typesTest) {
    return (MultipleXSDsTest){
        .encoder = { /* ... */ },
        .typeTest = { /* ... */ },
        .extendedTypeTest = typesTest,
        .hasExtendedTypeTest = true
    };
}
```

**encodeTestEXI.c shows manual encoding (283 lines):**
```c
errorCode encode(MultipleXSDsTest* testData, EXIPSchema* schemaPtr, FILE *outfile, ...) {
    // Manual EXIP serialization calls (what exipb would auto-generate)
    TRY_CATCH_ENCODE(serialize.startElement(&testStrm, qname, &valueType));
    TRY_CATCH_ENCODE(serialize.attribute(&testStrm, qname, true, &valueType));
    TRY_CATCH_ENCODE(serialize.intData(&testStrm, testData->encoder.testByte));
    // ... 280 more lines of manual serialize.* calls
}
```

### How the Prototype Maps to This Vision

| Vision Doc Feature | Prototype Status | Design Evolution |
|-------------------|------------------|------------------|
| **Fixed-size char arrays** | ✅ Implemented (`char[256]`) | Same approach |
| **Constructor pattern** | ✅ Implemented (`create_*()`) | Same approach |
| **Discriminated unions** | ✅ Implemented (`BasicTypeChoice` + union) | Same approach, more examples |
| **Nested structs** | ✅ Implemented (by value) | Same approach |
| **Optional fields** | ✅ `bool has_*` flags | **CHANGED: Now `OptionalT` wrappers** |
| **DateTime** | ✅ Custom struct | Same approach |
| **Enum mapping** | ✅ C enum + string table | Same approach |
| **Validation** | ❌ Not implemented | **NEW: `validate_*()` functions** |
| **Encode functions** | ❌ Hand-written 283 lines | **NEW: Auto-generate `encode_*()`** |
| **Decode functions** | ❌ Doesn't exist | **NEW: Auto-generate `decode_*()`** |

### Key Design Changes Since Prototype

**1. Optional Fields: `bool has_*` → `OptionalT` wrappers**

**Prototype approach:**
```c
typedef struct {
    int8_t testByte;
    bool hasTestByte;  // Separate flag
} EXIPEncoder;

// Usage
if (encoder.hasTestByte) {
    printf("Byte: %d\n", encoder.testByte);
}
```

**New vision approach:**
```c
typedef struct {
    int32_t value;
    bool exists;
} OptionalInt8;

typedef struct {
    OptionalInt8 testByte;  // Wrapped
} EXIPEncoder;

// Usage - uniform pattern
if (encoder.testByte.exists) {
    printf("Byte: %d\n", encoder.testByte.value);
}
```

**Why the change:**
- ✅ Uniform access pattern (`.exists`, `.value` everywhere)
- ✅ Type-safe (can't forget to check exists)
- ✅ Self-documenting (type shows optionality)
- ❌ Tradeoff: Extra indirection for non-strings

**2. Constructor Parameters: Pointer-or-NULL → Optional wrappers**

**Prototype approach:**
```c
TypesTest create_types_test(EnumType* greeting) {
    return (TypesTest){
        .enumTest = greeting ? *greeting : 0,
        .hasEnumTest = (greeting != NULL)
    };
}

// Usage - awkward
EnumType g = HELLO;
TypesTest t = create_types_test(&g);      // Take address
TypesTest t2 = create_types_test(NULL);   // NULL magic
```

**New vision approach:**
```c
TypesTest create_types_test(OptionalEnumType greeting) {
    return (TypesTest){
        .enumTest = greeting  // Just assign wrapper
    };
}

// Usage - explicit
TypesTest t = create_types_test(SOME(EnumType, HELLO));
TypesTest t2 = create_types_test(NONE(EnumType));
```

**Why the change:**
- ✅ No pointer-or-NULL magic
- ✅ Explicit presence (SOME/NONE)
- ✅ No address-of operator needed

**3. Validation Layer (NEW)**

The prototype has **no validation** - it's up to application code. The vision adds:

```c
// Generated validation function (NEW)
errorCode validate_Person(const Person* person) {
#if EXIP_VALIDATION_MODE >= 2
    if (person->age < 0 || person->age > 150) return EXIP_INVALID_EXI_INPUT;
    if (strlen(person->name) > 64) return EXIP_INVALID_EXI_INPUT;
#endif
    return EXIP_OK;
}

// Called automatically during encode
errorCode encode_Person(EXIStream* strm, const Person* person) {
    TRY(validate_Person(person));  // NEW
    TRY(bind_Person(strm, person));
    return EXIP_OK;
}
```

**4. Auto-Generated Encode/Decode (NEW)**

The prototype's `encodeTestEXI.c` contains 283 lines of **manual** `serialize.*()` calls. The vision automates this:

- **Prototype:** Hand-write every `serialize.startElement()`, `serialize.attribute()`, etc.
- **Vision:** `exipb` generates all encoding/decoding functions from schema

### Using the Prototype as Reference

When implementing `exipb`:

1. **Start with struct generation** - `exipe_data.h` shows the target layout
2. **Add constructors** - `exipe_data.c` shows the pattern
3. **Generate encode functions** - Automate what `encodeTestEXI.c` does manually
4. **Add decode functions** - Mirror encode pattern but use `parse.*()` API
5. **Add validation** - New feature not in prototype

The prototype proves the **struct design works**. The vision adds **automation** (codegen) and **safety** (validation).

## Challenges

1. **Complex XSD Features** - Some advanced XSD features are hard to map to C
2. **Memory Management** - Need clear patterns for dynamic arrays, strings
3. **Error Handling** - Need good error messages from generated code
4. **Large Schemas** - Code size could be significant for huge schemas
5. **Circular References** - Need forward declarations and careful ordering
6. **Type Conversions** - Schema-less mode requires string conversions for all typed fields

## Next Steps

1. Prototype schema parser using EXIP's TreeTable
2. Build simple struct generator for basic types
3. Add bind code generation (C struct → EXI)
4. Add unbind code generation (EXI → C struct)
5. Test with real-world schemas
6. Add advanced features (choices, unions, etc.)
7. Build command-line tool
8. Write comprehensive documentation

## Related Documents

### Background: Manual Patterns (What exipb Automates)

These documents describe **hand-written** patterns used before `exipb`:

- [DECODING_PATTERNS.md](DECODING_PATTERNS.md) - Manual schema/schemaless decoding handlers
- [MEMORY_MANAGEMENT_PATTERNS.md](MEMORY_MANAGEMENT_PATTERNS.md) - Manual memory management patterns

**Key insight:** Manual code must handle schema vs schemaless modes separately with duplicate logic and string conversions. **`exipb` generates code that automatically works in both modes.**

### Implementation References

- [ARCHITECTURE.md](ARCHITECTURE.md) - EXIP architecture overview
- [SCHEMA_GRAMMAR_GENERATION.md](SCHEMA_GRAMMAR_GENERATION.md) - Schema-informed grammar generation
- [EXI_DATA_TYPE_ENCODING.md](EXI_DATA_TYPE_ENCODING.md) - EXI type encoding details
- [src/grammarGen/](../../src/grammarGen/) - Existing schema processing code

---

**Status**: Vision / Planning Phase
**Priority**: High (would significantly improve usability)
**Complexity**: High (3-6 months development)
**Dependencies**: None (uses existing EXIP APIs)

## Struct Metadata Strategy (Design Decision)

### Minimal Metadata Principle

Generated structs include **only the metadata necessary for encode/decode correctness**. Validation is handled in generated decode functions, not via struct fields.

**Key separation:**
- **Struct fields** = what's needed for data storage and iteration (minimal)
- **Decode validation** = security checks in generated code (optional, compile-time controlled)

#### Always Generate (Struct Fields):

1. **Array count** (Required for iteration):
   ```c
   Module* modules;
   size_t modulesCount;  // Can't encode/iterate without count
   ```

2. **Binary data: pointer + length** (Not null-terminated, NULL = not present):
   ```c
   uint8_t* binaryData;      // NULL = not present
   size_t binaryDataLen;     // Can contain 0x00, need explicit length
   // No bool flag - use NULL check: if (binaryData != NULL)
   ```

3. **Optional primitive fields: value + flag** (Can't NULL check primitives):
   ```c
   int priority;          // Value (may be uninitialized if not present)
   bool hasPriority;      // Is it actually there?
   ```
   **Note:** For optional pointers (binary, arrays), use NULL check instead of flag - cleaner and more idiomatic C.

4. **Choice types: discriminator + union** (Type safety):
   ```c
   enum { TYPE_A, TYPE_B } choiceType;
   union { TypeA a; TypeB b; } choice;
   ```

5. **Strings: just storage** (Use strlen):
   ```c
   char name[256];        // Fixed - from maxLength constraint
   char* description;     // Unbounded - malloc'd
   // No length field - strlen() works for null-terminated strings
   ```

#### Never Generate (Struct Fields):

- ❌ Array capacity (only needed for dynamic growth, not our concern)
- ❌ String lengths (strlen works, avoid redundancy)
- ❌ Validation metadata (handled in decode functions, not struct fields)
- ❌ Presence masks (individual bools are clearer)

**Rationale:** Follow the established pattern from `exipe_data.h` which already demonstrates correct metadata usage. Structs stay minimal - validation logic lives in generated functions.

### Decode-Time Validation (Security Layer)

Validation happens in **generated decode functions**, controlled by compile-time defines. No extra struct fields needed because validation uses:
- Schema constraints (maxLength, minInclusive, maxOccurs from TreeTable)
- Incoming data (EXI streams are length-prefixed)
- Logic in generated code

**Example - String validation:**
```c
errorCode decode_person(Parser* parser, Person* person) {
    // Incoming string from EXI (length-prefixed)
    String incoming = /* from parser callback */;

#ifdef EXIPB_VALIDATION_STRICT
    // Validate against schema maxLength BEFORE copying
    if (incoming.length > 255) {
        return EXIP_INVALID_EXI_INPUT;  // Reject oversized string
    }
#endif

    // Safe to copy into fixed buffer
    memcpy(person->name, incoming.str, incoming.length);
    person->name[incoming.length] = '\0';
}
```

**Example - Array bounds validation:**
```c
errorCode decode_project(Parser* parser, Project* project) {
    size_t moduleCount = /* parsed from stream */;

#ifdef EXIPB_VALIDATION_STRICT
    // Validate against schema maxOccurs BEFORE allocating
    if (moduleCount > MAX_MODULES) {
        return EXIP_INVALID_EXI_INPUT;  // Reject too many elements
    }
#endif

    // Safe to allocate
    project->modules = malloc(moduleCount * sizeof(Module));
    project->modulesCount = moduleCount;
}
```

**Example - Range validation:**
```c
errorCode decode_person_age(Parser* parser, Person* person, Integer age_val) {
#ifdef EXIPB_VALIDATION_STRICT
    // Validate against schema minInclusive/maxInclusive
    if (age_val < 0 || age_val > 150) {
        return EXIP_INVALID_EXI_INPUT;  // Out of range
    }
#endif

    person->age = (int)age_val;
}
```

**Validation Modes (Compile-Time):**

```c
// No validation - production, trusted input, smallest/fastest code
// (default - no define set)

// Strict validation - development, security-critical, untrusted input
#define EXIPB_VALIDATION_STRICT
// Generates: if checks with error returns

// Debug validation - development, catch bugs early
#define EXIPB_VALIDATION_DEBUG
// Generates: assert() statements (crash on violation)
```

**What gets validated:**
- ✅ String lengths (against maxLength facet)
- ✅ Array bounds (against maxOccurs)
- ✅ Integer ranges (against minInclusive/maxInclusive)
- ✅ Occurrence counts (minOccurs/maxOccurs)
- ⚠️ Patterns (future - requires regex engine)
- ⚠️ Enumeration values (future - requires lookup tables)

**Key insight:** EXI streams are **length-prefixed**, not null-terminated. The length is in the stream before the data, so validation happens **before allocation** - perfect for security.

**Defense in depth:**
1. **EXIP core** validates EXI structure and grammar
2. **Generated validation** checks schema facets and constraints
3. **Application logic** handles business rules

Validation is **layered**, not duplicated. Generated code adds the facet layer that EXIP core doesn't enforce.

### Unbounded Array Strategy (maxOccurs="unbounded")

For initial version, generate unbounded arrays as **dynamic arrays** following exipe_data.h pattern:

```c
// Schema: <element name="modules" maxOccurs="unbounded"/>
struct Project {
    Module* modules;      // Pointer to array (malloc'd)
    size_t modulesCount;  // Required for iteration
    // No capacity field (not needed for encode/decode)
};
```

**Constructor ensures correctness:**
```c
Project create_project(Module* modules, size_t count) {
    return (Project){
        .modules = modules,
        .modulesCount = count
    };
}
```

**Decode validates before allocating:**
```c
errorCode decode_project_modules(Parser* parser, Project* project) {
    size_t count = /* count from EXI grammar */;

#ifdef EXIPB_VALIDATION_STRICT
    // Could add configurable max if schema has no maxOccurs bound
    if (count > EXIPB_DEFAULT_MAX_ARRAY_SIZE) {
        return EXIP_INVALID_EXI_INPUT;
    }
#endif

    project->modules = malloc(count * sizeof(Module));
    project->modulesCount = count;
}
```

**Configuration options for fixed-max vs dynamic allocation deferred to later phase.**

### bindapi.h Implementation Status

**Already implemented** in `src/common/src/bindapi.c`:

✅ **Type conversion helpers:**
- `floatToExipFloat()` - C float → EXIP Float struct (mantissa/exponent)
- `doubleToExipFloat()` - C double → EXIP Float struct
- `int32_to_str()` / `uint32_to_str()` - int → string without sprintf/strlen
- `charArrayToString()` - C string → EXIP String (does strlen once)
- `charArrayToStringFast()` - pointer arithmetic, avoids strlen

✅ **Mode-aware encoding:**
- `bindInt()` - checks `strm->schema`, routes to typed or string encoding
- `bindIntToString()` - schemaless int encoding
- `bindBoolToString()` - schemaless bool encoding

✅ **Direct encoding helpers:**
- `serializeFloatValue()` - C float → EXI (needs rename to `bindFloat`)
- `serializeBoolValue()` - bool → EXI (needs rename to `bindBool`)
- `serializeStringValue()` - C string → EXI (needs rename to `bindString`)

**Naming conventions TBD** - but principle is clear: **generated code uses `bind*` API, never `serialize.*` directly.** The `serialize.*` functions are low-level EXIP internal API.

### Design Work Still To Do

- IR (Intermediate Representation) design for schema → struct mapping
- TreeTable traversal strategy for field discovery
- Element name → struct field name mapping rules
- Handler registration patterns for decode callbacks
- Schema grammar state → struct field routing logic
- Validation code generation patterns (what checks to emit, when to check)

---

## Create/Encode/Decode Pattern

### User-Facing API: Create, Encode, Decode

**Encoding workflow** - user assembles structs bottom-up, then encodes:

```c
// User creates nested structs from inside out
Address addr = create_Address("123 Main St", "Springfield", 12345);
Person person = create_Person(12345, "John Doe", 30, true, addr);

// Encode takes the composed struct and encodes to EXI stream
EXIStream strm;
errorCode err = encode_Person(&strm, &person);
```

**Decoding workflow** - user calls decode, gets fully assembled struct:

```c
// Decode parses EXI and returns fully assembled struct
Person person;
errorCode err = decode_Person(&strm, &person);

// Use decoded data
printf("Name: %s, Age: %d\n", person.name, person.age);
```

**Key principle:** User controls composition via `create_` functions. `encode()` and `decode()` are the high-level entry points.

---

### Three-Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  User Layer (API)                                           │
│  - create_Person() - constructors                           │
│  - encode_Person() - high-level encode entry point          │
│  - decode_Person() - high-level decode entry point          │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  Binding Layer (Generated by exipb)                         │
│  - validate_Person() - validation (ifdef-able)              │
│  - bind_Person() - validates + encodes struct               │
│  - unbind_Person() - parses + populates struct              │
│  (Nested types call their own bind/unbind recursively)      │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  EXIP Core Layer (Existing)                                 │
│  - serialize.startElement(), serialize.intData(), etc.      │
│  - parse.parseNext(), handler callbacks                     │
└─────────────────────────────────────────────────────────────┘
```

---

### Internal Implementation: Bind/Unbind

**Encode implementation** - `encode()` calls `bind()` which calls `serialize.*`:

```c
// User-facing API
errorCode encode_Person(EXIStream* strm, const Person* person) {
#ifdef EXIPB_VALIDATION_STRICT
    // Validate entire struct before encoding
    if (person->age < 0 || person->age > 150) {
        return EXIP_INVALID_EXI_INPUT;
    }
    if (strlen(person->name) > 255) {
        return EXIP_INVALID_EXI_INPUT;
    }
#endif

    // Call internal binding implementation
    return bind_Person(strm, person);
}

// Generated binding layer
static errorCode bind_Person(EXIStream* strm, const Person* person) {
    QName qname;
    EXITypeClass valueType;

    // Build QName and call EXIP serialize API
    qname.uri = &NS_PERSON;
    qname.localName = &ELEM_PERSON;
    TRY(serialize.startElement(strm, qname, &valueType));

    // Encode fields using EXIP API
    TRY(serialize.intData(strm, person->id));
    TRY(serialize.stringData(strm, ...));

    // Nested struct - recursive bind
    TRY(bind_Address(strm, &person->address));

    TRY(serialize.endElement(strm));
    return EXIP_OK;
}
```

**Decode implementation** - `decode()` calls `unbind()` which recursively calls nested type unbind functions:

```c
// User-facing API
errorCode decode_Person(EXIStream* strm, Person* person) {
    // Call internal unbinding implementation
    return unbind_Person(strm, person);
}

// Generated binding layer
static errorCode unbind_Person(EXIStream* strm, Person* person) {
    // Parse events depth-first, populate fields as they arrive
    while (next_event(strm)) {
        if (SE "name") {
            // Parse string directly with validation
            String nameStr;
            TRY(parse.stringData(strm, &nameStr));
#ifdef EXIP_VALIDATION_MODE >= 2
            if (nameStr.length > sizeof(person->name) - 1) {
                return EXIP_INVALID_EXI_INPUT;
            }
#endif
            memcpy(person->name, nameStr.str, nameStr.length);
            person->name[nameStr.length] = '\0';
        }
        else if (SE "address") {
            // Nested struct - call its unbind function recursively
            TRY(unbind_Address(strm, &person->address));
        }
    }
    return EXIP_OK;
}

// Address unbind (same pattern as Person)
static errorCode unbind_Address(EXIStream* strm, Address* addr) {
    while (next_event(strm) != EE) {
        if (SE "street") {
            String str;
            TRY(parse.stringData(strm, &str));
            memcpy(addr->street, str.str, str.length);
            addr->street[str.length] = '\0';
        }
        if (SE "city") {
            String str;
            TRY(parse.stringData(strm, &str));
            memcpy(addr->city, str.str, str.length);
            addr->city[str.length] = '\0';
        }
        if (SE "zipCode") {
            TRY(parse.intData(strm, &addr->zipCode));
        }
    }
    return EXIP_OK;
}
```

**Validation placement:**

**Encode side** - validation at high level (`validate_*()` / `bind_*()`):
- Struct already exists, validate all constraints before encoding
- Check: ranges, string lengths, array counts
- Single validation pass at `encode_Person()` entry (calls `validate_Person()`)

**Decode side** - validation inline during unbind:
- Data arrives incrementally from stream
- Validate each piece immediately before copying to struct
- EXI length-prefixed → validate-then-copy pattern
- Security boundary - validates at lowest level, can't bypass

**Why different levels?**
- **Encode**: Struct complete, validate everything upfront at API boundary
- **Decode**: Data arrives piece-by-piece, validate each field as parsed
- **EXI advantage**: Length-prefixed means we validate BEFORE copying
- **Security**: Inline validation during unbind can't be bypassed

**Naming rationale:**
- **`encode`/`decode`** - user-facing API
- **`validate`/`bind`/`unbind`** - binding layer, generated by exipb (may be inlined later)
- **`serialize.*`/`parse.*`** - existing EXIP core API, lowest level

**Pattern summary:**
- **Encode**: User calls `create_*()` (bottom-up) → `encode()` calls `validate()` → `bind()` encodes via `serialize.*`
- **Decode**: User calls `decode()` → `unbind()` parses via `parse.*` and populates struct directly
- **Single pass** - no intermediate storage, validation inline with encoding/parsing
- **Minimal memory** - validate before allocate, only valid data stored

---

### Zero-Copy Immutable Unbind (Future Option)

Structs reference stream buffer directly - no string copying:

```c
// Stream buffer must outlive all structs
BinaryBuffer buf = loadFile("data.exi");
Parser parser;
initParser(&parser, buf, ...);

// Decode returns const - strings point into buf.buffer
const Person* person = decode_Person_view(&parser);

// User can read but not modify
printf("Name: %s\n", person->name);  // OK
person->age = 30;                    // Compile error - const

// CRITICAL: buf must stay alive while person is in use
destroyPerson(person);       // Free struct, not string data
freeBinaryBuffer(&buf);      // Now safe to free buffer
```

**Pros:**
- ✅ Zero-copy - minimal memory overhead (huge for embedded)
- ✅ Immutable - compile-time safety, no accidental modification
- ✅ Fast - no string copying, just pointer assignment
- ✅ Clear intent - const signals "read-only view"

**Cons:**
- ❌ Lifetime coupling - structs only valid while buffer alive
- ❌ User discipline required - must keep buffer in scope
- ❌ Can't modify - if user needs to change data, must copy manually
- ❌ Dangling pointer risk - if buffer freed too early, UB

**Key to usability:** Clear documentation and naming

```c
/**
 * Returns immutable view of Person. Strings point into stream buffer.
 * Returned struct valid only while stream buffer is alive.
 * Do NOT free buffer until done with returned Person.
 */
const Person* decode_Person_view(Parser* parser);
```

**Alternative: Provide both variants**
```c
// Zero-copy view (const, fast, borrowed)
const Person* decode_Person_view(Parser* parser);

// Owned copy (mutable, slower, independent lifetime)
Person* decode_Person_copy(Parser* parser);
```

---

### Decision for MVP

**Depth-first inline** (Option 1) is recommended for initial implementation:
- Standard pattern for recursive descent parsers
- Matches EXI's streaming model
- Lowest memory footprint
- Simplest implementation

**Future enhancements:**
- Zero-copy view mode (performance optimization)
- Two-pass assembly (if needed for forward references)
- Both documented here as options, not immediate requirements

---

## Encoding Function Generation Strategy

**Approach:** Generate one encoding function per struct type. Each function directly references generated `LnEntry` arrays using numeric indices. Nested structs call their respective encoders recursively.

**String References:**
```c
// Generated code uses numeric indices directly
errorCode encode_Person(EXIStream* strm, const Person* data) {
    QName qname;
    EXITypeClass valueType;

    // Reference existing LnEntry arrays with numeric indices
    qname.uri = &xmlscm_uriEntry[1].uriStr;
    qname.localName = &xmlscm_LnEntry_0[23].lnStr;  // No symbolic names needed
    qname.prefix = NULL;

    TRY(serialize.startElement(strm, qname, &valueType));
    // ... encode fields in schema order ...
    TRY(serialize.endElement(strm));
    return EXIP_OK;
}
```

**Key Principles:**
- No symbolic constants needed - function names (`encode_Person`) provide readability
- Numeric indices (`xmlscm_LnEntry_0[23]`) are generated from schema metadata
- Nested structs: call child encoders (`TRY(encode_Address(strm, &data->address))`)
- Arrays: loop and encode each element
- Optional fields: check presence flags before encoding
- Schema order: generator ensures fields encoded in correct sequence
- Recursive traversal mirrors schema hierarchy
