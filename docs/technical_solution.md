- [密码校验技术方案](#密码校验技术方案)
  - [问题](#问题)
  - [现状](#现状)
  - [技术方案](#技术方案)
    - [需要验证的项](#需要验证的项)
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

## 技术方案
   
1. 提供一个动态库`deepin_pw_check.so`，该动态库提供密码校验接口，返回密码校验结果。
    deepin_pw_check.so提供的接口
    ```c
    1. PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);  // 密码校验

    2. const char* err_to_string(PW_ERROR_TYPE err); //  校验结果描述
    ```

    提供以上两个接口，用户校验密码和获取错误值的描述信息。
      
2. 提供一个动态库`deepin_pw_check.so`，实现`pam`的`pam_sm_chauthtok`接口，该接口中调用上述1中的密码校验接口，并根据接口返回给`pam`校验结果。该动态库将会写入`/etc/pam.d/common-passwd`配置文件中，当使用`passwd`修改密码时，将走此校验。
   

### 需要验证的项
  1. 为空校验：任意输入框为空，反馈文案：密码不能为空；
  2. 当前密码校验：当前密码输入错误，反馈文案：密码错误；
  3. 新密码一致性校验：新密码和重复密码不一致，反馈文案：密码不一致；
  4. 新密码重复性校验：新密码和当前密码相同，反馈文案：新密码和旧密码不能相同；
  5. 新密码强壮性校验：新密码长度超出密码策略，反馈文案：密码长度不能超过xx位；最大密码长度限制为510位；
  6. 新密码强壮性校验：新密码长度少于密码策略，反馈文案：密码长度不能少于xx位；
  7. 新密码强壮性校验：新密码字符类型不符合密码策略，反馈文案：密码必须由字母、数字、符号三种类型组成；
  8. 服务器特有校验规则：
      1. 新密码强壮性校验：新密码不满足回文字符校验，反馈文案：密码不得含有连续4个以上的回文字符；
      2. 新密码强壮性校验：新密码不满足密码字典校验，反馈文案：密码不能是常见单词及组合；
      3. 用户密码配置文件读写授权校验：未通过授权验证，反馈文案：修改密码失败；
   

## 实验验证
    
### `deepin_pw_check.so` 接口及实现
   1. `PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path);`
    该接口根据用户输入的参数进行不同程度的密码校验，参数选项有：
    **LEVEL_STANDARD_CHECK     // 标准校验**
    **LEVEL_STRICT_CHECK       // 严格校验**
    **LEVEL_CREATE_USER        // 创建用户校验，将不会校验密码**
    其中**LEVEL_CREATE_USER**可以与其他两个参数组合，而**LEVEL_STANDARD_CHECK**与**LEVEL_STRICT_CHECK**将不能进行组合。
    具体校验代码如下：
        ```c
        PW_ERROR_TYPE deepin_pw_check(const char* user,const char* pw, int level, const char* dict_path) {

            struct Options * options = get_default_options(level,dict_path);
            if (options == NULL) {
                return PW_ERR_PARA;
            }

            if (is_empty(pw)) {
                return PW_ERR_PASSWORD_EMPTY;
            }

            if (!is_length_valid(pw,options->min_len,options->max_len)) {
                return PW_ERR_LENGTH_INVALID;
            }

            if (!is_type_valid(pw,options->must_contain)) {
                return PW_ERR_CHARACTER_INVALID;
            }

            if (options->palindrome) {
                if (!is_palindrome(pw,options->palindrome_min_num)) {
                    return PW_ERR_PALINDROME;
                }
            }

            if (options->check_word) {
                if (is_word(pw,options->dict_path)) {
                    return PW_ERR_WORD;
                }
            }

            if (options->password_match) {
                int ret = is_passwd_repeat(user,pw);
                
                if (ret == -2 || ret == -1){
                    return PW_ERR_USER;
                }else if (ret == 0) {
                    return PW_ERR_PW_REPEAT;
                }else if(ret != 1) {
                    return PW_ERR_INTERNAL;
                }
            } 
        
            return PW_NO_ERR;
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
            textdomain("deepin_pw_check");

            switch (err)
            {
            case PW_NO_ERR:
                return gettext("check success");
            case PW_ERR_PASSWORD_EMPTY:
                return gettext("password is empty");
            case PW_ERR_LENGTH_INVALID:
                return gettext("password's length is invalid");
            case PW_ERR_CHARACTER_INVALID:
                return gettext("password's character is invalid");
            case PW_ERR_PALINDROME:
                return gettext("password is palindrome");
            case PW_ERR_WORD:
                return gettext("password is based on word");
            case PW_ERR_PW_REPEAT:
                return gettext("password is repeat");
            case PW_ERR_PARA:
                return gettext("parameter options is invalid");
            case PW_ERR_INTERNAL:
                return gettext("internal error");
            case PW_ERR_USER:
                return gettext("invalid user");
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
2. 新旧密码一致性校验： 采用`pam-unix`库中的校验方案，从`/etc/shadow`文件中获取密文，将用户输入的明文加密成密文，将两个密文做对比，若一致，则新旧密码一致，反之则不一致。
    ```c
    int is_passwd_repeat(const char* user,const char* pw) {
        if (user == NULL) {
            return false;
        }

        extern int get_user_hash(const char* user,char* hash);
        extern int verify_pwd(const char *p, char *hash, unsigned int nullok);

        char * hash = (char*)malloc(256);
        int ret = 0;
        do{
            ret = get_user_hash(user,hash);
            if (ret < 0) {
                break;
            }
            ret = verify_pwd(pw,hash,1);

        }while(0);

        free(hash);
        hash = NULL;

        return ret;
    }
    ```
3. 获取`/etc/shadow`文件中用户密文： 由于`deepin_pw_check`的权限取决于调用者，因此访问`/etc/shadow`文件的权限则成为问题，为此提供`system bus`的`dbus`服务接口`com.deepin.daemon.Passwd.GetPasswdHash`，从该接口中获取用户的密文。而校验工作则在`deepin_pw_check`库中进行。
    ```c
    int get_user_hash(const char* user,char* hash) {
        sd_bus *bus = NULL;
        sd_bus_error err = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = NULL;
        char* res;
        if (user == NULL || hash == NULL){
            return -1;
        }

        int ret = sd_bus_open_system(&bus);
        if (ret < 0) {
            return ret;
        }
        do {
            DEBUG("call dbus method GetPasswdHash");
            ret = sd_bus_call_method(bus, DBUS_SERVICE, DBUS_PATH , DBUS_INTERFACE ,
                                "GetPasswdHash", &err , &reply , "s" , user);
            if (ret < 0) {
                ret = -2;
                break;
            }

            ret = sd_bus_message_read(reply, "s", &res);
            if (ret < 0) {
                break;
            }
            sprintf(hash,"%s",res);

        }while(0);

        sd_bus_error_free(&err);
        sd_bus_message_unref(reply);

        return ret;
    }
    ```
    `com.deepin.daemon.Passwd`服务的接口部分，采用`go`代码编写,使用`cgo`调用`c`代码并获取结果。
    ```go
    func (m *manager) GetPasswdHash(user string) (string, *dbus.Error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	var pw *C.cspwd
	pw = C.getspnam(C.CString(user))
	if pw == nil {
		return "", dbusutil.ToError(fmt.Errorf("get passwd for %s failed", user))
	}
	return C.GoString(pw.sp_pwdp), nil
    }

    ```
4. 明文加密及校验算法： 来源于`pam_unix`
    ```c
    int verify_pwd(const char *p, char *hash, unsigned int nullok)
    {
        size_t hash_len;
        char *pp = NULL;
        int retval;

        strip_hpux_aging(hash);
        hash_len = strlen(hash);
        if (!hash_len) {
            if (nullok) { 
                retval = 0;
            } else {
                retval = 1;
            }
        } else if (!p || *hash == '*' || *hash == '!') {
            retval = 1;
        } else {
            if (!strncmp(hash, "$1$", 3)) {
                pp = crypt_md5(p, hash);
                if (pp && strcmp(pp, hash) != 0) {
                    _ptr_delete(pp);
                    pp = crypt_md5(p, hash);
                }
            } else if (*hash != '$' && hash_len >= 13) {
                pp = bigcrypt(p, hash);
                if (pp && hash_len == 13 && strlen(pp) > hash_len) {
                    _pw_check_overwrite(pp + hash_len);
                }
            } else {
                pp = x_strdup(crypt(p, hash));
            }
            p = NULL;		/* no longer needed here */

            if (pp && strcmp(pp, hash) == 0) {
                retval = 0;
            } else {
                retval = 1;
            }
        }

        if (pp){
            _ptr_delete(pp);
        }
        return retval;
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