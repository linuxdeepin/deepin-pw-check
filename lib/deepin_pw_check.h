// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DEEPIN_PW_CHECK_H_
#define _DEEPIN_PW_CHECK_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PW_NO_ERR   = 0,
    PW_ERR_PASSWORD_EMPTY,      // password is empty
    PW_ERR_LENGTH_SHORT,        // password length is too short
    PW_ERR_LENGTH_LONG,         // password length is too long
    PW_ERR_CHARACTER_INVALID,   // password character is invalid
    PW_ERR_PALINDROME,          // password is palimdrome
    PW_ERR_WORD,                // password is based on a word
    PW_ERR_PW_REPEAT,           // password is repeat with old password
    PW_ERR_PW_MONOTONE,         // password is monotone character
    PW_ERR_PW_CONSECUTIVE_SAME, // password is consecutive same character
    PW_ERR_PW_FIRST_UPPERM,     // password is not uppercase with first letter
    PW_ERR_PARA,
    PW_ERR_INTERNAL,
    PW_ERR_USER,                // user name is error
    PW_ERR_CHARACTER_TYPE_TOO_FEW,        // too few password character types
    PW_ERR_SAME_AS_USERNAME     // password same as password
}PW_ERROR_TYPE;

/*
    standard check
    check length and character
*/
#define LEVEL_STANDARD_CHECK  (1)
/*
    strict check
    check length, whether is a word, whether is palindrome,
    whether character is valid.

    NOTE:
        can not combined with LEVEL_STANDARD_CHECK
*/
#define LEVEL_STRICT_CHECK    (2)

/*
    password strength value
*/
typedef enum {
    PASSWORD_STRENGTH_LEVEL_ERROR = 0,
    PASSWORD_STRENGTH_LEVEL_LOW,
    PASSWORD_STRENGTH_LEVEL_MIDDLE,
    PASSWORD_STRENGTH_LEVEL_HIGH,
}PASSWORD_LEVEL_TYPE;
// parameter @level is Deprecated.
PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);

PASSWORD_LEVEL_TYPE  get_new_passwd_strength_level(const char *newPasswd);

const char* err_to_string(PW_ERROR_TYPE err);

extern void set_debug_flag(int flag);

/*
    if error, return -1
*/
extern int get_pw_min_length(int level);

/*
    if error, return -1
*/
extern int get_pw_max_length(int level);

/*
    if error, return -1
*/
extern int get_pw_min_character_type(int level);

/*
    get the validate character policy type. is specified by /etc/deepin/dde.conf. "Password:VALIDATE_POLICY"
*/
extern char* get_pw_validate_policy(int level);

/*
    if error, return -1
*/
extern int get_pw_palimdrome_num(int level);

/*
    if error, return -1
*/
extern int get_pw_monotone_character_num(int level);

/*
    if error, return -1
*/
extern int get_pw_consecutive_same_character_num(int level);

// for grub2

// parameter @level is Deprecated.
PW_ERROR_TYPE deepin_pw_check_grub2(const char* user,const char* pw, int level, const char* dict_path);

PASSWORD_LEVEL_TYPE  get_new_passwd_strength_level_grub2(const char *newPasswd);

const char* err_to_string_grub2(PW_ERROR_TYPE err);

/*
    if error, return -1
*/
extern int get_pw_min_length_grub2(int level);

/*
    if error, return -1
*/
extern int get_pw_max_length_grub2(int level);

/*
    if error, return -1
*/
extern int get_pw_min_character_type_grub2(int level);

/*
    get the validate character policy type. is specified by /etc/deepin/grub2_edit_auth.conf "Password:VALIDATE_POLICY"
*/
extern char* get_pw_validate_policy_grub2(int level);

/*
    if error, return -1
*/
extern int get_pw_palimdrome_num_grub2(int level);

/*
    if error, return -1
*/
extern int get_pw_monotone_character_num_grub2(int level);

/*
    if error, return -1
*/
extern int get_pw_consecutive_same_character_num_grub2(int level);

#ifdef __cplusplus
}
#endif

#endif
