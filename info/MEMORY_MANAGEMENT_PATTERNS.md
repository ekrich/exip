# Memory Management Patterns in EXIP

## Overview

The EXIP library uses a managed memory allocation system to simplify memory management and prevent leaks. All memory allocations are tracked in `AllocList` structures and freed in bulk at the end of processing.

## Core Concepts

### AllocList Structure

The `AllocList` is a linked list that tracks all memory allocations for a particular context (EXI stream, schema, etc.). When processing completes or an error occurs, all tracked memory is freed in a single operation.

### Key Functions

Located in `exip/src/common/include/memManagement.h`:

```c
// Initialize an allocation list
errorCode initAllocList(AllocList* list);

// Allocate memory and track it in the list
void* memManagedAllocate(AllocList* list, size_t size);

// Free all memory tracked in a stream's allocation list
void freeAllMem(EXIStream* strm);

// Free all memory in a specific allocation list
void freeAllocList(AllocList* list);
```

## Standard Pattern

### ✅ CORRECT: Using Managed Allocation

Functions that allocate memory should accept an `AllocList*` parameter and use `memManagedAllocate()`:

```c
errorCode allocateStringMemoryManaged(CharType** str, Index UCSchars, AllocList* memList)
{
    (*str) = (CharType*) memManagedAllocate(memList, sizeof(CharType)*UCSchars);
    if ((*str) == NULL)
        return EXIP_MEMORY_ALLOCATION_ERROR;
    return EXIP_OK;
}
```

**Examples in codebase:**
- `exip/src/common/src/ASCII_stringManipulate.c:36` - `allocateStringMemoryManaged()`
- `exip/src/grammar/src/grammars.c:82` - Grammar rule allocation
- `exip/src/contentIO/src/EXIParser.c:116` - Schema allocation

### ✅ Usage Pattern

```c
// Initialize the allocation list
TRY(initAllocList(&parser->strm.memList));

// Allocate memory - tracked automatically
parser->strm.schema = memManagedAllocate(&parser->strm.memList, sizeof(EXIPSchema));

// ... processing ...

// Free everything at once - no individual frees needed
freeAllMem(&parser->strm);
```

## Guidelines for New Code

1. **Always use `memManagedAllocate()`** for allocations that need to persist beyond function scope
2. **Pass `AllocList*` parameter** to any function that allocates memory
3. **Never use `EXIP_MALLOC()` directly** unless there's a documented exceptional reason
4. **Never require callers to `EXIP_MFREE()`** individual allocations
5. **Use the appropriate AllocList:**
   - `&strm->memList` for EXI stream-related allocations
   - `&schema->memList` for schema-related allocations
   - Context-specific lists for other operations

## Common AllocList Locations

```c
// Parser context
parser->strm.memList

// Serializer context
strm->memList

// Schema context
schema->memList

// Tree table context
treeT->memList
```

## Benefits of Managed Allocation

1. **Automatic cleanup** - All memory freed in one operation
2. **Exception safety** - Error paths automatically clean up
3. **No manual tracking** - System tracks allocations
4. **Leak prevention** - Nothing falls through the cracks
5. **Simpler code** - No scattered free() calls
6. **Consistent pattern** - All code follows same approach

## When NOT to Use Managed Allocation

Use direct `EXIP_MALLOC()`/`EXIP_MFREE()` only for:

- **Temporary allocations** within a single function that are freed before return
- **Fixed-lifetime objects** with clear ownership (document why)
- **Performance-critical paths** where the overhead matters (rare, document why)

**Always document exceptions to the standard pattern.**

## Known Exceptions in Codebase

### Internal Data Structures (Acceptable)

These use direct `EXIP_MALLOC()` for internal system structures with clear lifetime management:

- **`hashtable.c`** - Hash table nodes and bucket arrays
  - Lines: 56, 58, 98, 163
  - Rationale: Internal data structure with explicit destroy function

- **`dynamicArray.c`** - Dynamic array expansion
  - Lines: 43, 73
  - Rationale: Internal resizable array implementation

- **`grammars.c`** - Grammar production arrays
  - Lines: 133, 153, 174, 216
  - Rationale: Performance-critical grammar compilation

- **`procTypes.c`** - Stack node allocation
  - Line: 80
  - Rationale: Internal stack implementation with push/pop

- **`memManagement.c`** - AllocList block allocation
  - Lines: 25, 37, 42
  - Rationale: Core memory management infrastructure

### Temporary Allocations (Acceptable)

**`streamIO/src/streamDecode.c:126-146`** - Binary data decoding:

```c
errorCode decodeBinary(EXIStream* strm, char** binary_val, Index* nbytes)
{
    TRY(decodeUnsignedInteger(strm, &length));
    *nbytes = (Index) length;
    (*binary_val) = (char*) EXIP_MALLOC(length);  // Line 136
    if((*binary_val) == NULL)
        return EXIP_MEMORY_ALLOCATION_ERROR;

    for(i = 0; i < length; i++)
    {
        TRY_CATCH(readBits(strm, 8, &int_val), EXIP_MFREE(*binary_val));
        (*binary_val)[i]=(char) int_val;
    }
    return EXIP_OK;
}
```

**Usage pattern** (bodyDecode.c:1118-1126):
```c
TRY(decodeBinary(strm, &binary_val, &nbytes));
TRY_CATCH(handler->binaryData(binary_val, nbytes, app_data), EXIP_MFREE(binary_val));
EXIP_MFREE(binary_val);  // Freed in same function scope
```

**Exception safety:**
- If `decodeBinary()` fails: `TRY` macro returns immediately, line 1126 never executes (no free needed)
- If `readBits()` fails inside `decodeBinary()`: `TRY_CATCH` at line 142 frees memory before returning
- If handler fails: `TRY_CATCH` at line 1122 frees memory before returning
- If all succeed: Normal free at line 1126

**Rationale:** Binary data (images, files, large byte arrays) could be megabytes in size. The `decodeBinary()` function in `streamDecode.c` allocates this buffer temporarily, and the caller in `bodyDecode.c` immediately frees it after the handler processes the data. This prevents memory bloat - using managed allocation would retain large buffers until stream destruction. Exception-safe pattern using TRY/TRY_CATCH macros ensures cleanup on all code paths.

### Legacy Non-Managed Functions (Should be Deprecated)

**`ASCII_stringManipulate.c`** has duplicate functions with managed and non-managed versions:

- ❌ **`allocateStringMemory()` (line 28)** - Uses `EXIP_MALLOC()` directly
  - ✅ Use `allocateStringMemoryManaged()` (line 36) instead

- ❌ **`cloneString()` (line 160)** - Uses `EXIP_MALLOC()` directly
  - ✅ Use `cloneStringManaged()` (line 172) instead

**Recommendation:** Mark non-managed versions as deprecated and update all callers.

