#include <stdio.h>
#include "../lib/deepin_pw_check.h"

int main()
{
    extern bool include_chinese(const char *data);
    if (include_chinese("fafadAADF!$'") != false)
    {
        return -1;
    }

    if (include_chinese("fafadAADF测试") != true)
    {
        return -1;
    }

    if (include_chinese("fafadAADF￥”") != true)
    {
        return -1;
    }

    return 0;
}