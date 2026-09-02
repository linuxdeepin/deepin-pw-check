// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_consecutive_same_character(const char* pw, int consecutive_num);

    if( is_consecutive_same_character("aaabbc",3) != true) {
        return -1;
    }

    if( is_consecutive_same_character("aabbcc",3) != false) {
        return -1;
    }

    return 0;
}