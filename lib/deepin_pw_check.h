
#ifndef _DEEPIN_PW_CHECK_H_
#define _DEEPIN_PW_CHECK_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PW_NO_ERR   = 0,
    PW_ERR_PASSWORD_EMPTY,
    PW_ERR_LENGTH_INVALID,
    PW_ERR_CHARACTER_INVALID,
    PW_ERR_PALINDROME,
    PW_ERR_WORD,
    PW_ERR_PW_REPEAT,
    PW_ERR_PARA,
    PW_ERR_INTERNAL,
    PW_ERR_USER,
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
    whether character is valid, compare the new and old password.

    NOTE: 
        can not combined with LEVEL_STANDARD_CHECK
*/ 
#define LEVEL_STRICT_CHECK    (2)
/*
    create user flag
    will not compare the new password and old password

    NOTE:
        LEVEL_CREATE_USER can combined with LEVEL_STANDARD_CHECK or LEVEL_STRICT_CHECK
*/
#define LEVEL_CREATE_USER     (4)


PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);

const char* err_to_string(PW_ERROR_TYPE err);

void set_debug_flag(int flag);

#ifdef __cplusplus
}
#endif

#endif