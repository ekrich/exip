/*==================================================================*\
|                EXIP - Embeddable EXI Processor in C                |
|--------------------------------------------------------------------|
|          Copyright (c) 2026 ekrich.org                             |
|          Licensed under the Apache License, Version 2.0            |
\===================================================================*/

/**
 * @file bindapi.c
 * @brief Implementation of binding API for code generation tools
 *
 * @date Jun 2, 2026
 * @author EXIP Contributors
 * @version 0.5
 */

#include "bindapi.h"
#include "EXISerializer.h"
#include <math.h>
#include <float.h>
#include <string.h>

// Forward declarations
static errorCode serializeIntValueAsString(EXIStream* strm, int32_t num);
static errorCode serializeUIntValueAsString(EXIStream* strm, uint32_t num);

/**
 * @brief Convert a C string (char array) to EXIP String representation (private helper)
 *
 * Converts a null-terminated C string to EXIP's String structure.
 * The resulting String points to the input buffer, no copy is made.
 * This is a private helper used internally by the bindapi.
 *
 * @param[in] str null-terminated C string
 * @param[out] exip_str pointer to EXIP String structure to populate
 */
static void charArrayToString(const char* str, String* exip_str)
{
	exip_str->str = (CharType*)str;
	exip_str->length = (Index)strlen(str);
}

/**
 * @brief High-performance adapter that maps custom math pointers using zero-overhead subtraction (private helper)
 *
 * Creates an EXIP String from a pointer within a buffer without calling strlen().
 * Uses pointer arithmetic to calculate length. This is a private helper used
 * internally by the bindapi for efficient string conversions.
 *
 * @param[in]  str_start Pointer to the active starting digit inside the buffer
 * @param[in]  buf Pointer to the baseline beginning of the allocation buffer
 * @param[in]  buf_len Physical allocation limit of the buffer
 * @param[out] exip_str Pointer to the target EXIP String structure to populate
 */
static void charArrayToStringFast(char *str_start, char *buf, size_t buf_len, String *exip_str)
{
    exip_str->str = (CharType*)str_start;
    char *buf_end = buf + buf_len - 1;
    exip_str->length = (Index)(buf_end - str_start);
}

/**
 * @brief Convert a native C int to EXIP Integer representation (private helper)
 *
 * Converts a native C int to EXIP's 64-bit Integer type.
 * This is a private inline helper used internally by the bindapi.
 *
 * @param[in] value native C int value
 * @return EXIP Integer value
 */
static inline Integer intToInteger(int value)
{
	return (Integer)value;
}

void floatToExipFloat(float value, Float* exip_float)
{
	if (value == 0.0f)
	{
		exip_float->mantissa = 0;
		exip_float->exponent = 0;
		return;
	}

	if (isnan(value) || isinf(value))
	{
		/* Special values - represent as mantissa only */
		if (isnan(value))
		{
			exip_float->mantissa = 0;
			exip_float->exponent = 0; /* NaN representation */
		}
		else if (isinf(value))
		{
			exip_float->mantissa = (value > 0) ? 1 : -1;
			exip_float->exponent = 16384; /* INF representation */
		}
		return;
	}

	/* Convert to decimal representation: value = mantissa * 10^exponent */
	double dval = (double)value;
	int16_t exp = 0;

	/* Shift decimal point right until no fractional part (up to precision limit) */
	while (dval != floor(dval) && exp > -18)
	{
		dval *= 10.0;
		exp--;
	}

	exip_float->mantissa = (int64_t)dval;
	exip_float->exponent = exp;
}

void doubleToExipFloat(double value, Float* exip_float)
{
	if (value == 0.0)
	{
		exip_float->mantissa = 0;
		exip_float->exponent = 0;
		return;
	}

	if (isnan(value) || isinf(value))
	{
		/* Special values - represent as mantissa only */
		if (isnan(value))
		{
			exip_float->mantissa = 0;
			exip_float->exponent = 0; /* NaN representation */
		}
		else if (isinf(value))
		{
			exip_float->mantissa = (value > 0) ? 1 : -1;
			exip_float->exponent = 16384; /* INF representation */
		}
		return;
	}

	/* Convert to decimal representation: value = mantissa * 10^exponent */
	int16_t exp = 0;

	/* Shift decimal point right until no fractional part (up to precision limit) */
	while (value != floor(value) && exp > -18)
	{
		value *= 10.0;
		exp--;
	}

	exip_float->mantissa = (int64_t)value;
	exip_float->exponent = exp;
}

errorCode serializeFloatValue(EXIStream* strm, float value)
{
	Float exip_float;
	floatToExipFloat(value, &exip_float);
	return serialize.floatData(strm, exip_float);
}

errorCode serializeDoubleValue(EXIStream* strm, double value)
{
	Float exip_float;
	doubleToExipFloat(value, &exip_float);
	return serialize.floatData(strm, exip_float);
}

errorCode serializeStringValue(EXIStream* strm, const char* str)
{
	String exip_str;
	charArrayToString(str, &exip_str);
	return serialize.stringData(strm, exip_str);
}

errorCode serializeIntValue(EXIStream* strm, int value)
{
	if (strm->schema != NULL) {
		// Schema mode - typed encoding
		Integer exip_int = intToInteger(value);
		return serialize.intData(strm, exip_int);
	} else {
		// Schemaless mode - string encoding
		return serializeIntValueAsString(strm, value);
	}
}

static errorCode serializeUIntValueAsString(EXIStream* strm, uint32_t num)
{
    char buf[UINT32_STR_MAX_LEN];
    String exip_str;

    char *str_start = uint32_to_str(num, buf, sizeof(buf));
    charArrayToStringFast(str_start, buf, sizeof(buf), &exip_str);

	errorCode err = serialize.stringData(strm, exip_str);
	return err;
}

static errorCode serializeIntValueAsString(EXIStream* strm, int32_t num)
{
    char buf[INT32_STR_MAX_LEN];
    String exip_str;

    char *str_start = int32_to_str(num, buf, sizeof(buf));
    charArrayToStringFast(str_start, buf, sizeof(buf), &exip_str);

	errorCode err = serialize.stringData(strm, exip_str);
    return err;
}

errorCode serializeBoolValue(EXIStream* strm, bool value)
{
	return serialize.booleanData(strm, value);
}

char* int32_to_str(int32_t num, char *buf, size_t len) {
	// Ensure we have at least room for a null terminator
    if (buf == NULL || len == 0) {
        return buf;
    }

    // Start writing from the very end of the buffer
    char *p = buf + len - 1;
    *p = '\0'; // Set the null terminator first

    // Handle 0 explicitly
    if (num == 0) {
        if (p > buf) {
            *(--p) = '0';
        }
        return p;
    }

    // Isolate the absolute value using uint32_t
    // This prevents a crash/overflow when processing INT32_MIN (-2147483648)
    uint32_t abs_num = (num < 0) ? (uint32_t)(-num) : (uint32_t)(num);

    // Extract digits sequentially in reverse order
    while (abs_num > 0 && p > buf) {
        *(--p) = (char)((abs_num % 10) + '0');
        abs_num /= 10;
    }

    // Append the negative sign if original number was negative
    if (num < 0 && p > buf) {
        *(--p) = '-';
    }

    // Returns the exact address where the string data begins
    return p;
}

char* uint32_to_str(uint32_t num, char *buf, size_t len) {
    if (buf == NULL || len == 0) {
        return buf;
    }

    char *p = buf + len - 1;
    *p = '\0'; // Set the null terminator

    if (num == 0) {
        if (p > buf) {
            *(--p) = '0';
        }
        return p;
    }

    // Direct extraction loop (no sign-checking required)
    while (num > 0 && p > buf) {
        *(--p) = (char)((num % 10) + '0');
        num /= 10;
    }

    return p;
}
