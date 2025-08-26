/*
 * base64Binary.c - Base64 encode/decode for xs:base64Binary values
 *
 * Copyright (c) 2025, Eric Richardson
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license text.
 */


#include "base64Binary.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Reverse lookup table: maps base64 characters to values 0–63
static const uint8_t base64ReverseTable[128] = {
    // ASCII 0–127; only A–Z, a–z, 0–9, +, / are mapped
    [ 'A' ] =  0, [ 'B' ] =  1, [ 'C' ] =  2, [ 'D' ] =  3,
    [ 'E' ] =  4, [ 'F' ] =  5, [ 'G' ] =  6, [ 'H' ] =  7,
    [ 'I' ] =  8, [ 'J' ] =  9, [ 'K' ] = 10, [ 'L' ] = 11,
    [ 'M' ] = 12, [ 'N' ] = 13, [ 'O' ] = 14, [ 'P' ] = 15,
    [ 'Q' ] = 16, [ 'R' ] = 17, [ 'S' ] = 18, [ 'T' ] = 19,
    [ 'U' ] = 20, [ 'V' ] = 21, [ 'W' ] = 22, [ 'X' ] = 23,
    [ 'Y' ] = 24, [ 'Z' ] = 25,
    [ 'a' ] = 26, [ 'b' ] = 27, [ 'c' ] = 28, [ 'd' ] = 29,
    [ 'e' ] = 30, [ 'f' ] = 31, [ 'g' ] = 32, [ 'h' ] = 33,
    [ 'i' ] = 34, [ 'j' ] = 35, [ 'k' ] = 36, [ 'l' ] = 37,
    [ 'm' ] = 38, [ 'n' ] = 39, [ 'o' ] = 40, [ 'p' ] = 41,
    [ 'q' ] = 42, [ 'r' ] = 43, [ 's' ] = 44, [ 't' ] = 45,
    [ 'u' ] = 46, [ 'v' ] = 47, [ 'w' ] = 48, [ 'x' ] = 49,
    [ 'y' ] = 50, [ 'z' ] = 51,
    [ '0' ] = 52, [ '1' ] = 53, [ '2' ] = 54, [ '3' ] = 55,
    [ '4' ] = 56, [ '5' ] = 57, [ '6' ] = 58, [ '7' ] = 59,
    [ '8' ] = 60, [ '9' ] = 61,
    [ '+' ] = 62, [ '/' ] = 63
};

void encodeBase64Binary(const uint8_t *in, size_t len, char *out) {
    size_t i, j;
    for (i = 0, j = 0; i + 2 < len; i += 3) {
        out[j++] = base64_table[(in[i] >> 2) & 0x3F];
        out[j++] = base64_table[((in[i] & 0x03) << 4) | (in[i + 1] >> 4)];
        out[j++] = base64_table[((in[i + 1] & 0x0F) << 2) | (in[i + 2] >> 6)];
        out[j++] = base64_table[in[i + 2] & 0x3F];
    }

    if (i < len) {
        out[j++] = base64_table[(in[i] >> 2) & 0x3F];
        if (i + 1 < len) {
            out[j++] = base64_table[((in[i] & 0x03) << 4) | (in[i + 1] >> 4)];
            out[j++] = base64_table[(in[i + 1] & 0x0F) << 2];
            out[j++] = '=';
        } else {
            out[j++] = base64_table[(in[i] & 0x03) << 4];
            out[j++] = '=';
            out[j++] = '=';
        }
    }

    out[j] = '\0'; // null terminator
}

size_t encodeBufferSize(size_t len) {
    return ((len + 2) / 3) * 4 + 1;
}

static inline bool isInvalidChar(unsigned char c) {
    return (c > 127) || ((base64ReverseTable[c] == 0) && (c != 'A'));
}

int decodeBase64Binary(const char *in, uint8_t *out, size_t maxOutLen)
{
    size_t outLen = 0;
    uint32_t buffer = 0;
    int bits = 0;

    for (size_t i = 0; in[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)in[i];

        // Ignore whitespace
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            continue;

        if (c == '=') {
            // Padding character – end of content
            break;
        }

        if (isInvalidChar(c))
            return -1;

        buffer = (buffer << 6) | base64ReverseTable[c];
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            if (outLen >= maxOutLen)
                return -1;
            out[outLen++] = (buffer >> bits) & 0xFF;
        }
    }

    return (int)outLen;
}

size_t decodeBufferSize(const char *in) {
    return (strlen(in) / 4) * 3;
}
