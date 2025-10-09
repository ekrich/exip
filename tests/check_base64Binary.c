/*
 * check_base64Binary.c - Test for base64Binary.h
 *
 * Copyright (c) 2025, Eric Richardson
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license text.
 */

#include "base64Binary.h"
#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Tests */
/*
Input "TQ==" (4 chars) → (4 / 4) * 3 = 3 max → actually 1 byte (M).

Input "TWE=" (4 chars) → max 3 → actually 2 bytes (Ma).

Input "TWFu" (4 chars) → max 3 → exactly 3 bytes (Man).
*/
START_TEST(test_encode01) {

	const char *base64Str = "SGVsbG8gV29ybGQh"; // "Hello World!"
    size_t inLen = strlen(base64Str);

    // allocate buffer
    size_t bufSize = decodeBufferSize(inLen);
    uint8_t out[bufSize];
    size_t outLen = 0;

    int rc = decodeBase64Binary(base64Str, inLen, out, &outLen);

    if (rc == 0) {
        printf("Decoded %zu bytes: ", outLen);
        for (size_t i = 0; i < outLen; i++) {
            printf("%c", out[i]); // prints characters
        }
        printf("\n");
    } else {
        printf("Decode failed, error %d\n", rc);
    }

    // uint8_t encode01[] = {'a', 'b', 'C', 'd'};
    // size_t encodeLen01 = encodeBufferSize(sizeof(encode01));
    // char *encodeResult01 = malloc(encodeLen01);
    // encodeBase64Binary(encode01, encodeLen01, encodeResult01);
    // printf("Base64Encode: '%s'\n", encodeResult01);
    // //   fail_unless(tmp_err_code == EXIP_OK,
    // //               "parsing the header returns an error code %d",
    // //               tmp_err_code);
    // fail_unless(true == true, "encode string of size %ld", encodeLen01);
    // got YWJDZAAAAAAA should be YWJDZA==
}
END_TEST

START_TEST(test_encode02) {
    const uint8_t data[] = {'M', 'a', 'n'};
	size_t outLen = sizeof(data);
    size_t outSize = encodeBufferSize(outLen);
    char *encoded = malloc(outSize);

    size_t actualLen;
    encodeBase64Binary(data, outLen, encoded, &actualLen);

    printf("Encoded: %s (len=%zu)\n", encoded, actualLen);
    free(encoded);
}
END_TEST

/* END: Tests */

Suite *encode_decode_suite(void) {
    Suite *s = suite_create("Base 64 Binary");

    {
        /* Grammars test case */
        TCase *tc_encode_decode = tcase_create("Base 64 Binary");
        tcase_add_test(tc_encode_decode, test_encode01);
		tcase_add_test(tc_encode_decode, test_encode02);
        suite_add_tcase(s, tc_encode_decode);
    }

    return s;
}

int main(void) {
    int number_failed;
    Suite *s = encode_decode_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
#ifdef _MSC_VER
    srunner_set_fork_status(sr, CK_NOFORK);
#endif
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
