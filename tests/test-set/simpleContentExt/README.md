## Sequence maxOccurs="unbounded" Test

Attempted reproducer for a grammar collision issue found in CCSDS NDM/XML 4.0 schemas.

### Original Issue (Not Reproduced)

CCSDS NDM/XML 4.0 schemas trigger a grammar collision error during schema generation:

```
> Grammar collision: duplicate adjacent elements in sequence not implemented yet at genUtils.c, line 269
```

The schemas return `EXIP_NOT_IMPLEMENTED_YET` (error code 1) and cannot be loaded.

**Reproducer Status:** Incomplete - the minimal schema here does not trigger the collision. Schema loads successfully with `EXIP_OK`.

### Issue Actually Found and Fixed

While attempting to reproduce the sequence unbounded issue, a **different bug was discovered**:

**Problem:** Schema loaded successfully but schema-informed decoding failed on empty elements with simpleContent extensions.

**Symptoms:**
- Schema generation: EXIP_OK
- Schemaless decoding: Works fine
- Schema-informed decoding: Failed with buffer underrun after empty element `<USER_DEFINED parameter="DECAY_DATE"/>`

**Root Cause:** The `isContent2Grammar` logic in `bodyDecode.c` only handled the case where `currNonTermID == contentIndex == 0`. For simpleContent extensions where content is at NT-1, the equality case `1 == 1` was missing.

**Fix:** Added general equality check in `bodyDecode.c` to handle `currNonTermID == contentIndex` for any value, ensuring proper second-level production handling.

### Test Files

- `test.xsd` - Minimal schema attempting to reproduce the pattern
- `test.xsd.exi` - EXI-encoded schema
- `test.xml` - Sample data
- `test.xml.exi` - EXI-encoded data (schema-informed)
- `test.xml.sl.exi` - EXI-encoded data (schemaless)

### Full Test Command

Currently the test mentioned above does not decode the `test.xml.exi` file but we can test via the following command which could be added to examples which serves as additional tests.

**Windows:**
Schema-informed and schemaless respectively.

```sh
./build/vs2022/Debug/exipd.exe -xml -schema=tests/test-set/simpleContentExt/test.xsd.exi tests/test-set/simpleContentExt/test.xml.exi

./build/vs2022/Debug/exipd.exe -xml tests/test-set/simpleContentExt/test.xml.sl.exi
```

**Linux/macOS:**

```sh
./bin/examples/exipd -xml -schema=tests/test-set/simpleContentExt/test.xsd.exi tests/test-set/simpleContentExt/test.xml.exi
```

**Grammar:**

Use the path above for Windows, just replace to `exipg.exe`.
```sh
./bin/utils/exipg -text -schema=tests/test-set/simpleContentExt/test.xsd.exi
```
