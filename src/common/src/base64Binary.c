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

// Encode binary data to Base64 string
// Caller must allocate output using encodeBufferSize(len)
// Writes '\0' terminator
// Returns actual length (not including '\0') in *outLen
void encodeBase64Binary(const uint8_t *in, size_t len, char *out, size_t *outLen)
{
    static const char base64Table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t i = 0, j = 0;
    while (i + 2 < len)
    {
        out[j++] = base64Table[(in[i] >> 2) & 0x3F];
        out[j++] = base64Table[((in[i] & 0x03) << 4) | ((in[i + 1] & 0xF0) >> 4)];
        out[j++] = base64Table[((in[i + 1] & 0x0F) << 2) | ((in[i + 2] & 0xC0) >> 6)];
        out[j++] = base64Table[in[i + 2] & 0x3F];
        i += 3;
    }

    if (i < len)
    {
        out[j++] = base64Table[(in[i] >> 2) & 0x3F];
        if (i + 1 < len)
        {
            out[j++] = base64Table[((in[i] & 0x03) << 4) | ((in[i + 1] & 0xF0) >> 4)];
            out[j++] = base64Table[(in[i + 1] & 0x0F) << 2];
            out[j++] = '=';
        }
        else
        {
            out[j++] = base64Table[(in[i] & 0x03) << 4];
            out[j++] = '=';
            out[j++] = '=';
        }
    }

    out[j] = '\0';   // null-terminate
    *outLen = j;     // store actual string length
}


size_t encodeBufferSize(size_t len) {
    // 4 chars for every 3 bytes, rounded up
    // Add space for null terminator
    return ((len + 2) / 3) * 4 + 1;
}

// Base64 decoding table (values: 0–63, or 0 for invalid chars except 'A')
static const uint8_t base64ReverseTable[256] = {
    ['A']=0,['B']=1,['C']=2,['D']=3,['E']=4,['F']=5,['G']=6,['H']=7,
    ['I']=8,['J']=9,['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
    ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
    ['Y']=24,['Z']=25,
    ['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,['g']=32,['h']=33,
    ['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,['o']=40,['p']=41,
    ['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,['w']=48,['x']=49,
    ['y']=50,['z']=51,
    ['0']=52,['1']=53,['2']=54,['3']=55,['4']=56,['5']=57,['6']=58,['7']=59,
    ['8']=60,['9']=61,
    ['+']=62, ['/']=63
};

static inline bool isInvalidChar(unsigned char c) {
    return (c > 127) || ((base64ReverseTable[c] == 0) && (c != 'A'));
}

// Decode Base64 string into binary data
// Caller must allocate output using decodeBufferSize(inLen)
// Returns number of bytes written in *outLen
// Returns 0 on success, nonzero on error
int decodeBase64Binary(const char *in, size_t inLen, uint8_t *out, size_t *outLen)
{
    if (inLen % 4 != 0)
        return -1; // invalid length

    size_t i = 0, j = 0;
    while (i < inLen)
    {
        unsigned char c1 = in[i++];
        unsigned char c2 = in[i++];
        unsigned char c3 = in[i++];
        unsigned char c4 = in[i++];

        if (isInvalidChar(c1) || isInvalidChar(c2) ||
            (c3 != '=' && isInvalidChar(c3)) ||
            (c4 != '=' && isInvalidChar(c4)))
        {
            return -2; // invalid character
        }

        uint32_t b1 = base64ReverseTable[c1];
        uint32_t b2 = base64ReverseTable[c2];
        uint32_t b3 = (c3 == '=') ? 0 : base64ReverseTable[c3];
        uint32_t b4 = (c4 == '=') ? 0 : base64ReverseTable[c4];

        out[j++] = (uint8_t)((b1 << 2) | (b2 >> 4));
        if (c3 != '=')
        {
            out[j++] = (uint8_t)(((b2 & 0x0F) << 4) | (b3 >> 2));
            if (c4 != '=')
            {
                out[j++] = (uint8_t)(((b3 & 0x03) << 6) | b4);
            }
        }
    }

    *outLen = j;
    return 0; // success
}

size_t decodeBufferSize(size_t in) {
    return (in / 4) * 3;
}
