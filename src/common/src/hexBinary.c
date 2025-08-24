/*
 * hexBinary.c - Hexadecimal encode/decode for xs:hexBinary values
 *
 * Copyright (c) 2025, Eric Richardson
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license text.
 */

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include "hexBinary.h"

/**
 * Encode binary data as hex string (uppercase).
 * `out` must be at least (2 * len + 1) bytes long.
 */
void encodeHexBinary(const uint8_t *in, size_t len, char *out)
{
    static const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; ++i) {
        out[2*i]     = hex[(in[i] >> 4) & 0xF];
        out[2*i + 1] = hex[in[i] & 0xF];
    }
    out[2*len] = '\0';
}

/**
 * Decode hex string into binary.
 * Returns number of bytes written to `out`, or -1 on error.
 * Assumes `in` is null-terminated.
 */
int decodeHexBinary(const char *in, uint8_t *out, size_t max_out_len)
{
    size_t len = 0;
    while (*in && *(in + 1)) {
        if (len >= max_out_len) return -1;

        char hi = toupper(*in++);
        char lo = toupper(*in++);

        uint8_t hval = (hi >= '0' && hi <= '9') ? hi - '0' :
                       (hi >= 'A' && hi <= 'F') ? hi - 'A' + 10 : 255;
        uint8_t lval = (lo >= '0' && lo <= '9') ? lo - '0' :
                       (lo >= 'A' && lo <= 'F') ? lo - 'A' + 10 : 255;

        if (hval > 15 || lval > 15) return -1;

        out[len++] = (hval << 4) | lval;
    }

    // Reject input with odd length
    if (*in != '\0') return -1;

    return (int)len;
}
