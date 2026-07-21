## Sequence maxOccurs="unbounded" Test

Attempted reproducer for a grammar collision issue found in CCSDS NDM/XML 4.0 schemas.

### Real-World Issue

CCSDS NDM/XML 4.0 schemas trigger a grammar collision error during schema generation:

```
> Grammar collision: duplicate adjacent elements in sequence not implemented yet at genUtils.c, line 269
```

The schemas return `EXIP_NOT_IMPLEMENTED_YET` (error code 1) and cannot be loaded.

### Reproducer Status

Incomplete - the minimal schema here does **not** trigger the collision. The `test_sequence_unbounded` test in `check_strict_grammar` currently passes (schema loads successfully with `EXIP_OK`).

### Test Files

- `test.xsd` - Minimal schema attempting to reproduce the pattern
- `test.xsd.exi` - EXI-encoded schema
- `test.xml` - Sample data
- `test.xml.exi` - EXI-encoded data

### Full Test Command

Currently the test mentioned above does not decode the `test.xml.exi` file but we can test via the following command which could be added to examples which serves as additional tests.

```sh
./build/vs2022/Debug/exipd.exe -xml -schema=tests/test-set/sequenceUnbounded/test.xsd.exi tests/test-set/sequenceUnbounded/test.xml.exi
```
