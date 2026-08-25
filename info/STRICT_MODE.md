# EXI Strict Mode in EXIP

## Overview

**Strict mode** is one of three EXI fidelity options (strict, default, preserve) that determines how closely the EXI encoding adheres to XML Schema constraints. This document explains strict mode's requirements, implications for grammar generation, and current EXIP conformance.

## EXI Fidelity Levels

### Strict Mode (fidelity: strict)
- **Schema required** - Cannot encode without a schema
- **No schema deviations** - Content must match schema exactly
- **No wildcards** - AT(*) and SE(*) productions forbidden
- **Type-specific encoding** - Values encoded according to schema types
- **Smallest size** - Most compact encoding possible
- **Event codes fully deterministic** - Encoder/decoder must agree exactly

### Default Mode
- Schema optional
- AT(*) and SE(*) wildcards allowed
- Can handle unexpected elements/attributes
- Slightly larger than strict

### Preserve Mode (fidelity: preserve)
- Preserves XML information items (comments, PIs, DTDs)
- Largest encoding size
- Maximum fidelity to original XML

## Strict Mode Requirements

### 1. Schema Validation
All content must conform to the provided XML Schema:
- Element names must exist in schema
- Attributes must match schema definitions
- Content types must be valid per schema
- Occurrence constraints (minOccurs, maxOccurs) enforced

### 2. Grammar Structure
Grammars contain **only** schema-defined productions:
```
NT-0: SE(element1) → NT-1    // Schema-defined element
      SE(element2) → NT-2    // Schema-defined element
      EE                      // End element
      
No AT(*) or SE(*) wildcards allowed
```

### 3. Event Code Determinism
Event codes must be **identical** between encoder and decoder:
- Production ordering must follow EXI spec exactly
- Schema order for SE(qname) productions (Section 8.5.4.3)
- QName lexical order within same event type
- Grammar rule indices must match

## Schema Order Requirement (Critical)

### EXI Specification Section 8.5.4.3

When multiple productions of the same event type exist in a grammar rule, they must be ordered:

**For SE(qname) and SE(uri:*) productions:**
1. **Schema order first** - Elements/types ordered by declaration sequence in XSD
2. **Lexical order second** - If schema order cannot determine (different namespaces), use QName comparison

**For AT(qname) productions:**
- QName lexical order only (URI then local-name)

### Example

Given this schema:
```xml
<xs:schema>
  <xs:element name="zebra" type="xs:string"/>   <!-- declared 1st -->
  <xs:element name="apple" type="xs:string"/>   <!-- declared 2nd -->
  <xs:element name="mango" type="xs:string"/>   <!-- declared 3rd -->
</xs:schema>
```

**Correct event code assignment** (by schema order):
```
Document Grammar Rule:
  SE(zebra)  → event code 0  (1 bit:  0)
  SE(apple)  → event code 1  (2 bits: 10)
  SE(mango)  → event code 2  (2 bits: 11)
```

**Incorrect** (by QName lexical order):
```
Document Grammar Rule:
  SE(apple)  → event code 0  (1 bit:  0)
  SE(mango)  → event code 1  (2 bits: 10)
  SE(zebra)  → event code 2  (2 bits: 11)
```

### Why Schema Order Matters

**Encoding `<zebra>` root element:**

With correct ordering:
- Encoder finds zebra at position 0
- Encodes event code `0` (1 bit)
- Decoder reads `0`, selects position 0 → zebra ✓

With incorrect ordering (EXIP current behavior):
- Encoder finds zebra at position 2 (alphabetically)
- Encodes event code `11` (2 bits)
- Decoder reads `11`, selects position 2 → mango ✗

**Result: Silent data corruption** - Wrong element decoded, no error reported.

## EXIP Conformance Status

### Current Behavior

EXIP uses **QName lexical ordering** for all production sorting:

```c
// src/grammarGen/src/treeTableToGrammars.c
static void assignCodes(ProtoGrammar* grammar)
{
    for (i = 0; i < grammar->count; i++)
    {
        qsort(grammar->rule[i].prod, grammar->rule[i].count, 
              sizeof(Production), compareProductions);
    }
}

static int compareProductions(const void* prod1, const void* prod2)
{
    // ... event type comparison ...
    
    // For SE(qname), uses QName comparison (URI then LN)
    return compareQNameID(&(p1->qnameId), &(p2->qnameId), 
                          &globalSchemaPtr->uriTable);
}
```

This **violates EXI spec** when schema declaration order differs from lexical order.

### Impact

**Strict mode interoperability:**
- ✗ Cannot decode strict mode EXI from other implementations
- ✗ Other implementations cannot decode EXIP strict mode EXI  
- ✗ Round-trip through different implementations fails
- ✓ EXIP-to-EXIP works (same ordering bug)
- ✓ Schemas with alphabetically-ordered declarations work

**Non-strict modes:**
- Mostly unaffected (wildcards provide fallbacks)
- Event codes may differ but decoding succeeds

### Detection

To check if a schema triggers non-conformance:

```bash
# Extract global element names in declaration order
grep '<xs:element name=' schema.xsd | sed 's/.*name="\([^"]*\)".*/\1/'

# If output is not alphabetically sorted, EXIP will mis-encode
```

## Required Fix: Schema Order Tracking

**See detailed design analysis and performance comparison below.**

### Recommended Solution: Store Schema Order in Production

Instead of a naive approach with O(N) linear search during every comparison, **store schema order directly in Production struct**:

**Benefits**:
- **Best performance**: O(M) lookups instead of O(M log M × N)
- **Simplest code**: compareProductions just compares integers
- **qsort compatible**: No need for insertion sort
- **Small memory cost**: 4 bytes/production (~2-20 KB typical)

### Solution Architecture

**1. Add Schema Order Field to Production:**
```c
struct Production
{
    uint32_t content;      // Event type + non-terminal
    Index typeId;
    QNameID qnameId;
    Index schemaOrder;     // NEW: Schema declaration order
};
```

**2. Schema Order Map with Hash Table (O(1) lookup):**

EXIP already has hash table infrastructure (see `stringManipulate.c`):

```c
struct SchemaOrderEntry
{
    QNameID qnameId;
    Index schemaOrder;
};

struct SchemaOrderMap
{
    DynArray dynArray;
    SchemaOrderEntry *entries;
    Index count;
    Index *hashTable;        // Hash table for O(1) lookup
    Index hashTableSize;
};

// Hash function (using EXIP's existing hash pattern)
static Index hashQNameID(QNameID qname, Index tableSize)
{
    // Similar to EXIP's string hashing (31 multiplier pattern)
    return ((qname.uriId * 31) + qname.lnId) % tableSize;
}
```

**3. Track Order During Schema Traversal:**
```c
Index globalEntryIndex = 0;

for(i = 0; i < schemaFileCount; i++)
    for(j = 0; j < treeTable[i].count; j++)
    {
        qEntry.globalSchemaEntryIndex = globalEntryIndex++;
        
        switch(qEntry.entry->element)
        {
            case ELEMENT_ELEMENT:
                handleElementEl(&ctx, &qEntry, ...);
                // Records qnameId + globalSchemaEntryIndex
                break;
            // ... other global declarations ...
        }
    }
```

**3. Populate Schema Order When Creating Productions:**
```c
// In addProduction() or when creating SE(qname)
if (eventType == EVENT_SE_QNAME)
{
    newProd->schemaOrder = hashLookupSchemaOrder(&map, qnameID);
}
else
{
    newProd->schemaOrder = INDEX_MAX;  // Not applicable
}
```

**4. Use Schema Order in Production Comparison:**
```c
static int compareProductions(const void* prod1, const void* prod2)
{
    Production* p1 = (Production*) prod1;
    Production* p2 = (Production*) prod2;
    
    // ... compare event types first ...
    
    if(GET_PROD_EXI_EVENT(p1->content) == EVENT_SE_QNAME)
    {
        // Compare stored schema order (no lookup needed)
        if(p1->schemaOrder < p2->schemaOrder)
            return -1;  // p1 comes first
        else if(p1->schemaOrder > p2->schemaOrder)
            return 1;   // p2 comes first
        
        // Fallback to QName comparison if schema order unavailable
        return compareQNameID(&p1->qnameId, &p2->qnameId, ...);
    }
    
    // Other event types use QName comparison
    return compareQNameID(&p1->qnameId, &p2->qnameId, ...);
}
```

**5. Keep qsort (No Insertion Sort Needed):**
```c
// Sort productions with correct schema order
qsort(grammar->rule[i].prod, grammar->rule[i].count, 
      sizeof(Production), compareProductions);
```

### Memory Overhead

**Build-time only** (exipg execution):
- Production struct: +4 bytes/production during grammar generation
- SchemaOrderMap: 12 bytes/element + 6 bytes/element hash table = 18 bytes/element
- Example: 100 elements, 500 productions = 1.8 KB map + 2 KB productions = 3.8 KB total
- **All freed after grammar generation**

**Grammar file (.exi)**: 
- schemaOrder field **NOT serialized** to file
- Productions written in sorted order (order is implicit)
- **Zero increase in file size**

**Runtime (embedded device)**:
- Production struct remains 12 bytes (no change from current EXIP)
- Productions loaded from .exi file already sorted
- **Zero runtime memory overhead**
- **Embedded systems completely unaffected!**

This approach is **ideal for embedded**:
- Pay the cost on powerful build machine (desktop/server running exipg)
- Embedded device gets smallest possible grammar files
- No runtime memory penalty

**Performance comparison**:
- Naive linear search: O(M log M × N) with O(N) linear search per comparison
- Recommended approach: O(M) with O(1) hash lookup per production creation
- **900-1000× faster for large schemas**

### Why Better Than Linear Search Approach

A naive approach using linear search has severe performance issues:
```c
// Naive approach: O(N) linear search during EVERY comparison
for (int i = 0; i < table->count; i++)  // O(N)
{
    if (entry->qnameId matches p1->qnameId)
        p1_index = entry->schemaOrder;
}
```

**Example**: 1000 elements, 500 productions:
- Comparisons during sort: 500 × log₂(500) ≈ 4,500
- Each comparison: O(1000) linear search
- **Total: 4.5 million operations!**

**Recommended approach**: Store order in Production (one lookup per production):
- Populate: 500 × O(1) hash lookup = 500 operations  
- Sort: 4,500 comparisons, no lookups
- **Total: 5,000 operations** (900× faster!)

See [SCHEMA_ORDER_ALTERNATIVES.md](../SCHEMA_ORDER_ALTERNATIVES.md) for detailed comparison of:
1. SchemaOrderMap (better naming)
2. Hash table for O(1) lookup
3. Storing order in Production (recommended)

---

## Alternative: Store in Existing Fields (If Memory Critical)

If the 4 bytes/production runtime cost is unacceptable, pack into existing fields:

```c
struct Production
{
    uint32_t content;
    Index typeId;
    QNameID qnameId;  // Can pack schema order into lnId upper bits
};
```

**Rejected because:**
- Premature optimization
- 4 bytes/production is negligible (2-20 KB typical)
- Bit packing reduces code clarity
- Limited to 65536 elements

**Start with clean solution** - add schemaOrder field. Optimize only if profiling shows memory issue.

## Testing Strict Mode

### Schema Order Validation

Create a test schema with non-alphabetical element order:

```xml
<!-- test-schema-order.xsd -->
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
  <xs:element name="zebra" type="xs:string"/>
  <xs:element name="apple" type="xs:string"/>
  <xs:element name="mango" type="xs:string"/>
  
  <xs:element name="root">
    <xs:complexType>
      <xs:choice>
        <xs:element ref="zebra"/>
        <xs:element ref="apple"/>
        <xs:element ref="mango"/>
      </xs:choice>
    </xs:complexType>
  </xs:element>
</xs:schema>
```

### Test Cases

**Test 1: Encode with EXIP, decode with another implementation**
```bash
# Encode <zebra> with EXIP
echo '<root><zebra>test</zebra></root>' | exipe -schema test.xsd.exi > test.exi

# Decode with ExiFicient/EXIfficient/other
# Expected: <zebra>test</zebra>
# Actual (current EXIP): <mango>test</mango>  ✗
```

**Test 2: Cross-implementation round-trip**
```bash
# Encode with ExiFicient
exificient -schema test.xsd -i test.xml -o test.exi

# Decode with EXIP
exipd -schema test.xsd.exi test.exi
# Expected: original content
# Actual: element names scrambled  ✗
```

**Test 3: Grammar debug output**
```bash
# With DEBUG_GRAMMAR_GEN=ON
exipg test.xsd.exi

# Check Document Grammar production order
# Should show: SE(zebra), SE(apple), SE(mango)
# Currently shows: SE(apple), SE(mango), SE(zebra)  ✗
```

## Workaround (Until Fixed)

For strict mode interoperability with other implementations:

**Declare schema elements in alphabetical order:**
```xml
<xs:schema>
  <xs:element name="apple" type="xs:string"/>
  <xs:element name="mango" type="xs:string"/>
  <xs:element name="zebra" type="xs:string"/>
</xs:schema>
```

This ensures schema order matches QName lexical order, making EXIP's sorting correct by coincidence.

**Limitation:** Not always feasible, especially with:
- Schemas from third parties
- Standards bodies (W3C, OASIS, etc.)
- Generated schemas
- Namespace-mixed schemas

## References

- **EXI 1.0 Specification** - W3C Recommendation 11 February 2014
  - Section 8.5.4.3: Event Code Assignment
  - Section 7.3: Fidelity Options
- **EXIP Grammar Generation** - [src/grammarGen/](src/grammarGen/)
  - [treeTableToGrammars.c](src/grammarGen/src/treeTableToGrammars.c) - Schema → Grammar conversion
  - [genUtils.c](src/grammarGen/src/genUtils.c) - Grammar utilities
- **Test Infrastructure** - [tests/check_strict_grammar.c](tests/check_strict_grammar.c)

## Recommendations

### For EXIP Users

**Current state:**
1. Strict mode is **not spec-compliant** for non-alphabetical schemas
2. Interoperability with other EXI implementations is **not guaranteed**
3. EXIP-to-EXIP communication works (same bug)

**Recommendations:**
- Alphabetize schema declarations if possible
- Use default/preserve mode for cross-implementation scenarios
- Test with reference implementations (ExiFicient, EXIfficient)
- Add schema order tracking to development roadmap

### For EXIP Contributors

**Priority:** High - affects strict mode correctness and interoperability

**Implementation steps:**
1. Add GlobalEntryTable to BuildContext
2. Track globalSchemaEntryIndex during schema traversal
3. Modify compareProductions() to use schema order for SE(qname)
4. Add test cases with non-alphabetical schemas
5. Verify against ExiFicient/EXIfficient output

**Estimated effort:** 
- Core implementation: 4-6 hours
- Testing and validation: 4-6 hours  
- Total: 8-12 hours

**Risk:** Low - Isolated to grammar generation, no runtime changes

---

**Document Status:** Analysis complete, fix architecture defined, implementation pending
