/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2026 ekrich.org                             |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file bindapi.h
 * @brief Binding API for code generation tools
 * Utilities for converting native C types to EXIP types for use by
 * binding generators (exipb) and other code generation tools
 *
 * @date Jun 2, 2026
 * @author EXIP Contributors
 * @version 0.5
 */

#ifndef BINDAPI_H_
#define BINDAPI_H_

#include "procTypes.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a C string (char array) to EXIP String representation
 *
 * Converts a null-terminated C string to EXIP's String structure.
 * The resulting String points to the input buffer, no copy is made.
 *
 * @param[in] str null-terminated C string
 * @param[out] exip_str pointer to EXIP String structure to populate
 */
void charArrayToString(const char* str, String* exip_str);

/**
 * @brief High-performance adapter that maps custom math pointers using zero-overhead subtraction.
 * @param[in]  str_start Pointer to the active starting digit inside the buffer.
 * @param[in]  buf Pointer to the baseline beginning of the allocation buffer.
 * @param[in]  buf_len Physical allocation limit of the buffer.
 * @param[out] exip_str Pointer to the target EXIP String structure to populate.
 */
void charArrayToStringFast(char *str_start, char *buf, size_t buf_len, String *exip_str);

/**
 * @brief Convert a native C int to EXIP Integer representation
 *
 * Converts a native C int to EXIP's 64-bit Integer type.
 *
 * @param[in] value native C int value
 * @return EXIP Integer value
 */
Integer intToInteger(int value);

/**
 * @brief Convert a native C float to EXIP Float representation
 *
 * Converts IEEE 754 single-precision float to EXIP's mantissa/exponent
 * representation for use with serialize.floatData() and related functions.
 *
 * @param[in] value native C float value
 * @param[out] exip_float pointer to EXIP Float structure to populate
 */
void floatToExipFloat(float value, Float* exip_float);

/**
 * @brief Convert a native C double to EXIP Float representation
 *
 * Converts IEEE 754 double-precision float to EXIP's mantissa/exponent
 * representation for use with serialize.floatData() and related functions.
 *
 * @param[in] value native C double value
 * @param[out] exip_float pointer to EXIP Float structure to populate
 */
void doubleToExipFloat(double value, Float* exip_float);

/**
 * @brief Serialize a native C float value to EXI stream
 *
 * Converts IEEE 754 single-precision float to EXIP's mantissa/exponent
 * representation and encodes it to the stream. Combines conversion and
 * serialization in one call for convenience.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C float value to encode
 * @return Error handling code
 */
errorCode serializeFloatValue(EXIStream* strm, float value);

/**
 * @brief Serialize a native C double value to EXI stream
 *
 * Converts IEEE 754 double-precision float to EXIP's mantissa/exponent
 * representation and encodes it to the stream. Combines conversion and
 * serialization in one call for convenience.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C double value to encode
 * @return Error handling code
 */
errorCode serializeDoubleValue(EXIStream* strm, double value);

/**
 * @brief Serialize a C string (char array) to EXI stream
 *
 * Converts a null-terminated C string to EXIP String and encodes it to the stream.
 * Combines conversion and serialization in one call for convenience.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] str null-terminated C string to encode
 * @return Error handling code
 */
errorCode serializeStringValue(EXIStream* strm, const char* str);

/**
 * @brief Serialize a native C int value to EXI stream
 *
 * Converts a native C int to EXIP Integer and encodes it to the stream.
 * Combines conversion and serialization in one call for convenience.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C int value to encode
 * @return Error handling code
 */
errorCode serializeIntValue(EXIStream* strm, int value);

/**
 * @brief Serialize a native C bool value to EXI stream
 *
 * Encodes a boolean value to the stream.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C bool value to encode
 * @return Error handling code
 */
errorCode serializeBoolValue(EXIStream* strm, bool value);

// Safe size: fits "-2147483648" (11 chars) + null terminator (1 char)
#define INT32_STR_MAX_LEN 12
#define UINT32_STR_MAX_LEN 11 // Max length for unsigned uint32

/**
 * @brief Converts a signed 32-bit integer to a null-terminated string.
 * @param[in]     num The integer value to convert.
 * @param[in,out] buf Pre-allocated buffer initialized and written to right-to-left.
 * @param[in]     len Total size of the pre-allocated destination buffer.
 * @return Pointer to the first valid numeric character inside the buffer.
 */
char* int32_to_str(int32_t num, char *buf, size_t len);

/**
 * @brief Converts an unsigned 32-bit integer to a null-terminated string.
 * @param[in]     num The unsigned integer value to convert.
 * @param[in,out] buf Pre-allocated buffer initialized and written to right-to-left.
 * @param[in]     len Total size of the pre-allocated destination buffer.
 * @return Pointer to the first valid numeric character inside the buffer.
 */
char* uint32_to_str(uint32_t num, char *buf, size_t len);

errorCode serializeUIntValueAsString(EXIStream* strm, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* BINDAPI_H_ */
