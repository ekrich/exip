/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2026 ekrich.org                             |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file exipe_data.c
 * @brief Implementation of data structure cleanup
 *
 * @date Jun 3, 2026
 * @author EXIP Contributors
 * @version 0.5
 */

#include "exipe_data.h"
#include <string.h>
#include <stdlib.h>

/* Full parameterized version would be:
 * TypesTest create_types_test(int8_t byteTest, DateTime dateTime,
 *                             uint8_t* binaryData, size_t binaryLen,
 *                             EnumType* greeting)
 */
TypesTest create_types_test(uint8_t* binaryData, size_t binaryLen, EnumType* greeting)
{
    return (TypesTest){
        .byteTest = 11,
        .dateTimeTest = {
            .dateTime = {
                .tm_year = 112,
                .tm_mon = 6,
                .tm_mday = 31,
                .tm_hour = 13,
                .tm_min = 33,
                .tm_sec = 55
            },
            .fSecs = 839,
            .fSecsOffset = 5,        // 839 × 10^-5 = 0.00839 seconds (8.39 milliseconds)
            .TimeZone = 0,
            .presenceMask = FRACT_PRESENCE
        },
        .binaryTest = binaryData,
        .binaryTestLen = (binaryData != NULL) ? binaryLen : 0,
        .enumTest = greeting ? *greeting : 0,
        .hasEnumTest = (greeting != NULL)
    };
}

MultipleXSDsTest create_test_data(TypesTest typesTest)
{
    return (MultipleXSDsTest){
        .encoder = {
            .content = "This is an example of serializing EXI streams using EXIP low level API",
            .version = "0.2",
            .testByte = 55,
            .hasTestByte = true
        },
        .description = "This is a test of processing XML schemes with multiple XSD files",
        .testSetup = {
            .content = "Simple test element with single attribute",
            .goal = "Verify that the implementation works!"
        },
        .typeTest = {
            .choiceType = BASIC_TYPE_BOOL,
            .choice = { .boolValue = true },
            .id = 1001
        },
        .hasExtendedTypeTest = true,
        .extendedTypeTest = typesTest
    };
}
