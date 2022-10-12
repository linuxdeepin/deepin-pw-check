// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _COMMON_H_
#define _COMMON_H_

#define _pw_check_free(X) \
do {                 \
    if (X) {         \
        free(X);     \
        X=NULL;      \
    }                \
} while (0)

#define _pw_check_overwrite(x)        \
do {                             \
     register char *__xx__;      \
     if ((__xx__=(x)))           \
          while (*__xx__)        \
               *__xx__++ = '\0'; \
} while (0)

#define _ptr_delete(xx)		\
{				\
	_pw_check_overwrite(xx);	\
	_pw_check_free(xx);		\
}

#endif