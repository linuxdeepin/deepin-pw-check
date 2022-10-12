// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "common.h"

#define  x_strdup(s)  ( (s) ? strdup(s):NULL )


static void
strip_hpux_aging(char *hash)
{
	static const char valid[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789./";
	if ((*hash != '$') && (strlen(hash) > 13)) {
		for (hash += 13; *hash != '\0'; hash++) {
			if (strchr(valid, *hash) == NULL) {
				*hash = '\0';
				break;
			}
		}
	}
}


extern char *crypt_md5(const char *pw, const char *salt);
extern char *bigcrypt(const char *key, const char *salt);


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