// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main() {
    extern PW_ERROR_TYPE is_length_valid(const char* pw, int min_len, int max_len);
    
    if( is_length_valid("123456",6,6) != PW_NO_ERR ) {
        return -1;
    }

    return 0;
}