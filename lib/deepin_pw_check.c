
#include "deepin_pw_check.h"
#include "debug.h"
#include <iniparser/dictionary.h>
#include <iniparser/iniparser.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

#define CHARACTER_TYPE_OF_STANDARD_CHECK (1)
#define CHARACTER_TYPE_OF_STRICT_CHECK (4)

#define MIN_LEN_OF_STANDARD_CHECK (1)
#define MAX_LEN_OF_STANDARD_CHECK (512)

#define MIN_LEN_OF_STRICT_CHECK (8)
#define MAX_LEN_OF_STRICT_CHECK (512)

#define PASSWD_CONF_FILE "/etc/deepin/dde.conf"

#define BUFF_SIZE (512)
static char out_buff[BUFF_SIZE];

struct Options {
    bool enabled;
    int max_len;
    int min_len;
    char character_type[512];
    int character_num_required;
    int palindrome_min_num;
    char dict_path[256];
    int check_word;
    bool first_letter_uppercase;
    int monotone_character_num;         // 单调字符个数
    int consecutive_same_character_num; // 连续相似字符个数
};

void get_validate_policy(char *data) {
    FILE *f = fopen(PASSWD_CONF_FILE, "r");
    char buff[512];

    if (f == NULL) {
        return;
    }

    while (!feof(f)) {
        fgets(buff, 512, f);
        if (!strncmp(buff, "VALIDATE_POLICY", strlen("VALIDATA_POLICY"))) {
            char *p = strchr(buff, '=');
            char *end = strchr(buff, '\n');

            if (p != NULL) {
                int space_cnt = 1;
                int find_quot = 0;
                while (p[space_cnt] == ' ') {
                    space_cnt++;
                }
                while (p[space_cnt] == '\"') {
                    space_cnt++;
                    find_quot = 1;
                }
                p = &p[space_cnt];
                if (end != NULL) {
                    int sub = 0;
                    if (find_quot && (p[end - p - 1] == '\"')) {
                        sub = 1;
                    }
                    strncpy(data, p, end - p - sub);
                    data[end - p - sub] = '\0';
                } else {
                    int sub = 0;
                    if (find_quot && (p[strlen(p) - 1] == '\"')) {
                        sub = 1;
                    }
                    strncpy(data, p, strlen(p) - sub);
                    data[strlen(p) - sub] = '\0';
                }
            }
            break;
        }
    }
    fclose(f);
}

static int load_pwd_conf(struct Options *options) {
    dictionary *dic;
    const char *dict_buff;
    char read_buff[512];
    int retry_cnt = 0;
retry:
    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        if (retry_cnt < 1) {
            DEBUG("exec pwd-conf-update to create <%s>", PASSWD_CONF_FILE);
            system("pwd-conf-update");
            retry_cnt++;
            goto retry;
        }
        return -1;
    }

    options->enabled = iniparser_getboolean(dic, "Password:STRONG_PASSWORD", 1);
    options->min_len = iniparser_getint(dic, "Password:PASSWORD_MIN_LENGTH", 1);
    options->max_len = iniparser_getint(dic, "Password:PASSWORD_MAX_LENGTH", 512);
    if (iniparser_find_entry(dic, "Password:VALIDATE_POLICY") == 0) {
        strcpy(read_buff,
               "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;~`"
               "!@#$%^&*()-_+=|\\{}[]:\"'<>,.?/");
    } else {
        DEBUG("get_validate_policy");
        get_validate_policy(read_buff);
    }
    // buff = (char*)iniparser_getstring(dic, "Password:VALIDATE_POLICY",
    // "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;~!@#$\%^&*()[]{}\\|/?,.<>");
    strcpy(options->character_type, read_buff);
    options->character_num_required = iniparser_getint(dic, "Password:VALIDATE_REQUIRED", 1);
    options->palindrome_min_num = iniparser_getint(dic, "Password:PALINDROME_NUM", 0);
    options->check_word = iniparser_getint(dic, "Password:WORD_CHECK", 0);
    dict_buff = iniparser_getstring(dic, "Password:DICT_PATH", "");
    strcpy(options->dict_path, dict_buff);
    options->monotone_character_num = iniparser_getint(dic, "Password:MONOTONE_CHARACTER_NUM", 0);
    options->consecutive_same_character_num =
            iniparser_getint(dic, "Password:CONSECUTIVE_SAME_CHARACTER_NUM", 0);
    options->first_letter_uppercase =
            iniparser_getboolean(dic, "Password:FIRST_LETTER_UPPERCASE", 0);

    iniparser_freedict(dic);
    return 0;
}

struct Options *get_default_options(int level, const char *dict_path) {

    struct Options *options = (struct Options *)malloc(sizeof(struct Options));

    if (load_pwd_conf(options) == -1) {
        free(options);
        return NULL;
    }

    if (dict_path != NULL && (strcmp(dict_path, "") != 0)) {
        if (strcmp(options->dict_path, "") == 0) {
            options->dict_path[0] = '\0';
        } else {
            strcpy(options->dict_path, dict_path);
        }
    }

    return options;
}

bool is_empty(const char *pw) {
    if (strlen(pw) == 0) {
        return true;
    }
    return false;
}

bool is_palindrome(const char *pw, int palindrome_min_num) {
    int length = strlen(pw);

    for (int i = 0; i < length - 1; i++) {
        if (pw[i] != pw[length - i - 1]) {
            return false;
        }
    }
    if (length >= palindrome_min_num * 2) {
        return true;
    }
    return false;
}

PW_ERROR_TYPE is_length_valid(const char *pw, int min_len, int max_len) {
    int length = strlen(pw);

    if (length < min_len) {
        return PW_ERR_LENGTH_SHORT;
    }

    if (length > max_len) {
        return PW_ERR_LENGTH_LONG;
    }

    return PW_NO_ERR;
}

bool is_type_valid(const char *pw, char *character_type, int character_num_required) {

    DEBUG("called,pw is %s,character policy is %s,required is %d",
          pw,
          character_type,
          character_num_required);

    int pass = 0;
    char *p = NULL;

    char *character_type_tmp = (char *)malloc(strlen(character_type) + 1);
    strcpy(character_type_tmp, character_type);

    p = strtok(character_type_tmp, ";");
    int length = strlen(pw);
    char all_character[512] = "\0";

    while (p != NULL) {
        char *next_data_addr = NULL;

        // 如果当前字符是特殊字符，并且下个 ; 之后的内容也是特殊字符, 则他们作为一组判断
        if ((*p >= 33 && *p <= 64) || (*p >= 91 && *p <= 96) || (*p >= 123 && *p <= 126)) {

            next_data_addr = p + strlen(p) + 1;
            if ((*next_data_addr >= 33 && *next_data_addr <= 64) ||
                (*next_data_addr >= 91 && *next_data_addr <= 96) ||
                (*next_data_addr >= 123 && *next_data_addr <= 126)) {
                char p_tmp[BUFF_SIZE];
                memset(p_tmp, 0, BUFF_SIZE);
                memcpy(p_tmp, p, strlen(p));
                int offset = strlen(p);
                p_tmp[strlen(p)] = ';';
                offset += 1;
                p = strtok(NULL, ";");
                memcpy(p_tmp + offset, p, strlen(p));
                p = p_tmp;
            }
        }

        sprintf(all_character, "%s%s", all_character, p);
        DEBUG("p is %s, all_character is %s", p, all_character);
        int ok = 0;
        for (int i = 0; i < length; i++) {
            if (ok == 1) {
                break;
            }
            for (int j = 0; j < strlen(p); j++) {
                if (pw[i] == p[j]) {
                    pass++;
                    ok = 1;
                    DEBUG("pw %s includes character of %s", pw, p);
                    break;
                }
            }
        }
        p = strtok(NULL, ";");
    }

    free(character_type_tmp);

    DEBUG("all required character type is: %s", all_character);

    int expect_str_len = strlen(all_character);
    // 判断pw的每一个字符是否都在expect_str里，如果有一个不是，则有未指定的字符
    for (int i = 0; i < length; i++) {
        int find = 0;
        for (int j = 0; j < expect_str_len; j++) {
            if (pw[i] == all_character[j]) {
                find = 1;
            }
        }
        if (find == 0) {
            return false;
        }
    }

    if (pass < character_num_required) {
        return false;
    }

    return true;
}

bool is_word(const char *pw, const char *dict_path) {
    extern int word_check(const char *pw, const char *dict_path);

    return word_check(pw, dict_path);
}

void get_adjacent_character(char c, char *next, char *last) {
    const char keyboard_character[][20] = {"!@#$\%^&*()_+",
                                           "QWERTYUIOP{}",
                                           "ASDFGHJKL:\"|",
                                           "ZXCVBNM<>?",
                                           "qwertyuiop[]",
                                           "asdfghjkl;'\\",
                                           "zxcvbnm,./"};
    int group_num = sizeof(keyboard_character) / sizeof(keyboard_character[0]);

    for (int i = 0; i < group_num; i++) {
        int length = strlen(keyboard_character[i]);
        for (int j = 0; j < length; j++) {
            if (keyboard_character[i][j] == c) {
                if (j > 0) {
                    *last = keyboard_character[i][j - 1];
                } else {
                    *last = 0;
                }

                if (j < length - 1) {
                    *next = keyboard_character[i][j + 1];
                } else {
                    *next = 0;
                }
                goto END;
            }
        }
    }
END:
    return;
}

bool is_monotone_character(const char *pw, int monotone_num) {
    DEBUG("character is %s, montone_num is %d", pw, monotone_num);
    int length = strlen(pw);
    int monotone_increase_num = 1;
    int monotone_decrease_num = 1;
    int keyboard_monotone_increase_num = 1;
    int keyboard_monotone_decrease_num = 1;
    char next = 0, last = 0;
    for (int i = 0; i < length - 1; i++) {
        if (pw[i] == pw[i + 1] + 1) {
            monotone_increase_num++;
        } else if (pw[i] == pw[i + 1] - 1) {
            monotone_decrease_num++;
        } else {
            monotone_decrease_num = 1;
            monotone_increase_num = 1;
        }

        get_adjacent_character(pw[i], &next, &last);
        DEBUG("character %c , next is %c, last is %c", pw[i], next, last);
        if (next != 0 && (pw[i + 1] == next)) {
            keyboard_monotone_increase_num++;
        } else if (next != 0 && (pw[i + 1] == last)) {
            keyboard_monotone_decrease_num++;
        } else {
            keyboard_monotone_increase_num = 1;
            keyboard_monotone_decrease_num = 1;
        }

        if (monotone_increase_num >= monotone_num || monotone_decrease_num >= monotone_num) {
            return true;
        }
        if (keyboard_monotone_increase_num >= monotone_num ||
            keyboard_monotone_decrease_num >= monotone_num) {
            return true;
        }
    }
    return false;
}

bool is_consecutive_same_character(const char *pw, int consecutive_num) {
    int length = strlen(pw);
    int consecutive_same_num = 1;
    DEBUG("pw is %s, consecutive_num is %d", pw, consecutive_num);
    for (int i = 0; i < length - 1; i++) {
        if (pw[i] == pw[i + 1]) {
            consecutive_same_num++;
            DEBUG("%c is consecutive %d times", pw[i + 1], consecutive_same_num);
        } else {
            consecutive_same_num = 1;
        }

        if (consecutive_same_num >= consecutive_num) {
            return true;
        }
    }
    return false;
}

bool is_first_letter_uppercase(const char *pw) {
    int length = strlen(pw);
    if (length > 0) {
        if (pw[0] >= 'A' && pw[0] <= 'Z') {
            return true;
        }
        return false;
    }
    return false;
}

PW_ERROR_TYPE deepin_pw_check(const char *user, const char *pw, int level, const char *dict_path) {
    struct Options *options = get_default_options(level, dict_path);
    if (options == NULL) {
        return PW_ERR_PARA;
    }

    DEBUG("read config is\n"
          "\tenabled:%d\n"
          "\tmin_len:%d\n"
          "\tmax_len:%d\n"
          "\tcharacter_type:%s\n"
          "\tcharacter_num_required:%d\n"
          "\tcheck_word:%d\n"
          "\tmonotone_character_num:%d\n"
          "\tconsecutive_same_character_num:%d\n"
          "\tfirst_letter_uppercase:%d\n"
          "\tdict_path=%s",
          options->enabled,
          options->min_len,
          options->max_len,
          options->character_type,
          options->character_num_required,
          options->check_word,
          options->monotone_character_num,
          options->consecutive_same_character_num,
          options->first_letter_uppercase,
          options->dict_path);

    PW_ERROR_TYPE ret = PW_NO_ERR;

    do {
        DEBUG("check is_empty");
        if (is_empty(pw)) {
            ret = PW_ERR_PASSWORD_EMPTY;
            break;
        }

        if (!options->enabled) {
            return ret;
        }
        DEBUG("check is_length_valid");
        if (PW_NO_ERR != (ret = is_length_valid(pw, options->min_len, options->max_len))) {
            break;
        }
        DEBUG("check is_first_letter_uppercase");
        if (options->first_letter_uppercase) {
            if (!is_first_letter_uppercase(pw)) {
                ret = PW_ERR_PW_FIRST_UPPERM;
                break;
            }
        }

        DEBUG("check is_type_valid");
        if (!is_type_valid(pw, options->character_type, options->character_num_required)) {
            ret = PW_ERR_CHARACTER_INVALID;
            break;
        }

        DEBUG("check is_palindrome");
        if (options->palindrome_min_num && options->palindrome_min_num > 0) {
            if (is_palindrome(pw, options->palindrome_min_num)) {
                ret = PW_ERR_PALINDROME;
                break;
            }
        }

        DEBUG("check is_word");
        if (options->check_word) {
            if (is_word(pw, options->dict_path)) {
                ret = PW_ERR_WORD;
                break;
            }
        }

        DEBUG("check is_monotone_character");
        if (options->monotone_character_num && options->monotone_character_num > 0) {
            if (is_monotone_character(pw, options->monotone_character_num)) {
                ret = PW_ERR_PW_MONOTONE;
                break;
            }
        }
        DEBUG("check is_consecutive_same_character");
        if (options->consecutive_same_character_num &&
            options->consecutive_same_character_num > 0) {
            if (is_consecutive_same_character(pw, options->consecutive_same_character_num)) {
                ret = PW_ERR_PW_CONSECUTIVE_SAME;
                break;
            }
        }

    } while (0);

    free(options);

    return ret;
}

int get_pw_min_length(int level) {
    UNUSED(level);

    dictionary *dic;

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        return -1;
    }

    int min_len = iniparser_getint(dic, "Password:PASSWORD_MIN_LENGTH", 1);

    iniparser_freedict(dic);

    return min_len;
}

int get_pw_max_length(int level) {
    UNUSED(level);

    dictionary *dic;

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        return -1;
    }

    int max_len = iniparser_getint(dic, "Password:PASSWORD_MAX_LENGTH", 1);

    iniparser_freedict(dic);

    return max_len;
}

int get_pw_min_character_type(int level) {
    UNUSED(level);

    dictionary *dic;

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        return -1;
    }

    int validate_required = iniparser_getint(dic, "Password:VALIDATE_REQUIRED", 1);

    iniparser_freedict(dic);

    return validate_required;
}

char *get_pw_validate_policy(int level) {
    UNUSED(level);

    dictionary *dic;

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        return "";
    }

    const char *read_type = iniparser_getstring(dic, "Password:VALIDATE_POLICY", "");

    strcpy(out_buff, read_type);
    iniparser_freedict(dic);
    return out_buff;
}

int get_pw_palimdrome_num(int level) {
    UNUSED(level);

    dictionary *dic;

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE))) {
        DEBUG("ERROR: open file %s failed!", PASSWD_CONF_FILE);
        return -1;
    }

    int num = iniparser_getint(dic, "Password:PALINDROME_NUM", 0);

    iniparser_freedict(dic);

    return num;
}

const char *err_to_string(PW_ERROR_TYPE err) {
    if (err >= PW_ERR_MAX) {
        return gettext("Invalid error type");
    }

    setlocale(LC_ALL, "");
    textdomain("deepin-pw-check");
    char tmp_buff[BUFF_SIZE];
    int len = 0;
    int num = 0;
    switch (err) {
    case PW_NO_ERR:
        return gettext("Checking successful");
    case PW_ERR_PASSWORD_EMPTY:
        return gettext("The password cannot be empty");
    case PW_ERR_LENGTH_SHORT:
        len = get_pw_min_length(0);
        snprintf(out_buff, BUFF_SIZE, gettext("Password must have at least %d characters"), len);
        return out_buff;
    case PW_ERR_LENGTH_LONG:
        len = get_pw_max_length(0);
        snprintf(out_buff, BUFF_SIZE, gettext("Password must be no more than %d characters"), len);
        return out_buff;
    case PW_ERR_CHARACTER_INVALID:
        get_validate_policy(tmp_buff);
        snprintf(out_buff, BUFF_SIZE, gettext("Password can only contain %s"), tmp_buff);
        return out_buff;
    case PW_ERR_PALINDROME:
        num = get_pw_palimdrome_num(0);
        snprintf(out_buff,
                 BUFF_SIZE,
                 gettext("Password must not contain more than %d palindrome characters"),
                 num);
        return out_buff;
    case PW_ERR_PW_CONSECUTIVE_SAME:
        return gettext("Create a strong password please");
    case PW_ERR_PW_MONOTONE:
        return gettext("Create a strong password please");
    case PW_ERR_PW_FIRST_UPPERM:
        return gettext("Create a strong password please");
    case PW_ERR_WORD:
        return gettext("Do not use common words and combinations as password");
    case PW_ERR_PARA:
        return gettext("Parameter options are invalid");
    case PW_ERR_INTERNAL:
        return gettext("Internal error");
    case PW_ERR_USER:
        return gettext("Invalid user");
    default:
        return gettext("It does not meet password rules");
    }

    return "";
}
