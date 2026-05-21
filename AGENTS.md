# AI Agent Guidelines for EXIP

This document provides guidelines for AI assistants (Claude, GPT, Copilot, etc.) contributing to the EXIP project.

## Project Overview

**EXIP** (Embeddable EXI Processor in C) is a C library for parsing and serializing EXI (Efficient XML Interchange) streams. It's designed for embedded systems with emphasis on low memory footprint and portability.

**License**: Apache 2.0 (see [LICENSE.txt](LICENSE.txt))

## Core Principles

### 1. EXI-First Architecture

- ✅ **Use EXI-encoded formats** for all schema processing
- ❌ **Do NOT add XML parsers** - EXIP is EXI-only by design
- ✅ Schemas must be EXI-encoded with `Preserve.prefixes` enabled
- ✅ Use existing `TreeTable` infrastructure from `src/grammarGen/`

### 2. Schema Format Support

**Supported:**
- ✅ **XSD (XML Schema Definition)** - ONLY this format
- Must be EXI-encoded using external tools:
  - **EXIficient** (Java) - Recommended, actively maintained
  - **OpenEXI** (Java) - Mentioned in EXIP documentation

**Explicitly Out of Scope:**
- ❌ **DTD** (Document Type Definition) - Will never be supported
- ❌ **RELAX NG** (RNG/RNC) - Not needed, adds unnecessary complexity
- ❌ Plain XML parsing - Use EXI format only

### 3. Dual Mode Support

All features must support **BOTH** processing modes:

1. **Schema-informed Mode**
   - Uses XML Schema for optimized encoding
   - Typed data handlers (intData, boolData, etc.)
   - Better compression and performance

2. **Schemaless Mode**
   - No schema required
   - String-only data handlers
   - More flexible but less efficient

**Critical**: Never design features that only work in one mode. Both modes must be first-class citizens.

## Coding Standards

### File Headers

All new source files should include the standard EXIP header:

```c
/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Licensed under the Apache License, Version 2.0            |
|  The full license terms and conditions are located in LICENSE.txt  |
\===================================================================*/
```

**Note**: Some older files reference "BSD 3-Clause License" in headers. The project transitioned to Apache 2.0 for explicit patent grant protections. All new code should use the Apache 2.0 header above, as specified in LICENSE.txt.

### Naming Conventions

- **Tools**: Prefix with `exip` (e.g., `exipg`, `exipd`, `exipe`)
- **Functions**: camelCase (e.g., `initParser`, `parseNext`)
- **Types**: PascalCase (e.g., `EXIStream`, `Parser`)
- **Struct members**: snake_case (e.g., `app_data`, `has_options`, `version_number`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `EXIP_OK`, `INDEX_MAX`)
- **Macros**: UPPER_SNAKE_CASE (e.g., `SET_STRICT`, `WITH_COMPRESSION`)

### File Organization

```
include/          - Public API headers
src/
  ├── common/     - Utilities (memory, hash tables, dynamic arrays)
  ├── contentIO/  - Parser and serializer implementations
  ├── grammar/    - Grammar management
  ├── grammarGen/ - Schema to grammar conversion
  ├── streamIO/   - Low-level bit/byte operations
  └── stringTables/ - String table management
examples/         - Sample code
docs/user/        - Public user documentation (TeX format)
info/             - Internal project documentation (Markdown)
scripts/          - various script to run locally or via CI
```

### Error Handling

```c
errorCode tmp_err_code = EXIP_UNEXPECTED_ERROR;

// Propagate errors
TRY(functionCall());

// Handle errors before propagating
TRY_CATCH(functionCall(), {
    // Cleanup code
    freeAllocList(&list);
});
```

### Memory Management

- Use `AllocList` for tracked allocations
- Use `EXIP_MALLOC()` and `EXIP_FREE()` macros
- Clean up with `freeAllocList()` on error paths
- Dynamic arrays grow in chunks (configured per array)

### String Handling

- Strings are length-prefixed: `struct String { CharType* str; Index length; }`
- Default implementation is ASCII (can be replaced for Unicode)
- Use `asciiToString()` for conversions
- Never assume null-termination unless explicitly created

## Architecture Patterns

### 1. Content Handler Pattern (Observer)

```c
// Parser invokes callbacks
ContentHandler handler;
handler.startElement = my_startElement;
handler.stringData = my_stringData;
parser.handler = handler;
```

### 2. Function Pointer Tables

```c
// Serializer API
const EXISerializer serialize;
serialize.startDocument(&stream);
serialize.startElement(&stream, qname, &valueType);
```

### 3. Grammar State Machine

- States = Non-terminals (grammar rules)
- Transitions = Productions
- Input = Event codes

### 4. Tracked Memory Allocation

```c
AllocList memList;
initAllocList(&memList);
void* ptr = EXIP_MALLOC(&memList, size);
// ... use ptr ...
freeAllocList(&memList);  // Frees all tracked allocations
```

## Common Patterns

### Unified Data Handling (Schema + Schemaless)

```c
// Generic data value
typedef union {
    Integer intVal;
    bool boolVal;
    String stringVal;
} DataValue;

// Single receiver handles both modes
errorCode receiveData(const char* elementName, DataType type,
                     DataValue value, void* app_data) {
    if(type == DATA_TYPE_INT) {
        target->field = (int)value.intVal;  // Schema mode
    } else if(type == DATA_TYPE_STRING) {
        target->field = atoi(value.stringVal.str);  // Schemaless mode
    }
}
```

### Detecting Schema Mode

```c
switch(parser.strm.header.opts.schemaIDMode) {
    case SCHEMA_ID_NIL:     // Explicitly schemaless
    case SCHEMA_ID_EMPTY:   // Built-in types only
    case SCHEMA_ID_SET:     // Schema required
    case SCHEMA_ID_ABSENT:  // Optional (check strict flag)
}
```

## Performance Considerations

- Hash tables for O(1) lookups (schema processing, serialization)
- Bit-packed encoding minimizes size
- Event code caching in grammars
- Dynamic array chunking reduces reallocations
- Zero-copy where possible

## Testing Guidelines

- Test both schema-informed AND schemaless modes
- Test with and without strict mode
- Test with byte-aligned and bit-packed encodings
- Verify interoperability with other EXI processors
- Include examples in `examples/` directory

## Documentation Requirements

When adding features, update:

1. **Code comments** - Doxygen style for public APIs
2. **ARCHITECTURE.md** - If architecture changes
3. **Examples** - Add working sample code
4. **User guides** - If user-facing functionality

### Documentation Style

- Use markdown for all documentation
- Include working code examples
- Link between related documents
- Keep ARCHITECTURE.md updated with design decisions

## Build System

- Primary: Make (cross-platform)
- Secondary: Visual Studio projects (Windows)
- Support: gcc, clang, MSVC compilers
- Target: C99 standard
- Platform: Windows, Linux, macOS, embedded

## Common Pitfalls to Avoid

### ❌ Don't:
- Add XML parsing capabilities
- Support DTD or RELAX NG
- Create schema-only or schemaless-only features
- Expect `exipe` to encode arbitrary XML (it's a fixed test fixture)
- Manually edit generated files (staticEXIOptions.c, staticXmlSchema.c)
- Use unbounded allocations
- Assume null-terminated strings
- Hardcode buffer sizes in public APIs
- Break ABI compatibility without major version bump

### ✅ Do:
- Reuse existing `TreeTable` for schema processing
- Support both schema modes equally
- Use tracked memory allocation
- Handle both bit-packed and byte-aligned encodings
- Test with real-world schemas
- Profile memory usage for embedded targets
- Document EXI-specific concepts

## Key Files to Reference

- [ARCHITECTURE.md](info/ARCHITECTURE.md) - System architecture and design
- [info/](info/) - Additional design documents and patterns
- [examples/README.md](examples/README.md) - Example file organization and usage
- [src/contentIO/README.md](src/contentIO/README.md) - Regenerating staticEXIOptions.c
- [src/grammarGen/README.md](src/grammarGen/README.md) - Regenerating staticXmlSchema.c

### Scripts
- [scripts/run-examples](scripts/run-examples) - Run functional examples
- [scripts/run-valgrind-tests](scripts/run-valgrind-tests) - Memory leak testing
- [scripts/run-valgrind-examples](scripts/run-valgrind-examples) - Profile examples

### Examples
- [examples/simpleDecoding/exipd.c](examples/simpleDecoding/exipd.c) - Decoder (arbitrary input)
- [examples/simpleEncoding/exipe.c](examples/simpleEncoding/exipe.c) - Encoder (fixed test document)

## External Tools

### Schema Encoding Tools
Use these to encode XSD schemas to EXI format:

- **EXIficient** (Java) - https://github.com/EXIficient/exificient
  - Recommended: Active, well-maintained
  - Usage: `exificient -schema schema.xsd -preservePrefixes > schema.xsd.exi`

- **OpenEXI** (Java)
  - Mentioned in original EXIP documentation
  - Usage: `java -jar OpenEXIGUI.jar` (GUI) or command-line interface

### Testing Tools
- EXIP's own `exipd` and `exipe` utilities
- EXIficient test suite for interoperability
- W3C EXI test suite

## Version Strategy

- **Version**: 0.5+ (see headers)
- **API Stability**: Public API in `include/` is stable
- **Internal Changes**: Internal headers may change between minor versions
- **Breaking Changes**: Require major version bump

## Community

- **License**: Apache 2.0 - permissive for commercial use with patent grant protection
- **Issues**: Report via project issue tracker
- **Contributions**: Follow coding standards, test both modes
- **Discussion**: Reference W3C EXI specification for clarification

## W3C EXI Specification References

- **Specification**: https://www.w3.org/TR/exi/
- **Primer**: https://www.w3.org/TR/exi-primer/
- **Schema**: https://www.w3.org/XML/Schema

## Quick Reference

### Decision Matrix

| Question | Answer |
|----------|--------|
| XML parsing? | ❌ No - Use EXI only |
| DTD support? | ❌ No - Out of scope |
| RELAX NG support? | ❌ No - Out of scope |
| XSD support? | ✅ Yes - EXI-encoded only |
| Schema mode only? | ❌ No - Support both modes |
| Schemaless mode only? | ❌ No - Support both modes |
| Add new tool? | ✅ Yes - Prefix with `exip` |
| External dependencies? | ❌ Minimize - Embedded focus |
| Unicode? | ⚠️ Limited - ASCII default, extensible |
| Thread-safe? | ⚠️ Not currently - Future enhancement |

---

**Last Updated**: 2026
**For EXIP Version**: 0.5+
