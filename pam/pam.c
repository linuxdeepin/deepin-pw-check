#include "../lib/debug.h"
#include "../lib/deepin_pw_check.h"
#include <libintl.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#undef DEBUG
#define DEBUG(format, ...)                                                                         \
    do {                                                                                           \
        if (get_debug_flag()) {                                                                    \
            pam_syslog(pamh,                                                                       \
                       LOG_DEBUG,                                                                  \
                       "[FILE: %s] [FUNC: %s] [LINE: %d] : " format "\n",                          \
                       __FILE__,                                                                   \
                       __FUNCTION__,                                                               \
                       __LINE__,                                                                   \
                       ##__VA_ARGS__);                                                             \
        }                                                                                          \
    } while (0)

struct pam_paras {
    int level;
    const char *dict_path;
    int retry_cnt;
    int debug;
};

int parse_argv(int argc, const char **argv, struct pam_paras *paras) {
    for (int i = 0; argc-- > 0; ++argv) {
        if (!strncmp(*argv, "level=", 6)) {
            paras->level = atoi(*argv + 6);
        } else if (!strncmp(*argv, "dict_path=", 10)) {
            paras->dict_path = *argv + 10;
        } else if (!strncmp(*argv, "retry=", 6)) {
            paras->retry_cnt = atoi(*argv + 6);
        } else if (!strcmp(*argv, "debug")) {
            paras->debug = 1;
        }
    }

    return 0;
}

int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv) {

    struct pam_paras paras = {
            .debug = 0,
            .dict_path = NULL,
            .retry_cnt = 0,
    };
    int ret = parse_argv(argc, argv, &paras);
    if (ret < 0) {
        return PAM_ABORT;
    }

    set_debug_flag(paras.debug);

    DEBUG("called");
    const char *user;
    const char *dict_path = NULL;

    char outbuf[256];
    int finally_result = 0;

    if (flags & PAM_PRELIM_CHECK) {
        return PAM_SUCCESS;
    } else if (flags & PAM_UPDATE_AUTHTOK) {

        if (paras.retry_cnt <= 0) {
            paras.retry_cnt = 1;
        }

        DEBUG("level is %d", paras.level);
        ret = pam_get_user(pamh, &user, NULL);

        if (ret != PAM_SUCCESS || user == NULL) {
            DEBUG("get user from pam failed: %s", pam_strerror(pamh, ret));
            return PAM_USER_UNKNOWN;
        }

        const char *new_token = NULL;

        int cur_cnt = paras.retry_cnt;
        while (cur_cnt) {

            cur_cnt--;

            ret = pam_get_authtok(pamh, PAM_AUTHTOK, &new_token, NULL);
            if (ret == PAM_SUCCESS) {
                goto check;
            }

            ret = pam_get_authtok_noverify(pamh, &new_token, gettext("New password: "));

            if (ret != PAM_SUCCESS) {
                pam_syslog(pamh,
                           LOG_ERR,
                           "pam_get_authtok_noverify returned error: %s",
                           pam_strerror(pamh, ret));
                DEBUG("pam_get_authtok_noverify returned error: %s", pam_strerror(pamh, ret));
                continue;
            } else if (new_token == NULL) { /* user aborted password change, quit */
                return PAM_AUTHTOK_ERR;
            }

            DEBUG("new password is %s\n", new_token);
            ret = deepin_pw_check(user, new_token, paras.level, paras.dict_path);

            if (ret != PW_NO_ERR) {
                sprintf(outbuf, "Bad Password: %s\n", err_to_string((PW_ERROR_TYPE)ret));
                printf(gettext(outbuf));
                continue;
            }

            char *new_token2;
            ret = pam_prompt(pamh,
                             PAM_PROMPT_ECHO_OFF,
                             &new_token2,
                             gettext("Retype new password: "));

            if (ret != PAM_SUCCESS) {
                pam_syslog(pamh,
                           LOG_ERR,
                           "pam_get_authtok_noverify returned error: %s",
                           pam_strerror(pamh, ret));
                DEBUG("pam_get_authtok_noverify returned error: %s", pam_strerror(pamh, ret));
                continue;
            } else if (new_token == NULL) { /* user aborted password change, quit */
                return PAM_AUTHTOK_ERR;
            }

            if (strcmp(new_token, new_token2)) {
                sprintf(outbuf, "Sorry, passwords do not match\n");
                printf(gettext(outbuf));
                continue;
            }
        check:
            ret = deepin_pw_check(user, new_token, paras.level, paras.dict_path);
            DEBUG("check ret: %d", ret);

            if (ret != PW_NO_ERR) {
                continue;
            }

            // pam_unix 会校验此部分
            // extern int verify_pwd(const char *p, char *hash, unsigned int nullok);
            // struct spwd *spwd = getspnam(user);
            // if (spwd == NULL) {
            //     DEBUG("can not get hash password");
            //     return PAM_PERM_DENIED;
            // }

            // int verify_result = verify_pwd(new_token, spwd->sp_pwdp, 1);
            // if (verify_result == 0) {
            //     ret = PW_ERR_PW_REPEAT;
            // }

            // if (ret == PW_ERR_PW_REPEAT) {
            //     DEBUG("new password is same with old password");
            //     sprintf(outbuf, "Sorry, passwords not changed\n");
            //     printf(gettext(outbuf));
            //     continue;
            // }
            finally_result = 1;
        }
        if (finally_result) {
            DEBUG("success");
            return PAM_SUCCESS;
        } else {
            DEBUG("failed");
            return PAM_AUTHTOK_ERR;
        }
    }

    return PAM_SERVICE_ERR;
}
