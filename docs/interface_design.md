# 密码校验强度配置接口
密码校验强度提供了若干接口供其他程序调用，包括获取当前相关配置，设置密码校验强度，其中设置密码校验强度需要鉴权。

# 字段说明
1. 配置文件路径：/etc/deepin/dde.conf
2. 配置策略: 
   1. `STRONG_PASSWORD=true`
   该字段用于配置是否开启强密码策略，默认为true，表示开启，只有该字段为true的时候，下面的配置选项才会生效，否则密码为任意长度 >=1 的任意字符。
   2. `PASSWORD_MIN_LENGTH=1`
   密码最小长度配置
   3. `PASSWORD_MAX_LENGTH=512`
   密码最大长度配置
   4. `VALIDATE_POLICY="1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;~!@#$%^&*()[]{}\\|/<>"`
   密码匹配的字符分组策略，以分号（；）分组。注意如果需要支持空格，请勿将空格配置在每个分组的头和尾部
   5. `VALIDATE_REQUIRED=1`
   密码强度策略，根据密码的分组策略（VALIDATE_POLICY）来决定密码强度需要支持的最小密码组数
   例如：
    密码的分组:
    `VALIDATE_POLICY="1234567890;abcdefghijklmnopqrstuvwxyz;ABCDEFGHIJKLMNOPQRSTUVWXYZ;~!@#$%^&*()[]{}\\|/?,<>"`
    密码强度策略：`VALIDATE_REQUIRED=3`

# DBus 服务
提供 system bus上的服务 `org.deepin.dde.PasswdConf1`,路径名为 `/org/deepin/dde/PasswdConf`, 接口名为 `org.deepin.dde.PasswdConf1`。

# 接口说明

- `WriteConfig(char* data) -> ()`
    写密码校验强度配置，配置数据为ini格式，且需要遵循相关字段描述。调用该接口会发起鉴权操作。
    - 参数:
        data: 密码校验强度配置，为ini格式
---

- `ReadConfig() -> (char* data)`
    读密码校验强度配置，配置数据为ini格式。
    - 返回值:
        data: 密码校验强度配置，为ini格式
---

- `GetLengthLimit() -> (int minLength, int maxLength)`
    获取密码校验配置中的长度限制。
    - 返回值:
        minLength: 最小长度限制
        maxLength: 最大长度限制
---

- `SetLengthLimit(int minLength, int maxLength) -> ()`
    设置密码校验配置中的长度限制。调用该接口会发起鉴权操作。
    - 参数:
        minLength: 最小长度限制
        maxLength: 最大长度限制
---

- `GetValidatePolicy() -> (char* validatePolicy)`
    获取密码校验配置中的校验策略。
    - 返回值:
        validatePolicy: 密码校验策略
---

- `SetValidatePolicy(char* validatePolicy) -> ()`
    设置密码校验配置中的校验策略。调用该接口会发起鉴权操作。
    - 参数:
        validatePolicy: 密码校验策略
---

- `GetValidateRequired() -> (int validateRequired)`
    获取密码校验配置中的强度策略。
    - 返回值:
        validateRequired: 密码强度策略
---

- `SetValidateRequired(int validateRequired) -> ()`
    设置密码校验配置中的强度策略。调用该接口会发起鉴权操作。
    - 参数:
        validatePolicy: 密码强度策略
---

- `SetEnabled(bool enable) -> ()`
    设置密码校验配置是否使用。调用该接口会发起鉴权操作。
    - 参数:
        enable: 是否使用
---

- `SetEnabled() -> (bool enable)`
    获取密码校验配置是否使用。
    - 返回值:
        enable: 是否使用
---

- `GetFirstLetterUpper() -> (int enable)`
    获取首字母是否需要大写的配置。
    - 返回值:
        enable: 是否启用
---

- `SetFirstLetterUpper(int enbale) -> ()`
    设置首字母是否需要大写的配置。调用该接口会发起鉴权操作。
    - 参数:
        enable: 是否启用
---

- `Reset() -> ()`
    恢复已经备份的配置。如果有备份的配置，则可正常恢复，否则恢复失败。调用该接口会发起鉴权操作。
---

- `Backup() -> ()`
    备份当前配置。如果之前已有备份的配置，则会覆盖之前的配置。调用该接口会发起鉴权操作。
---