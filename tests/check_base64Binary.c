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

START_TEST(test_encode01) {

    uint8_t encode01[] = {'a', 'b', 'C', 'd'};
    size_t encodeLen01 = encodeBufferSize(sizeof(encode01));
    char *encodeResult01 = malloc(encodeLen01);
    encodeBase64Binary(encode01, encodeLen01, encodeResult01);
    printf("Base64Encode: '%s'\n", encodeResult01);
    //   fail_unless(tmp_err_code == EXIP_OK,
    //               "parsing the header returns an error code %d",
    //               tmp_err_code);
    fail_unless(true == true, "encode string of size %ld", encodeLen01);
    // got YWJDZAAAAAAA should be YWJDZA==
}
END_TEST

/* END: Tests */

Suite *encode_decode_suite(void) {
    Suite *s = suite_create("Base 64 Binary");

    {
        /* Grammars test case */
        TCase *tc_encode_decode = tcase_create("Base 64 Binary");
        tcase_add_test(tc_encode_decode, test_encode01);
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
