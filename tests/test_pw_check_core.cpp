// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <cstdlib>

extern "C" {
#include "deepin_pw_check.h"
#include "debug.h"

/* ---- formerly static functions (exposed via static= macro) ---- */

void get_validate_policy(char *data, const char *conf_file);

PW_ERROR_TYPE deepin_pw_check_by_conf(const char *user, const char *pw,
                                       int level, const char *dict_path,
                                       const char *conf_file);

PASSWORD_LEVEL_TYPE get_new_passwd_strength_level_by_conf(const char *newPasswd,
                                                           const char *conf_file);

int get_pw_min_length_by_conf(int level, const char *conf_file);
int get_pw_max_length_by_conf(int level, const char *conf_file);
int get_pw_min_character_type_by_conf(int level, const char *conf_file);
char *get_pw_validate_policy_by_conf(int level, const char *conf_file);
int get_pw_palimdrome_num_by_conf(int level, const char *conf_file);
int get_pw_monotone_character_num_by_conf(int level, const char *conf_file);
int get_pw_consecutive_same_character_num_by_conf(int level, const char *conf_file);
const char *err_to_string_by_conf(PW_ERROR_TYPE err, const char *conf_file);

/* ---- non-static but not in header ---- */
bool is_empty(const char *pw);
bool is_palindrome(const char *pw, int palindrome_min_num);
bool is_include_palindrome(const char *pw, int palindrome_min_num);
PW_ERROR_TYPE is_length_valid(const char *pw, int min_len, int max_len);
bool include_chinese(const char *data);
PW_ERROR_TYPE is_type_valid(const char *pw, char *character_type,
                            int character_num_required);
bool is_word(const char *pw, const char *dict_path);
void get_adjacent_character(char c, char *next, char *last);
bool is_monotone_character(const char *pw, int monotone_num);
bool is_consecutive_same_character(const char *pw, int consecutive_num);
bool is_first_letter_uppercase(const char *pw);
}

/* Stub for word_check (avoids cracklib dependency).
 * Returns 1 for "password", 0 for all other passwords. */
extern "C" int word_check(const char *pw, const char *dict_path) {
    (void)dict_path;
    if (pw && strcmp(pw, "password") == 0) return 1;
    return 0;
}

/* Test config file paths */
static const char *CONF_BASIC    = TESTDATA_DIR "/dde.conf";
static const char *CONF_STRICT   = TESTDATA_DIR "/dde_strict.conf";
static const char *CONF_DISABLED = TESTDATA_DIR "/dde_disabled.conf";
static const char *CONF_USERNAME = TESTDATA_DIR "/dde_username.conf";
static const char *CONF_GRUB2    = TESTDATA_DIR "/grub2_edit_auth.conf";
static const char *CONF_NOPOLICY = TESTDATA_DIR "/dde_nopolicy.conf";
static const char *CONF_MISSING  = TESTDATA_DIR "/nonexistent.conf";
static const char *CONF_WORD     = TESTDATA_DIR "/dde_word.conf";

/* =====================================================================
 * is_empty
 * ===================================================================== */
TEST(IsEmptyTest, EmptyString) {
    EXPECT_TRUE(is_empty(""));
}

TEST(IsEmptyTest, NonEmptyString) {
    EXPECT_FALSE(is_empty("a"));
    EXPECT_FALSE(is_empty("password123"));
}

/* =====================================================================
 * is_palindrome
 * ===================================================================== */
TEST(IsPalindromeTest, TruePalindrome) {
    EXPECT_TRUE(is_palindrome("12344321", 4));
    EXPECT_TRUE(is_palindrome("abcba", 1));
    EXPECT_TRUE(is_palindrome("abba", 2));
}

TEST(IsPalindromeTest, NotPalindrome) {
    EXPECT_FALSE(is_palindrome("1234432", 4));
    EXPECT_FALSE(is_palindrome("abcdef", 1));
}

TEST(IsPalindromeTest, BoundaryLength) {
    EXPECT_TRUE(is_palindrome("abba", 2));
    EXPECT_FALSE(is_palindrome("abc", 2));
}

TEST(IsPalindromeTest, PalindromeMinNumTooLarge) {
    EXPECT_FALSE(is_palindrome("abba", 3));
    EXPECT_FALSE(is_palindrome("aa", 2));
}

TEST(IsPalindromeTest, SingleChar) {
    EXPECT_TRUE(is_palindrome("a", 0));
}

/* =====================================================================
 * is_include_palindrome
 * ===================================================================== */
TEST(IsIncludePalindromeTest, ContainsPalindrome) {
    EXPECT_TRUE(is_include_palindrome("ac123454321", 4));
    EXPECT_TRUE(is_include_palindrome("123454321ac", 4));
    EXPECT_TRUE(is_include_palindrome("ac123454321ac", 4));
}

TEST(IsIncludePalindromeTest, NoPalindrome) {
    EXPECT_FALSE(is_include_palindrome("abcdef", 4));
    EXPECT_FALSE(is_include_palindrome("123456", 4));
}

TEST(IsIncludePalindromeTest, ExactPalindrome) {
    EXPECT_TRUE(is_include_palindrome("abba", 2));
}

/* =====================================================================
 * is_length_valid
 * ===================================================================== */
TEST(IsLengthValidTest, ValidLength) {
    EXPECT_EQ(PW_NO_ERR, is_length_valid("123456", 6, 6));
    EXPECT_EQ(PW_NO_ERR, is_length_valid("123456", 1, 10));
    EXPECT_EQ(PW_NO_ERR, is_length_valid("ab", 2, 10));
}

TEST(IsLengthValidTest, TooShort) {
    EXPECT_EQ(PW_ERR_LENGTH_SHORT, is_length_valid("12345", 6, 10));
    EXPECT_EQ(PW_ERR_LENGTH_SHORT, is_length_valid("", 1, 10));
}

TEST(IsLengthValidTest, TooLong) {
    EXPECT_EQ(PW_ERR_LENGTH_LONG, is_length_valid("1234567", 1, 6));
    EXPECT_EQ(PW_ERR_LENGTH_LONG, is_length_valid("abcdefghijk", 1, 10));
}

TEST(IsLengthValidTest, BoundaryValues) {
    EXPECT_EQ(PW_NO_ERR, is_length_valid("123456", 6, 6));
    EXPECT_EQ(PW_ERR_LENGTH_SHORT, is_length_valid("12345", 6, 6));
    EXPECT_EQ(PW_ERR_LENGTH_LONG, is_length_valid("1234567", 6, 6));
}

/* =====================================================================
 * include_chinese
 * ===================================================================== */
TEST(IncludeChineseTest, AsciiOnly) {
    EXPECT_FALSE(include_chinese("fafadAADF!$'"));
    EXPECT_FALSE(include_chinese(""));
    EXPECT_FALSE(include_chinese("123456"));
}

TEST(IncludeChineseTest, ContainsChinese) {
    EXPECT_TRUE(include_chinese("fafadAADF\xe6\xb5\x8b\xe8\xaf\x95"));
    EXPECT_TRUE(include_chinese("\xe4\xb8\xad\xe6\x96\x87"));
}

TEST(IncludeChineseTest, HighBitSpecialChars) {
    EXPECT_TRUE(include_chinese("\xa3\xa4"));
}

/* =====================================================================
 * is_type_valid
 * ===================================================================== */
TEST(IsTypeValidTest, ValidSingleType) {
    EXPECT_EQ(PW_NO_ERR, is_type_valid("123456", "1234567890", 1));
    EXPECT_EQ(PW_NO_ERR, is_type_valid("ABCDEF", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1));
    EXPECT_EQ(PW_NO_ERR, is_type_valid("abcdef", "abcdefghijklmnopqrstuvwxyz", 1));
}

TEST(IsTypeValidTest, MultipleTypesMet) {
    EXPECT_EQ(PW_NO_ERR, is_type_valid("123456A",
        "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2));
    EXPECT_EQ(PW_NO_ERR, is_type_valid("123456Aa!",
        "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;"
        "!\"#$%&'()*+,-./"
        ":;<=>?@[\\]^_`{|}~ /", 4));
}

TEST(IsTypeValidTest, TooFewTypes) {
    EXPECT_EQ(PW_ERR_CHARACTER_TYPE_TOO_FEW,
        is_type_valid("123456", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2));
    EXPECT_EQ(PW_ERR_CHARACTER_TYPE_TOO_FEW,
        is_type_valid("abc", "abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2));
}

TEST(IsTypeValidTest, InvalidCharacter) {
    EXPECT_EQ(PW_ERR_CHARACTER_INVALID,
        is_type_valid("123456", "12345", 1));
    EXPECT_EQ(PW_ERR_CHARACTER_INVALID,
        is_type_valid("123456a", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2));
    EXPECT_EQ(PW_ERR_CHARACTER_INVALID,
        is_type_valid("123456Aa!", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2));
}

TEST(IsTypeValidTest, ContainsChinese) {
    EXPECT_EQ(PW_ERR_CHARACTER_INVALID,
        is_type_valid("\xe6\xb5\x8b\xe8\xaf\x95", "1234567890;abcdefghijklmnopqrstuvwxyz", 1));
}

TEST(IsTypeValidTest, SpecialCharGroups) {
    EXPECT_EQ(PW_NO_ERR, is_type_valid("1A!",
        "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ;!"
        "\"#$%&'()*+,-./"
        ":;<=>?@[\\]^_`{|}~ /", 3));
    EXPECT_EQ(PW_NO_ERR, is_type_valid("1Aa!",
        "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;!"
        "\"#$%&'()*+,-./"
        ":;<=>?@[\\]^_`{|}~ /", 4));
}

TEST(IsTypeValidTest, AllSameType) {
    EXPECT_EQ(PW_NO_ERR, is_type_valid("aaaaaa",
        "abcdefghijklmnopqrstuvwxyz", 1));
}

TEST(IsTypeValidTest, EmptyPassword) {
    EXPECT_EQ(PW_ERR_CHARACTER_TYPE_TOO_FEW, is_type_valid("", "1234567890", 1));
}

/* =====================================================================
 * get_adjacent_character
 * ===================================================================== */
TEST(GetAdjacentCharacterTest, MiddleOfRow) {
    char next = 0, last = 0;
    get_adjacent_character('w', &next, &last);
    EXPECT_EQ('e', next);
    EXPECT_EQ('q', last);
}

TEST(GetAdjacentCharacterTest, FirstInRow) {
    char next = 0, last = 0;
    get_adjacent_character('q', &next, &last);
    EXPECT_EQ('w', next);
    EXPECT_EQ(0, last);
}

TEST(GetAdjacentCharacterTest, LastInRow) {
    char next = 0, last = 0;
    get_adjacent_character('p', &next, &last);
    EXPECT_EQ('[', next);
    EXPECT_EQ('o', last);
}

TEST(GetAdjacentCharacterTest, NumberRowMiddle) {
    char next = 0, last = 0;
    get_adjacent_character('5', &next, &last);
    EXPECT_EQ(0, next);
    EXPECT_EQ(0, last);
}

TEST(GetAdjacentCharacterTest, ShiftedNumberRow) {
    char next = 0, last = 0;
    get_adjacent_character('@', &next, &last);
    EXPECT_EQ('#', next);
    EXPECT_EQ('!', last);
}

TEST(GetAdjacentCharacterTest, CharacterNotFound) {
    char next = 'x', last = 'y';
    get_adjacent_character(' ', &next, &last);
    EXPECT_EQ('x', next);
    EXPECT_EQ('y', last);
}

/* =====================================================================
 * is_monotone_character
 * ===================================================================== */
TEST(IsMonotoneCharacterTest, IncreasingSequence) {
    EXPECT_TRUE(is_monotone_character("123456", 3));
    EXPECT_TRUE(is_monotone_character("abcdef", 3));
}

TEST(IsMonotoneCharacterTest, DecreasingSequence) {
    EXPECT_TRUE(is_monotone_character("654321", 3));
    EXPECT_TRUE(is_monotone_character("fedcba", 3));
}

TEST(IsMonotoneCharacterTest, KeyboardMonotone) {
    EXPECT_TRUE(is_monotone_character("ewq", 3));
    EXPECT_TRUE(is_monotone_character("qwerty", 3));
}

TEST(IsMonotoneCharacterTest, NotMonotone) {
    EXPECT_FALSE(is_monotone_character("12ABab", 3));
    EXPECT_FALSE(is_monotone_character("a1b2c3", 3));
}

TEST(IsMonotoneCharacterTest, SpecialCharMonotone) {
    EXPECT_TRUE(is_monotone_character("!@#$%^&*()_+", 3));
    EXPECT_TRUE(is_monotone_character("P{}", 3));
    EXPECT_TRUE(is_monotone_character(":\"|", 3));
    EXPECT_TRUE(is_monotone_character("<>?", 3));
    EXPECT_TRUE(is_monotone_character("p[]", 3));
    EXPECT_TRUE(is_monotone_character(";'\\", 3));
    EXPECT_TRUE(is_monotone_character(",./", 3));
}

TEST(IsMonotoneCharacterTest, NotMonotoneSpecial) {
    EXPECT_FALSE(is_monotone_character("$%!", 3));
}

TEST(IsMonotoneCharacterTest, ShortString) {
    EXPECT_FALSE(is_monotone_character("a", 3));
    EXPECT_FALSE(is_monotone_character("ab", 3));
}

TEST(IsMonotoneCharacterTest, ExactThreshold) {
    EXPECT_TRUE(is_monotone_character("123", 3));
    EXPECT_FALSE(is_monotone_character("12a", 3));
}

TEST(IsMonotoneCharacterTest, MonotoneNumOne) {
    EXPECT_TRUE(is_monotone_character("ab", 1));
    EXPECT_FALSE(is_monotone_character("a", 1));
}

/* =====================================================================
 * is_consecutive_same_character
 * ===================================================================== */
TEST(IsConsecutiveSameTest, HasConsecutive) {
    EXPECT_TRUE(is_consecutive_same_character("aaabbc", 3));
    EXPECT_TRUE(is_consecutive_same_character("aaa", 3));
    EXPECT_TRUE(is_consecutive_same_character("aabbb", 3));
}

TEST(IsConsecutiveSameTest, NoConsecutive) {
    EXPECT_FALSE(is_consecutive_same_character("aabbcc", 3));
    EXPECT_FALSE(is_consecutive_same_character("ababab", 2));
}

TEST(IsConsecutiveSameTest, Boundary) {
    EXPECT_TRUE(is_consecutive_same_character("aa", 2));
    EXPECT_FALSE(is_consecutive_same_character("ab", 2));
    EXPECT_FALSE(is_consecutive_same_character("a", 2));
}

TEST(IsConsecutiveSameTest, ExactThreshold) {
    EXPECT_TRUE(is_consecutive_same_character("xxx", 3));
    EXPECT_FALSE(is_consecutive_same_character("xxy", 3));
}

/* =====================================================================
 * is_first_letter_uppercase
 * ===================================================================== */
TEST(IsFirstLetterUppercaseTest, UppercaseFirst) {
    EXPECT_TRUE(is_first_letter_uppercase("Fdajflkajsdl"));
    EXPECT_TRUE(is_first_letter_uppercase("A"));
    EXPECT_TRUE(is_first_letter_uppercase("Zbc"));
}

TEST(IsFirstLetterUppercaseTest, LowercaseFirst) {
    EXPECT_FALSE(is_first_letter_uppercase("fdajflkajsdl"));
    EXPECT_FALSE(is_first_letter_uppercase("aBC"));
}

TEST(IsFirstLetterUppercaseTest, NonLetterFirst) {
    EXPECT_FALSE(is_first_letter_uppercase("1abcdef"));
    EXPECT_FALSE(is_first_letter_uppercase("!abcdef"));
}

TEST(IsFirstLetterUppercaseTest, EmptyString) {
    EXPECT_FALSE(is_first_letter_uppercase(""));
}

/* =====================================================================
 * is_word (uses stub word_check)
 * ===================================================================== */
TEST(IsWordTest, StubReturnsTrueForPassword) {
    EXPECT_TRUE(is_word("password", "/some/dict"));
}

TEST(IsWordTest, StubReturnsFalseForOther) {
    EXPECT_FALSE(is_word("notaword", "/some/dict"));
    EXPECT_FALSE(is_word("test", nullptr));
}

/* =====================================================================
 * get_validate_policy
 * ===================================================================== */
TEST(GetValidatePolicyTest, ReadsPolicy) {
    char data[512] = {0};
    get_validate_policy(data, CONF_BASIC);
    EXPECT_NE(nullptr, strstr(data, "1234567890"));
}

TEST(GetValidatePolicyTest, MissingFile) {
    char data[512] = {0};
    get_validate_policy(data, CONF_MISSING);
    EXPECT_EQ('\0', data[0]);
}

/* =====================================================================
 * get_pw_*_by_conf functions
 * ===================================================================== */
TEST(GetPwMinLengthByConfTest, BasicConf) {
    EXPECT_EQ(2, get_pw_min_length_by_conf(0, CONF_BASIC));
    EXPECT_EQ(2, get_pw_min_length_by_conf(1, CONF_BASIC));
}

TEST(GetPwMinLengthByConfTest, StrictConf) {
    EXPECT_EQ(8, get_pw_min_length_by_conf(0, CONF_STRICT));
}

TEST(GetPwMinLengthByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_min_length_by_conf(0, CONF_MISSING));
}

TEST(GetPwMaxLengthByConfTest, BasicConf) {
    EXPECT_EQ(10, get_pw_max_length_by_conf(0, CONF_BASIC));
}

TEST(GetPwMaxLengthByConfTest, StrictConf) {
    EXPECT_EQ(512, get_pw_max_length_by_conf(0, CONF_STRICT));
}

TEST(GetPwMaxLengthByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_max_length_by_conf(0, CONF_MISSING));
}

TEST(GetPwMinCharacterTypeByConfTest, BasicConf) {
    EXPECT_EQ(1, get_pw_min_character_type_by_conf(0, CONF_BASIC));
}

TEST(GetPwMinCharacterTypeByConfTest, StrictConf) {
    EXPECT_EQ(3, get_pw_min_character_type_by_conf(0, CONF_STRICT));
}

TEST(GetPwMinCharacterTypeByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_min_character_type_by_conf(0, CONF_MISSING));
}

TEST(GetPwValidatePolicyByConfTest, ReturnsPolicy) {
    char *policy = get_pw_validate_policy_by_conf(0, CONF_BASIC);
    ASSERT_NE(nullptr, policy);
    EXPECT_NE(nullptr, strstr(policy, "1234567890"));
}

TEST(GetPwValidatePolicyByConfTest, MissingFile) {
    char *policy = get_pw_validate_policy_by_conf(0, CONF_MISSING);
    EXPECT_STREQ("", policy);
}

TEST(GetPwPalimdromeNumByConfTest, BasicConf) {
    EXPECT_EQ(0, get_pw_palimdrome_num_by_conf(0, CONF_BASIC));
}

TEST(GetPwPalimdromeNumByConfTest, StrictConf) {
    EXPECT_EQ(3, get_pw_palimdrome_num_by_conf(0, CONF_STRICT));
}

TEST(GetPwPalimdromeNumByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_palimdrome_num_by_conf(0, CONF_MISSING));
}

TEST(GetPwMonotoneNumByConfTest, BasicConf) {
    EXPECT_EQ(0, get_pw_monotone_character_num_by_conf(0, CONF_BASIC));
}

TEST(GetPwMonotoneNumByConfTest, StrictConf) {
    EXPECT_EQ(3, get_pw_monotone_character_num_by_conf(0, CONF_STRICT));
}

TEST(GetPwMonotoneNumByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_monotone_character_num_by_conf(0, CONF_MISSING));
}

TEST(GetPwConsecutiveSameNumByConfTest, BasicConf) {
    EXPECT_EQ(0, get_pw_consecutive_same_character_num_by_conf(0, CONF_BASIC));
}

TEST(GetPwConsecutiveSameNumByConfTest, StrictConf) {
    EXPECT_EQ(3, get_pw_consecutive_same_character_num_by_conf(0, CONF_STRICT));
}

TEST(GetPwConsecutiveSameNumByConfTest, MissingFile) {
    EXPECT_EQ(-1, get_pw_consecutive_same_character_num_by_conf(0, CONF_MISSING));
}

/* =====================================================================
 * get_new_passwd_strength_level_by_conf
 * ===================================================================== */
TEST(GetStrengthLevelByConfTest, NullPassword) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_ERROR,
        get_new_passwd_strength_level_by_conf(nullptr, CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, MissingConfig) {
    /* load_pwd_conf may fork/exec /usr/bin/pwd-conf-update on config load
     * failure, causing system side effects. The expected value remains
     * deterministic: CONF_MISSING points to a nonexistent path, so after
     * retry load_pwd_conf still returns -1. */
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_ERROR,
        get_new_passwd_strength_level_by_conf("Abc123!@#", CONF_MISSING));
}

TEST(GetStrengthLevelByConfTest, LowLevel) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_LOW,
        get_new_passwd_strength_level_by_conf("a", CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, MiddleLevel) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_MIDDLE,
        get_new_passwd_strength_level_by_conf("abcABC", CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, HighLevel) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_HIGH,
        get_new_passwd_strength_level_by_conf("abcABC123!", CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, OnlyNumbers) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_LOW,
        get_new_passwd_strength_level_by_conf("123456789012", CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, EmptyPassword) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_LOW,
        get_new_passwd_strength_level_by_conf("", CONF_BASIC));
}

TEST(GetStrengthLevelByConfTest, StrictConfHigh) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_HIGH,
        get_new_passwd_strength_level_by_conf("Abcdef1234!", CONF_STRICT));
}

TEST(GetStrengthLevelByConfTest, StrictConfLow) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_LOW,
        get_new_passwd_strength_level_by_conf("ab", CONF_STRICT));
}

TEST(GetStrengthLevelByConfTest, StrictConfMiddle) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_MIDDLE,
        get_new_passwd_strength_level_by_conf("Abcdef12", CONF_STRICT));
}

/* =====================================================================
 * deepin_pw_check_by_conf
 * ===================================================================== */
TEST(DeepinPwCheckByConfTest, EmptyPassword) {
    EXPECT_EQ(PW_ERR_PASSWORD_EMPTY,
        deepin_pw_check_by_conf("user", "", 1, nullptr, CONF_BASIC));
}

TEST(DeepinPwCheckByConfTest, DisabledReturnsOk) {
    /* Empty password "" is caught by is_empty before !enabled check,
     * so only test with non-empty password "x". */
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "x", 1, nullptr, CONF_DISABLED));
}

TEST(DeepinPwCheckByConfTest, TooShort) {
    EXPECT_EQ(PW_ERR_LENGTH_SHORT,
        deepin_pw_check_by_conf("user", "a", 1, nullptr, CONF_BASIC));
}

TEST(DeepinPwCheckByConfTest, TooLong) {
    std::string longpw(11, 'a');
    EXPECT_EQ(PW_ERR_LENGTH_LONG,
        deepin_pw_check_by_conf("user", longpw.c_str(), 1, nullptr, CONF_BASIC));
}

TEST(DeepinPwCheckByConfTest, ValidPassword) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "abc123", 1, nullptr, CONF_BASIC));
}

TEST(DeepinPwCheckByConfTest, MissingConfig) {
    /* load_pwd_conf may fork/exec /usr/bin/pwd-conf-update on config load
     * failure, causing system side effects. The expected value remains
     * deterministic: CONF_MISSING points to a nonexistent path, so after
     * retry load_pwd_conf still returns -1 -> get_default_options returns
     * NULL -> PW_ERR_PARA. */
    EXPECT_EQ(PW_ERR_PARA,
        deepin_pw_check_by_conf("user", "abc123", 1, nullptr, CONF_MISSING));
}

TEST(DeepinPwCheckByConfTest, FirstLetterUppercaseFail) {
    EXPECT_EQ(PW_ERR_PW_FIRST_UPPERM,
        deepin_pw_check_by_conf("user", "abcdef12!", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, FirstLetterUppercasePass) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "Aq7!mK3z", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, SameAsUsername) {
    EXPECT_EQ(PW_ERR_SAME_AS_USERNAME,
        deepin_pw_check_by_conf("Abc12345", "Abc12345", 1, nullptr, CONF_USERNAME));
}

TEST(DeepinPwCheckByConfTest, DifferentFromUsername) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "Abc12345", 1, nullptr, CONF_USERNAME));
}

TEST(DeepinPwCheckByConfTest, InvalidCharacter) {
    EXPECT_EQ(PW_ERR_CHARACTER_INVALID,
        deepin_pw_check_by_conf("user", "ab\x01", 1, nullptr, CONF_BASIC));
}

TEST(DeepinPwCheckByConfTest, TooFewTypes) {
    EXPECT_EQ(PW_ERR_CHARACTER_TYPE_TOO_FEW,
        deepin_pw_check_by_conf("user", "AQMRBNXZ", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, Palindrome) {
    EXPECT_EQ(PW_ERR_PALINDROME,
        deepin_pw_check_by_conf("user", "Ax9aaaaaa", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, MonotoneCharacter) {
    EXPECT_EQ(PW_ERR_PW_MONOTONE,
        deepin_pw_check_by_conf("user", "Abc12345", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, ConsecutiveSame) {
    EXPECT_EQ(PW_ERR_PW_CONSECUTIVE_SAME,
        deepin_pw_check_by_conf("user", "Ax9!kQzzz", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, ValidStrictPassword) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "Ax7!kQ9z", 1, nullptr, CONF_STRICT));
}

TEST(DeepinPwCheckByConfTest, WordCheckFail) {
    /* word_check stub returns 1 for "password", triggering PW_ERR_WORD. */
    EXPECT_EQ(PW_ERR_WORD,
        deepin_pw_check_by_conf("user", "password", 1, nullptr, CONF_WORD));
}

TEST(DeepinPwCheckByConfTest, WordCheckPass) {
    /* Non-word password passes word_check. */
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "abc123", 1, nullptr, CONF_WORD));
}

TEST(DeepinPwCheckByConfTest, DictPathProvided) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_by_conf("user", "abc123", 1, "/some/path", CONF_BASIC));
}

/* =====================================================================
 * err_to_string_by_conf
 * ===================================================================== */
TEST(ErrToStringByConfTest, NoError) {
    const char *s = err_to_string_by_conf(PW_NO_ERR, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, PasswordEmpty) {
    const char *s = err_to_string_by_conf(PW_ERR_PASSWORD_EMPTY, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, LengthShort) {
    const char *s = err_to_string_by_conf(PW_ERR_LENGTH_SHORT, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, LengthLong) {
    const char *s = err_to_string_by_conf(PW_ERR_LENGTH_LONG, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, CharacterInvalid) {
    const char *s = err_to_string_by_conf(PW_ERR_CHARACTER_INVALID, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, Palindrome) {
    const char *s = err_to_string_by_conf(PW_ERR_PALINDROME, CONF_STRICT);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, ConsecutiveSame) {
    const char *s = err_to_string_by_conf(PW_ERR_PW_CONSECUTIVE_SAME, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, Monotone) {
    const char *s = err_to_string_by_conf(PW_ERR_PW_MONOTONE, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, FirstUpperm) {
    const char *s = err_to_string_by_conf(PW_ERR_PW_FIRST_UPPERM, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, Word) {
    const char *s = err_to_string_by_conf(PW_ERR_WORD, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, Para) {
    const char *s = err_to_string_by_conf(PW_ERR_PARA, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, Internal) {
    const char *s = err_to_string_by_conf(PW_ERR_INTERNAL, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, User) {
    const char *s = err_to_string_by_conf(PW_ERR_USER, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, PwRepeat) {
    const char *s = err_to_string_by_conf(PW_ERR_PW_REPEAT, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, CharacterTypeTooFew) {
    const char *s = err_to_string_by_conf(PW_ERR_CHARACTER_TYPE_TOO_FEW, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, SameAsUsername) {
    const char *s = err_to_string_by_conf(PW_ERR_SAME_AS_USERNAME, CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

TEST(ErrToStringByConfTest, DefaultCase) {
    const char *s = err_to_string_by_conf(static_cast<PW_ERROR_TYPE>(9999), CONF_BASIC);
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

/* =====================================================================
 * Public API wrappers (use hardcoded /etc/deepin/dde.conf, expect failure)
 * ===================================================================== */
TEST(PublicApiTest, DeepinPwCheckMissingConfig) {
    /* Smoke test: PASSWD_CONF_FILE is hardcoded, load_pwd_conf may fork/exec
     * pwd-conf-update on failure. Just verify no crash. */
    deepin_pw_check("user", "test", 1, nullptr);
}

TEST(PublicApiTest, GetPwMinLengthMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_min_length(1);
}

TEST(PublicApiTest, GetPwMaxLengthMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_max_length(1);
}

TEST(PublicApiTest, GetPwMinCharTypeMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_min_character_type(1);
}

TEST(PublicApiTest, GetPwPalimdromeNumMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_palimdrome_num(1);
}

TEST(PublicApiTest, GetPwMonotoneNumMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_monotone_character_num(1);
}

TEST(PublicApiTest, GetPwConsecutiveSameNumMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_consecutive_same_character_num(1);
}

TEST(PublicApiTest, GetStrengthLevelMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_new_passwd_strength_level("Abc123!");
}

TEST(PublicApiTest, ErrToStringReturnsNonNull) {
    const char *s = err_to_string(PW_NO_ERR);
    ASSERT_NE(nullptr, s);
}

TEST(PublicApiTest, GetPwValidatePolicyMissingConfig) {
    /* Smoke test: hardcoded config path, no specific return value asserted. */
    get_pw_validate_policy(1);
}

/* =====================================================================
 * Grub2 API (uses overridden PASSWD_CONF_FILE_GRUB2 path)
 * ===================================================================== */
TEST(Grub2ApiTest, DeepinPwCheckGrub2) {
    EXPECT_EQ(PW_NO_ERR,
        deepin_pw_check_grub2("user", "abc", 1, nullptr));
}

TEST(Grub2ApiTest, DeepinPwCheckGrub2Empty) {
    EXPECT_EQ(PW_ERR_PASSWORD_EMPTY,
        deepin_pw_check_grub2("user", "", 1, nullptr));
}

TEST(Grub2ApiTest, DeepinPwCheckGrub2TooShort) {
    EXPECT_EQ(PW_ERR_LENGTH_SHORT,
        deepin_pw_check_grub2("user", "ab", 1, nullptr));
}

TEST(Grub2ApiTest, DeepinPwCheckGrub2TooLong) {
    std::string longpw(9, 'a');
    EXPECT_EQ(PW_ERR_LENGTH_LONG,
        deepin_pw_check_grub2("user", longpw.c_str(), 1, nullptr));
}

TEST(Grub2ApiTest, GetPwMinLengthGrub2) {
    EXPECT_EQ(3, get_pw_min_length_grub2(0));
}

TEST(Grub2ApiTest, GetPwMaxLengthGrub2) {
    EXPECT_EQ(8, get_pw_max_length_grub2(0));
}

TEST(Grub2ApiTest, GetPwMinCharTypeGrub2) {
    EXPECT_EQ(1, get_pw_min_character_type_grub2(0));
}

TEST(Grub2ApiTest, GetPwValidatePolicyGrub2) {
    char *s = get_pw_validate_policy_grub2(0);
    ASSERT_NE(nullptr, s);
    EXPECT_NE(nullptr, strstr(s, "1234567890"));
}

TEST(Grub2ApiTest, GetPwPalimdromeNumGrub2) {
    EXPECT_EQ(0, get_pw_palimdrome_num_grub2(0));
}

TEST(Grub2ApiTest, GetPwMonotoneNumGrub2) {
    EXPECT_EQ(0, get_pw_monotone_character_num_grub2(0));
}

TEST(Grub2ApiTest, GetPwConsecutiveSameNumGrub2) {
    EXPECT_EQ(0, get_pw_consecutive_same_character_num_grub2(0));
}

TEST(Grub2ApiTest, GetStrengthLevelGrub2) {
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_HIGH,
        get_new_passwd_strength_level_grub2("abcABC123!"));
    EXPECT_EQ(PASSWORD_STRENGTH_LEVEL_LOW,
        get_new_passwd_strength_level_grub2("a"));
}

TEST(Grub2ApiTest, ErrToStringGrub2AllTypes) {
    PW_ERROR_TYPE types[] = {
        PW_NO_ERR, PW_ERR_PASSWORD_EMPTY, PW_ERR_LENGTH_SHORT,
        PW_ERR_LENGTH_LONG, PW_ERR_CHARACTER_INVALID, PW_ERR_PALINDROME,
        PW_ERR_WORD, PW_ERR_PW_REPEAT, PW_ERR_PW_MONOTONE,
        PW_ERR_PW_CONSECUTIVE_SAME, PW_ERR_PW_FIRST_UPPERM, PW_ERR_PARA,
        PW_ERR_INTERNAL, PW_ERR_USER, PW_ERR_CHARACTER_TYPE_TOO_FEW,
        PW_ERR_SAME_AS_USERNAME
    };
    for (auto t : types) {
        const char *s = err_to_string_grub2(t);
        EXPECT_NE(nullptr, s);
        EXPECT_STRNE("", s);
    }
}

TEST(Grub2ApiTest, ErrToStringGrub2Default) {
    const char *s = err_to_string_grub2(static_cast<PW_ERROR_TYPE>(9999));
    ASSERT_NE(nullptr, s);
    EXPECT_STRNE("", s);
}

/* =====================================================================
 * set_debug_flag / get_debug_flag (from debug.c, linked via pw_check_core_obj)
 * ===================================================================== */
TEST(DebugFlagTest, SetAndGet) {
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
    set_debug_flag(1);
    EXPECT_EQ(1, get_debug_flag());
    set_debug_flag(0);
    EXPECT_EQ(0, get_debug_flag());
}

