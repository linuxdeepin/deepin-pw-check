// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_monotone_character(const char* pw, int monotone_num);

    if( is_monotone_character("123456",3) != true) {
        return -1;
    }

    if( is_monotone_character("12ABab",3) != false) {
        return -1;
    }

    if( is_monotone_character("ewq",3) != true) {
        return -1;
    }

    if( is_monotone_character("!@#$\%^&*()_+",3) != true) {
        return -1;
    }

    if( is_monotone_character("$\%!^",3) != false) {
        return -1;
    }

    if( is_monotone_character("P{}",3) != true) {
        return -1;
    }

    if( is_monotone_character(":\"|",3) != true) {
        return -1;
    }

    if( is_monotone_character("<>?",3) != true) {
        return -1;
    }

    if( is_monotone_character("p[]",3) != true) {
        return -1;
    }

    if( is_monotone_character(";'\\",3) != true) {
        return -1;
    }

    if( is_monotone_character(",./",3) != true) {
        return -1;
    }

    return 0;
}