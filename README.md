# deepin-pw-check
deepin-pw-check is a tool to verify the validity of the password

# dependencies

## build dependencies

_The **master** branch is current development branch, build dependencies may changes without update README.md, refer to `./debian/control` for a working build depends list_

* cracklib-runtime
* deepin-gettext-tools
* golang-dbus-dev
* golang-github-linuxdeepin-go-dbus-factory-dev
* golang-github-linuxdeepin-go-lib-dev
* golang-go
* libcrack2-dev
* libiniparser-dev
* libpam-dev

# interface
```c
1. PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);  // 密码校验

2. const char* err_to_string(PW_ERROR_TYPE err); //  校验结果描述

3. void set_debug_flag(int flag);   // 是否开启调试开关
```

# usage
```c
#include <deepin_pw_check.h>
#include <stdio.h>
int main() {
    PW_ERROR_TYPE ret = deepin_pw_check("test", "123", LEVEL_STANDARD_CHECK, NULL);
    if (ret != PW_NO_ERR) {
        printf("deepin_pw_check err: %s\n", err_to_string(ret));
    }
    return 0;
}
```

# link
```c
gcc xxx.c -o xxx -ldeepin_pw_check
```