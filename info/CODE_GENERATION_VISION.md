# Code Generation from Schema - Vision Document

## Why XSD + EXI is Better Than Protobuf

XSD's type system is **vastly richer** than Protobuf's `.proto`, enabling better code generation, stronger guarantees, and functional composition patterns that eliminate code duplication.

### Type System Comparison

**XSD provides:**
- `xs:choice` → Tagged unions → Pattern matching
- `xs:extension` / `xs:restriction` → Type hierarchies
- Abstract types → Polymorphism
- Substitution groups → Type families
- Facets (`minInclusive`, `maxLength`, `pattern`) → Compile-time constraints
- `default` and `fixed` values → Schema-enforced defaults
- `minOccurs="0"` / `minOccurs="1"` → Explicit optional vs required fields
- `minOccurs` / `maxOccurs` → Array size constraints (e.g., 1-10 items)
- Value bounds (`minInclusive`, `maxInclusive`) → Integer/decimal range validation
- String constraints (`minLength`, `maxLength`, `pattern`) → Length and format validation

**Protobuf provides:**
- Flat messages with only composition support
- `oneof` (weak choice, no discriminator type)
- Fake inheritance via duplicated fields across message types
- No abstract types or type hierarchies
- No compile-time validation of constraints
- No default value support (deprecated in proto3)
- **proto3: All fields optional by default** - no way to declare required fields (removed from proto2)
- **proto2: `required` keyword deprecated** - Google recommends against using it
- **No array bounds** - `repeated` fields have no min/max constraints
- **No value bounds** - integers/floats have no range validation
- **No string length limits** - no minLength/maxLength facets
- **No pattern validation** - no regex constraints

**Key limitation:** Protobuf has **zero real OOP support**. You can only compose messages. To simulate inheritance, you must manually duplicate common fields across every message type that needs them. This creates maintenance burden and error-prone repetition.

### Security: Protobuf's Lack of Validation is Dangerous for Embedded

Protobuf's absence of schema-enforced constraints creates **serious security vulnerabilities**, especially in resource-constrained embedded systems:

#### Buffer Overflow Risks

**Protobuf (unbounded):**
```c
// Schema: message Person { string name = 1; }
// Generated struct has no bounds
char* name;  // Can be ANY length - attacker sends 10MB string

// Decoder allocates unbounded memory
name = malloc(incoming_length);  // Attacker controls this!
memcpy(name, input, incoming_length);  // Potential heap overflow
```

**XSD+EXI (bounded):**
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
// Generated with compile-time bounds
char name[65];  // Fixed size, no malloc

// Decoder enforces maxLength from schema
if (incoming_length > 64) return EXIP_INVALID_EXI_INPUT;
memcpy(name, input, min(incoming_length, 64));  // Safe
```

#### Denial of Service (Resource Exhaustion)

**Protobuf vulnerability:**
```protobuf
message Packet {
  repeated int32 values = 1;  // NO maxOccurs limit
}
```
Attacker sends: 10 million `values` → decoder allocates 40MB → embedded device crashes (out of memory).

**XSD+EXI mitigation:**
```xml
<xs:element name="values" type="xs:int" minOccurs="0" maxOccurs="1000"/>
```
Schema enforces max 1000 items. EXI decoder rejects messages exceeding this limit BEFORE allocating memory.

#### Injection Attacks (No Pattern Validation)

**Protobuf vulnerability:**
```protobuf
message Command {
  string sql_query = 1;  // NO pattern constraint
}
```
Attacker sends: `sql_query = "'; DROP TABLE users; --"` → No schema-level defense.

**XSD+EXI mitigation:**
```xml
<xs:element name="userId">
  <xs:simpleType>
    <xs:restriction base="xs:string">
      <xs:pattern value="[A-Za-z0-9]{8,16}"/>  <!-- Alphanumeric, 8-16 chars -->
    </xs:restriction>
  </xs:simpleType>
</xs:element>
```
Schema rejects malformed input at decode time (with super-strict validation enabled).

#### Integer Overflow (No Range Validation)

**Protobuf vulnerability:**
```protobuf
message Config {
  int32 buffer_size = 1;  // NO bounds
}
```
Attacker sends: `buffer_size = -1` or `2147483647` → Application allocates huge/invalid buffer → crash or exploit.

**XSD+EXI mitigation:**
```xml
<xs:element name="bufferSize">
  <xs:simpleType>
    <xs:restriction base="xs:int">
      <xs:minInclusive value="1"/>
      <xs:maxInclusive value="4096"/>
    </xs:restriction>
  </xs:simpleType>
</xs:element>
```
Schema enforces 1-4096 range. Decoder rejects out-of-range values before they reach application logic.

#### Why Embedded Systems Are Particularly Vulnerable

1. **Limited Memory** - No virtual memory, fixed RAM. Unbounded allocations = instant crash.
2. **No OS Protections** - Bare-metal systems lack memory protection units (MPU). Buffer overflow = memory corruption.
3. **Long Lifecycles** - Embedded devices (automotive, medical, IoT) run for years. Vulnerabilities persist.
4. **Network-Exposed** - IoT devices receive untrusted input from the internet. Schema validation is first line of defense.
5. **Safety-Critical** - Medical devices, automotive systems. Memory corruption can be fatal.

#### Defense-in-Depth with XSD+EXI

**Layer 1: Schema Design** - Define maxLength, maxOccurs, minInclusive, pattern in XSD
**Layer 2: Compile-Time** - Generated code uses fixed arrays (`char[65]`), not pointers
**Layer 3: Generated Code Validation** - exipb inserts bounds checks in encode/decode functions
**Layer 4: Decode-Time** - EXI decoder enforces schema constraints before data reaches app

#### Generated Code Validation (Practical Defense)

The binding generator can emit validation code directly into encode/decode functions, enforcing schema constraints **even if someone manually constructs an attack struct**:

**Example: Range validation on decode**
```c
// Schema: <xs:minInclusive value="0"/> <xs:maxInclusive value="150"/>
errorCode decode_person(Parser* parser, Person* person) {
    // ... parse age from EXI stream ...

    // GENERATED VALIDATION CHECK
    if (person->age < 0 || person->age > 150) {
        return EXIP_INVALID_EXI_INPUT;  // Reject out-of-range
    }

    return EXIP_OK;
}
```

**Example: Array bounds on encode**
```c
// Schema: <xs:element name="tags" maxOccurs="100"/>
errorCode encode_person(EXIStream* strm, const Person* person) {
    // GENERATED VALIDATION CHECK
    if (person->tagsCount > 100) {
        return EXIP_INVALID_EXI_INPUT;  // Catch attack before encoding
    }

    for (size_t i = 0; i < person->tagsCount; i++) {
        // ... encode each tag ...
    }
    return EXIP_OK;
}
```

**Example: String length on encode**
```c
// Schema: <xs:maxLength value="64"/>
errorCode encode_person(EXIStream* strm, const Person* person) {
    size_t nameLen = strlen(person->name);

    // GENERATED VALIDATION CHECK
    if (nameLen > 64) {
        return EXIP_INVALID_EXI_INPUT;  // Catch oversized string
    }

    // Safe to encode
    asciiToString(person->name, &strVal, &strm->memList, false);
    TRY(serialize.stringData(strm, strVal));
    return EXIP_OK;
}
```

**Debug vs Release Modes:**
```c
// exipb --validation=debug: Use asserts (fast, crash on violation)
#ifdef EXIPB_VALIDATION_DEBUG
    assert(person->age >= 0 && person->age <= 150);
#endif

// exipb --validation=strict: Use runtime checks (return error codes)
#ifdef EXIPB_VALIDATION_STRICT
    if (person->age < 0 || person->age > 150) {
        return EXIP_INVALID_EXI_INPUT;
    }
#endif

// exipb --validation=off: No checks (production, trust input)
```

**Benefits:**
- ✅ **Catches malicious data** - Even manually constructed attack structs are validated
- ✅ **Zero EXIP core changes** - All validation in generated code
- ✅ **Configurable** - Debug (asserts), strict (checks), off (performance)
- ✅ **Schema-driven** - Constraints from XSD automatically become validation code
- ✅ **Early detection** - Encode validation catches bad data before hitting wire
- ✅ **Decode defense** - Reject invalid data before it reaches application logic

**The Security Model:**

The **receiver** is the critical security boundary. Any **legitimate receiving system** must use the **generated decode code** with validation enabled. This is the defense against untrusted input:

1. **Sender** (trusted or compromised) → Encodes EXI stream
2. **Network transmission** → EXI binary stream (untrusted)
3. **Receiver uses generated decode** → **Validates incoming data, rejects attacks**

**Attack scenarios and defenses:**

- **Compromised sender** bypasses generated encode, crafts malicious EXI → **Receiver's decode validation catches it**
- **Network attacker** intercepts and modifies stream → **Receiver's decode validation rejects it**
- **Programming bug** on sender creates invalid struct → **Receiver's decode catches it** (encode validation helps too, but not the security boundary)

**Encode validation is defense-in-depth, not the security boundary:**

Encode validation (on the sender) helps catch bugs and provides belt-and-suspenders, but a determined attacker can bypass it:
```c
// Attacker can skip generated encode entirely
// encode_person(strm, &person);  // BYPASSED

// Attacker crafts raw EXI bytes with buffer overflow attempt
uint8_t malicious_exi[] = { /* hand-crafted attack */ };
fwrite(malicious_exi, 1, sizeof(malicious_exi), network_socket);
```

**Decode validation (on the receiver) is the security boundary:**

The receiver has **two layers of validation** - both defend against attacks:

**Layer 1: EXIP Core Decoder (Structure & Grammar)**
```c
// EXIP's parser validates:
// - EXI stream structure (well-formed binary)
// - Grammar state matching (correct element order)
// - Type matching in strict mode (intData matches grammar's expected type)
errorCode err = parse.parseNext(parser);  // Returns error if grammar violated
```

**Layer 2: Generated Binding Code (Facets & Constraints)**
```c
// Generated decode adds constraint validation AFTER EXIP accepts the structure:
errorCode decode_person(Parser* parser, Person* person) {
    // ... EXIP layer already parsed and validated grammar ...

    // GENERATED VALIDATION (facets from schema)
    // Overhead: Simple integer comparisons (nanoseconds)
    if (person->age < 0 || person->age > 150) {
        return EXIP_INVALID_EXI_INPUT;  // REJECT CONSTRAINT VIOLATION
    }

    if (person->tagsCount > 100) {
        return EXIP_INVALID_EXI_INPUT;  // REJECT ARRAY BOUNDS VIOLATION
    }

    return EXIP_OK;
}
```

**Validation Overhead:**

The overhead is **negligible** - simple integer comparisons:
- Range check: 2 comparisons + 1 branch (`age < 0 || age > 150`)
- Array bound: 1 comparison + 1 branch (`tagsCount > 100`)
- Modern CPUs: ~1-2 nanoseconds per check, branch prediction handles common case

Cost comparison:
- **EXI decoding**: Microseconds (bitstream parsing, grammar state, memory allocation)
- **Validation checks**: Nanoseconds (integer comparison)
- **Ratio**: Validation is <0.1% of decode time

For embedded systems, this is **essentially free** compared to network I/O or the parsing itself.

**Array Count and Binary Length Encoding:**

EXI encodes arrays and binary data differently:

**Arrays (repeated elements):**
```xml
<!-- Schema: maxOccurs="100" -->
<tag>employee</tag>
<tag>manager</tag>
<tag>admin</tag>
```
- **No count sent** in EXI stream
- Each `<tag>` encoded sequentially
- Grammar state tracks occurrences, enforces maxOccurs
- Decoder builds array: `tags[0]`, `tags[1]`, `tags[2]` → `tagsCount = 3`
- Attacker can't "claim 10, send 8" - count IS the number decoded
- Attacker sending 200 items → grammar rejects after 100th (maxOccurs)

**Binary data (base64Binary/hexBinary):**
```xml
<binaryTest><!-- raw bytes --></binaryTest>
```
- **Length IS sent** in EXI stream: `[Length=150] [150 bytes]`
- Length-prefix encoding (not delimiter-based)
- **EXIP behavior (current):**
  1. Reads length from stream
  2. Allocates memory (`malloc(length)`)
  3. Reads exactly `length` bytes
  4. **Does NOT validate against maxLength facet**
- **Generated code validation (after EXIP):**
  ```c
  // After EXIP allocates and decodes
  if (test->binaryTestLen > MAX_BINARY_LENGTH) {
      free(test->binaryTest);  // Clean up
      return EXIP_INVALID_EXI_INPUT;
  }
  ```
  Catches constraint violations, but allocation already happened
- **Future enhancement:** EXIP "Super Strict Mode" could validate maxLength **before** allocating (validate-then-allocate pattern)

**Security benefit of length-prefix encoding:**
- ✅ Deterministic - know exactly how many bytes to read
- ✅ No scanning for delimiters
- ✅ Read exactly `length` bytes, no buffer overrun
- ✅ **Generated code validates BEFORE allocating** - length comes first in stream, check against maxLength facet, then malloc
- ⚠️ EXIP core: Currently allocates without checking facets (doesn't know about maxLength)
- 💡 Future EXIP enhancement: Core could accept facet limits and validate before allocating (optional)

**Encoder Validation Limitations (C Memory Safety):**

The encoder faces a fundamental C limitation: **it cannot validate pointer+length integrity**. This issue **only affects dynamic data** (binary, unbounded arrays) - **fixed buffers in structs are safe**.

**Safe: Fixed buffers (embedded in struct)**
```c
struct Person {
    char name[256];    // Buffer IN struct, size known at compile time
    int age;
};

Person p;
strcpy(p.name, "Alice");
encode_Person(&p);  // ✅ Safe - compiler knows buffer is 256 bytes, can't over-read
```

**Unsafe: Pointer + length (only if constructor bypassed)**
```c
// Binary data is generated for xs:base64Binary and xs:hexBinary XSD types
struct TypesTest {
    uint8_t* binaryTest;      // Pointer to binary data (NOT in struct)
    size_t binaryTestLen;     // User CLAIMS this is the length
    // NULL = not present, no separate bool flag needed
};
```

**Note:** Binary fields (`uint8_t*` + length) are only generated for:
- `xs:base64Binary` - binary data encoded as base64 in XML
- `xs:hexBinary` - binary data encoded as hex in XML

For optional pointer+length data, we need **both fields**:
- **Pointer** - where the data is (NULL = not present)
- **Length** - how much data (not null-terminated, explicit length required)

No bool flag needed - check `if (ptr != NULL)` to determine presence.

**The Problem (only when bypassing constructors):**
```c
// What if this happens?
uint8_t small_buf[10];
TypesTest test;
test.binaryTest = small_buf;
test.binaryTestLen = 1000;     // LIES! Only 10 bytes allocated

// Encoder trusts the struct
encode_types_test(strm, &test);
// → Reads 1000 bytes from 10-byte buffer
// → Buffer over-read, undefined behavior, crash or memory leak
```

**The encoder has NO WAY to know the actual allocated size.** C doesn't track allocation sizes at runtime. It must trust `binaryTestLen`.

**This issue only affects pointer+length pairs:**
- ✅ Binary data (`xs:base64Binary`, `xs:hexBinary`) - always pointer+length
- ✅ Unbounded arrays (`maxOccurs="unbounded"`) - always pointer+count
- ✅ Unbounded strings (no `maxLength`) - pointer (if using pointers instead of default fixed buffer)

**Fixed buffers are immune:**
- ✅ Strings with `maxLength` → `char[N]` - size baked into struct, no separate length needed (use strlen)
- ✅ Bounded arrays with known `maxOccurs` → `Type array[N]` - size baked into struct
- ✅ All primitive fields - no pointers, no length tracking, just value+bool flag if optional

**Partial Defenses (Encode Validation):**

**1. Constructor enforces correctness (best practice - prevents the issue):**
```c
TypesTest create_types_test(uint8_t* data, size_t len, EnumType* enumVal) {
    return (TypesTest){
        .binaryTest = data,
        .binaryTestLen = len  // User provides both together
    };
}

// Usage encourages correctness
uint8_t buf[150];
TypesTest test = create_types_test(buf, sizeof(buf), NULL);  // Correct by construction
```

**2. Encode validation checks schema maxLength:**
```c
errorCode encode_types_test(EXIStream* strm, const TypesTest* test) {
    // VALIDATION: Check against schema constraint
    if (test->binaryTestLen > MAX_BINARY_LENGTH) {
        return EXIP_INVALID_EXI_INPUT;  // Catches obviously wrong lengths
    }

    // But CAN'T catch: allocated 10, claimed 15
    serialize.binaryData(strm, test->binaryTest, test->binaryTestLen);
}
```

**3. Sanity checks (partial):**
```c
// Check for obvious lies
if (test->binaryTest == NULL && test->binaryTestLen > 0) {
    return EXIP_INVALID_EXI_INPUT;  // Pointer NULL but claims non-zero length
}

if (test->binaryTest != NULL && test->binaryTestLen == 0) {
    // Valid - pointer set but zero length (empty data)
}
```

**4. What encode validation CANNOT catch:**
- ❌ User allocates 10 bytes, claims 1000 → buffer over-read
- ❌ User allocates 1000 bytes, claims 10 → works but wastes bandwidth
- ❌ User frees memory, then encodes → use-after-free
- ❌ Pointer to stack variable that goes out of scope
- ❌ Uninitialized pointer with valid-looking address

This is C's fundamental memory unsafety. No runtime check can validate pointer validity without OS-level memory tracking (AddressSanitizer, Valgrind, etc.).

**Why Decode Validation IS Complete:**

On **decode**, the situation is much better:

```c
errorCode decode_types_test(Parser* parser, TypesTest* test) {
    // EXI stream contains length=150
    uint32_t length = decode_length_from_stream(parser);

    // VALIDATE BEFORE ALLOCATING
    if (length > MAX_BINARY_LENGTH) {
        return EXIP_INVALID_EXI_INPUT;  // Reject before malloc
    }

    // Safe allocation - WE control it
    test->binaryTest = malloc(length);
    if (!test->binaryTest) {
        return EXIP_OUT_OF_MEMORY;  // Handle allocation failure
    }

    test->binaryTestLen = length;  // Guaranteed correct
    test->hasBinaryTest = true;

    // Read exactly 'length' bytes
    read_bytes(parser, test->binaryTest, length);
}
```

**Decode controls allocation** → length is guaranteed correct → fully validated.
**Encode trusts user struct** → user controls allocation → can only check constraints, not memory safety (for pointer+length pairs).

**Summary:**
- **Fixed buffers** (`char[256]`, `int array[10]`) - **always safe**, size in struct definition
- **Pointer+length** (binary data, unbounded arrays) - **safe if using constructors**, unsafe if manually constructed
- **Optional fields** - primitives use bool flag, pointers use NULL check (cleaner, more idiomatic C)
- **Best practice**: Always use `create_*()` constructors - correct by construction
- **Validation**: Catches schema violations (maxLength exceeded), NOT allocation bugs (under-allocated buffer)

**Security Implications:**

This explains why **decode validation is the security boundary**:

| Layer | Input Trust | Allocation Control | What Can Be Validated |
|-------|-------------|-------------------|----------------------|
| **Decode** | Untrusted (network) | We control | ✅ Schema constraints (maxLength, ranges)<br>✅ Memory safety (validate before allocate)<br>✅ Pointer+length integrity (we set both) |
| **Encode** | Trusted (local struct) | User controls | ✅ Schema constraints (maxLength, ranges)<br>❌ Memory safety (C can't verify)<br>❌ Pointer+length integrity (user set, we trust) |

**Bottom line:** Encode validation catches **schema constraint violations** (out-of-range, too-long strings, too-many array items). It cannot catch **memory safety bugs** (under-allocated buffers, dangling pointers). That's a fundamental C limitation. The best defense is:
1. Use constructors (correct by construction)
2. Enable encode validation (catches constraint violations)
3. Good coding practices (code review, static analysis, AddressSanitizer in test)
4. **Defense-in-depth on decode** (the security boundary for untrusted input)

**Defense-in-Depth:**
1. **EXIP strict mode** catches structural attacks (wrong element order, type mismatches, occurrence violations)
2. **Generated validation** catches constraint attacks (out-of-range values, oversized arrays/strings)

An attacker must defeat BOTH layers. If EXIP accepts the stream (valid structure), generated code still rejects constraint violations. This is **layered security** - even if one layer has a bug, the other catches it.

The key: **Validate on decode (untrusted input), optionally validate on encode (catch bugs early).**

Systems that DON'T use generated decode code (hand-rolled parsers) must implement validation manually. But that's their choice to bypass the safety layer.

With Protobuf, you must implement ALL validation manually in application code. With XSD+EXI, the schema IS the security policy, enforced automatically by **generated decode code**.

**Bottom line:** Protobuf's "simplicity" (no constraints) is a **security liability** in embedded systems. XSD's "complexity" (rich constraints) is **defense-in-depth by design**.

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

The discriminator (`type`) is explicit and type-safe. With Protobuf `oneof`, you check which field is set at runtime with no compile-time guarantees.

### XSD Extension → Type-Safe Polymorphism with Embedded Base

C doesn't have true inheritance, but we can achieve **type-safe polymorphism** through the "embed base as first field" pattern. This is the standard C approach used by GTK, Linux kernel, and other large C codebases.

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

**Generated C:**
```c
// Base type (can be instantiated directly)
typedef struct {
    char name[256];
    int age;
} Person;

// Extended type - base MUST be first field for pointer casting
typedef struct {
    Person base;       // CRITICAL: must be first!
    int employeeId;
} Employee;

// Type-safe constructor
Employee create_employee(const char* name, int age, int id) {
    Employee emp = {0};
    strncpy(emp.base.name, name, 255);
    emp.base.age = age;
    emp.employeeId = id;
    return emp;
}
```

**Polymorphic usage (pointer casting works):**
```c
void print_person(Person* p) {
    printf("Name: %s, Age: %d\n", p->name, p->age);
}

Employee emp = create_employee("Alice", 30, 12345);

// Upcast works because base is first field (C guarantees this)
print_person((Person*)&emp);  ✅ Valid, safe, standard C pattern

// Direct access to derived fields
printf("Employee ID: %d\n", emp.employeeId);
```

**Embedded-friendly:** Fixed layout, no pointers, stack-allocatable.

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

**Generated C (Embed + Discriminator):**
```c
// Base type (abstract - not instantiated directly)
typedef struct {
    char color[64];
    int id;
} Shape;

// Concrete subtypes - base MUST be first field
typedef struct {
    Shape base;      // MUST be first!
    double radius;
} Circle;

typedef struct {
    Shape base;      // MUST be first!
    double width;
    double height;
} Rectangle;

// Discriminator enum for type safety
typedef enum {
    SHAPE_TYPE_CIRCLE,
    SHAPE_TYPE_RECTANGLE
} ShapeType;

// Polymorphic wrapper (what <element name="shape" type="Shape"/> generates)
typedef struct {
    ShapeType type;      // Runtime type tag
    union {              // C11 anonymous union
        Circle circle;
        Rectangle rectangle;
    };
} ShapeElement;
```

**Type-safe constructors (impossible to create invalid state):**
```c
ShapeElement create_circle(const char* color, int id, double radius) {
    ShapeElement shape = {0};
    shape.type = SHAPE_TYPE_CIRCLE;
    strncpy(shape.circle.base.color, color, 63);
    shape.circle.base.id = id;
    shape.circle.radius = radius;
    return shape;
}

ShapeElement create_rectangle(const char* color, int id, double w, double h) {
    ShapeElement shape = {0};
    shape.type = SHAPE_TYPE_RECTANGLE;
    strncpy(shape.rectangle.base.color, color, 63);
    shape.rectangle.base.id = id;
    shape.rectangle.width = w;
    shape.rectangle.height = h;
    return shape;
}
```

**Pattern matching (type-specific processing):**
```c
void process_shape(ShapeElement* shape) {
    switch (shape->type) {
        case SHAPE_TYPE_CIRCLE:
            printf("Circle: color=%s, radius=%.2f\n",
                   shape->circle.base.color,
                   shape->circle.radius);
            break;
        case SHAPE_TYPE_RECTANGLE:
            printf("Rectangle: color=%s, %.2fx%.2f\n",
                   shape->rectangle.base.color,
                   shape->rectangle.width,
                   shape->rectangle.height);
            break;
    }
}
```

**Polymorphic access (common base fields):**
```c
// Helper: upcast to base type
Shape* get_base_shape(ShapeElement* shape) {
    // Safe because 'base' is first field in ALL union members
    return (Shape*)shape;  // Direct cast - union is anonymous
}

// Process common fields without switch
void print_common(ShapeElement* shape) {
    Shape* s = get_base_shape(shape);
    printf("ID: %d, Color: %s\n", s->id, s->color);
}
```

**Type safety guarantees:**
- ✅ Compiler enforces discriminator matches union field (via constructors)
- ✅ Switch statements are exhaustive (compiler warning if case missing)
- ✅ Pointer casting is safe (C guarantees first-field layout)
- ✅ Can't access wrong union field without checking discriminator first
- ✅ No vtables, no function pointers, no runtime overhead

**Embedded-friendly aspects:**
- ✅ No malloc required (stack or static allocation)
- ✅ Fixed size known at compile time (`sizeof(ShapeElement)`)
- ✅ No pointers (except for unbounded arrays/optional elements)
- ✅ Cache-friendly (contiguous memory)
- ✅ Zero runtime dispatch overhead
- ✅ Works in bare-metal environments

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
3. Generate base struct + all concrete subtypes with embedded base
4. Generate discriminator enum from subtype names
5. Generate wrapper struct with discriminator + union for polymorphic elements
6. Generate type-safe constructor per subtype
7. Generate helper for upcast to base type

**With Protobuf, you'd either:**
- Duplicate base fields in every message type (error-prone, violates DRY)
- Use awkward wrapper messages with oneof (no type safety, manual management)

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

### Wire Format: EXI vs Protobuf

**EXI advantages:**
- Schema-informed compression (often **smaller** than Protobuf)
- XSD validation built-in (Protobuf has no validation)
- Supports XML text AND binary (one schema, two formats)
- Handles complex XSD features (inheritance, choice, facets)
- W3C standard, not Google-only

**Protobuf advantages:**
- Simpler (but less powerful)
- More language support (though this can change with exipb)
- Better known (but EXI is proven in IoT/automotive/aerospace)

### The Vision

Combine:
1. **XSD's rich type system** - for proper domain modeling
2. **EXI's binary efficiency** - for compact wire format
3. **Functional C API** - for composable, type-safe code
4. **Pattern matching** - for duplication-free processing

Result: A **Protobuf killer** for domains that need:
- Rich schemas (finance, healthcare, legal, aerospace)
- Type hierarchies and polymorphism
- Validation and constraints
- Both binary efficiency AND human-readable XML fallback

The composable constructor pattern + XSD's type system creates something fundamentally better than Protobuf's flat message approach.

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
                    │bind_person_impl()│  Binding layer (generated)
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
                    │unbind_person_impl│  Binding layer (generated)
                    │  + extract_*()   │
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
// Encoding - user calls after composing struct with create_*()
errorCode encode_Person(EXIStream* strm, const Person* person);

// Decoding - user calls to parse stream into struct
errorCode decode_Person(Parser* parser, Person* person);
```

**Internal implementation (not exposed to user):**
```c
// Binding layer - validates and encodes
static errorCode bind_person_impl(EXIStream* strm, const Person* person);

// Binding layer - parses and assembles
static errorCode unbind_person_impl(Parser* parser, Person* person);

// Assembly helpers - extract fields from stream
static errorCode extract_Address(Parser* parser, Address* addr);
static errorCode extract_string(Parser* parser, char* dest, size_t maxLen);
```

### Design Principles

**1. Three-layer architecture**
- **User API**: `create_*()`, `encode_*()`, `decode_*()` - clean interface
- **Binding layer**: `bind_*_impl()`, `unbind_*_impl()`, `extract_*()` - generated implementation
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
static errorCode unbind_person_impl(Parser* parser, Person* person) {
    // Schema-informed parse: EXIP calls intData handler
    // Binding knows which field based on schema grammar state
    person->age = received_int;  // Direct assignment
}
```

#### Schemaless Mode (String Conversion)
```c
// No schema grammar, everything is strings
// Element name: "age", value: "30" (as string)
static errorCode unbind_person_impl(Parser* parser, Person* person) {
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

### Constructor Pattern for Data Initialization

Generated code provides **constructor-style functions** that return fully initialized structs:

```c
// Each struct type gets a create_<typename>() constructor
// Optional parameters use pointers (NULL = not present)
static inline TypesTest create_types_test(EnumType* greeting);
static inline Person create_person(int age, const char* name);

// Sub-objects are constructed separately, then passed to parent constructors
EnumType greeting = HEJ;
TypesTest types = create_types_test(&greeting);      // With optional enum
TypesTest types2 = create_types_test(NULL);          // Without optional enum

MultipleXSDsTest data = create_test_data(types);     // Compose!

// Cleanup only needed if dynamic allocations exist
destroy_multiple_xsds_test(&data);
```

**Benefits:**
- Single-statement initialization with parameters
- No separate `init()` function needed - constructors handle everything
- Lengths and optional flags calculated automatically
- NULL pointer = "optional element not present"
- Composable - build sub-objects first, pass them up
- Reusable - call constructor again with different parameters

**Default and Fixed Values:**

XSD schemas can specify `default` and `fixed` attribute values:

```xml
<!-- Default value (used if element/attribute absent) -->
<xs:element name="priority" type="xs:int" default="5"/>

<!-- Fixed value (must always be this value if present) -->
<xs:attribute name="version" type="xs:string" fixed="1.0"/>
```

Generated constructors handle these automatically:

```c
static inline Task create_task(int* priority) {
    return (Task){
        .priority = priority ? *priority : 5,  // Use default=5 if NULL
        .hasPriority = (priority != NULL)
    };
}

// For fixed values, generate as const:
static const char ATTR_VERSION[] = "1.0";  // Fixed, not a parameter
```

**Key points:**
- `default` → used when optional parameter is NULL
- `fixed` → not a parameter, generated as compile-time constant
- Without `default`, NULL means "not present" with dummy value in struct
- Specify defaults in schema for better generated code and documentation

**Binary Data (base64Binary, hexBinary):**

Binary data requires both pointer and length tracking:

```c
typedef struct {
    uint8_t* binaryData;      // Pointer to binary data
    size_t binaryDataLen;     // Length in bytes
    bool hasBinaryData;       // Optional flag
} Message;

// Constructor with optional binary data
static inline Message create_message(uint8_t* data, size_t len) {
    return (Message){
        .binaryData = data,
        .binaryDataLen = len,
        .hasBinaryData = (data != NULL && len > 0)  // Auto-calculate
    };
}

// Usage with static binary data
static uint8_t sample[] = {0x02, 0x6d, 0x2f, 0xa5};
Message msg = create_message(sample, sizeof(sample));

// Usage without binary (optional not present)
Message msg2 = create_message(NULL, 0);
```

EXI binary encoding is NOT self-describing - `serialize.binaryData(strm, data, length)` requires explicit length. The length must be tracked in the struct alongside the pointer.

#### When to Generate Constructor Functions

The binding generator should be **selective** about when to create `create_XXX()` functions. Not every struct needs one.

#### Memory Ownership Pattern

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

## Overview

Automatically generate C structures and bind/unbind code from XML Schema (XSD) files, supporting both schema-informed and schemaless EXI processing.

## Goals

1. **Generate C structures** from XSD that mirror the schema
2. **Generate bind code** (C struct → EXI)
3. **Generate unbind code** (EXI → C struct)
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

//==============================================================================
// OOP-style vtable API (following EXIP's parse.*, serialize.* pattern)
//==============================================================================

struct PersonOps {
    void (*init)(Person* p);
    void (*destroy)(Person* p);
    errorCode (*encode_person)(EXIStream* strm, const Person* p);
    errorCode (*decode_person)(Parser* parser, Person* p);
    errorCode (*encodeDocument)(const char* filename, const Person* p, EXIPSchema* schema);
    errorCode (*decodeDocument)(const char* filename, Person* p, EXIPSchema* schema);
};

struct AddressOps {
    void (*init)(Address* a);
    void (*destroy)(Address* a);
    errorCode (*encode_address)(EXIStream* strm, const Address* a);
    errorCode (*decode_address)(Parser* parser, Address* a);
};

// Global vtables (const, stored in ROM)
extern const struct PersonOps person;
extern const struct AddressOps address;

#endif /* PERSON_H */
```

**Usage example:**
```c
Person p;
person.init(&p);
person.encode_person(strm, &p);
person.destroy(&p);
```

This follows EXIP's existing OOP vtable pattern (`parse.*`, `serialize.*`, `option.*`).

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
// Initialization functions (internal)
//==============================================================================

static void init_person_impl(Person* person) {
    memset(person, 0, sizeof(Person));
    person->tags.items = NULL;
    person->tags.count = 0;
    person->tags.capacity = 0;
    address.init(&person->address);  // Use vtable for nested types
}

static void init_address_impl(Address* addr) {
    memset(addr, 0, sizeof(Address));
}

static void destroy_person_impl(Person* person) {
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

errorCode encode_address(EXIStream* stream, const Address* address) {
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

errorCode encode_person(EXIStream* stream, const Person* person) {
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
    TRY(encode_address(stream, &person->address));

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

errorCode decode_address(Parser* parser, Address* address) {
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

// Similar implementation for decode_person(Parser*, Person*)...
// Complex types auto-call child decode_<type>() for nested structures

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

    err = person.encode_person(&stream, person_data);
    if(err != EXIP_OK) goto cleanup;

    err = serialize.endDocument(&stream);

cleanup:
    serialize.closeEXIStream(&stream);
    fclose(outfile);
    return err;
}

static errorCode decode_person_document_impl(const char* filename, Person* person_data,
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
    err = person.decode_person(&parser, person_data);

    parse.destroyParser(&parser);
    return err;
}

//==============================================================================
// Vtable definitions (const, stored in ROM)
//==============================================================================

const struct PersonOps person = {
    .init = init_person_impl,
    .destroy = destroy_person_impl,
    .encode_person = encode_person_impl,
    .decode_person = decode_person_impl,
    .encodeDocument = encode_person_document_impl,
    .decodeDocument = decode_person_document_impl
};

const struct AddressOps address = {
    .init = init_address_impl,
    .destroy = destroy_address_impl,
    .encode_address = encode_address_impl,
    .decode_address = decode_address_impl
};
```

### Usage Example

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
3. Add bind code generation (C struct → EXI)
4. Add unbind code generation (EXI → C struct)
5. Test with real-world schemas
6. Add advanced features (choices, unions, etc.)
7. Build command-line tool
8. Write comprehensive documentation

## Related Documents

- [UNIFIED_DECODING_PATTERNS.md](UNIFIED_DECODING_PATTERNS.md) - Manual decoding patterns
- [DECODING_TO_STRUCT.md](DECODING_TO_STRUCT.md) - Hand-written struct decoding
- [ARCHITECTURE.md](ARCHITECTURE.md) - EXIP architecture
- [DECODING_PATTERNS.md](DECODING_PATTERNS.md) - Decoding patterns and memory management
- [SCHEMA_GRAMMAR_GENERATION.md](SCHEMA_GRAMMAR_GENERATION.md) - Schema-informed grammar generation
- [EXI_DATA_TYPE_ENCODING.md](EXI_DATA_TYPE_ENCODING.md) - EXI type encoding details
- [BINARY_ENCODING_DESIGN.md](../BINARY_ENCODING_DESIGN.md) - Base64/hex encoding design for schemaless mode
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
│  - bind_person_impl() - validates + encodes struct          │
│  - unbind_person_impl() - parses + assembles struct         │
│  - extract_Address() - internal assembly helpers            │
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
    return bind_person_impl(strm, person);
}

// Generated binding layer
static errorCode bind_person_impl(EXIStream* strm, const Person* person) {
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
    TRY(bind_address_impl(strm, &person->address));

    TRY(serialize.endElement(strm));
    return EXIP_OK;
}
```

**Decode implementation** - `decode()` calls `unbind()` which uses `extract()` helpers:

```c
// User-facing API
errorCode decode_Person(EXIStream* strm, Person* person) {
    // Call internal unbinding implementation
    return unbind_person_impl(strm, person);
}

// Generated binding layer
static errorCode unbind_person_impl(EXIStream* strm, Person* person) {
    // Parse events depth-first, extract fields as they arrive
    while (next_event(strm)) {
        if (SE "name") {
            TRY(extract_string(strm, person->name, sizeof(person->name)));
        }
        else if (SE "address") {
            // Nested struct - recursive extract
            TRY(extract_Address(strm, &person->address));
        }
    }
    return EXIP_OK;
}

// Internal assembly helper (not exposed to user)
static errorCode extract_Address(EXIStream* strm, Address* addr) {
    while (next_event(strm) != EE) {
        if (SE "street") TRY(extract_string(strm, addr->street, sizeof(addr->street)));
        if (SE "city") TRY(extract_string(strm, addr->city, sizeof(addr->city)));
        if (SE "zipCode") TRY(extract_int(strm, &addr->zipCode));
    }
    return EXIP_OK;
}

// Low-level extract helper with validation
static errorCode extract_string(EXIStream* strm, char* dest, size_t maxLen) {
    String incoming = /* parsed from EXIP handler callback */;

#ifdef EXIPB_VALIDATION_STRICT
    // Validate BEFORE copying (EXI is length-prefixed)
    if (incoming.length > maxLen - 1) {
        return EXIP_INVALID_EXI_INPUT;
    }
#endif

    memcpy(dest, incoming.str, incoming.length);
    dest[incoming.length] = '\0';
    return EXIP_OK;
}
```

**Validation placement:**

**Encode side** - validation at high level (`encode()`):
- Struct already exists, validate all constraints before encoding
- Check: ranges, string lengths, array counts
- Single validation pass at `encode_Person()` entry

**Decode side** - validation at low level (`extract_*()` helpers):
- Data arrives incrementally from stream
- Validate each piece immediately before copying/allocating
- EXI length-prefixed → validate-then-allocate pattern
- Security boundary - validates at lowest level, can't bypass

**Why different levels?**
- **Encode**: Struct complete, validate everything upfront at API boundary
- **Decode**: Data arrives piece-by-piece, validate each field as parsed
- **EXI advantage**: Length-prefixed means we validate BEFORE allocation
- **Security**: Decode validation in `extract_*` can't be bypassed

**Naming rationale:**
- **`encode`/`decode`** - matches Java marshall/unmarshall concept, user-facing API
- **`bind`/`unbind`** - binding layer, generated by exipb, internal implementation
- **`serialize.*`/`parse.*`** - existing EXIP core API, lowest level

**Pattern summary:**
- **Encode**: User calls `create_` (bottom-up) → `encode()` validates → `bind()` encodes via `serialize.*`
- **Decode**: User calls `decode()` → `unbind()` parses via `parse.*` → `extract_*()` validates + assembles
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
