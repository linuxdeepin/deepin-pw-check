
#include <stdio.h>
#include <stdbool.h>
#include "deepin_pw_check.h"
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <libintl.h>
#include "debug.h"

#define CHARACTER_TYPE_OF_STANDARD_CHECK    (1)
#define CHARACTER_TYPE_OF_STRICT_CHECK      (4)

#define MIN_LEN_OF_STANDARD_CHECK           (1)
#define MAX_LEN_OF_STANDARD_CHECK           (512)

#define MIN_LEN_OF_STRICT_CHECK             (8)
#define MAX_LEN_OF_STRICT_CHECK             (512)

enum PW_ASCII_TYPE {
    TYPE_NUM = 1<<0,
    TYPE_CHAR_UPPER = 1<<1,
    TYPE_CHAR_LOWER = 1<<2,
    TYPE_SYMBOL = 1<<3
};

struct Options {
    int max_len;
    int min_len;
    int must_contain;
    bool palindrome;
    int palindrome_min_num;
    const char* dict_path;
    int check_word;
};

bool is_level_valid(int level) {
    if ((level & (LEVEL_STANDARD_CHECK | LEVEL_STRICT_CHECK)) == (LEVEL_STANDARD_CHECK | LEVEL_STRICT_CHECK)) {
        return false;
    }

    unsigned int all_level = LEVEL_STANDARD_CHECK | LEVEL_STRICT_CHECK;

    if (level & ~all_level) {
        return false;
    }
    return true;
}


struct Options* get_default_options(int level,const char* dict_path) {
    if (!is_level_valid(level)) {
        return NULL;
    }

    struct Options* options = (struct Options*)malloc(sizeof(struct Options));

    if (level & LEVEL_STANDARD_CHECK) {
        options->min_len = MIN_LEN_OF_STANDARD_CHECK;
        options->max_len = MAX_LEN_OF_STANDARD_CHECK;
        options->must_contain = 0;
        options->dict_path = NULL;
        options->palindrome = false;
        options->palindrome_min_num = 0;
        options->check_word = 0;
    } else if (level & LEVEL_STRICT_CHECK) {
        options->min_len = MIN_LEN_OF_STRICT_CHECK;
        options->max_len = MAX_LEN_OF_STRICT_CHECK;
        options->must_contain = TYPE_NUM | TYPE_CHAR_LOWER | TYPE_CHAR_UPPER | TYPE_SYMBOL;
        options->palindrome = true;
        options->palindrome_min_num = 4;
        options->dict_path = dict_path;
        options->check_word = 1;
    }

    return options;
}


bool is_empty(const char* pw) {
    if (strlen(pw) == 0){
        return true;
    }
    return false;
}

bool is_palindrome(const char * pw,int num) {
    int length = strlen(pw);

	for (int i = 0; i < length-1; i++) {
		if (pw[i] != pw[length-i-1]) {
			return false;
		}
	}
	if (length >= num*2) {
		return true;
	}
	return false;
}

PW_ERROR_TYPE is_length_valid(const char* pw,int min,int max) {
    int length = strlen(pw);
    
    if (length < min ) {
        return PW_ERR_LENGTH_SHORT;
    }

    if(length > max) {
        return PW_ERR_LENGTH_LONG;
    }

    return PW_NO_ERR;
}

bool is_type_valid(const char* pw,int type) {
    char expect_str[200];
    int expect_str_len=0;
    int expect_pass = 0;
    int pass = 0;
    int length = strlen(pw);
    DEBUG("called,pw is %s,type is %x",pw,type);

    if (type == 0) {
       sprintf(expect_str,"%s","0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#\%^&*()[]{}|,.<>");
       goto CHECK;
    }

    if ((type&TYPE_NUM) == TYPE_NUM) {
        expect_pass++;
        char specified_str[] = "0123456789";
        for(int i =0 ; i < length ; i++){
            if (pw[i] >= '0' && pw[i] <= '9') {
                DEBUG("0-9 is ok");
                pass++;
                break;
            }
        }
        sprintf(expect_str,"%s%s",expect_str,specified_str);
	}
	if ((type&TYPE_CHAR_UPPER) == TYPE_CHAR_UPPER) {
        expect_pass++;
        char specified_str[] = "abcdefghijklmnopqrstuvwxyz";
        for(int i =0 ; i < length ; i++){
            if (pw[i] >= 'a' && pw[i] <= 'z') {
                pass++;
                DEBUG("a-z is ok");
                break;
            }
        }
		sprintf(expect_str,"%s%s",expect_str,specified_str);
	}
	if ((type&TYPE_CHAR_LOWER) == TYPE_CHAR_LOWER) {
        expect_pass++;
        char specified_str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for(int i =0 ; i < length ; i++){
            if (pw[i] >= 'A' && pw[i] <= 'Z') {
                pass++;
                DEBUG("A-Z is ok");
                break;
            }
        }
		sprintf(expect_str,"%s%s",expect_str,specified_str);
	}
	if ((type&TYPE_SYMBOL) == TYPE_SYMBOL) {
        expect_pass++;
        int ok = 0;
        char specified_str[] = "~!@#\%^&*()[]{}|,.<>";
        for(int i =0 ; i < length ; i++){
            if (ok == 1){
                break;
            }
            for (int j=0; j < strlen(specified_str) ; j++){
                if (pw[i] == specified_str[j]){
                    pass++;
                    ok = 1;
                    DEBUG("special character is ok");
                    break;
                }
            }
        }
        sprintf(expect_str,"%s%s",expect_str,specified_str);
	}

    if (expect_pass != pass){
        return false;
    }

CHECK:
    expect_str_len = strlen(expect_str);
    // 判断pw的每一个字符是否都在expect_str里，如果有一个不是，则有未指定的字符
    for(int i =0 ; i < length ; i++){
        int find = 0;
        for(int j =0; j < expect_str_len;j++){
            if (pw[i] == expect_str[j]) {
                find = 1;
            }
        }
        if (find == 0){
            return false;
        }
    }

    return true;
}

bool is_word(const char* pw,const char* dict_path) {
    extern int word_check(const char* pw,const char* dict_path);

    return word_check(pw,dict_path);
}

PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path) {

    struct Options * options = get_default_options(level,dict_path);
    if (options == NULL) {
        return PW_ERR_PARA;
    }

    PW_ERROR_TYPE ret = PW_NO_ERR;
    do {
        if (is_empty(pw)) {
            ret = PW_ERR_PASSWORD_EMPTY;
            break;
        }

        if (PW_NO_ERR != (ret = is_length_valid(pw,options->min_len,options->max_len))) {
            break;
        }

        if (!is_type_valid(pw,options->must_contain)) {
            ret = PW_ERR_CHARACTER_INVALID;
            break;
        }

        if (options->palindrome) {
            if (is_palindrome(pw,options->palindrome_min_num)) {
                ret = PW_ERR_PALINDROME;
                break;
            }
        }

        if (options->check_word) {
            if (is_word(pw,options->dict_path)) {
                ret = PW_ERR_WORD;
                break;
            }
        }

    }while(0);

    free(options);

    return ret;
}


const char* err_to_string(PW_ERROR_TYPE err){
    if (err >= PW_ERR_MAX) {
        return gettext("invalid error type");
    }

    setlocale(LC_ALL, "");
    textdomain("deepin-pw-check");

    switch (err)
    {
    case PW_NO_ERR:
        return gettext("check success");
    case PW_ERR_PASSWORD_EMPTY:
        return gettext("password is empty");
    case PW_ERR_LENGTH_SHORT:
        return gettext("password's length is too short");
    case PW_ERR_LENGTH_LONG:
        return gettext("password's length is too long");
    case PW_ERR_CHARACTER_INVALID:
        return gettext("password's character is invalid");
    case PW_ERR_PALINDROME:
        return gettext("password is palindrome");
    case PW_ERR_WORD:
        return gettext("password is based on word");
    case PW_ERR_PARA:
        return gettext("parameter options is invalid");
    case PW_ERR_INTERNAL:
        return gettext("internal error");
    case PW_ERR_USER:
        return gettext("invalid user");
    default:
        return gettext("invalid password");
    }

    return "";
}

int get_pw_min_length(int level) {
    if (!is_level_valid(level)) {
        return -1;
    }
    if (level & LEVEL_STANDARD_CHECK) {
        return MIN_LEN_OF_STANDARD_CHECK;
    } else if (level & LEVEL_STRICT_CHECK) {
        return MIN_LEN_OF_STRICT_CHECK;
    }
    return -1;
}

int get_pw_max_length(int level) {
    if (!is_level_valid(level)) {
        return -1;
    }
    if (level & LEVEL_STANDARD_CHECK) {
        return MAX_LEN_OF_STANDARD_CHECK;
    } else if (level & LEVEL_STRICT_CHECK) {
        return MAX_LEN_OF_STRICT_CHECK;
    }
    return -1;
}

int get_pw_min_character_type(int level) {
    if (!is_level_valid(level)) {
        return -1;
    }
    if (level & LEVEL_STANDARD_CHECK) {
        return CHARACTER_TYPE_OF_STANDARD_CHECK;
    } else if (level & LEVEL_STRICT_CHECK) {
        return CHARACTER_TYPE_OF_STRICT_CHECK;
    }
    return -1;
}
