// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

#define PASSWD_CONF_FILE        "./testdata/dde.conf"

int main() {
    extern const char* err_to_string(PW_ERROR_TYPE err);
    set_debug_flag(1);
    printf("%s\n",err_to_string(PW_ERR_CHARACTER_INVALID));

    return 0;
}