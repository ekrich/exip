/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2026 ekrich.org                             |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file bindapi.h
 * @brief External application-facing API for EXI serialization with native C types
 *
 * The bindapi provides a high-level interface for applications and code generation
 * tools (exipb) to serialize EXI streams using native C types (int, float, double, char*)
 * without directly handling EXIP's internal types (Integer, Float struct, String struct).
 *
 * Key features:
 * - Works with standard C types instead of EXIP types
 * - Automatically handles schema-informed and schemaless mode switching
 * - Designed for binding generators and application code
 * - Hides EXIP implementation details
 *
 * Optimization: Define SCHEMA_ONLY to eliminate schemaless mode code at compile time,
 * removing runtime branches and string conversion overhead for embedded systems.
 *
 * For low-level control, use EXISerializer.h which works directly with EXIP types.
 *
 * @date Jun 2, 2026
 * @author EXIP Contributors
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
errorCode bindInt(EXIStream* strm, int value);

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

/**
 * @brief Serialize a native C int value to EXI stream as string (schemaless mode)
 *
 * Converts a native C int to string representation and encodes it to the stream.
 * Used internally for schemaless mode encoding.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C int value to encode as string
 * @return Error handling code
 */
errorCode bindIntToString(EXIStream* strm, int32_t value);

/**
 * @brief Serialize a native C bool value to EXI stream as string (schemaless mode)
 *
 * Converts a native C bool to string representation ("true" or "false") and
 * encodes it to the stream. Used internally for schemaless mode encoding.
 *
 * @param[in, out] strm EXI stream object
 * @param[in] value native C bool value to encode as string
 * @return Error handling code
 */
errorCode bindBoolToString(EXIStream* strm, bool value);

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

#ifdef __cplusplus
}
#endif

#endif /* BINDAPI_H_ */
