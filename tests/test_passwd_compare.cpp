// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {
#include "common.h"

/* formerly static, exposed via static= macro on passwd_compare_obj */
void strip_hpux_aging(char *hash);

/* from md5_crypt.c (linked via md5_obj) */
char *crypt_md5(const char *pw, const char *salt);

/* from bigcrypt.c (linked via bigcrypt_obj) */
char *bigcrypt(const char *key, const char *salt);

/* from passwd_compare.c */
int verify_pwd(const char *p, char *hash, unsigned int nullok);
}

/* Helper: generate a DES crypt hash for a password using libc crypt().
 * We need this for the generic crypt() code path in verify_pwd. */
#include <crypt.h>

static char *make_des_hash(const char *password, const char *salt) {
    char *result = crypt(password, salt);
    if (result) return strdup(result);
    return nullptr;
}

/* =====================================================================
 * strip_hpux_aging — static function exposed via static= macro
 * ===================================================================== */
TEST(StripHpuxAgingTest, NoStrippingForDollarHash) {
    /* Hashes starting with '$' are not stripped. */
    char hash[] = "$1$abcdefgh$passwordhashstring";
    strip_hpux_aging(hash);
    /* Should remain unchanged */
    EXPECT_EQ('$', hash[0]);
}

TEST(StripHpuxAgingTest, NoStrippingForShortHash) {
    /* Hashes <= 13 chars are not stripped. */
    char hash[] = "abcdefghijklm"; /* exactly 13 chars */
    strip_hpux_aging(hash);
    EXPECT_EQ(0, strcmp(hash, "abcdefghijklm"));
}

TEST(StripHpuxAgingTest, StripsInvalidCharsAfter13) {
    /* Hash > 13 chars with invalid char after position 13 gets truncated. */
    char hash[] = "abcdefghijklm:invalid"; /* ':' is not in valid set */
    strip_hpux_aging(hash);
    /* Should be truncated at the ':' */
    EXPECT_EQ(0, strcmp(hash, "abcdefghijklm"));
}

TEST(StripHpuxAgingTest, KeepsValidCharsAfter13) {
    /* Hash > 13 chars with only valid chars after position 13 is kept. */
    char hash[] = "abcdefghijklmABCD1234./";
    strip_hpux_aging(hash);
    /* Should remain unchanged since all chars are valid */
    EXPECT_EQ(0, strcmp(hash, "abcdefghijklmABCD1234./"));
}

TEST(StripHpuxAgingTest, StripsAtFirstInvalidChar) {
    /* Multiple invalid chars — truncates at first one. */
    char hash[] = "abcdefghijklm!@#$";
    strip_hpux_aging(hash);
    EXPECT_EQ(0, strcmp(hash, "abcdefghijklm"));
}

TEST(StripHpuxAgingTest, ValidCharsAllTypes) {
    /* All valid chars: A-Z a-z 0-9 . / */
    char hash[] = "abcdefghijklmABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
    strip_hpux_aging(hash);
    /* Should remain unchanged — all chars valid */
    EXPECT_STREQ("abcdefghijklmABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./", hash);
}

/* =====================================================================
 * verify_pwd — empty hash + nullok
 * ===================================================================== */
TEST(VerifyPwdTest, EmptyHashNullokTrue) {
    char hash[] = "";
    EXPECT_EQ(0, verify_pwd("password", hash, 1));
}

TEST(VerifyPwdTest, EmptyHashNullokFalse) {
    char hash[] = "";
    EXPECT_EQ(1, verify_pwd("password", hash, 0));
}

/* =====================================================================
 * verify_pwd — NULL password
 * ===================================================================== */
TEST(VerifyPwdTest, NullPassword) {
    char hash[] = "$1$saltsal$somehash";
    EXPECT_EQ(1, verify_pwd(nullptr, hash, 1));
}

/* =====================================================================
 * verify_pwd — hash starting with '*' or '!'
 * ===================================================================== */
TEST(VerifyPwdTest, HashStartsWithStar) {
    char hash[] = "*lockedaccount";
    EXPECT_EQ(1, verify_pwd("password", hash, 1));
}

TEST(VerifyPwdTest, HashStartsWithBang) {
    char hash[] = "!disabledaccount";
    EXPECT_EQ(1, verify_pwd("password", hash, 1));
}

/* =====================================================================
 * verify_pwd — MD5 ($1$) path
 * ===================================================================== */
TEST(VerifyPwdTest, Md5CorrectPassword) {
    /* Generate a valid MD5 hash, then verify it. */
    char *encrypted = crypt_md5("testpassword", "$1$verifysa");
    ASSERT_NE(nullptr, encrypted);

    /* Make a mutable copy for verify_pwd (strip_hpux_aging may modify) */
    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("testpassword", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, Md5WrongPassword) {
    char *encrypted = crypt_md5("correctpass", "$1$saltsal");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(1, verify_pwd("wrongpassword", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, Md5RetryPath) {
    /* When crypt_md5 first result doesn't match, the retry path is taken.
     * This happens naturally with a wrong password. */
    char *encrypted = crypt_md5("realpass", "$1$abc");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    /* Wrong password triggers the retry (strcmp != 0), then still fails. */
    EXPECT_EQ(1, verify_pwd("fakepass", hash, 1));

    free(encrypted);
    free(hash);
}

/* =====================================================================
 * verify_pwd — bigcrypt path (non-$, hash_len >= 13)
 * ===================================================================== */
TEST(VerifyPwdTest, BigcryptCorrectPassword) {
    /* Use bigcrypt to generate a hash, then verify it. */
    char *encrypted = bigcrypt("testpassword", "ab");
    ASSERT_NE(nullptr, encrypted);
    ASSERT_GE(strlen(encrypted), 13U);

    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("testpassword", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, BigcryptWrongPassword) {
    char *encrypted = bigcrypt("correctpass", "ab");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(1, verify_pwd("wrongpass", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, BigcryptLongPasswordCorrect) {
    /* Long password spanning multiple segments. */
    char *encrypted = bigcrypt("verylongpassword123", "ab");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("verylongpassword123", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, BigcryptTruncatePath) {
    /* When hash_len == 13 and pp is longer, the extra is overwritten.
     * This exercises the hash_len == 13 special case in bigcrypt path. */
    char *encrypted = bigcrypt("shortpw", "ab");
    ASSERT_NE(nullptr, encrypted);

    /* If the hash is exactly 13 chars (single segment), the truncate
     * path in verify_pwd is exercised. */
    char *hash = strdup(encrypted);
    if (strlen(hash) == 13) {
        EXPECT_EQ(0, verify_pwd("shortpw", hash, 1));
    }
    free(encrypted);
    free(hash);
}

/* =====================================================================
 * verify_pwd — generic crypt() path
 * ===================================================================== */
TEST(VerifyPwdTest, GenericCryptCorrectPassword) {
    /* Use a hash format that doesn't start with '$' and is < 13 chars
     * won't work (needs >= 13 for bigcrypt). Use a $5$ or $6$ hash
     * to hit the else branch (generic crypt). */
    char *encrypted = crypt("testpassword", "$5$saltsalt");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("testpassword", hash, 1));

    free(hash);
}

TEST(VerifyPwdTest, GenericCryptWrongPassword) {
    char *encrypted = crypt("correctpass", "$5$saltsalt");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(1, verify_pwd("wrongpass", hash, 1));

    free(hash);
}

TEST(VerifyPwdTest, GenericCryptSha512) {
    char *encrypted = crypt("mypassword", "$6$rounds=5000$saltsalt");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("mypassword", hash, 1));

    /* Wrong password */
    char *hash2 = strdup(encrypted);
    EXPECT_EQ(1, verify_pwd("notmypassword", hash2, 1));

    /* Do not free(encrypted): crypt() returns a static/TLS buffer, not heap. */
    free(hash);
    free(hash2);
}

/* =====================================================================
 * verify_pwd — boundary / edge cases
 * ===================================================================== */
TEST(VerifyPwdTest, EmptyPasswordMd5) {
    char *encrypted = crypt_md5("", "$1$emptysal");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, EmptyPasswordWrongMd5) {
    char *encrypted = crypt_md5("nonempty", "$1$saltsal");
    ASSERT_NE(nullptr, encrypted);

    char *hash = strdup(encrypted);
    EXPECT_EQ(1, verify_pwd("", hash, 1));

    free(encrypted);
    free(hash);
}

TEST(VerifyPwdTest, NullokDoesNotAffectNonEmptyHash) {
    /* nullok only matters when hash is empty. */
    char *encrypted = crypt_md5("testpass", "$1$saltsal");
    ASSERT_NE(nullptr, encrypted);

    char *hash1 = strdup(encrypted);
    char *hash2 = strdup(encrypted);
    EXPECT_EQ(0, verify_pwd("testpass", hash1, 0));
    EXPECT_EQ(0, verify_pwd("testpass", hash2, 1));

    free(encrypted);
    free(hash1);
    free(hash2);
}

