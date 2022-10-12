# deepin-pw-check
deepin-pw-check 是一个密码规则校验工具。

# 依赖
请查看“debian/control”文件中提供的“Depends”。

## 编译依赖
请查看“debian/control”文件中提供的“Build-Depends”。

## 安装

### 构建过程

1. 确保已经安装了所有的编译依赖。

2. 构建:
```
$ cd deepin-pw-check
$ make
```

3. 安装:
```
$ sudo make install
```

安装完成后可执行二进制文件在`/usr/lib/deepin-pw-check`。

# 用法
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

## 获得帮助
如果您遇到任何其他问题，您可能会发现这些渠道很有用：

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC Channel](https://webchat.freenode.net/?channels=deepin)
* [Official Forum](https://bbs.deepin.org/)
* [Wiki](https://wiki.deepin.org/)
* [Developer Center](https://github.com/linuxdeepin/deepin-pw-check)

## 贡献指南

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

deepin-pw-check在[GPL-3.0-or-later](LICENSE)下发布。