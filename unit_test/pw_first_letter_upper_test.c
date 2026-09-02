// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern bool is_first_letter_uppercase(const char* pw);
    
    if( is_first_letter_uppercase("fdajflkajsdl") != false) {
        return -1;
    }

    if( is_first_letter_uppercase("Fdajflkajsdl") != true) {
        return -1;
    }

    return 0;
}