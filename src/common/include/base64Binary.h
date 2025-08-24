/*
 * base64Binary.h - Base64 encode/decode for xs:base64Binary values
 *
 * Copyright (c) 2025, Eric Richardson
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BASE64BINARY_H
#define BASE64BINARY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encode binary data to Base64 string using standard RFC 4648 alphabet.
 *
 * The output will be null-terminated and padded with '=' characters as needed.
 *
 * Buffer size requirements:
 *   - Output buffer must be at least ((len + 2) / 3) * 4 + 1 bytes.
 *
 * @param in       Pointer to input bytes
 * @param len      Length of input bytes
 * @param out      Output buffer (must follow size guidelines above)
 */
void encodeBase64Binary(const uint8_t *in, size_t len, char *out);

/**
 * Decode a Base64 string (RFC 4648) to binary data.
 *
 * Supports standard '=' padding. Ignores whitespace.
 *
 * Buffer size requirements:
 *   - Output buffer must be at least (strlen(in) / 4) * 3 bytes.
 *
 * @param in           Null-terminated Base64 string
 * @param out          Output buffer for decoded bytes
 * @param max_out_len  Maximum number of bytes to write to out
 *
 * @return Number of bytes written to out, or -1 on error
 */
int decodeBase64Binary(const char *in, uint8_t *out, size_t max_out_len);

#ifdef __cplusplus
}
#endif

#endif /* BASE64BINARY_H */
