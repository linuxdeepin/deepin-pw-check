// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {
#include "common.h"
char *bigcrypt(const char *key, const char *salt);
}

/* =====================================================================
 * bigcrypt — basic functionality
 * ===================================================================== */
TEST(BigCryptTest, ShortPassword) {
    /* Short password (< 8 chars) → single segment */
    char *result = bigcrypt("abc", "ab");
    ASSERT_NE(nullptr, result);
    /* Traditional DES crypt with salt "ab" produces 13-char output */
    EXPECT_EQ(strlen(result), 13U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, EmptyPassword) {
    /* Empty password → keylen=0, n_seg=1 */
    char *result = bigcrypt("", "ab");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(strlen(result), 13U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, ExactlyEightChars) {
    /* 8-char password → n_seg = 1 + ((8-1)/8) = 1 */
    char *result = bigcrypt("12345678", "ab");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(strlen(result), 13U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, NineCharsTwoSegments) {
    /* 9-char password → n_seg = 1 + ((9-1)/8) = 2 */
    char *result = bigcrypt("123456789", "ab");
    ASSERT_NE(nullptr, result);
    /* Two segments: 2 + 11 = 13 for first, + 11 for second = 24 */
    EXPECT_EQ(strlen(result), 24U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, SixteenCharsTwoSegments) {
    /* 16-char password → n_seg = 1 + ((16-1)/8) = 2 */
    char *result = bigcrypt("12345678abcdefgh", "ab");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(strlen(result), 24U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, SeventeenCharsThreeSegments) {
    /* 17-char password → n_seg = 1 + ((17-1)/8) = 3 */
    char *result = bigcrypt("12345678901234567", "ab");
    ASSERT_NE(nullptr, result);
    EXPECT_EQ(strlen(result), 35U); /* 13 + 11 + 11 */
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, VeryLongPasswordTruncated) {
    /* Password longer than MAX_PASS_LEN*SEGMENT_SIZE (128) should be
     * truncated to MAX_PASS_LEN segments. */
    std::string longpw(200, 'x');
    char *result = bigcrypt(longpw.c_str(), "ab");
    ASSERT_NE(nullptr, result);
    /* MAX_PASS_LEN=16 segments: 2 + 16*11 = 178 */
    EXPECT_EQ(strlen(result), 178U);
    EXPECT_EQ(0, strncmp(result, "ab", 2));
    free(result);
}

TEST(BigCryptTest, DeterministicOutput) {
    char *r1 = bigcrypt("testpass", "xy");
    char *r2 = bigcrypt("testpass", "xy");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STREQ(r1, r2);
    free(r1);
    free(r2);
}

TEST(BigCryptTest, DifferentPasswordsDifferentOutput) {
    char *r1 = bigcrypt("password1", "ab");
    char *r2 = bigcrypt("password2", "ab");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STRNE(r1, r2);
    free(r1);
    free(r2);
}

TEST(BigCryptTest, DifferentSaltsDifferentOutput) {
    char *r1 = bigcrypt("samepass", "ab");
    char *r2 = bigcrypt("samepass", "cd");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STRNE(r1, r2);
    free(r1);
    free(r2);
}

TEST(BigCryptTest, SaltLongerThanTwoChars) {
    /* When salt length == SALT_SIZE + ESEGMENT_SIZE (2 + 11 = 13),
     * the password is terminated early at SEGMENT_SIZE (8). */
    char *result = bigcrypt("longpassword", "abccccccccccc");
    ASSERT_NE(nullptr, result);
    /* This path truncates key at 8 chars */
    free(result);
}

TEST(BigCryptTest, VerifyCorrectPassword) {
    /* Encrypt a password, then verify: re-encrypting with the same
     * salt should produce the same hash. */
    char *encrypted = bigcrypt("verifypw", "ab");
    ASSERT_NE(nullptr, encrypted);

    char *reencrypted = bigcrypt("verifypw", "ab");
    ASSERT_NE(nullptr, reencrypted);
    EXPECT_STREQ(encrypted, reencrypted);

    free(encrypted);
    free(reencrypted);
}

TEST(BigCryptTest, WrongPasswordDifferentHash) {
    char *encrypted = bigcrypt("correctpw", "ab");
    ASSERT_NE(nullptr, encrypted);

    char *wrong = bigcrypt("wrongpw", "ab");
    ASSERT_NE(nullptr, wrong);
    EXPECT_STRNE(encrypted, wrong);

    free(encrypted);
    free(wrong);
}

TEST(BigCryptTest, MultiSegmentConsistency) {
    /* A password that spans multiple segments should be consistently
     * encrypted. */
    std::string pw = "abcdefghijklmno"; /* 15 chars → 2 segments */
    char *r1 = bigcrypt(pw.c_str(), "ab");
    char *r2 = bigcrypt(pw.c_str(), "ab");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STREQ(r1, r2);
    free(r1);
    free(r2);
}

