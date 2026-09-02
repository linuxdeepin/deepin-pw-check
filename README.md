# deepin-pw-check
deepin-pw-check is a tool to verify the validity of the password.

# dependencies
You can also check the "Depends" provided in the debian/control file.

## build dependencies
You can also check the "Build-Depends" provided in the debian/control file.

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

2. Build:
```
$ cd deepin-pw-check
$ make
```

3. Install:
```
$ sudo make install
```

The executable binary file could be found at `/usr/libexec/deepin-pw-check` after the installation is finished.

# Usage
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

## Getting help
You may find these channels useful if you encounter any other issues:

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC Channel](https://webchat.freenode.net/?channels=deepin)
* [Official Forum](https://bbs.deepin.org/)
* [Wiki](https://wiki.deepin.org/)
* [Developer Center](https://github.com/linuxdeepin/deepin-pw-check)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

deepin-pw-check is licensed under [GPL-3.0-or-later](LICENSE).