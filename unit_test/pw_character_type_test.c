// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main()
{
    extern PW_ERROR_TYPE is_type_valid(const char *pw, char *character_type, int character_num_required);
    if (is_type_valid("123456", "12345", 1) != PW_ERR_CHARACTER_INVALID)
    {
        return -1;
    }

    if (is_type_valid("123456", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2) != PW_ERR_CHARACTER_TYPE_TOO_FEW)
    {
        return -1;
    }

    if (is_type_valid("123456A", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2) != PW_NO_ERR)
    {
        return -1;
    }

    if (is_type_valid("123456a", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2) != PW_ERR_CHARACTER_INVALID)
    {
        return -1;
    }

    if (is_type_valid("123456Aa!", "1234567890;ABCDEFGHIJKLMNOPQRSTUVWXYZ", 2) != PW_ERR_CHARACTER_INVALID)
    {
        return -1;
    }

    if (is_type_valid("123456Aa!", "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                   ":;<=>?@[\\]^_`{|}~ /",
                      4) != PW_NO_ERR)
    {
        return -1;
    }

    if (is_type_valid("123456Aa!", "1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;!\"#$%&'()*+,-./"
                                   ":<=>?@[\\]^_`{|}~ /",
                      4) != PW_NO_ERR)
    {
        return -1;
    }

    return 0;
}