
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
    PW_ERR_MAX,
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

// parameter @level is Deprecated.
PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);

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
    get the validate charater policy type. is specified by /etc/deepin/dde.conf. "Password:VALIDATE_POLICY"
*/
extern char* get_pw_validate_policy(int level);

/*
    if error, return -1
*/
extern int get_pw_palimdrome_num(int level);

#ifdef __cplusplus
}
#endif

#endif