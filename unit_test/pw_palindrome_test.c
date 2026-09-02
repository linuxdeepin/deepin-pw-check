// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main()
{
    extern bool is_palindrome(const char *pw, int palindrome_min_num);
    extern bool is_include_palindrome(const char *pw, int palindrome_min_num);

    set_debug_flag(1);
    if (is_palindrome("1234432", 4) != false)
    {
        return -1;
    }

    if (is_palindrome("12344321", 4) != true)
    {
        return -1;
    }

    if (is_palindrome("123454321", 5) != false)
    {
        return -1;
    }

    if (is_palindrome("123454321", 4) != true)
    {
        return -1;
    }

    if (is_include_palindrome("ac123454321", 4) != true)
    {
        return -1;
    }

    if (is_include_palindrome("123454321ac", 4) != true)
    {
        return -1;
    }

    if (is_include_palindrome("ac123454321ac", 4) != true)
    {
        return -1;
    }

    return 0;
}