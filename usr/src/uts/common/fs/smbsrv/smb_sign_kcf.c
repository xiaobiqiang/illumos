/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2015 Nexenta Systems, Inc.  All rights reserved.
 */

/*
 * Helper functions for SMB signing using the
 * Kernel Cryptographic Framework (KCF)
 *
 * There are two implementations of these functions:
 * This one (for kernel) and another for user space:
 * See: lib/smbsrv/libfksmbsrv/common/fksmb_sign_pkcs.c
 */

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/crypto/api.h>
#include <smbsrv/smb_kproto.h>
#include <smbsrv/smb_signing.h>

/*
 * SMB1 signing helpers:
 * (getmech, init, update, final)
 */

int
smb_md5_getmech(smb_sign_mech_t *mech)
{
	crypto_mech_type_t t;

	t = crypto_mech2id(SUN_CKM_MD5);
	if (t == CRYPTO_MECH_INVALID)
		return (-1);
	mech->cm_type = t;
	return (0);
}

/*
 * Start the KCF session, load the key
 */
int
smb_md5_init(smb_sign_ctx_t *ctxp, smb_sign_mech_t *mech)
{
	int rv;

	rv = crypto_digest_init(mech, ctxp, NULL);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}

/*
 * Digest one segment
 */
int
smb_md5_update(smb_sign_ctx_t ctx, void *buf, size_t len)
{
	crypto_data_t data;
	int rv;

	bzero(&data, sizeof (data));
	data.cd_format = CRYPTO_DATA_RAW;
	data.cd_length = len;
	data.cd_raw.iov_base = buf;
	data.cd_raw.iov_len = len;

	rv = crypto_digest_update(ctx, &data, 0);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}

/*
 * Get the final digest.
 */
int
smb_md5_final(smb_sign_ctx_t ctx, uint8_t *digest16)
{
	crypto_data_t out;
	int rv;

	bzero(&out, sizeof (out));
	out.cd_format = CRYPTO_DATA_RAW;
	out.cd_length = MD5_DIGEST_LENGTH;
	out.cd_raw.iov_len = MD5_DIGEST_LENGTH;
	out.cd_raw.iov_base = (void *)digest16;

	rv = crypto_digest_final(ctx, &out, 0);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}

/*
 * SMB2 signing helpers:
 * (getmech, init, update, final)
 */

int
smb2_hmac_getmech(smb_sign_mech_t *mech)
{
	crypto_mech_type_t t;

	t = crypto_mech2id(SUN_CKM_SHA256_HMAC);
	if (t == CRYPTO_MECH_INVALID)
		return (-1);
	mech->cm_type = t;
	return (0);
}

/*
 * Start the KCF session, load the key
 */
int
smb2_hmac_init(smb_sign_ctx_t *ctxp, smb_sign_mech_t *mech,
    uint8_t *key, size_t key_len)
{
	crypto_key_t ckey;
	int rv;

	bzero(&ckey, sizeof (ckey));
	ckey.ck_format = CRYPTO_KEY_RAW;
	ckey.ck_data = key;
	ckey.ck_length = key_len * 8; /* in bits */

	rv = crypto_mac_init(mech, &ckey, NULL, ctxp, NULL);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}

/*
 * Digest one segment
 */
int
smb2_hmac_update(smb_sign_ctx_t ctx, uint8_t *in, size_t len)
{
	crypto_data_t data;
	int rv;

	bzero(&data, sizeof (data));
	data.cd_format = CRYPTO_DATA_RAW;
	data.cd_length = len;
	data.cd_raw.iov_base = (void *)in;
	data.cd_raw.iov_len = len;

	rv = crypto_mac_update(ctx, &data, 0);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}

/*
 * Note, the SMB2 signature is the first 16 bytes of the
 * 32-byte SHA256 HMAC digest.
 */
int
smb2_hmac_final(smb_sign_ctx_t ctx, uint8_t *digest16)
{
	uint8_t full_digest[SHA256_DIGEST_LENGTH];
	crypto_data_t out;
	int rv;

	bzero(&out, sizeof (out));
	out.cd_format = CRYPTO_DATA_RAW;
	out.cd_length = SHA256_DIGEST_LENGTH;
	out.cd_raw.iov_len = SHA256_DIGEST_LENGTH;
	out.cd_raw.iov_base = (void *)full_digest;

	rv = crypto_mac_final(ctx, &out, 0);
	if (rv == CRYPTO_SUCCESS)
		bcopy(full_digest, digest16, 16);

	return (rv == CRYPTO_SUCCESS ? 0 : -1);
}
