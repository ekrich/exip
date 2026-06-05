/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2026 ekrich.org                             |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file exipe_data.h
 * @brief Clean C data structures for exipe-test schema
 *
 * Generated from exipe-test.xsd, exipe-test-nested.xsd, exipe-test-types.xsd
 *
 * This demonstrates what exipb (binding generator) would produce:
 * - Pure C types, no EXIP types exposed
 * - Fixed-size arrays where schema defines maxLength
 * - Nested structs for complex types
 * - Choice types mapped to tagged unions
 * - DateTime using standard C struct tm + extensions
 *
 * @date Jun 3, 2026
 * @author EXIP Contributors
 * @version 0.5
 */

#ifndef EXIPE_DATA_H_
#define EXIPE_DATA_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Local DateTime type (identical to EXIP's EXIPDateTime but no EXIP dependency)
 *
 * Used for xs:dateTime schema types. Based on standard C struct tm with XML Schema extensions.
 *
 * Fractional seconds precision is configurable via fSecsOffset:
 * - fSecsOffset = 3: milliseconds   (fSecs=839 → 0.839 seconds = 839 ms)
 * - fSecsOffset = 5: 10 microseconds (fSecs=839 → 0.00839 seconds = 8.39 ms)
 * - fSecsOffset = 6: microseconds    (fSecs=839000 → 0.000839 seconds = 839 μs)
 * - fSecsOffset = 9: nanoseconds     (fSecs=839000000 → 0.000839 seconds = 839000 ns)
 *
 * Maximum precision: 999,999,999 nanoseconds (uint32_t limit)
 */
typedef struct {
    struct tm dateTime;      // Standard C time: year, month, day, hour, min, sec
    uint32_t fSecs;          // Fractional seconds value
    int8_t fSecsOffset;      // Exponent for fSecs: value × 10^-offset (e.g., 3=ms, 6=μs, 9=ns)
    int16_t TimeZone;        // TZHours * 64 + TZMinutes (range: -14:00 to +14:00)
    uint8_t presenceMask;    // Flags: TZONE_PRESENCE (0x01), FRACT_PRESENCE (0x02)
} DateTime;

// Presence mask flags
#define TZONE_PRESENCE 0x01
#define FRACT_PRESENCE 0x02

/**
 * @brief EXIPEncoder element (simpleContent with attributes)
 *
 * Schema: <element name="EXIPEncoder">
 *   <complexType>
 *     <simpleContent>
 *       <extension base="string">
 *         <attribute name="version" type="string"/>
 *         <attribute name="testByte" use="optional" type="byte"/>
 *       </extension>
 *     </simpleContent>
 *   </complexType>
 * </element>
 */
typedef struct {
    char content[256];       // The text content (base string)
    char version[32];        // @version attribute (required)
    int8_t testByte;         // @testByte attribute (optional)
    bool hasTestByte;        // Flag: is testByte present?
} EXIPEncoder;

/**
 * @brief testSetup element (simpleContent with attribute)
 *
 * Schema: <element name="testSetup">
 *   <complexType>
 *     <simpleContent>
 *       <extension base="string">
 *         <attribute name="goal" type="string"/>
 *       </extension>
 *     </simpleContent>
 *   </complexType>
 * </element>
 */
typedef struct {
    char content[256];       // The text content
    char goal[128];          // @goal attribute
} TestSetup;

/**
 * @brief Choice type (mapped to tagged union)
 *
 * Schema: <complexType name="basicType">
 *   <choice>
 *     <element name="bool" type="boolean"/>
 *     <element name="int" type="int"/>
 *   </choice>
 *   <attribute name="id" type="int"/>
 * </complexType>
 */
typedef enum {
    BASIC_TYPE_NONE,
    BASIC_TYPE_BOOL,
    BASIC_TYPE_INT
} BasicTypeChoice;

typedef struct {
    BasicTypeChoice choiceType;  // Which choice is active
    union {
        bool boolValue;          // <bool> value
        int intValue;            // <int> value
    } choice;
    int id;                      // @id attribute
} BasicType;

/**
 * @brief Enumeration type
 *
 * Schema: <simpleType name="enumType">
 *   <restriction base="string">
 *     <enumeration value="hello"/>
 *     <enumeration value="hi"/>
 *     <enumeration value="hey"/>
 *     <enumeration value="hej"/>
 *   </restriction>
 * </simpleType>
 *
 * Note: C enum names are uppercase versions of XSD enumeration values
 */
typedef enum {
    HELLO = 0,
    HI = 1,
    HEY = 2,
    HEJ = 3
} EnumType;

/**
 * @brief Lookup table for EnumType to string conversion
 *
 * Usage:
 *   const char* str = enum_type_strings[HEJ].str;  // "hej"
 *   size_t len = enum_type_strings[HEJ].len;       // 3
 */
static const struct {
    const char* str;
    size_t len;
} enum_type_strings[] = {
    [HELLO] = {"hello", 5},
    [HI]    = {"hi", 2},
    [HEY]   = {"hey", 3},
    [HEJ]   = {"hej", 3}
};

/**
 * @brief Extended types test
 *
 * Schema: <complexType name="typesTest">
 *   <sequence>
 *     <element name="byteTest" type="byte"/>
 *     <element name="dateTimeTest" type="dateTime"/>
 *     <element name="binaryTest" type="base64Binary" minOccurs="0"/>
 *     <element name="enumTest" type="enumType" minOccurs="0"/>
 *   </sequence>
 * </complexType>
 */
typedef struct {
    int8_t byteTest;                 // <byteTest>
    DateTime dateTimeTest;           // <dateTimeTest>

    // Optional elements
    uint8_t* binaryTest;             // <binaryTest> (optional, base64Binary)
    size_t binaryTestLen;            // Length of binary data
    bool hasBinaryTest;              // Flag: is binaryTest present?

    EnumType enumTest;               // <enumTest> (optional)
    bool hasEnumTest;                // Flag: is enumTest present?
} TypesTest;

/**
 * @brief Root element structure
 *
 * Schema: <element name="MultipleXSDsTest">
 *   <complexType>
 *     <sequence>
 *       <element ref="EXIPEncoder"/>
 *       <element name="description" type="string"/>
 *       <element ref="testSetup"/>
 *       <element name="type-test" type="basicType"/>
 *       <element name="extendedTypeTest" type="typesTest" minOccurs="0"/>
 *     </sequence>
 *   </complexType>
 * </element>
 */
typedef struct {
    EXIPEncoder encoder;             // <EXIPEncoder>
    char description[256];           // <description>
    TestSetup testSetup;             // <testSetup>
    BasicType typeTest;              // <type-test>

    // Optional element
    TypesTest extendedTypeTest;      // <extendedTypeTest> (optional)
    bool hasExtendedTypeTest;        // Flag: is extendedTypeTest present?
} MultipleXSDsTest;

/**
 * @brief Constructor-style functions that return fully initialized structs
 *
 * These allow declarative initialization with parameters:
 *   TypesTest types = create_types_test(&greeting);
 *   MultipleXSDsTest data = create_test_data(types);
 *
 * All fields are initialized - lengths and optional flags calculated automatically.
 * Sub-objects are constructed separately, then passed to parent constructors.
 * No separate init() function needed - constructors handle everything.
 */

/**
 * @brief Construct TypesTest with optional greeting
 *
 * Full parameterized version would be:
 *   TypesTest create_types_test(int8_t byteTest, DateTime dateTime,
 *                               uint8_t* binaryData, size_t binaryLen,
 *                               EnumType* greeting)
 *
 * @param[in] greeting Pointer to enum value (HELLO, HI, HEY, or HEJ), or NULL if not present
 * @return Fully initialized TypesTest structure
 */
TypesTest create_types_test(EnumType* greeting);

/**
 * @brief Construct MultipleXSDsTest with pre-constructed TypesTest
 *
 * @param[in] typesTest Pre-constructed TypesTest sub-object
 * @return Fully initialized MultipleXSDsTest structure
 */
MultipleXSDsTest create_test_data(TypesTest typesTest);

/**
 * @brief Free any dynamically allocated memory in the structure
 *
 * Only needed if binary data was allocated. Safe to call on stack-allocated structs.
 *
 * @param[in,out] data Pointer to structure to clean up
 */
void destroy_multiple_xsds_test(MultipleXSDsTest* data);

#ifdef __cplusplus
}
#endif

#endif /* EXIPE_DATA_H_ */
