# XML Schema Annotation Tests

Tests for xs:annotation, xs:documentation, and xs:appinfo handling.

## Test Coverage

**annotation-test.xsd** contains annotations in various locations:

1. **Schema-level** (line 7-16)
   - Tests top-level schema annotation with documentation and appinfo
   - Validates namespace handling in appinfo content

2. **Element annotation** (line 19-23)
   - Simple element with documentation

3. **Complex type annotation** (line 26-28, 31-36)
   - Named complex type with annotation
   - **Critical: Annotation inside xs:sequence** (line 31)
     - This tests the fix at treeTableToGrammars.c:635
     - Before fix: Returns EXIP_UNEXPECTED_ERROR
     - After fix: Annotation is skipped, processing continues

4. **Attribute annotation** (line 48-52)
   - Attribute with documentation

5. **Simple type with restriction** (line 56-67)
   - Annotation on simple type
   - Annotation inside xs:restriction

6. **Local complex type**
   - Element with inline complex type definition
   - Nested annotations at multiple levels

7. **Enumeration with annotations**
   - Annotation on enumeration type
   - Annotations inside individual enumeration values

8. **Multiple documentation elements**
   - Multiple xs:documentation (different languages)
   - Mixed documentation and appinfo

9. **Choice compositor**
   - Annotation inside xs:choice

10. **Union and List types**
   - Annotations in xs:union and xs:list

11. **Extension with annotations**
   - Annotations in xs:complexContent and xs:extension

**malformed-annotation-test.xsd** tests annotations in invalid locations:
- Annotation after type definition (should be before)
- Annotation between sequence elements (not allowed)
- Annotation after restriction facets (should be before)
- Multiple annotations at same level (only one allowed per component)
- Annotation as last child (should be first child)

## Expected Behavior

Both annotation-test.xsd and malformed-annotation-test.xsd should pass:
- Annotations are parsed but ignored (not stored)
- Grammar generation succeeds
- No crashes or errors
- Tests confirm the existing `ignoredElement` counter mechanism filters annotations regardless of placement

## Implementation Notes

Annotations are filtered during schema parsing in **treeTableBuild.c** (lines 338-352):
- `startElement` handler increments `ignoredElement` counter for ELEMENT_ANNOTATION, ELEMENT_DOCUMENTATION, ELEMENT_APPINFO
- `endElement` handler decrements counter
- Any content within annotation elements is automatically skipped

This filtering happens **before** grammar generation, so annotations never reach the grammar building phase. Annotations carry **no grammar information** - they are metadata for humans and tools, not for EXI encoding.
