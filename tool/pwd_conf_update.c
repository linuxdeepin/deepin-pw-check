#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <iniparser/iniparser.h>
#include <iniparser/dictionary.h>
#include <errno.h>

#define PASSWD_CONF_FILE_PATH           "/etc/deepin/dde.conf"
#define OS_VERSION_FILE_PATH            "/etc/deepin-version"

#define PROFESSIONAL_OS_TYPE_STRING     "Professional"
#define SERVER_OS_TYPE_STRING           "Server"

static bool debug_flag = 0;

#define DEBUG(format, ...) do {     \
                    if (debug_flag) {     \
                        printf("[FILE: %s] [FUNC: %s] [LINE: %d] : "format"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);       \
                    }       \
                }while(0)

typedef enum {
    OS_UNEXPECTED_ERR = -1,
    OS_PROFESSIONAL = 0,
    OS_SERVER,
    OS_UNKNOWN_TYPE,
}OS_TYPE;

OS_TYPE get_current_os_type() {
    dictionary *dic;
    OS_TYPE ret;
	if(NULL == (dic = iniparser_load(OS_VERSION_FILE_PATH))){
		DEBUG("ERROR: open file failed!");
        return OS_UNEXPECTED_ERR;
	}
 
    const char * os_type = iniparser_getstring(dic,"Release:Type","Professional");
    
    DEBUG("os is %s", os_type);

    if (strcmp(os_type, PROFESSIONAL_OS_TYPE_STRING) == 0) {
        ret = OS_PROFESSIONAL;
    } else if (strcmp(os_type, SERVER_OS_TYPE_STRING) == 0) {
        ret = OS_SERVER;
    } else {
        ret = OS_UNKNOWN_TYPE;
    }

    iniparser_freedict(dic);
    return ret;
}

void help() {
    printf("Usage: pwd_conf_update [-dh]\n\n\
    \t-d         open debug info\n\
    \t-h         show this help\n\n");
}

int parse_args(int argc, char** argv) {
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

int update_conf(OS_TYPE os_type) {
    dictionary *dic;
 
	if(NULL == (dic = iniparser_load(PASSWD_CONF_FILE_PATH))){
		DEBUG("ERROR: open file failed!");
        return -1;
	}

    char append_string[512] = {0};

    if (iniparser_find_entry(dic,"Password:PALINDROME_NUM") == 0) {
        sprintf(append_string,"%sPALINDROME_NUM = %d\n",append_string, os_type == OS_PROFESSIONAL? 0:4);
        DEBUG("set PALINDROME_NUM");
    }

    if (iniparser_find_entry(dic,"Password:WORD_CHECK") == 0) {
        sprintf(append_string,"%sWORD_CHECK = %d\n",append_string, os_type == OS_PROFESSIONAL? 0:1);
        DEBUG("set WORD_CHECK");
    }

    if (iniparser_find_entry(dic,"Password:MONOTONE_CHARACTER_NUM") == 0) {
        sprintf(append_string,"%sMONOTONE_CHARACTER_NUM = %d\n",append_string, os_type == OS_PROFESSIONAL? 0:3);
        DEBUG("set MONOTONE_CHARACTER_NUM");
    }

    if (iniparser_find_entry(dic,"Password:CONSECUTIVE_SAME_CHARACTER_NUM") == 0) {
        sprintf(append_string,"%sCONSECUTIVE_SAME_CHARACTER_NUM = %d\n",append_string, os_type == OS_PROFESSIONAL? 0:3);
        DEBUG("set CONSECUTIVE_SAME_CHARACTER_NUM");
    }

    if (iniparser_find_entry(dic,"Password:DICT_PATH") == 0) {
        sprintf(append_string,"%sDICT_PATH = %s\n",append_string, "");
        DEBUG("set WORD_CHECK");
    }

    if (iniparser_find_entry(dic,"Password:FIRST_LETTER_UPPERCASE") == 0) {
        sprintf(append_string,"%sFIRST_LETTER_UPPERCASE = %s\n",append_string, "false");
        DEBUG("set FIRST_LETTER_UPPERCASE");
    }

    DEBUG("append string is %s", append_string);

    if (strlen(append_string) == 0) {
        return 0;
    }

    iniparser_freedict(dic);

    FILE* fd = fopen(PASSWD_CONF_FILE_PATH, "a");
    if (fd == NULL) {
        printf("open %s err: %s\n", PASSWD_CONF_FILE_PATH, strerror(errno));
        return -1;
    }

    if ( 0 == fwrite(append_string, 1, strlen(append_string), fd)) {
        fclose(fd);
        printf("write %s err\n", PASSWD_CONF_FILE_PATH);
        return -1;
    }
    fclose(fd);
    
    return 0;
}

int main(int argc, char** argv) {

    parse_args(argc, argv);

    OS_TYPE os_type = get_current_os_type();

    if (os_type == OS_UNEXPECTED_ERR || os_type == OS_UNKNOWN_TYPE) {
        printf("can not update configure, err: %d\n", os_type);
        exit(EXIT_FAILURE);
    }

    int ret = update_conf(os_type);
    if (ret == -1) {
        printf("update failed, err: %d\n", ret);
    }

    return ret;
}