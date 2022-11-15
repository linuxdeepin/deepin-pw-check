- [密码校验技术方案](#密码校验技术方案)
  - [问题](#问题)
  - [现状](#现状)
  - [解决方案及考虑点](#解决方案及考虑点)
  - [技术方案](#技术方案)
  - [实验验证](#实验验证)
    - [`deepin_pw_check.so` 接口及实现](#deepin_pw_checkso-接口及实现)
    - [接口校验实现方案](#接口校验实现方案)
    - [pam库校验接口](#pam库校验接口)
  - [总结](#总结)

# 密码校验技术方案

## 问题
   在uos系统中，设置密码有多种方式，可以通过后端dbus接口，可以通过passwd命令，而关于设置密码时的密码校验，却没有统一的接口，故每个设置密码的地方都需要自己去做密码合法性校验，这就会导致使用某一种方式能设置成功的密码，在其他地方修改密码的时候，填写当前密码导致密码校验不通过的问题。

## 现状

已知的设置密码及校验的地方

1. 安装器 -> 通过`passwd`设置密码

2. 控制中心新建用户 -> 通过正则校验密码合法性，校验方案根据`/etc/deepin/dde.conf`配置文件而定，将明文密码生成密文，调用后端接口写`/etc/shadow`文件来设置密码

3. 控制中心修改密码 -> 通过`passwd`设置密码，使用passwd的pam服务来校验密码合法性

针对以上问题，uos系统需要提供一个统一的接口，用来实现对密码合法性的校验，满足密码校验一致性的需求，解决上述问题。

## 解决方案及考虑点
解决方案为提供一套统一的密码校验接口，给前端应用如控制中心，安装器等调用，同时提供一套pam接口，给passwd等命令行程序使用。

此方案需要考虑兼容 `/etc/deepin/dde.conf` 配置文件，兼容方式为
1. 若 `/etc/deepin/dde.conf` 配置文件存在，则读取该配置并使用。
2. 若该配置文件不存在，则根据产品需求以及系统型号生成一套默认的配置，写入到 `/etc/deepin/dde.conf` 配置文件，后续继续使用该配置文件
3. 若产品上关于密码校验的需求有更新，由该接口进行更新配置，并写入到 `/etc/deepin/dde.conf` 配置文件。

## 技术方案
   
1. 提供一个动态库`deepin_pw_check.so`，该动态库提供密码校验接口，返回密码校验结果。
    deepin_pw_check.so提供的接口
    ```c
    1. PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);  // 密码校验

    2. const char* err_to_string(PW_ERROR_TYPE err); //  校验结果描述
    ```

    提供以上两个接口，用户校验密码和获取错误值的描述信息。
      
2. 提供一个动态库`pam_deepin_pw_check.so`，实现`pam`的`pam_sm_chauthtok`接口，该接口中调用上述1中的密码校验接口，并根据接口返回给`pam`校验结果。该动态库将会写入`/etc/pam.d/common-passwd`配置文件中，当使用`passwd`修改密码时，将走此校验。

3. 提供一个 dbus 服务 deepin_pw_check，该服务用来操作 `/etc/deepin/dde.conf` 配置文件，可以通过该接口获取当前配置，也可通过该接口修改配置，修改配置需要鉴权。服务名为 `org.deepin.dde.PasswdConf1`,路径名为 `/org/deepin/dde/PasswdConf1`, 接口名为 `org.deepin.dde.PasswdConf1`。

4. 提供一个工具 pwd_conf_update，该工具可以用于手动更新 `/etc/deepin/dde.conf` 配置。一般在安装时调用，用于更新配置文件字段。
## 实验验证
    
### `deepin_pw_check.so` 接口及实现
   1. `PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);`
    该接口中 level 参数暂时无用。
    具体校验代码如下：
        ```c
        PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path) {

        struct Options * options = get_default_options(level,dict_path);
        if (options == NULL) {
            return PW_ERR_PARA;
        }

        PW_ERROR_TYPE ret = PW_NO_ERR;
        
        do {
            if (is_empty(pw)) {
                ret = PW_ERR_PASSWORD_EMPTY;
                break;
            }
            
            if (options->enabled) {
                return ret;
            }

            if (PW_NO_ERR != (ret = is_length_valid(pw, options->min_len, options->max_len))) {
                break;
            }

            if (options->first_letter_uppercase) {
                if (!is_first_letter_uppercase(pw)) {
                    ret = PW_ERR_PW_FIRST_UPPERM;
                    break;
                }
            }

            if (!is_type_valid(pw, options->character_type, options->character_num_required)) {
                ret = PW_ERR_CHARACTER_INVALID;
                break;
            }

            if (options->palindrome_min_num && options->palindrome_min_num > 0) {
                if (is_palindrome(pw, options->palindrome_min_num)) {
                    ret = PW_ERR_PALINDROME;
                    break;
                }
            }

            if (options->check_word) {
                if (is_word(pw,options->dict_path)) {
                    ret = PW_ERR_WORD;
                    break;
                }
            }

            if (options->monotone_character_num && options->monotone_character_num > 0) {
                if ( is_monotone_character(pw, options->monotone_character_num )) {
                    ret = PW_ERR_PW_CONSECUTIVE_SAME;
                }
            }

            if (options->consecutive_same_character_num && options->consecutive_same_character_num > 0) {
                if ( is_consecutive_same_character(pw, options->consecutive_same_character_num)) {
                    ret = PW_ERR_PW_CONSECUTIVE_SAME;
                }
            }

        }while(0);

        free(options);

        return ret;
        }
            
        ```
        如上代码所示，校验接口先根据参数获取默认的校验选项，其次根据这些选项依次进行密码是否为空、密码长度是否符合要求、密码字符种类是否符合要求、是否是回文数、是否是常见单词，是否和当前密码一致的检测，若任何一项有误，均会返回。
        
   2. `const char* err_to_string(PW_ERROR_TYPE err);`
    该接口获取`deepin_pw_check`返回错误值的具体描述信息。
    具体校验代码如下：
        ```c
        const char* err_to_string(PW_ERROR_TYPE err){

        if (err >= PW_ERR_MAX) {
            return gettext("invalid error type");
        }

        setlocale(LC_ALL, "");
        textdomain("deepin-pw-check");
        char tmp_buff[BUFF_SIZE];
        int len = 0;
        int num = 0;
        switch (err)
        {
        case PW_NO_ERR:
            return gettext("check success");
        case PW_ERR_PASSWORD_EMPTY:
            return gettext("password cannot be empty");
        case PW_ERR_LENGTH_SHORT:
            len = get_pw_min_length(0);
            snprintf(out_buff, BUFF_SIZE, gettext("Password must have at least %d characters"), len);
            return out_buff;
        case PW_ERR_LENGTH_LONG:
            len = get_pw_max_length(0);
            snprintf(out_buff, BUFF_SIZE, gettext("Password must be no more than %d characters"), len);
            return out_buff;
        case PW_ERR_CHARACTER_INVALID:
            strcpy(tmp_buff, get_pw_validate_policy(0));
            snprintf(out_buff, BUFF_SIZE, gettext("Password can only contain %s"), tmp_buff);
            return out_buff;
        case PW_ERR_PALINDROME:
            num = get_pw_palimdrome_num(0);
            snprintf(out_buff, BUFF_SIZE, gettext("Password must not contain more than %d palindrome characters"), num);
            return out_buff;
        case PW_ERR_PW_CONSECUTIVE_SAME:
            return gettext("It does not meet password rules");
        case PW_ERR_PW_MONOTONE:
            return gettext("It does not meet password rules");
        case PW_ERR_PW_FIRST_UPPERM:
            return gettext("It does not meet password rules");
        case PW_ERR_WORD:
            return gettext("Do not use common words and combinations as password");
        case PW_ERR_PARA:
            return gettext("Parameter options is invalid");
        case PW_ERR_INTERNAL:
            return gettext("Internal error");
        case PW_ERR_USER:
            return gettext("Invalid user");
        default:
            return gettext("It does not meet password rules");
        }
            return "";
        }
        ``` 
### 接口校验实现方案  
1. 密码字典校验 ：采用`libcrack2-dev`库提供的接口实现
    ```c
    bool is_word(const char* pw,const char* dict_path) {
        extern int word_check(const char* pw,const char* dict_path);

        return !word_check(pw,dict_path);
    }

    int word_check(const char* pw,const char* dict_path) {
        PWDICT *pwp;
        unsigned int notfound;

        if (dict_path == NULL){
            dict_path = GetDefaultCracklibDict();
        }
        if (!(pwp = PWOpen(dict_path, "r")))
        {
            return -1;
        }
        
        notfound = PW_WORDS(pwp);

        for (int i = 0; r_destructors[i]; i++)
        {
            char *a;

            if (!(a = Mangle((char*)pw, r_destructors[i])))
            {
                continue;
            }

            if (FindPW(pwp, a) != notfound)
            {
                return 1;
            }
        }

        PWClose(pwp);
        return 0;
    }
    ```
### pam库校验接口
    
实现`pam_sm_chauthtok`函数，并调用`deepin_pw_check`方法进行校验
    
```c
int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv) {

    struct pam_paras paras = {
        .debug = 0,
        .dict_path = NULL,
        .retry_cnt = 0,
    };
    int ret = parse_argv(argc, argv, &paras);
    if (ret < 0) {
        return PAM_ABORT;
    }

    set_debug_flag(paras.debug);

    DEBUG("called");
    const char* user;
    const char * dict_path = NULL;

    char outbuf[256];
    int finally_result =0;

    extern bool is_level_valid(int level);

    if (!is_level_valid(paras.level)){
        return PAM_SERVICE_ERR;
    }

    if (flags & PAM_PRELIM_CHECK) {
        return PAM_SUCCESS;
    } else if (flags & PAM_UPDATE_AUTHTOK){

        if (paras.retry_cnt <= 0) {
            paras.retry_cnt = 1;
        }

        DEBUG("level is %d", paras.level);
        ret = pam_get_user(pamh, &user, NULL);
        
        if ( ret != PAM_SUCCESS || user == NULL) {
            DEBUG("get user from pam failed: %s", pam_strerror(pamh, ret));
            return PAM_USER_UNKNOWN;
        }

        const char* new_token = NULL;

        int cur_cnt = paras.retry_cnt;
        while( cur_cnt ) {
            
            cur_cnt--;

            ret = pam_get_authtok_noverify(pamh, &new_token, gettext("New password: "));

            if (ret != PAM_SUCCESS) {
                pam_syslog(pamh, LOG_ERR, "pam_get_authtok_noverify returned error: %s",pam_strerror(pamh, ret));
                DEBUG("pam_get_authtok_noverify returned error: %s",pam_strerror(pamh, ret));
                continue;
            } else if (new_token == NULL) {      /* user aborted password change, quit */
                return PAM_AUTHTOK_ERR;
            }

            DEBUG("new password is %s\n",new_token);
            ret = deepin_pw_check(user, new_token, paras.level | LEVEL_CREATE_USER, paras.dict_path);

            if (ret != PW_NO_ERR){
                sprintf(outbuf,"Bad Password: %s\n",err_to_string((PW_ERROR_TYPE)ret));
                printf(gettext(outbuf));
                continue;
            }

            char* new_token2;
            ret = pam_prompt(pamh, PAM_PROMPT_ECHO_OFF, &new_token2, gettext("Retype new password: "));

            if (ret != PAM_SUCCESS) {
                pam_syslog(pamh, LOG_ERR, "pam_get_authtok_noverify returned error: %s",pam_strerror(pamh, ret));
                DEBUG("pam_get_authtok_noverify returned error: %s",pam_strerror(pamh, ret));
                continue;
            } else if (new_token == NULL) {      /* user aborted password change, quit */
                return PAM_AUTHTOK_ERR;
            }

            if(strcmp(new_token,new_token2)) {
                sprintf(outbuf,"Sorry, passwords do not match\n");
                printf(gettext(outbuf));
                continue;
            }
            
            finally_result = 1;
            
        }
        if (finally_result){
            DEBUG("success");
            return PAM_SUCCESS;
        }else{
            DEBUG("failed");
            return PAM_AUTHTOK_ERR;
        }
        
    }

    return PAM_SERVICE_ERR;
}
```
## 总结
使用以上方案，使用同一套代码校验规则，可以解决当前uos系统上遇到的密码校验规则不一致导致的一系列问题。