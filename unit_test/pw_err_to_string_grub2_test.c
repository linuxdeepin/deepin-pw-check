// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    set_debug_flag(1);
    printf("%s\n",err_to_string_grub2(PW_ERR_CHARACTER_INVALID));
    printf("%s\n",err_to_string_grub2(PW_ERR_LENGTH_SHORT));
    printf("%s\n",err_to_string_grub2(PW_ERR_LENGTH_LONG));

    return 0;
}
