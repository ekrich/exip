# EXIP Architecture Overview

**EXIP - Embeddable EXI Processor in C**

## Executive Summary

EXIP is a C library implementing the W3C EXI (Efficient XML Interchange) format specification. It provides efficient parsing and serialization of XML documents using EXI encoding, with support for both schema-informed and schema-less processing modes. The architecture is designed for embedded systems with emphasis on low memory footprint and portability.

## Core Concepts

### EXI Format
EXI (Efficient XML Interchange) is a binary XML format that:
- Dramatically reduces XML document size (10x-100x compression typical)
- Improves parsing performance through compact binary representation
- Maintains full XML information set fidelity
- Supports optional schema-informed encoding for further optimization

### Processing Modes
1. **Schema-less Mode**: Processes EXI without schema knowledge (more flexible)
2. **Schema-informed Mode**: Uses XML Schema to optimize encoding/decoding (more efficient)
3. **Strict Mode**: Schema-informed with strict validation
4. **EXI Profile Mode**: Restricted subset for severely constrained embedded devices (schema-only)

#### EXI Profile

EXI Profile is a W3C specification (http://www.w3.org/TR/exi-profile/) defining a restricted subset of EXI for embedded systems with severe constraints (16-bit microcontrollers, limited RAM/flash).

**Profile Restrictions**:
- Schema-informed mode only (no schemaless)
- No built-in element grammars (`maximumNumberOfBuiltInElementGrammars = 0`)
- No built-in productions (`maximumNumberOfBuiltInProductions = 0`)
- No local value partitions (`localValuePartitions = 0`)
- Minimal string tables and grammar complexity

**EXIP Configuration**:
- `EXI_PROFILE_DEFAULT ON` - Enables Profile mode, disables schemaless support
- `EXI_PROFILE_DEFAULT OFF` - Full EXI support (default for both PC and embedded builds)

**Historical Note**: EXIP was designed in the 16-bit microcontroller era when EXI Profile constraints were necessary. Modern 32-bit embedded platforms (ARM Cortex-M) can typically support full EXI mode, though Profile mode remains available for the most constrained devices.

**Impact on bindapi**: When `EXI_PROFILE_DEFAULT ON`, bindapi's schemaless conversion paths are excluded at compile time, reducing code size to Profile-compliant minimum.

#### Detecting Schema Mode

After parsing the header, check `parser.strm.header.opts.schemaIDMode`:

- `SCHEMA_ID_SET`: Schema ID provided - schema-informed mode required
- `SCHEMA_ID_NIL`: Explicitly schemaless
- `SCHEMA_ID_EMPTY`: Built-in XML Schema types only
- `SCHEMA_ID_ABSENT`: No explicit schema info - optional (use if available)

Strict mode (`WITH_STRICT(opts.enumOpt)`) always requires schema.

**Example:**
```c
parse.parseHeader(&parser, false);
switch(parser.strm.header.opts.schemaIDMode) {
    case SCHEMA_ID_SET:    /* Schema required - ID in opts.schemaID */
    case SCHEMA_ID_NIL:    /* Schemaless mode */
    case SCHEMA_ID_EMPTY:  /* Built-in types only */
    case SCHEMA_ID_ABSENT: /* Optional - use schema if available */
}
```

## API Design Patterns

EXIP provides two API styles for maximum flexibility:

### OOP-Style VTable API (Recommended)

The **preferred approach** uses global vtables that encapsulate implementation details:

```c
// Parser vtable (global const: parse)
Parser parser;
parse.initParser(&parser, buffer, app_data);
parse.parseHeader(&parser, false);
parse.setSchema(&parser, schema);
while (parse.parseNext(&parser) == EXIP_OK) { /* ... */ }
parse.destroyParser(&parser);

// Serializer vtable (global const: serialize)
EXIStream stream;
serialize.initHeader(&stream);
serialize.initStream(&stream, buffer, schema);
serialize.exiHeader(&stream);
serialize.startDocument(&stream);
serialize.startElement(&stream, qname);
serialize.stringData(&stream, value);
serialize.endElement(&stream);
serialize.endDocument(&stream);
serialize.closeEXIStream(&stream);

// Options builder vtable (global const: option)
EXIOptions* opts = parse.options(&parser);
option.strict(opts, true);
option.blockSize(opts, 5000);
option.preserve(opts, PRESERVE_COMMENTS);
```

**Benefits**:
- **Encapsulation**: Hides internal struct layout (`parser.strm.header.opts` → `parse.options()`)
- **Consistency**: Same pattern across parse/serialize/options APIs
- **Thread-safe**: Global vtables are const, instances are user-provided
- **Future-proof**: Can add functions without breaking ABI
- **Generated code**: exipb binding generator uses this pattern
- **Progressive disclosure**: Easy to drop to direct function calls if needed

**Memory cost**: ~8 bytes per vtable pointer (stored in ROM/Flash, not RAM)

### Direct Function Call API (Advanced)

For performance-critical code or embedded targets, direct function calls bypass vtable indirection:

```c
// Direct calls (same functions, no vtable)
Parser parser;
initParser(&parser, buffer, app_data);
parseHeader(&parser, false);
// ... etc
```

**When to use**:
- Ultra-tight loops where function pointer overhead matters
- Debugging/profiling (easier to trace)
- Power users who understand internal structures

**Trade-off**: Couples code to implementation details

### Macro-Based API (Expert)

For maximum control, direct struct access with helper macros:

```c
Parser parser;
initParser(&parser, buffer, app_data);
SET_STRICT(parser.strm.header.opts.enumOpt);
parser.strm.header.opts.blockSize = 5000;
```

**When to use**: Only when you need fine-grained control and accept maintenance burden

### API Progression

EXIP's API design supports progressive enhancement:

1. **Beginner**: OOP vtable (`parse.*`, `serialize.*`, `option.*`)
2. **Intermediate**: Direct function calls (`initParser()`, `parseNext()`)
3. **Advanced**: Macros + direct struct access (`SET_STRICT`, `parser.strm.header.opts`)

**Recommendation**: Start with OOP vtables. Only drop to lower levels when profiling shows a bottleneck or you need capabilities the high-level API doesn't expose.

## Architectural Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    PUBLIC API LAYER                          │
│  (include/)                                                  │
│  - EXIParser.h          - EXISerializer.h                   │
│  - contentHandler.h     - grammarGenerator.h                │
│  - procTypes.h          - errorHandle.h                     │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                  CONTENT I/O LAYER                          │
│  (src/contentIO/)                                           │
│  - EXI document parsing and serialization                   │
│  - Event-driven SAX-like interface                          │
│  - Header encoding/decoding                                 │
│  - Body encoding/decoding                                   │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────┬───────────────────────────────┐
│      GRAMMAR LAYER          │    STRING TABLE LAYER         │
│  (src/grammar/)             │    (src/stringTables/)        │
│  - EXI grammar management   │    - URI partitions           │
│  - Grammar rules/productions│    - Local name partitions    │
│  - Built-in grammars        │    - Prefix tables            │
│                             │    - Value tables             │
└─────────────────────────────┴───────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                  STREAM I/O LAYER                           │
│  (src/streamIO/)                                            │
│  - Low-level bit/byte stream operations                    │
│  - Stream encoding/decoding primitives                      │
│  - Buffer management                                        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                 COMMON UTILITIES                            │
│  (src/common/)                                              │
│  - Memory management        - Hash tables                   │
│  - Dynamic arrays          - String manipulation            │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│               GRAMMAR GENERATION (Optional)                 │
│  (src/grammarGen/)                                          │
│  - XML Schema to EXI grammar conversion                     │
│  - TreeTable intermediate representation                    │
│  - Type hierarchy resolution                                │
└─────────────────────────────────────────────────────────────┘
```

## Module Dependencies

The EXIP codebase is organized into modular components with clear dependencies:

**Runtime Library Modules:**
```
==========================================================================================
|             Module              |                      Dependencies                    |
==========================================================================================
|         common                  |                           N/A
------------------------------------------------------------------------------------------
|      streamIO                   |                         common
------------------------------------------------------------------------------------------
|     stringTables                |                         common
------------------------------------------------------------------------------------------
|        grammar                  |             common, streamIO, stringTables
------------------------------------------------------------------------------------------
|      contentIO                  |         common, streamIO, stringTables, grammar
------------------------------------------------------------------------------------------
|    grammarGen                   |   common, streamIO, stringTables, grammar, contentIO
------------------------------------------------------------------------------------------
```

**Build-Time Utilities (utils/):**
- **exipg** (schemaGen/): Grammar generator - creates static EXI grammar representations from XSD files. Used internally for EXIOptions schema.
- **exipb** (codeGen/): Binding generator - generates C structs and helper functions from XSD schemas (in development).
- **schemaLoader**: Shared utilities for loading EXI-encoded XSD files, used by both exipg and exipb.

These utilities are **not part of the runtime library** - they're development tools used during build/code generation.


**Key Points:**
- **common**: Foundation module with no dependencies - memory management, hash tables, dynamic arrays
- **streamIO** and **stringTables**: Low-level modules depending only on common utilities
- **grammar**: Depends on streamIO and stringTables for grammar rule management
- **contentIO**: Core parsing/serialization depending on grammar and string tables
- **grammarGen**: Optional module for schema processing, depends on all other modules

## Module Descriptions

### 1. Public API Layer

EXIP provides two public API layers for different use cases:

#### bindapi ([bindapi.h](include/bindapi.h)) - External Application API
**Purpose**: High-level API for applications and code generation tools using native C types

**Target Audience**:
- Binding generators (exipb)
- Application developers who want to work with standard C types
- Users who need automatic schema/schemaless mode handling

**Key Features**:
- Works with native C types: `int`, `float`, `double`, `char*`
- Hides EXIP internal types (`Integer`, `Float` struct, `String` struct)
- Automatically detects schema vs schemaless mode and routes appropriately
- No manual type conversion required

**API Functions**:
- `serializeIntValue()`: Serialize int (auto-detects schema/schemaless)
- `serializeFloatValue()`: Serialize float (auto-detects schema/schemaless)
- `serializeDoubleValue()`: Serialize double (auto-detects schema/schemaless)
- `serializeStringValue()`: Serialize C string (auto-detects schema/schemaless)
- `serializeBoolValue()`: Serialize boolean (auto-detects schema/schemaless)
- `floatToExipFloat()`: Convert C float to EXIP Float (schema mode only)
- `doubleToExipFloat()`: Convert C double to EXIP Float (schema mode only)
- `int32_to_str()`, `uint32_to_str()`: Convert integers to strings (schemaless mode)

**When to use**: Default choice for applications and generated bindings. Simplifies code and handles mode switching automatically.

#### EXISerializer ([EXISerializer.h](include/EXISerializer.h)) - Internal Low-Level API
**Purpose**: Low-level serialization API using EXIP internal types

**Target Audience**:
- Advanced users needing fine-grained control
- Performance-critical code
- Internal EXIP implementation

**Key Features**:
- Works with EXIP types: `Integer` (int64_t), `Float` struct, `String` struct
- Direct control over encoding decisions
- No automatic mode switching - caller must handle branching

**API Functions** (via `serialize` vtable):
- `serialize.intData()`: Encode Integer type
- `serialize.floatData()`: Encode Float struct (mantissa/exponent)
- `serialize.stringData()`: Encode String struct (pointer + length)
- `serialize.booleanData()`: Encode boolean
- `serialize.startElement()`, `serialize.endElement()`: Document structure
- `serialize.attribute()`: Encode attributes

**When to use**: Only when you need direct control and are willing to handle EXIP's internal type system and manual schema/schemaless branching.

**API Relationship**:
```
Application Code (native C types: int, float, char*)
         ↓
    bindapi.h (external API - auto mode switching)
         ↓
  EXISerializer.h (internal API - EXIP types)
         ↓
    Binary EXI Stream
```

#### EXIParser ([EXIParser.h](include/EXIParser.h))
**Purpose**: Parse EXI streams into application events

**Key Components**:
- `Parser` struct: Main parser state container
  - `EXIStream strm`: Binary stream being parsed
  - `ContentHandler handler`: Callback functions for events
  - `void* app_data`: User application data

**API Functions**:
- `initParser()`: Initialize parser with input buffer
- `parseHeader()`: Parse EXI header and options
- `setSchema()`: Attach schema for schema-informed parsing
- `parseNext()`: Parse next content item (event-driven)
- `pushEXIData()`: Push more data for streaming parsing
- `destroyParser()`: Clean up resources

**Usage Pattern**:
```c
Parser parser;
initParser(&parser, buffer, app_data);
parseHeader(&parser, false);
setSchema(&parser, schema);
while (parseNext(&parser) == EXIP_OK) {
    // Callbacks in handler are invoked
}
destroyParser(&parser);
```

**Recommended OOP-Style API** (Preferred):
EXIP provides vtable-based APIs that encapsulate internal structures and provide a cleaner, more maintainable interface:

```c
// OOP-style parsing (recommended)
Parser parser;
parse.initParser(&parser, buffer, app_data);
parse.parseHeader(&parser, false);
parse.setSchema(&parser, schema);
while (parse.parseNext(&parser) == EXIP_OK) {
    // Callbacks invoked
}
parse.destroyParser(&parser);
```

The global `parse` vtable provides all parser operations through function pointers. This approach:
- Hides implementation details
- Provides consistent API across parsing/serialization/options
- Is the pattern generated code and bindings should use
- Maintains backward compatibility (direct function calls still work)

See "API Design Patterns" section below for full details on the OOP pattern.


#### EXISerializer ([EXISerializer.h](include/EXISerializer.h))
**Purpose**: Serialize XML-like events into EXI format

**Key Components**:
- `EXISerializer` struct: Function pointer table for serialization
- `EXIStream`: Output stream container

**API Categories**:
1. **Document Structure**:
   - `startDocument()`, `endDocument()`
   - `startElement()`, `endElement()`
   - `attribute()`

2. **Data Content**:
   - `intData()`, `booleanData()`, `stringData()`
   - `floatData()`, `binaryData()`, `dateTimeData()`
   - `decimalData()`, `listData()`, `qnameData()`

3. **EXI Specific**:
   - `exiHeader()`: Encode EXI header
   - `selfContained()`: Mark independent elements
   - `namespaceDeclaration()`: Handle namespace prefixes

4. **EXIP Specific**:
   - `initHeader()`, `initStream()`
   - `closeEXIStream()`, `flushEXIData()`

**Fast API**: `serializeEvent()` for low-level schema-informed encoding

#### ContentHandler ([contentHandler.h](include/contentHandler.h))
**Purpose**: SAX-like callback interface for parser events

**Event Callbacks**:
- Document: `startDocument()`, `endDocument()`
- Elements: `startElement(qname)`, `endElement()`
- Attributes: `attribute(qname)`
- Data: `intData()`, `booleanData()`, `stringData()`, etc.
- Special: `nilData()`, `qnameData()`, `namespaceDeclaration()`
- Errors: `warning()`, `error()`, `fatalError()`

#### Core Types ([procTypes.h](include/procTypes.h))
**Purpose**: Fundamental data structures used throughout EXIP

**Key Type Definitions**:
- **Basic Types**:
  - `Integer`, `UnsignedInteger`: 64-bit integers
  - `Float`: EXI decimal representation (mantissa + exponent)
  - `Decimal`: Alias for Float
  - `Index`, `SmallIndex`: Size types for indexing
  - `String`: Length-prefixed string (`CharType* str`, `Index length`)

- **EXI Structures**:
  - `QName`: Qualified name (uri, localName, prefix)
  - `QNameID`: QName position in string tables (uriId, lnId)
  - `EXIPDateTime`: DateTime with fractional seconds and timezone
  - `EXIOptions`: Encoding options (alignment, compression, fidelity)
  - `EXIheader`: Header with cookie and options
  - `EXIStream`: Complete stream state

- **Grammar Types**:
  - `EventType`: Event codes (SE, EE, AT, CH, etc.)
  - `EXIType`: Value types (STRING, INTEGER, FLOAT, etc.)
  - `Production`: Grammar production rule
  - `GrammarRule`: Collection of productions
  - `EXIGrammar`: Complete grammar with rules

- **Schema Types**:
  - `SimpleType`: Simple type definition with facets
  - `EXIPSchema`: Complete schema object (grammars, types, string tables)

- **Memory Management**:
  - `AllocList`: Tracked allocations for cleanup
  - `DynArray`: Meta-data for dynamic arrays

### 2. Content I/O Layer (src/contentIO/)

#### Parser Implementation ([EXIParser.c](src/contentIO/src/EXIParser.c))
**Responsibilities**:
- Initialize parser state machine
- Coordinate header and body parsing
- Manage grammar stack during parsing
- Invoke content handler callbacks
- Handle streaming input (push parsing)

#### Serializer Implementation ([EXISerializer.c](src/contentIO/src/EXISerializer.c))
**Responsibilities**:
- Encode document structure into EXI
- Manage serialization state
- Apply grammar rules during encoding
- Handle schema-typed values
- Support streaming output

#### Body Encoding/Decoding ([bodyEncode.c](src/contentIO/src/bodyEncode.c), [bodyDecode.c](src/contentIO/src/bodyDecode.c))
**Responsibilities**:
- Encode/decode element and attribute events
- Handle character data
- Process schema-typed values
- Manage string table entries
- Apply grammar productions

#### Header Encoding/Decoding ([headerEncode.c](src/contentIO/src/headerEncode.c), [headerDecode.c](src/contentIO/src/headerDecode.c))
**Responsibilities**:
- EXI header cookie handling
- Options encoding/decoding
- Schema ID processing
- Distinguish cookie format (1.0 vs preview)

### 3. Grammar Management Layer (src/grammar/)

#### Grammar Operations ([grammars.c](src/grammar/src/grammars.c))
**Purpose**: Core grammar manipulation and traversal

**Key Operations**:
- Grammar rule creation and modification
- Production matching
- Event code calculation
- Grammar augmentation (built-in element/fragment grammars)
- String table integration

**Grammar Types**:
1. **Built-in Grammars**:
   - Document Grammar: Root grammar for documents
   - Fragment Grammar: For fragment processing
   - Element Grammar: Dynamic grammar for schema-less elements

2. **Schema-informed Grammars**:
   - Element Grammars: Per global element
   - Type Grammars: Per complex type
   - Simple Type Grammars: For schema-typed values

**Event Codes**:
Productions are organized by event code length. Event codes determine which production to use:
- Length 1: Direct array lookup (most common)
- Length 2+: Hierarchical lookup

### 4. String Tables Layer (src/stringTables/)

#### String Table Management ([sTables.c](src/stringTables/src/sTables.c))
**Purpose**: Efficient string compression via string tables

**Structure** ([sTables.h](src/stringTables/include/sTables.h)):
```
UriTable
  └─ UriEntry[] (per namespace)
      ├─ LnTable (local names in this namespace)
      │   └─ LnEntry[]
      │       ├─ lnStr (local name string)
      │       ├─ elemGrammar (index of element grammar)
      │       ├─ typeGrammar (index of type grammar)
      │       └─ VxTable (value cross-table, optional)
      ├─ PfxTable (prefixes for this namespace)
      └─ uriStr (namespace URI string)

ValueTable (global values)
  └─ ValueEntry[]
      └─ valueStr (string value)
```

**Operations**:
- Add URI/local name to tables
- Lookup string by ID (QNameID)
- Lookup ID by string (during serialization)
- Add/retrieve string values
- Value cross-table management (for compact value IDs)

**String Compaction**:
- Repeated strings are assigned integer IDs
- QNames encoded as (uriId, lnId) pair
- Values may be indexed for further compression

### 5. Stream I/O Layer (src/streamIO/)

#### Low-level Stream Operations

**Stream Reading** ([streamRead.c](src/streamIO/src/streamRead.c)):
- `readBits()`: Read arbitrary bit sequences
- `readNBitUnsignedInteger()`: Read n-bit unsigned integer
- `readUnsignedInteger()`: Read variable-length unsigned integer
- `readString()`: Read length-prefixed string

**Stream Writing** ([streamWrite.c](src/streamIO/src/streamWrite.c)):
- `writeBits()`: Write arbitrary bit sequences
- `writeNBitUnsignedInteger()`: Write n-bit unsigned integer
- `writeUnsignedInteger()`: Write variable-length unsigned integer
- `writeString()`: Write length-prefixed string

**Stream Encoding** ([streamEncode.c](src/streamIO/src/streamEncode.c)):
- Encode typed values (integers, floats, dates, etc.)
- Handle different alignment modes (bit-packed, byte-aligned)
- Apply type restrictions from schema

**Stream Decoding** ([streamDecode.c](src/streamIO/src/streamDecode.c)):
- Decode typed values
- Handle alignment padding
- Validate against schema constraints

**Utilities** ([ioUtil.c](src/streamIO/src/ioUtil.c)):
- Bit pointer manipulation
- Calculate bits needed for value range
- Buffer management (read chunks, flush)

### 6. Common Utilities (src/common/)

#### Memory Management ([memManagement.c](src/common/src/memManagement.c))
**Purpose**: Tracked memory allocation for easy cleanup

**Features**:
- `AllocList`: Maintains list of allocations
- `EXIP_MALLOC()`: Tracked allocation
- `EXIP_MFREE()`: Tracked deallocation
- `freeAllocList()`: Bulk cleanup

**Benefits**:
- Prevents memory leaks
- Simplified error handling (cleanup on error)
- Suitable for embedded systems

#### Dynamic Arrays ([dynamicArray.c](src/common/src/dynamicArray.c))
**Purpose**: Growable arrays with automatic resizing

**Structure**:
```c
struct ConcreteDynamicArray {
    DynArray dynArray;  // metadata (entrySize, chunkEntries, arrayEntries)
    EntryType* base;    // pointer to array
    Index count;        // number of used entries
};
```

**Operations**:
- `addEmptyDynEntry()`: Grow array by one entry
- `addDynEntry()`: Add entry with data
- Automatic expansion by chunks

#### Hash Tables ([hashtable.c](src/common/src/hashtable.c))
**Purpose**: Fast string lookups during serialization

**Usage**:
- Type lookups during schema processing
- Global value lookups during serialization
- Element/attribute reference resolution

**Algorithm**: DJB2 hash function (efficient for strings)

#### String Manipulation ([ASCII_stringManipulate.c](src/common/src/ASCII_stringManipulate.c))
**Purpose**: Character encoding abstraction

**Functions**:
- String comparison
- String copying
- Character validation
- Case conversion

**Extensibility**: Can be replaced for Unicode support

### 7. Grammar Generation Layer (src/grammarGen/)

#### XML Schema Processing

**TreeTable Representation** ([treeTableSchema.h](src/grammarGen/include/treeTableSchema.h)):
Intermediate structure for XML Schema parsing:

```
TreeTable
  └─ TreeTableEntry[] (schema elements)
      ├─ element (ELEMENT_ELEMENT, ELEMENT_COMPLEX_TYPE, etc.)
      ├─ attributePointers[] (name, type, ref, minOccurs, etc.)
      ├─ child → QualifiedTreeTableEntry (nested elements or type refs)
      ├─ next → TreeTableEntry (sibling elements)
      └─ supertype → QualifiedTreeTableEntry (base type for extensions)
```

**Processing Pipeline**:

1. **Build TreeTables** ([treeTableBuild.c](src/grammarGen/src/treeTableBuild.c)):
   - Parse XML Schema (in EXI or XSD format)
   - Create TreeTableEntry for each schema element
   - Store attribute values
   - Build initial string tables

2. **Resolve References** ([treeTableManipulate.c](src/grammarGen/src/treeTableManipulate.c)):
   - Link type="..." to global type definitions
   - Link ref="..." to global element declarations
   - Link base="..." to base types (inheritance)
   - Handle include/import statements
   - Process substitution groups
   - Detect recursive definitions (loopDetection)

3. **Generate Grammars** ([treeTableToGrammars.c](src/grammarGen/src/treeTableToGrammars.c)):
   - Convert TreeTable to EXIGrammar objects
   - Create productions for elements, attributes, content
   - Handle complex types, simple types, groups
   - Apply schema constraints (minOccurs, maxOccurs, choice, sequence)
   - Build complete EXIPSchema object

**Proto-Grammars** ([protoGrammars.h](src/grammarGen/include/protoGrammars.h)):
Temporary grammar representation during generation, allows easier manipulation before finalizing.

**Schema Formats Supported**:
- EXI-encoded XML Schema (most efficient)
- XSD (plain XML Schema)
- Built-in XML Schema types

## Data Flow

### Parsing Flow

```
EXI Binary Stream
      │
      ▼
┌─────────────────┐
│  Parse Header   │ → Extract EXI Options
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Load Schema    │ → Build/Load Grammars
│  (optional)     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Initialize     │ → Setup Grammar Stack
│  Parser State   │   Setup String Tables
└────────┬────────┘
         │
         ▼
    Parse Loop ─────────────────────┐
         │                          │
         ▼                          │
┌─────────────────┐                │
│  Read Event     │                │
│  Code from      │                │
│  Stream         │                │
└────────┬────────┘                │
         │                          │
         ▼                          │
┌─────────────────┐                │
│  Match Event    │                │
│  Code to        │                │
│  Production     │                │
└────────┬────────┘                │
         │                          │
         ▼                          │
┌─────────────────┐                │
│  Process Event  │ → Update String Tables  │
│  - SE: Push     │   Update Grammar Stack  │
│  - EE: Pop      │                │
│  - AT: Handle   │                │
│  - CH: Decode   │                │
└────────┬────────┘                │
         │                          │
         ▼                          │
┌─────────────────┐                │
│  Invoke Content │                │
│  Handler        │                │
│  Callback       │                │
└────────┬────────┘                │
         │                          │
         └──────────────────────────┘
         │
         ▼
   End Document
```

### Serialization Flow

```
Application Calls
      │
      ▼
┌─────────────────┐
│  Init Header    │ → Set EXI Options
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Write Header   │ → Encode Options to Stream
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Load Schema    │ → Build/Load Grammars
│  (optional)     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Init Stream    │ → Setup Grammar Stack
│                 │   Setup String Tables
└────────┬────────┘
         │
         ▼
  Serialize Loop ─────────────────┐
         │                        │
         ▼                        │
┌─────────────────┐              │
│  startElement / │              │
│  endElement /   │              │
│  attribute /    │              │
│  *Data          │              │
└────────┬────────┘              │
         │                        │
         ▼                        │
┌─────────────────┐              │
│  Find Matching  │              │
│  Production in  │              │
│  Current Grammar│              │
└────────┬────────┘              │
         │                        │
         ▼                        │
┌─────────────────┐              │
│  Write Event    │              │
│  Code to Stream │              │
└────────┬────────┘              │
         │                        │
         ▼                        │
┌─────────────────┐              │
│  Update State   │ → Add to String Tables │
│  - SE: Push     │   Update Grammar Stack │
│  - EE: Pop      │                        │
│  - AT: Handle   │                        │
│  - CH: Encode   │                        │
└────────┬────────┘                        │
         │                                  │
         └──────────────────────────────────┘
         │
         ▼
┌─────────────────┐
│  Flush Buffer   │ → Write to Output
└─────────────────┘
```

## Key Algorithms

### 1. Event Code Matching

Event codes determine which grammar production to apply:

```
For event code [2, 1, 0] with lengths [2, 3, 4]:
1. Check if code part 0 (value 2) < production count
2. If production at index (pCount - 1 - 2) has multi-part code:
   → Recursively check part 1 and part 2
3. Match found → Apply production
   → Execute event (SE, AT, etc.)
   → Transition to next grammar state
```

### 2. String Table Lookup

**During Parsing** (Decode ID → String):
```
Given QNameID (uriId=4, lnId=34):
1. Access schema.uriTable.uri[4]
2. Access uri[4].lnTable.ln[34]
3. Return ln[34].lnStr
```

**During Serialization** (Encode String → ID):
```
Given QName string ("http://example.com", "element"):
1. Search/add URI in uriTable → get uriId
2. Search/add localName in uri[uriId].lnTable → get lnId
3. Encode (uriId, lnId) as integers
```

### 3. Grammar Stack Management

```
Current: DocContent Grammar
   │
   │ SE(root) → Push ElementContent Grammar for "root"
   ▼
Current: ElementContent Grammar for "root"
   │
   │ SE(child) → Push ElementContent Grammar for "child"
   ▼
Current: ElementContent Grammar for "child"
   │
   │ EE → Pop back to "root" grammar
   ▼
Current: ElementContent Grammar for "root"
   │
   │ EE → Pop back to DocContent
   ▼
Current: DocContent Grammar
   │
   │ ED → Complete
```

### 4. Variable-Length Integer Encoding

EXI uses a variable-length unsigned integer encoding:

```
Value range          Encoding
───────────────────────────────
0-127        :  0xxxxxxx (1 byte)
128-16383    :  1xxxxxxx 0xxxxxxx (2 bytes)
16384+       :  1xxxxxxx 1xxxxxxx ... 0xxxxxxx (n bytes)

Most significant bit = continuation flag
Remaining 7 bits = data
```

### 5. Schema-Informed Type Encoding

When schema provides type information, values are encoded compactly:

**Integer with range [0, 100]**:
- Needs only 7 bits (⌈log₂(101)⌉ = 7)
- No length prefix needed

**Enumeration with 5 values**:
- Needs only 3 bits (⌈log₂(5)⌉ = 3)
- Mapped to 0-4

**List of integers**:
- Encode item count once
- Encode each item according to type

## Memory Management Strategy

### Allocation Tracking

Every allocation is tracked in `AllocList`:
```c
AllocList list;
initAllocList(&list);
void* ptr = EXIP_MALLOC(&list, size);
// ... use ptr ...
freeAllocList(&list);  // Frees all tracked allocations
```

### Hierarchical Allocation

```
EXIPSchema
  ├─ memList (AllocList)
  │    └─ All schema-level allocations
  │
  └─ grammarTable.grammar[i]
       └─ rule[j].production[] (allocated from schema memList)

EXIStream
  ├─ memList (AllocList)
  │    └─ Dynamic grammar productions
  │    └─ Grammar stack nodes
  │
  └─ valueTable.value[] (tracked separately for reuse)
```

### Dynamic Arrays

Grow in chunks to reduce reallocations:
```
Initial size: 10 entries
After 10 additions: reallocate to 20 entries
After 20 additions: reallocate to 30 entries
(chunk size is configurable per array)
```

## Configuration and Portability

### Build Configuration (exipConfig.h)

Platform-specific settings:
- `EXIP_DEBUG`: Enable/disable debugging
- `EXIP_INDEX`: Size type for indexing
- `EXIP_INTEGER`: Integer representation
- `EXIP_FLOAT`: Float representation
- `CHAR_TYPE`: Character encoding (default: ASCII)
- `DYN_ARRAY_USE`: Enable dynamic arrays
- `HASH_TABLE_USE`: Enable hash table optimizations
- `VALUE_CROSSTABLE_USE`: Enable value cross-tables

### Alignment Modes

1. **Bit-Packed** (default): Maximum compression
   - No padding between values
   - Requires bit-level operations

2. **Byte-Aligned**: Faster processing
   - Values aligned to byte boundaries
   - Slight size increase

3. **Pre-Compression**: Byte-aligned + compression ready
   - Prepared for deflate compression
   - Not fully implemented in EXIP

### Fidelity Options

Control what XML features to preserve:
- **Comments**: Preserve XML comments
- **Processing Instructions**: Preserve PIs
- **DTD**: Preserve DOCTYPE declarations
- **Prefixes**: Preserve namespace prefixes
- **Lexical Values**: Preserve exact lexical form

## Error Handling

### Error Codes ([errorHandle.h](include/errorHandle.h))

```c
enum errorCode {
    EXIP_OK = 0,                           // Success
    EXIP_NOT_IMPLEMENTED_YET = 1,          // Feature not implemented
    EXIP_UNEXPECTED_ERROR = 2,             // General error
    EXIP_HASH_TABLE_ERROR = 3,             // Hash table operation failed
    EXIP_OUT_OF_BOUND_BUFFER = 4,          // Buffer overflow
    EXIP_NULL_POINTER_REF = 5,             // Null pointer access
    EXIP_MEMORY_ALLOCATION_ERROR = 6,      // malloc() failed
    EXIP_INVALID_EXI_HEADER = 7,           // Corrupt header
    EXIP_INCONSISTENT_PROC_STATE = 8,      // State machine error
    EXIP_INVALID_EXI_INPUT = 9,            // Invalid EXI encoding
    EXIP_BUFFER_END_REACHED = 10,          // Need more input data
    EXIP_PARSING_COMPLETE = 11,            // Parsing finished successfully
    EXIP_INVALID_EXIP_CONFIGURATION = 12,  // Invalid API usage
    EXIP_HEADER_OPTIONS_MISMATCH = 15,     // Conflicting options
    EXIP_HANDLER_STOP = 16                 // User stopped parsing
};
```

### Error Handling Macros

```c
errorCode tmp_err_code;

// Propagate errors up the call stack
TRY(functionCall());

// Handle errors before propagating
TRY_CATCH(functionCall(), {
    // Cleanup code
    freeAllocList(&list);
});
```

## Performance Considerations

### Optimization Strategies

1. **Hash Tables**: Fast lookups during serialization (O(1) vs O(n))
2. **Event Code Caching**: Precomputed event codes in grammars
3. **Dynamic Array Chunking**: Reduce reallocation overhead
4. **Bit-Packed Encoding**: Minimize stream size
5. **Schema-Informed Grammars**: Eliminate ambiguity, reduce event codes

### Memory Footprint

Typical memory usage:
- **Schema Object**: 10KB - 1MB (depends on schema complexity)
- **Parser/Serializer State**: 1KB - 10KB
- **String Tables**: Grows with document size
- **Grammar Stack**: ~100 bytes per nesting level

### Performance Tuning

Compile-time options:
- Disable hash tables for small schemas (full scan faster)
- Disable value cross-tables for memory savings
- Use byte-alignment for faster bit operations
- Disable debugging for production builds

## Design Patterns

### 1. Opaque Structures with Function Tables

```c
// Public opaque structure
struct EXISerializer {
    errorCode (*startDocument)(EXIStream* strm);
    errorCode (*endDocument)(EXIStream* strm);
    // ... more function pointers
};

// Usage
extern const EXISerializer serialize;
serialize.startDocument(&stream);
```

**Benefits**: ABI stability, clear API boundary

### 2. Content Handler (Observer Pattern)

```c
struct ContentHandler {
    errorCode (*startElement)(QName qname, void* app_data);
    errorCode (*endElement)(void* app_data);
    // ... more callbacks
};
```

**Benefits**: Decouples parser from application logic

### 3. Allocator Pattern

```c
AllocList memList;
void* ptr = EXIP_MALLOC(&memList, size);
// Automatic cleanup
freeAllocList(&memList);
```

**Benefits**: Simplified resource management, no leaks

### 4. Grammar State Machine

Each grammar is a state machine:
- States = Non-terminals (grammar rules)
- Transitions = Productions (grammar productions)
- Input = Event codes

**Benefits**: Formal specification compliance, extensibility

## Testing and Validation

### Test Coverage

EXIP includes comprehensive tests:
- Unit tests per module
- Integration tests for parsing/serialization
- Conformance tests against EXI spec
- Schema-informed vs schema-less tests
- Interoperability tests with other EXI processors

### Debugging Support

When `EXIP_DEBUG` is ON:
- Detailed logging of stream I/O
- Grammar rule traversal traces
- Event code details
- Memory allocation tracking

Example debug output:
```
>RULE
NT-1:
    SE (qname: 4:34)    NT-2    0   description
    SE (qname: 4:75)    NT-3    1   notes
>> 0x0 (3 bits)  @8:4
Write string, len 3: new
>> 0x6E  @9:6  @10:5
```

## Future Enhancements

Potential areas for extension:
1. **Full Compression Support**: Implement pre-compression and compression modes
2. **Unicode Support**: Replace ASCII string manipulation
3. **Parallel Processing**: Thread-safe parsing/serialization
4. **Streaming Optimizations**: Zero-copy buffer handling
5. **Validation**: Runtime schema validation (partially implemented)
6. **Restricted Character Sets**: Parse XSD pattern facets (regex) to derive and apply restricted character set encodings for improved compression. This includes validation of pattern facets in strict mode (e.g., email patterns, alphanumeric constraints)
7. **Hex and Binary Encoding Utilities**: Add built-in support for hexBinary and base64Binary encoding/decoding as a separate optional utility library (`libexip-utils` or `libexip-xml`). This would support EXI→XML text conversion (e.g., `exipd -text` output) and server-side use cases (Python/Scala bindings for AI/storage workflows) without adding bloat to embedded deployments. Core `libexip` remains zero-dependency; utilities are only linked when XML text I/O is needed

## References

### EXI Specification
- W3C EXI 1.0: https://www.w3.org/TR/exi/
- EXI Primer: https://www.w3.org/TR/exi-primer/

### Related Technologies
- XML: https://www.w3.org/XML/
- XML Schema: https://www.w3.org/XML/Schema
- SAX API: https://www.saxproject.org/

### EXIP Resources
- Project Website: https://exip.sourceforge.net/
- Source Repository: (see LICENSE.txt for details)
- License: BSD 3-Clause

---

**Document Version**: 1.0
**Last Updated**: 2024
**EXIP Version**: 0.5+
