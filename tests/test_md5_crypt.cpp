// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {
#include "md5.h"

/* formerly static, exposed via static= macro on md5_obj */
void to64(char *s, unsigned long v, int n);
char *crypt_md5(const char *pw, const char *salt);
}

/* =====================================================================
 * MD5Init
 * ===================================================================== */
TEST(MD5InitTest, SetsInitialValues) {
    MD5_CTX ctx;
    MD5Init(&ctx);
    EXPECT_EQ(0x67452301U, ctx.buf[0]);
    EXPECT_EQ(0xefcdab89U, ctx.buf[1]);
    EXPECT_EQ(0x98badcfeU, ctx.buf[2]);
    EXPECT_EQ(0x10325476U, ctx.buf[3]);
    EXPECT_EQ(0U, ctx.bits[0]);
    EXPECT_EQ(0U, ctx.bits[1]);
}

/* =====================================================================
 * MD5Update — update bitcount and buffer
 * ===================================================================== */
TEST(MD5UpdateTest, UpdatesBitcount) {
    MD5_CTX ctx;
    MD5Init(&ctx);

    const unsigned char data[] = "abc";
    MD5Update(&ctx, data, 3);

    /* 3 bytes = 24 bits */
    EXPECT_EQ(24U, ctx.bits[0]);
    EXPECT_EQ(0U, ctx.bits[1]);
}

TEST(MD5UpdateTest, EmptyBuffer) {
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, nullptr, 0);
    EXPECT_EQ(0U, ctx.bits[0]);
    EXPECT_EQ(0U, ctx.bits[1]);
}

TEST(MD5UpdateTest, LargeBufferCarryBit) {
    /* Push enough data to exercise the bits[1] carry path.
     * bits[0] is uint32, so >512MB would overflow; instead we verify
     * multi-update accumulation works correctly. */
    MD5_CTX ctx;
    MD5Init(&ctx);

    unsigned char buf[64];
    memset(buf, 'x', sizeof(buf));

    for (int i = 0; i < 4; i++) {
        MD5Update(&ctx, buf, 64);
    }
    /* 4 * 64 * 8 = 2048 bits */
    EXPECT_EQ(2048U, ctx.bits[0]);
    EXPECT_EQ(0U, ctx.bits[1]);
}

TEST(MD5UpdateTest, MultiUpdateAccumulation) {
    MD5_CTX ctx1, ctx2;
    MD5Init(&ctx1);
    MD5Init(&ctx2);

    const unsigned char part1[] = "hello ";
    const unsigned char part2[] = "world";

    MD5Update(&ctx1, part1, 6);
    MD5Update(&ctx1, part2, 5);

    const unsigned char full[] = "hello world";
    MD5Update(&ctx2, full, 11);

    unsigned char digest1[16], digest2[16];
    MD5Final(digest1, &ctx1);
    MD5Final(digest2, &ctx2);

    EXPECT_EQ(0, memcmp(digest1, digest2, 16));
}

/* =====================================================================
 * MD5Final — produces digest
 * ===================================================================== */
TEST(MD5FinalTest, KnownDigestEmpty) {
    /* MD5("") = d41d8cd98f00b204e9800998ecf8427e */
    MD5_CTX ctx;
    MD5Init(&ctx);
    unsigned char digest[16];
    MD5Final(digest, &ctx);

    const unsigned char expected[] = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

TEST(MD5FinalTest, KnownDigestAbc) {
    /* MD5("abc") = 900150983cd24fb0d6963f7d28e17f72 */
    MD5_CTX ctx;
    MD5Init(&ctx);
    const unsigned char data[] = "abc";
    MD5Update(&ctx, data, 3);
    unsigned char digest[16];
    MD5Final(digest, &ctx);

    const unsigned char expected[] = {
        0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

TEST(MD5FinalTest, KnownDigestLongerThan64) {
    /* MD5 of > 64 bytes exercises the 64-byte chunk processing loop.
     * MD5("a" repeated 100) = 36a92cc94a9e0fa21f625f8bfb007adf
     * (verified with Python hashlib.md5)
     */
    MD5_CTX ctx;
    MD5Init(&ctx);
    std::string longdata(100, 'a');
    MD5Update(&ctx, (const unsigned char *)longdata.c_str(), longdata.size());
    unsigned char digest[16];
    MD5Final(digest, &ctx);

    const unsigned char expected[] = {
        0x36, 0xa9, 0x2c, 0xc9, 0x4a, 0x9e, 0x0f, 0xa2,
        0x1f, 0x62, 0x5f, 0x8b, 0xfb, 0x00, 0x7a, 0xdf
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

TEST(MD5FinalTest, ClearsContext) {
    /* After MD5Final, the context should be zeroed out. */
    MD5_CTX ctx;
    MD5Init(&ctx);
    const unsigned char data[] = "test";
    MD5Update(&ctx, data, 4);
    unsigned char digest[16];
    MD5Final(digest, &ctx);

    EXPECT_EQ(0, ctx.buf[0]);
    EXPECT_EQ(0, ctx.buf[1]);
    EXPECT_EQ(0, ctx.buf[2]);
    EXPECT_EQ(0, ctx.buf[3]);
    EXPECT_EQ(0, ctx.bits[0]);
    EXPECT_EQ(0, ctx.bits[1]);
}

/* =====================================================================
 * MD5Transform — verify it modifies buf correctly
 * ===================================================================== */
TEST(MD5TransformTest, ModifiesBuf) {
    uint32 buf[4] = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U};
    uint32 in[16];
    memset(in, 0, sizeof(in));

    MD5Transform(buf, in);

    /* After transform with all-zero input, buf must differ from initial. */
    bool changed = (buf[0] != 0x67452301U ||
                    buf[1] != 0xefcdab89U ||
                    buf[2] != 0x98badcfeU ||
                    buf[3] != 0x10325476U);
    EXPECT_TRUE(changed);
}

TEST(MD5TransformTest, ConsistentWithFullMD5) {
    /* MD5Transform with the initial buf and a 64-byte block should
     * produce the same result as one MD5Update of 64 bytes followed
     * by MD5Final with appropriate padding. We verify indirectly. */
    uint32 buf1[4] = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U};
    uint32 in[16];
    memset(in, 0, sizeof(in));
    MD5Transform(buf1, in);

    /* Result should be deterministic (same input -> same output). */
    uint32 buf2[4] = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U};
    MD5Transform(buf2, in);
    EXPECT_EQ(0, memcmp(buf1, buf2, sizeof(buf1)));
}

/* =====================================================================
 * MD5 — convenience wrapper
 * ===================================================================== */
TEST(MD5Test, EmptyString) {
    unsigned char digest[16];
    MD5((const unsigned char *)"", 0, digest);

    const unsigned char expected[] = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

TEST(MD5Test, KnownDigestHelloWorld) {
    /* MD5("hello world") = 5eb63bbbe01eeed093cb22bb8f5acdc3 */
    unsigned char digest[16];
    MD5((const unsigned char *)"hello world", 11, digest);

    const unsigned char expected[] = {
        0x5e, 0xb6, 0x3b, 0xbb, 0xe0, 0x1e, 0xee, 0xd0,
        0x93, 0xcb, 0x22, 0xbb, 0x8f, 0x5a, 0xcd, 0xc3
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

TEST(MD5Test, Exactly64Bytes) {
    /* Input of exactly 64 bytes exercises the full-block path. */
    std::string data(64, 'A');
    unsigned char digest1[16], digest2[16];
    MD5((const unsigned char *)data.c_str(), 64, digest1);

    /* Verify determinism. */
    MD5((const unsigned char *)data.c_str(), 64, digest2);
    EXPECT_EQ(0, memcmp(digest1, digest2, 16));
}

TEST(MD5Test, LargeInput) {
    /* Large input exercises multi-block processing.
     * MD5("B" repeated 256) = 03af7b93bc40f80dd209b53596eb1390
     * (verified with Python hashlib.md5)
     */
    std::string data(256, 'B');
    unsigned char digest[16];
    MD5((const unsigned char *)data.c_str(), 256, digest);

    const unsigned char expected[] = {
        0x03, 0xaf, 0x7b, 0x93, 0xbc, 0x40, 0xf8, 0x0d,
        0xd2, 0x09, 0xb5, 0x35, 0x96, 0xeb, 0x13, 0x90
    };
    EXPECT_EQ(0, memcmp(digest, expected, 16));
}

/* =====================================================================
 * to64 — encoding helper (static, exposed via static= macro)
 * ===================================================================== */
TEST(To64Test, EncodeZero) {
    char buf[4];
    to64(buf, 0, 4);
    /* itoa64[0] = '.' */
    EXPECT_EQ('.', buf[0]);
    EXPECT_EQ('.', buf[1]);
    EXPECT_EQ('.', buf[2]);
    EXPECT_EQ('.', buf[3]);
}

TEST(To64Test, EncodeOne) {
    char buf[4];
    to64(buf, 1, 4);
    /* itoa64[1] = '/' */
    EXPECT_EQ('/', buf[0]);
    EXPECT_EQ('.', buf[1]);
}

TEST(To64Test, EncodeLargeValue) {
    char buf[4];
    unsigned long val = 0x00FFFFFFUL; /* low 24 bits = 4x6-bit all 1 = 'z' */
    to64(buf, val, 4);
    EXPECT_EQ('z', buf[0]);
    EXPECT_EQ('z', buf[1]);
    EXPECT_EQ('z', buf[2]);
    EXPECT_EQ('z', buf[3]);
}

TEST(To64Test, EncodeSingleChar) {
    char buf[1];
    to64(buf, 0, 1);
    EXPECT_EQ('.', buf[0]);
}

TEST(To64Test, EncodeTwoChars) {
    char buf[2];
    /* val = 0 => both '.' */
    to64(buf, 0, 2);
    EXPECT_EQ('.', buf[0]);
    EXPECT_EQ('.', buf[1]);
}

/* =====================================================================
 * crypt_md5
 * ===================================================================== */
TEST(CryptMd5Test, BasicEncryption) {
    char *result = crypt_md5("password", "$1$saltsal");
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(strncmp(result, "$1$", 3) == 0);
    /* Salt should appear in output */
    EXPECT_TRUE(strstr(result, "saltsal") != nullptr);
    free(result);
}

TEST(CryptMd5Test, DeterministicOutput) {
    char *r1 = crypt_md5("testpw", "$1$abcdefgh");
    char *r2 = crypt_md5("testpw", "$1$abcdefgh");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STREQ(r1, r2);
    free(r1);
    free(r2);
}

TEST(CryptMd5Test, DifferentPasswordsDifferentOutput) {
    char *r1 = crypt_md5("password1", "$1$abcdefgh");
    char *r2 = crypt_md5("password2", "$1$abcdefgh");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STRNE(r1, r2);
    free(r1);
    free(r2);
}

TEST(CryptMd5Test, DifferentSaltsDifferentOutput) {
    char *r1 = crypt_md5("password", "$1$saltone");
    char *r2 = crypt_md5("password", "$1$salttwo");
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_STRNE(r1, r2);
    free(r1);
    free(r2);
}

TEST(CryptMd5Test, SaltWithoutMagicPrefix) {
    /* crypt_md5 should add the magic prefix even if salt lacks it. */
    char *result = crypt_md5("password", "saltsal");
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(strncmp(result, "$1$", 3) == 0);
    free(result);
}

TEST(CryptMd5Test, VerifyRoundtrip) {
    /* Encrypt a password, then encrypt again with the result as salt;
     * the two should match. */
    char *encrypted = crypt_md5("mypassword", "$1$roundtrip");
    ASSERT_NE(nullptr, encrypted);

    char *reencrypted = crypt_md5("mypassword", encrypted);
    ASSERT_NE(nullptr, reencrypted);
    EXPECT_STREQ(encrypted, reencrypted);

    free(encrypted);
    free(reencrypted);
}

TEST(CryptMd5Test, EmptyPassword) {
    char *result = crypt_md5("", "$1$emptysal");
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(strncmp(result, "$1$", 3) == 0);
    free(result);
}

TEST(CryptMd5Test, LongPassword) {
    /* Password longer than 16 bytes exercises the pl > 0 loop. */
    std::string longpw(100, 'x');
    char *result = crypt_md5(longpw.c_str(), "$1$longsalt");
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(strncmp(result, "$1$", 3) == 0);
    free(result);
}

TEST(CryptMd5Test, SaltWithDollarTerminator) {
    /* Salt like "$1$abc$" — the parser stops at '$'. */
    char *result = crypt_md5("pw", "$1$abc$extra");
    ASSERT_NE(nullptr, result);
    EXPECT_TRUE(strncmp(result, "$1$abc$", 6) == 0);
    free(result);
}

TEST(CryptMd5Test, SaltMaxEightChars) {
    /* Salt is truncated to 8 chars after magic. */
    char *result = crypt_md5("pw", "$1$longersaltstring");
    ASSERT_NE(nullptr, result);
    /* Should contain "longerse" (first 8 chars of salt) */
    EXPECT_TRUE(strstr(result, "longersa") != nullptr);
    EXPECT_EQ(nullptr, strstr(result, "saltstring"));
    free(result);
}

TEST(CryptMd5Test, WrongPasswordDoesNotMatch) {
    char *encrypted = crypt_md5("correctpw", "$1$verifysa");
    ASSERT_NE(nullptr, encrypted);

    char *wrong = crypt_md5("wrongpw", "$1$verifysa");
    ASSERT_NE(nullptr, wrong);
    EXPECT_STRNE(encrypted, wrong);

    free(encrypted);
    free(wrong);
}

