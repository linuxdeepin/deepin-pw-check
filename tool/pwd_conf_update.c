// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <iniparser/iniparser.h>
#include <iniparser/dictionary.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>

#define PASSWD_CONF_FILE_PATH "/etc/deepin/dde.conf"
#define OS_VERSION_FILE_PATH "/etc/os-version"

#define PROFESSIONAL_OS_TYPE "Professional"
#define SERVER_OS_TYPE_STRING "Server"
#define EULER_OS_TYPE_STRING "Euler"

#define E_OS_TYPE_STRING "E"
#define HOME_OS_TYPE_STRING "Home"

static bool debug_flag = 0;

#define DEBUG(format, ...)                                                                         \
    do {                                                                                           \
        if (debug_flag) {                                                                          \
            printf("[FILE: %s] [FUNC: %s] [LINE: %d] : " format "\n",                              \
                   __FILE__,                                                                       \
                   __FUNCTION__,                                                                   \
                   __LINE__,                                                                       \
                   ##__VA_ARGS__);                                                                 \
        }                                                                                          \
    } while (0)

typedef enum {
    OS_UNEXPECTED_ERR = -1,
    OS_PROFESSIONAL = 0,
    OS_SERVER,
    OS_EULER,
    OS_E,
    OS_HOME,
    OS_UNKNOWN_TYPE,
} OS_TYPE;

const char *os_type_string[] = {
        [OS_PROFESSIONAL] = PROFESSIONAL_OS_TYPE,
        [OS_SERVER] = SERVER_OS_TYPE_STRING,
        [OS_EULER] = EULER_OS_TYPE_STRING,
        [OS_E] = E_OS_TYPE_STRING,
        [OS_HOME] = HOME_OS_TYPE_STRING,
        [OS_UNKNOWN_TYPE] = "UNKNOWN",
};

typedef struct {
    int min_length;
    int max_length;
    char *validate_policy;
    int validate_required;
    int palindorme_num;
    int word_check;
    int monotone_same_character_num;
    int consecutive_same_character_num;
    int first_letter_uppercase;
} _default_conf;

const _default_conf default_conf[] = {
        [OS_PROFESSIONAL] =
                {
                        .min_length = 1,
                        .max_length = 510,
                        .validate_policy = "1234567890;abcdefghijklmnopqrstuvwxyz;"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                           ":;<=>?@[\\]^_`{|}~/",
                        .validate_required = 1,
                        .palindorme_num = 0,
                        .word_check = 0,
                        .monotone_same_character_num = 0,
                        .consecutive_same_character_num = 0,
                        .first_letter_uppercase = 0,
                },
        [OS_SERVER] =
                {
                        .min_length = 8,
                        .max_length = 510,
                        .validate_policy = "1234567890;abcdefghijklmnopqrstuvwxyz;"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                           ":;<=>?@[\\]^_`{|}~/",
                        .validate_required = 3,
                        .palindorme_num = 4,
                        .word_check = 1,
                        .monotone_same_character_num = 3,
                        .consecutive_same_character_num = 3,
                        .first_letter_uppercase = 0,
                },
        [OS_EULER] =
                {
                        .min_length = 8,
                        .max_length = 510,
                        .validate_policy = "1234567890;abcdefghijklmnopqrstuvwxyz;"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                           ":;<=>?@[\\]^_`{|}~/",
                        .validate_required = 3,
                        .palindorme_num = 4,
                        .word_check = 1,
                        .monotone_same_character_num = 3,
                        .consecutive_same_character_num = 3,
                        .first_letter_uppercase = 0,
                },
        [OS_E] =
                {
                        .min_length = 1,
                        .max_length = 510,
                        .validate_policy = "1234567890;abcdefghijklmnopqrstuvwxyz;"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                           ":;<=>?@[\\]^_`{|}~/",
                        .validate_required = 1,
                        .palindorme_num = 0,
                        .word_check = 0,
                        .monotone_same_character_num = 0,
                        .consecutive_same_character_num = 0,
                        .first_letter_uppercase = 0,
                },
        [OS_HOME] =
                {
                        .min_length = 1,
                        .max_length = 510,
                        .validate_policy = "1234567890;abcdefghijklmnopqrstuvwxyz;"
                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                           ":;<=>?@[\\]^_`{|}~/",
                        .validate_required = 1,
                        .palindorme_num = 0,
                        .word_check = 0,
                        .monotone_same_character_num = 0,
                        .consecutive_same_character_num = 0,
                        .first_letter_uppercase = 0,
                },
};

OS_TYPE get_current_os_type() {
    dictionary *dic;
    OS_TYPE ret;
    FILE *f = fopen(OS_VERSION_FILE_PATH, "r");

    if (f == NULL) {
        /* 其他发行版的可能不存在 os-version 文件，这里默认走专业版配置 */
        return OS_PROFESSIONAL;
    }

    fclose(f);

    if (NULL == (dic = iniparser_load(OS_VERSION_FILE_PATH))) {
        DEBUG("ERROR: open file failed!");
        return OS_UNEXPECTED_ERR;
    }

    const char *os_type = iniparser_getstring(dic, "Version:OsBuild", "11018.100");
    if(os_type == NULL){
        printf("os_type is null");
        return OS_UNKNOWN_TYPE;
    }

    printf("os_type : %s\n",os_type);

    if(strlen(os_type) < 4){
        printf("os_type length err: %lu\n",strlen(os_type));
        return OS_UNKNOWN_TYPE;
    }

    if (os_type[1] == '1') {
        ret = OS_PROFESSIONAL;
    } else if (os_type[1] == '2') {
        ret = OS_SERVER;
        if (os_type[3] == '3') {
            ret = OS_EULER;
        }
    } else if (os_type[1] == '3') {
        ret = OS_SERVER;
    } else {
        ret = OS_UNKNOWN_TYPE;
    }

    DEBUG("os is %s", os_type_string[ret]);

    iniparser_freedict(dic);
    return ret;
}

void help() {
    printf("Usage: pwd_conf_update [-dh]\n\n\
    \t-d         open debug info\n\
    \t-h         show this help\n\n");
}

int parse_args(int argc, char **argv) {
    int opt;
    int ignore_help = 0;
    int show_help = 0;
    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
        case 'd':
            debug_flag = 1;
            ignore_help = 1;
            break;
        case 'h':
            show_help = 1;
            break;
        default:
            help();
            exit(EXIT_FAILURE);
        }
    }
    if (show_help && ignore_help == 0) {
        help();
        exit(0);
    }
    return 0;
}

void get_validate_policy(char *data) {
    FILE *f = fopen(PASSWD_CONF_FILE_PATH, "r");
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

int update_conf(OS_TYPE os_type) {
    dictionary *dic;

    int err = access(PASSWD_CONF_FILE_PATH, F_OK);
    if (err) {
        DEBUG("check %s err: %d, create it", PASSWD_CONF_FILE_PATH, err);
        FILE *fd = fopen(PASSWD_CONF_FILE_PATH, "a");
        if (fd == NULL) {
            printf("open %s err: %s\n", PASSWD_CONF_FILE_PATH, strerror(errno));
            return -1;
        }
        int num = fwrite("[Password]\n", 1, strlen("[Password]\n"), fd);
        if (num <= 0) {
            printf("write data to %s err: %s\n", PASSWD_CONF_FILE_PATH, strerror(errno));
            return -1;
        }
        fclose(fd);
    }

    if (NULL == (dic = iniparser_load(PASSWD_CONF_FILE_PATH))) {
        DEBUG("ERROR: open file failed!");
        return -1;
    }

    char append_string[2048] = {0};
    int offset = 0;
    sprintf(append_string + offset, "[Password]\n");
    offset = strlen(append_string);

    // 如果找不到该字段，则写默认的
    if (iniparser_find_entry(dic, "Password:STRONG_PASSWORD") == 0) {
        sprintf(append_string + offset, "STRONG_PASSWORD = %s\n", "true");
        DEBUG("set STRONG_PASSWORD");
    } else {
        // 如果找到了该字段
        // 如果是服务器版，强制覆盖配置
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset, "STRONG_PASSWORD = %s\n", "true");
        //     DEBUG("restore STRONG_PASSWORD");
        // } else {
        // 如果不是服务器版，则维持原配置
        sprintf(append_string + offset,
                "STRONG_PASSWORD = %s\n",
                iniparser_getboolean(dic, "Password:STRONG_PASSWORD", false) ? "true" : "false");
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:PASSWORD_MIN_LENGTH") == 0) {
        sprintf(append_string + offset,
                "PASSWORD_MIN_LENGTH = %d\n",
                default_conf[os_type].min_length);
        DEBUG("set PASSWORD_MIN_LENGTH");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "PASSWORD_MIN_LENGTH = %d\n",
        //             default_conf[os_type].min_length);
        //     DEBUG("restore PASSWORD_MIN_LENGTH");
        // } else {
        sprintf(append_string + offset,
                "PASSWORD_MIN_LENGTH = %d\n",
                iniparser_getint(dic,
                                 "Password:PASSWORD_MIN_LENGTH",
                                 default_conf[os_type].min_length));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:PASSWORD_MAX_LENGTH") == 0) {
        sprintf(append_string + offset,
                "PASSWORD_MAX_LENGTH = %d\n",
                default_conf[os_type].max_length);
        DEBUG("set PASSWORD_MAX_LENGTH");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "PASSWORD_MAX_LENGTH = %d\n",
        //             default_conf[os_type].max_length);
        //     DEBUG("restore PASSWORD_MAX_LENGTH");
        // } else {
        sprintf(append_string + offset,
                "PASSWORD_MAX_LENGTH = %d\n",
                iniparser_getint(dic,
                                 "Password:PASSWORD_MAX_LENGTH",
                                 default_conf[os_type].max_length));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:VALIDATE_POLICY") == 0) {
        sprintf(append_string + offset,
                "VALIDATE_POLICY = %s\n",
                default_conf[os_type].validate_policy);
        DEBUG("set VALIDATE_POLICY");
    } else {
        // char cmd[512];
        // sprintf(cmd, "sed \"/^VALIDATE_POLICY.*/\"d -i %s", PASSWD_CONF_FILE_PATH);
        // system(cmd);
        // sprintf(append_string + offset,
        //         "VALIDATE_POLICY = \"%s\"\n",
        //         default_conf[os_type].validate_policy);
        // DEBUG("set VALIDATE_POLICY after delete");

        char buff[512];
        get_validate_policy(buff);
        sprintf(append_string + offset, "VALIDATE_POLICY = %s\n", buff);
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:VALIDATE_REQUIRED") == 0) {
        sprintf(append_string + offset,
                "VALIDATE_REQUIRED = %d\n",
                default_conf[os_type].validate_required);
        DEBUG("set VALIDATE_REQUIRED");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "VALIDATE_REQUIRED = %d\n",
        //             default_conf[os_type].validate_required);
        //     DEBUG("restore VALIDATE_REQUIRED");
        // } else {
        sprintf(append_string + offset,
                "VALIDATE_REQUIRED = %d\n",
                iniparser_getint(dic,
                                 "Password:VALIDATE_REQUIRED",
                                 default_conf[os_type].validate_required));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:PALINDROME_NUM") == 0) {
        sprintf(append_string + offset,
                "PALINDROME_NUM = %d\n",
                default_conf[os_type].palindorme_num);
        DEBUG("set PALINDROME_NUM");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "PALINDROME_NUM = %d\n",
        //             default_conf[os_type].palindorme_num);
        //     DEBUG("restore PALINDROME_NUM");
        // } else {
        sprintf(append_string + offset,
                "PALINDROME_NUM = %d\n",
                iniparser_getint(dic,
                                 "Password:PALINDROME_NUM",
                                 default_conf[os_type].palindorme_num));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:WORD_CHECK") == 0) {
        sprintf(append_string + offset, "WORD_CHECK = %d\n", default_conf[os_type].word_check);
        DEBUG("set WORD_CHECK");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset, "WORD_CHECK = %d\n",
        //     default_conf[os_type].word_check); DEBUG("restore WORD_CHECK");
        // } else {
        sprintf(append_string + offset,
                "WORD_CHECK = %d\n",
                iniparser_getint(dic, "Password:WORD_CHECK", default_conf[os_type].word_check));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:MONOTONE_CHARACTER_NUM") == 0) {
        sprintf(append_string + offset,
                "MONOTONE_CHARACTER_NUM = %d\n",
                default_conf[os_type].monotone_same_character_num);
        DEBUG("set MONOTONE_CHARACTER_NUM");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "MONOTONE_CHARACTER_NUM = %d\n",
        //             default_conf[os_type].monotone_same_character_num);
        //     DEBUG("restore MONOTONE_CHARACTER_NUM");
        // } else {
        sprintf(append_string + offset,
                "MONOTONE_CHARACTER_NUM = %d\n",
                iniparser_getint(dic,
                                 "Password:MONOTONE_CHARACTER_NUM",
                                 default_conf[os_type].monotone_same_character_num));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:CONSECUTIVE_SAME_CHARACTER_NUM") == 0) {
        sprintf(append_string + offset,
                "CONSECUTIVE_SAME_CHARACTER_NUM = %d\n",
                default_conf[os_type].consecutive_same_character_num);
        DEBUG("set CONSECUTIVE_SAME_CHARACTER_NUM");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset,
        //             "CONSECUTIVE_SAME_CHARACTER_NUM = %d\n",
        //             default_conf[os_type].consecutive_same_character_num);
        //     DEBUG("restore CONSECUTIVE_SAME_CHARACTER_NUM");
        // } else {
        sprintf(append_string + offset,
                "CONSECUTIVE_SAME_CHARACTER_NUM = %d\n",
                iniparser_getint(dic,
                                 "Password:CONSECUTIVE_SAME_CHARACTER_NUM",
                                 default_conf[os_type].consecutive_same_character_num));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:DICT_PATH") == 0) {
        sprintf(append_string + offset, "DICT_PATH = %s\n", "");
        DEBUG("set DICT_PATH");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset, "DICT_PATH = %s\n", "");
        //     DEBUG("restore DICT_PATH");
        // } else {
        sprintf(append_string + offset,
                "DICT_PATH = %s\n",
                iniparser_getstring(dic, "Password:DICT_PATH", ""));
        // }
    }
    offset = strlen(append_string);

    if (iniparser_find_entry(dic, "Password:FIRST_LETTER_UPPERCASE") == 0) {
        sprintf(append_string + offset, "FIRST_LETTER_UPPERCASE = %s\n", "false");
        DEBUG("set FIRST_LETTER_UPPERCASE");
    } else {
        // if (OS_SERVER == os_type) {
        //     sprintf(append_string + offset, "FIRST_LETTER_UPPERCASE = %s\n", "false");
        //     DEBUG("restore FIRST_LETTER_UPPERCASE");
        // } else {
        sprintf(append_string + offset,
                "FIRST_LETTER_UPPERCASE = %s\n",
                iniparser_getboolean(dic, "Password:FIRST_LETTER_UPPERCASE", false) ? "true"
                                                                                    : "false");
        // }
    }

    DEBUG("append string :%s", append_string);

    if (strlen(append_string) == 0) {
        return 0;
    }

    iniparser_freedict(dic);

    FILE *fd = fopen(PASSWD_CONF_FILE_PATH, "w");
    if (fd == NULL) {
        printf("open %s err: %s\n", PASSWD_CONF_FILE_PATH, strerror(errno));
        return -1;
    }

    if (0 == fwrite(append_string, 1, strlen(append_string), fd)) {
        fclose(fd);
        printf("write %s err\n", PASSWD_CONF_FILE_PATH);
        return -1;
    }
    fclose(fd);

    if (-1 == chmod(PASSWD_CONF_FILE_PATH, 0644)) {
        printf("chmod for %s fail: %s", PASSWD_CONF_FILE_PATH, strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    parse_args(argc, argv);

    OS_TYPE os_type = get_current_os_type();

    if (os_type == OS_UNEXPECTED_ERR || os_type == OS_UNKNOWN_TYPE) {
        printf("can not update configure, err: %d\n", os_type);
        exit(0);
    }

    int ret = update_conf(os_type);
    if (ret == -1) {
        printf("update failed, err: %d\n", ret);
    }

    return 0;
}
