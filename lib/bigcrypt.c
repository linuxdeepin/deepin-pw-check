// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdlib.h>
#include "common.h"
#include <crypt.h>


#define MAX_PASS_LEN       16
#define SEGMENT_SIZE       8
#define SALT_SIZE          2
#define KEYBUF_SIZE        ((MAX_PASS_LEN*SEGMENT_SIZE)+SALT_SIZE)
#define ESEGMENT_SIZE      11
#define CBUF_SIZE          ((MAX_PASS_LEN*ESEGMENT_SIZE)+SALT_SIZE+1)

char *bigcrypt(const char *key, const char *salt)
{
	char *dec_c2_cryptbuf;
#ifdef HAVE_CRYPT_R
	struct crypt_data *cdata;
#endif
	unsigned long int keylen, n_seg, j;
	char *cipher_ptr, *plaintext_ptr, *tmp_ptr, *salt_ptr;
	char keybuf[KEYBUF_SIZE + 1];


	/* reset arrays */
	dec_c2_cryptbuf = (char*)malloc(CBUF_SIZE);
	if (!dec_c2_cryptbuf) {
		return NULL;
	}

	memset(keybuf, 0, KEYBUF_SIZE + 1);
	memset(dec_c2_cryptbuf, 0, CBUF_SIZE);

	/* fill KEYBUF_SIZE with key */
	strncpy(keybuf, key, KEYBUF_SIZE);

	/* deal with case that we are doing a password check for a
	   conventially encrypted password: the salt will be
	   SALT_SIZE+ESEGMENT_SIZE long. */
	if (strlen(salt) == (SALT_SIZE + ESEGMENT_SIZE))
		keybuf[SEGMENT_SIZE] = '\0';	/* terminate password early(?) */

	keylen = strlen(keybuf);

	if (!keylen) {
		n_seg = 1;
	} else {
		/* work out how many segments */
		n_seg = 1 + ((keylen - 1) / SEGMENT_SIZE);
	}

	if (n_seg > MAX_PASS_LEN)
		n_seg = MAX_PASS_LEN;	/* truncate at max length */

	cipher_ptr = dec_c2_cryptbuf;
	plaintext_ptr = keybuf;

	tmp_ptr = crypt(plaintext_ptr, salt);	/* libc crypt() */

	if (tmp_ptr == NULL) {
		free(dec_c2_cryptbuf);
		return NULL;
	}

	strncpy(cipher_ptr, tmp_ptr, 13);
	cipher_ptr += ESEGMENT_SIZE + SALT_SIZE;
	plaintext_ptr += SEGMENT_SIZE;


	salt_ptr = cipher_ptr - ESEGMENT_SIZE;


	if (n_seg > 1) {
		for (j = 2; j <= n_seg; j++) {

			tmp_ptr = crypt(plaintext_ptr, salt_ptr);

			if (tmp_ptr == NULL) {
				_pw_check_overwrite(dec_c2_cryptbuf);
				free(dec_c2_cryptbuf);
				return NULL;
			}

			strncpy(cipher_ptr, (tmp_ptr + SALT_SIZE), ESEGMENT_SIZE);

			cipher_ptr += ESEGMENT_SIZE;
			plaintext_ptr += SEGMENT_SIZE;
			salt_ptr = cipher_ptr - ESEGMENT_SIZE;
		}
	}

	return dec_c2_cryptbuf;
}
