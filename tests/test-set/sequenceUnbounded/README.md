# Sequence maxOccurs="unbounded" Test

This test documents a known EXIP limitation with `<sequence maxOccurs="unbounded">` patterns.

## Issue

EXIP currently triggers an assertion at `src/grammarGen/src/genUtils.c:268` when processing schemas containing:

```xml
<xsd:sequence maxOccurs="unbounded">
   <xsd:element .../>
   <xsd:element .../>
</xsd:sequence>
```

This pattern is valid W3C XML Schema but EXIP's collision detection is too strict.

## Test Files

- `test.xsd` - Minimal schema demonstrating the pattern
- `test.xml` - Sample data exercising the schema
- `check_sequence_unbounded.c` - Test code (in `tests/` directory)

## Pattern

The schema represents: `(item? value?)*`

This allows:
- Zero or more sequence iterations
- Each iteration can have 0-1 item and 0-1 value
- Items can appear after values across sequence boundaries

## Expected Behavior

**Current:** Assertion failure at genUtils.c:268

**Once fixed:** Grammar generation should succeed (EXIP_OK)

## Why This Matters

1. CCSDS NDM/XML 4.0 specification uses this pattern
2. Pattern is memory-efficient (loop-back edges, no rule explosion)
3. Infrastructure already exists (same as element-level maxOccurs="unbounded")
4. Fix is likely relaxing overly-strict validation

## Running the Test

```bash
cd build/vs2022/Debug  # or Release
./check_sequence_unbounded.exe ../../tests/test-set
```

Expected: Currently fails with assertion. Once fixed, should pass.
