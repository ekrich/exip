/*
 * hexBinary.h - Hexadecimal (Base16) encode/decode for xs:hexBinary values
 *
 * Copyright (c) 2025, Eric Richardson
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef HEXBINARY_H
#define HEXBINARY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encode binary data xs:hexBinary to uppercase hexadecimal (Base16) string.
 *
 * @param in       Pointer to input byte array
 * @param len      Number of input bytes
 * @param out      Output buffer (must be at least (2 * len + 1) bytes)
 */
void encodeHexBinary(const uint8_t *in, size_t len, char *out);

/**
 * Decode a hexadecimal xs:hexBinary (Base16) string to binary data.
 *
 * @param in           Null-terminated hex string (uppercase or lowercase)
 * @param out          Output buffer for decoded bytes
 * @param max_out_len  Maximum number of bytes to write to `out`
 *
 * @return Number of bytes written to `out`, or -1 on error
 */
int decodeHexBinary(const char *in, uint8_t *out, size_t max_out_len);

#ifdef __cplusplus
}
#endif

#endif /* HEXBINARY_H */
