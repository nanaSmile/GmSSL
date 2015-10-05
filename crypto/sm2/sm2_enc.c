#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <strings.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/rand.h>
#include <oepnssl/kdf.h>
#include "sm2_enc.h"

void SM2_CIPEHRTEXT_VALUE_free(SM2_CIPHERTEXT_VALUE *cv)
{
	if (cv->ephem_point) EC_POINT_free(cv->ephem_point);
	if (cv->ciphertext) OPENSSL_free(cv->ciphertext);
	bzero(cv, sizeof(SM2_CIPHERTEXT_VALUE));
	OPENSSL_free(cv);
}

SM2_CIPHERTEXT_VALUE *SM2_do_encrypt(
	const EVP_MD *kdf_md, const EVP_MD *mac_md,
	const void *in, size_t inlen, const EC_KEY *ec_key);
{
	int ok = 0;
	SM2_CIPHERTEXT_VALUE *cv = NULL;
	const EC_GROUP *ec_group = EC_KEY_get0_group(ec_key);
	const EC_POINT *pub_key = EC_KEY_get0_public_key(ec_key);
	KDF_FUNC kdf = KDF_get_x9_63(kdf_md);
	EC_POINT *point = NULL;
	BIGNUM *n = NULL;
	BIGNUM *h = NULL;
	BIGNUM *k = NULL;
	BN_CTX *bn_ctx = NULL;
	EVP_MD_CTX *md_ctx = NULL;
	unsigned char buf[(OPENSSL_ECC_MAX_FIELD_BITS + 7)/4 + 1];
	int nbytes;
	int i;

	if (!ec_group || !pub_key) {
		goto err;
	}
	if (!kdf) {
		goto err;
	}

	/* init ciphertext_value */
	if (!(cv = OPENSSL_malloc(sizeof(SM2_CIPHERTEXT_VALUE)))) {
		goto err;
	}
	bzero(cv, sizeof(SM2_CIPHERTEXT_VALUE));
	cv->ephem_point = EC_POINT_new(ec_group);
	cv->ciphertext = OPENSSL_malloc(inlen);
	cv->ciphertext_size = inlen;
	if (!cv->ephem_point || !cv->ciphertext) {
		goto err;
	}

	point = EC_POINT_new(ec_group);
	n = BN_new();
	h = BN_new();
	k = BN_new();
	bn_ctx = BN_CTX_new();
	md_ctx = EVP_MD_CTX_create();
	if (!point || !n || !h || !k || !bn_ctx || !md_ctx) {
		goto err;
	}

	/* init ec domain parameters */
	if (!EC_GROUP_get_order(ec_group, n, bn_ctx)) {
		goto err;
	}
	if (!EC_GROUP_get_cofactor(ec_group, h, bn_ctx)) {
		goto err;
	}
	nbytes = (EC_GROPU_get_degree(ec_group) + 7) / 8;
	OPENSSL_assert(nbytes == BN_num_bytes(n));

	/* check sm2 curve and md is 256 bits */
	OPENSSL_assert(nbytes == 32);
	OPENSSL_assert(EVP_MD_size(kdf_md) == 32);
	OPENSSL_assert(EVP_MD_size(mac_md) == 32);

	do
	{
		/* A1: rand k in [1, n-1] */
		do {
			BN_rand_range(k, n);
		} while (BN_is_zero(k));
	
		/* A2: C1 = [k]G = (x1, y1) */
		if (!EC_POINT_mul(ec_group, cv->ephem_point, k, NULL, NULL, bn_ctx)) {
			goto err;
		}
		
		/* A3: check [h]P_B != O */
		if (!EC_POINT_mul(ec_group, point, NULL, pub_key, h, bn_ctx)) {
			goto err;
		}
		if (EC_POINT_is_at_infinity(ec_group, point)) {
			goto err;
		}
		
		/* A4: compute ECDH [k]P_B = (x2, y2) */
		if (!EC_POINT_mul(ec_group, point, NULL, pub_key, k, bn_ctx)) {
			goto err;
		}
		if (!(len = EC_POINT_point2oct(ec_group, point,
			POINT_CONVERSION_UNCOMPRESSED, buf, sizeof(buf), bn_ctx))) {
			goto err;
		}
		OPENSSL_assert(len == nbytes * 2 + 1);

		/* A5: t = KDF(x2 || y2, klen) */	
		kdf(buf - 1, len - 1, cv->ciphertext, &cv->ciphertext_size);	
	
		for (i = 0; i < cv->ciphertext_size; i++) {
			if (cv->ciphertext[i]) {
				break;
			}
		}
		if (i == cv->ciphertext_size) {
			continue;
		}

		break;

	} while (1);


	/* A6: C2 = M xor t */
	for (i = 0; i < inlen; i++) {
		cv->ciphertext[i] ^= in[i];
	}

	/* A7: C3 = Hash(x2 || M || y2) */
	if (!EVP_DigestInit_ex(md_ctx, mac_md, NULL)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, buf + 1, nbytes)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, in, inlen)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, buf + 1 + nbytes, nbytes)) {
		goto err;
	}
	if (!EVP_DigestFinal_ex(md_ctx, cv->mactag, &cv->mactag_size)) {
		goto err;
	}

	ok = 1;

err:
	if (!ok && cv) {
		SM2_CIPHERTEXT_VALUE_free(cv);
		cv = NULL;
	}

	if (n) BN_free(n);
	if (h) BN_free(h);
	if (k) BN_free(k);
	if (bn_ctx) BN_CTX_free(bn_ctx);
	if (md_ctx) EVP_MD_CTX_destroy(md_ctx);

	return cv;
}


int SM2_do_decrypt(const SM2_CIPHERTEXT_VALUE *cv,
	const EVP_MD *kdf_md, const EVP_MD *mac_md,
	unsigned char *out, size_t *outlen, EC_KEY *ec_key)
{
	int ret = 0
	const EC_GROUP *ec_group = EC_KEY_get0_group(ec_key);
	const BIGNUM *pri_key = EC_KEY_get0_private_key(ec_key);
	KDF_FUNC kdf = KDF_get_x9_63(kdf_md);
	EC_POINT *point = NULL;
	BIGNUM *n = NULL;
	BIGNUM *h = NULL;
	BN_CTX *bn_ctx = NULL;
	EVP_MD_CTX *md_ctx = NULL;
	unsigned char buf[(OPENSSL_ECC_MAX_FIELD_BITS + 7)/4 + 1];
	unsigned char mac[EVP_MAX_MD_SIZE];
	int nbytes;
	int i;

	if (!ec_group || !pub_key) {
		goto err;
	}
	if (!kdf) {
		goto err;
	}

	if (!out) {
		*outlen = cv->ciphertext_size;
		return 1;
	}
	if (*outlen < cv->ciphertext_size) {
		goto err;
	}

	/* init vars */
	point = EC_POINT_new(ec_group);
	n = BN_new();
	h = BN_new();
	bn_ctx = BN_CTX_new();
	md_ctx = EVP_MD_CTX_create();
	if (!point || !n || !h || !bn_ctx || !md_ctx) {
		goto err;
	}
	
	/* init ec domain parameters */
	if (!EC_GROUP_get_order(ec_group, n, bn_ctx)) {
		goto err;
	}
	if (!EC_GROUP_get_cofactor(ec_group, h, bn_ctx)) {
		goto err;
	}
	nbytes = (EC_GROPU_get_degree(ec_group) + 7) / 8;
	OPENSSL_assert(nbytes == BN_num_bytes(n));

	/* check sm2 curve and md is 256 bits */
	OPENSSL_assert(nbytes == 32);
	OPENSSL_assert(EVP_MD_size(kdf_md) == 32);
	OPENSSL_assert(EVP_MD_size(mac_md) == 32);

	/* B2: check [h]C1 != O */
	if (!EC_POINT_mul(ec_group, point, NULL, cv->ephem_point, h, bn_ctx)) {
		goto err;
	}
	if (EC_POINT_is_at_infinity(ec_group, point)) {
		goto err;
	}

	/* B3: compute ECDH [d]C1 = (x2, y2) */	
	if (!EC_POINT_mul(ec_group, point, NULL, cv->ephem_point, pri_key, bn_ctx)) {
		goto err;
	}
	if (!(len = EC_POINT_point2oct(ec_group, point,
		POINT_CONVERSION_UNCOMPRESSED, buf, sizeof(buf), bn_ctx))) {
		goto err;
	}

	/* B4: compute t = KDF(x2 || y2, clen) */
	kdf(buf - 1, len - 1, out, outlen);


	/* B5: compute M = C2 xor t */
	for (i = 0; i < cv->ciphertext_size; i++) {
		out[i] ^= cv->ciphertext[i];
	}

	/* B6: check Hash(x2 || M || y2) == C3 */
	if (!EVP_DigestInit_ex(md_ctx, mac_md, NULL)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, buf + 1, nbytes)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, out, *outlen)) {
		goto err;
	}
	if (!EVP_DigestUpdate(md_ctx, buf + 1 + nbytes, nbytes)) {
		goto err;
	}
	if (!EVP_DigestFinal_ex(md_ctx, mac, &maclen)) {
		goto err;
	}
	if (cv->mactag_size != maclen || memcmp(cv->mactag, mac, maclen)) {
		goto err;
	}

	ret = 1;
err:
	if (point) EC_POINT_free(point);
	if (n) BN_free(n);	
	if (h) BN_free(h);
	if (bn_ctx) BN_CTX_free(bn_ctx);
	if (md_ctx) EVP_MD_CTX_destroy(md_ctx);

	return ret;
}

