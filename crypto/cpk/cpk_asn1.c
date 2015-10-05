#include <openssl/asn1t.h>
#include "cpk.h"

ASN1_SEQUENCE(CPK_MASTER_SECRET) = {
	ASN1_SIMPLE(CPK_MASTER_SECRET, version, LONG),
	ASN1_SIMPLE(CPK_MASTER_SECRET, id, X509_NAME),
	ASN1_SIMPLE(CPK_MASTER_SECRET, pkey_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_MASTER_SECRET, map_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_MASTER_SECRET, secret_factors, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CPK_MASTER_SECRET)
IMPLEMENT_ASN1_FUNCTIONS(CPK_MASTER_SECRET)
IMPLEMENT_ASN1_DUP_FUNCTION(CPK_MASTER_SECRET)

ASN1_SEQUENCE(CPK_PUBLIC_PARAMS) = {
	ASN1_SIMPLE(CPK_PUBLIC_PARAMS, version, LONG),
	ASN1_SIMPLE(CPK_PUBLIC_PARAMS, id, X509_NAME),
	ASN1_SIMPLE(CPK_PUBLIC_PARAMS, pkey_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_PUBLIC_PARAMS, map_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_PUBLIC_PARAMS, public_factors, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CPK_PUBLIC_PARAMS)
IMPLEMENT_ASN1_FUNCTIONS(CPK_PUBLIC_PARAMS)
IMPLEMENT_ASN1_DUP_FUNCTION(CPK_PUBLIC_PARAMS)


CPK_MASTER_SECRET *d2i_CPK_MASTER_SECRET_bio(BIO *bp, CPK_MASTER_SECRET **master) {
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(CPK_MASTER_SECRET), bp, master);
}

int i2d_CPK_MASTER_SECRET_bio(BIO *bp, CPK_MASTER_SECRET *master) {
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(CPK_MASTER_SECRET), bp, master);
}

CPK_PUBLIC_PARAMS *d2i_CPK_PUBLIC_PARAMS_bio(BIO *bp, CPK_PUBLIC_PARAMS **params) {
	return ASN1_item_d2i_bio(ASN1_ITEM_rptr(CPK_PUBLIC_PARAMS), bp, params);
}

int i2d_CPK_PUBLIC_PARAMS_bio(BIO *bp, CPK_PUBLIC_PARAMS *params) {
	return ASN1_item_i2d_bio(ASN1_ITEM_rptr(CPK_PUBLIC_PARAMS), bp, params);
}



/* This is the ANY DEFINED BY table for the top level PKCS#7 structure */
ASN1_ADB_TEMPLATE(cpkcmsdefault) = ASN1_EXP_OPT(CPK_CMS, d.other, ASN1_ANY, 0);

ASN1_ADB(CPK_CMS) = {
	ADB_ENTRY(NID_pkcs7_data, ASN1_NDEF_EXP_OPT(CPK_CMS, d.data, ASN1_OCTET_STRING_NDEF, 0)),
	ADB_ENTRY(NID_pkcs7_signed, ASN1_NDEF_EXP_OPT(CPK_CMS, d.sign, CPK_SIGNED, 0)),
	ADB_ENTRY(NID_pkcs7_enveloped, ASN1_NDEF_EXP_OPT(CPK_CMS, d.enveloped, CPK_ENVELOPE, 0)),
	ADB_ENTRY(NID_pkcs7_signedAndEnveloped, 
		ASN1_NDEF_EXP_OPT(CPK_CMS, d.signed_and_enveloped, CPK_SIGN_ENVELOPE, 0)),
} ASN1_ADB_END(CPK_CMS, 0, type, 0, &cpkcmsdefault_tt, NULL);


ASN1_NDEF_SEQUENCE(CPK_CMS) = {
	ASN1_SIMPLE(CPK_CMS, type, ASN1_OBJECT),
	ASN1_ADB_OBJECT(CPK_CMS)
}ASN1_NDEF_SEQUENCE_END(CPK_CMS)
IMPLEMENT_ASN1_FUNCTIONS(CPK_CMS)
IMPLEMENT_ASN1_NDEF_FUNCTION(CPK_CMS)
IMPLEMENT_ASN1_DUP_FUNCTION(CPK_CMS)


ASN1_NDEF_SEQUENCE(CPK_SIGNED) = {
	ASN1_SIMPLE(CPK_SIGNED, version, LONG),
	ASN1_SET_OF(CPK_SIGNED, digest_algors, X509_ALGOR),
	ASN1_SIMPLE(CPK_SIGNED, contents, CPK_CMS),
	ASN1_IMP_SEQUENCE_OF_OPT(CPK_SIGNED, cert, X509, 0),
	ASN1_IMP_SET_OF_OPT(CPK_SIGNED, crl, X509_CRL, 1),
	ASN1_SET_OF(CPK_SIGNED, signer_infos, CPK_SIGNER_INFO)
} ASN1_NDEF_SEQUENCE_END(CPK_SIGNED)
IMPLEMENT_ASN1_FUNCTIONS(CPK_SIGNED)


ASN1_SEQUENCE(CPK_SIGNER_INFO) = {
	ASN1_SIMPLE(CPK_SIGNER_INFO, version, LONG),
	ASN1_SIMPLE(CPK_SIGNER_INFO, signer, X509_NAME),
	ASN1_SIMPLE(CPK_SIGNER_INFO, digest_algor, X509_ALGOR),
	ASN1_IMP_SEQUENCE_OF_OPT(CPK_SIGNER_INFO, signed_attr, X509_ATTRIBUTE, 0),
	ASN1_SIMPLE(CPK_SIGNER_INFO, sign_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_SIGNER_INFO, signature, ASN1_OCTET_STRING),
	ASN1_IMP_SET_OF_OPT(CPK_SIGNER_INFO, unsigned_attr, X509_ATTRIBUTE, 1)
} ASN1_SEQUENCE_END(CPK_SIGNER_INFO)
IMPLEMENT_ASN1_FUNCTIONS(CPK_SIGNER_INFO)
IMPLEMENT_ASN1_DUP_FUNCTION(CPK_SIGNER_INFO)


ASN1_NDEF_SEQUENCE(CPK_ENVELOPE) = {
	ASN1_SIMPLE(CPK_ENVELOPE, version, LONG),
	ASN1_SET_OF(CPK_ENVELOPE, recip_infos, CPK_RECIP_INFO),
	ASN1_SIMPLE(CPK_ENVELOPE, enc_data, CPK_ENC_CONTENT)
} ASN1_NDEF_SEQUENCE_END(CPK_ENVELOPE)
IMPLEMENT_ASN1_FUNCTIONS(CPK_ENVELOPE)


ASN1_SEQUENCE(CPK_RECIP_INFO) = {
	ASN1_SIMPLE(CPK_RECIP_INFO, version, LONG),
	ASN1_SIMPLE(CPK_RECIP_INFO, recipient, X509_NAME),
	ASN1_SIMPLE(CPK_RECIP_INFO, enc_algor, X509_ALGOR),
	ASN1_SIMPLE(CPK_RECIP_INFO, enc_data, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(CPK_RECIP_INFO)
IMPLEMENT_ASN1_FUNCTIONS(CPK_RECIP_INFO)
IMPLEMENT_ASN1_DUP_FUNCTION(CPK_RECIP_INFO)


ASN1_NDEF_SEQUENCE(CPK_ENC_CONTENT) = {
	ASN1_SIMPLE(CPK_ENC_CONTENT, content_type, ASN1_OBJECT),
	ASN1_SIMPLE(CPK_ENC_CONTENT, enc_algor, X509_ALGOR),
	ASN1_IMP_OPT(CPK_ENC_CONTENT, enc_data, ASN1_OCTET_STRING, 0)
} ASN1_NDEF_SEQUENCE_END(CPK_ENC_CONTENT)
IMPLEMENT_ASN1_FUNCTIONS(CPK_ENC_CONTENT)


ASN1_NDEF_SEQUENCE(CPK_SIGN_ENVELOPE) = {
	ASN1_SIMPLE(CPK_SIGN_ENVELOPE, version, LONG),
	ASN1_SET_OF(CPK_SIGN_ENVELOPE, recip_infos, CPK_RECIP_INFO),
	ASN1_SET_OF(CPK_SIGN_ENVELOPE, digest_algors, X509_ALGOR),
	ASN1_SIMPLE(CPK_SIGN_ENVELOPE, enc_data, CPK_ENC_CONTENT),
	ASN1_IMP_SET_OF_OPT(CPK_SIGN_ENVELOPE, cert, X509, 0),
	ASN1_IMP_SET_OF_OPT(CPK_SIGN_ENVELOPE, crl, X509_CRL, 1),
	ASN1_SET_OF(CPK_SIGN_ENVELOPE, signer_infos, CPK_SIGNER_INFO)
} ASN1_NDEF_SEQUENCE_END(CPK_SIGN_ENVELOPE)
IMPLEMENT_ASN1_FUNCTIONS(CPK_SIGN_ENVELOPE)

