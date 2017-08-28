#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <syslog.h>

#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>

// NOTE. keep following order...
#include <trousers/tss.h>
#include <trousers/trousers.h>

#include "pca_utils.h"
/* for debugging */
#include "debug_log.h"
#include "debug_tpm.h"


//#define	HOMEDIR	"htdocs/pca/"
#define	HOMEDIR	"./"

int do_dump_file = 0; /* skip dump_file */
//int debug_level = DLOG_FATAL; /* default: no message */


/* clear workplace */
void do_clear(void)
{
	syslog(LOG_DEBUG, "Signing AIK finished\n");
	closelog();
	fini_dlog();
}


/**
 * @brief parsing and storing identity request from given data
 * @param buf byte arrary containing identify request
 * @param pReq pointer to store identity request
 * @return 0 on success, otherwise -1
 */
int parse_identity_request(char *buf, TPM_IDENTITY_REQ *pReq)
{
	int ecode = 0; /* default: OK */
	UINT64 offset = 0;
	TSS_RESULT result;

	dlog(DLOG_DEBUG, "before Trspi_UnloadBlob_IDENTITY_REQ: offset <%ju>\n", offset);
	result = Trspi_UnloadBlob_IDENTITY_REQ(&offset, (BYTE *)buf, pReq);
	dlog(DLOG_DEBUG, "after Trspi_UnloadBlob_IDENTITY_REQ: offset <%ju>\n", offset);

	if (TPM_ERROR(result)) {
		dlog(DLOG_ERROR, "error in Trspi_UnloadBlob_IDENTITY_REQ: %s\n", Trspi_Error_String(result));
		ecode = -1; /* error */
	}

	return ecode;
}


/* Load private key from Key(PEM) file
 * Load public key from Cert(PEM) file
 * NOTE. Refer BUGS of pem(3SSL)
 */
RSA *load_RSA_key_PEM(char *kfname, int is_private)
{
	FILE *rfp = NULL;
	RSA *pkey = NULL;
	X509 *x509 = NULL;

	rfp = fopen(kfname, "r");
	if (rfp == NULL) {
		dlog(DLOG_ERROR, "fail to fopen(%s): %m\n", kfname);
		return NULL;
	}

	if (is_private) {
		pkey = PEM_read_RSAPrivateKey(rfp, NULL, NULL, NULL);
	} else {
		/* public key should be extract from certificate */
		x509 = PEM_read_X509(rfp, NULL, NULL, NULL);
		if (x509 == NULL) {
			dlog(DLOG_ERROR, "fail to read Cert File\n");
		} else {
			EVP_PKEY *pcaKey = X509_get_pubkey(x509);
			if (pcaKey == NULL) {
				dlog(DLOG_ERROR, "fail to get public key\n");
			} else {
				pkey = EVP_PKEY_get1_RSA(pcaKey);
			}
			X509_free(x509);
		}
	}

	if (pkey == NULL) {
		dlog(DLOG_ERROR, "fail to load RSA %s Key File\n", is_private ? "Private" : "Public");
		ERR_print_errors_dlog();
	}

	fclose(rfp);

	return pkey;
}


#define	CHECK_EXIPRATION
#ifdef	CHECK_EXIPRATION
/*
 * return -1 on error
 * NOTE - remove this and use ASN1_TIME_diff, for openssl 1.0.2 +
 */
time_t ASN1_TIME_to_timeT(ASN1_TIME *atime)
{
	char tbuf[128]; /* debug */
	char *tstr;
	int i;
	struct tm t;

	if ((NULL == atime->data) || (13 > atime->length)) {
		dlog(DLOG_ERROR, "<%s> invalid data<%p> or length<%d>\n", __func__, atime->data, atime->length);
		return -1;
	}

	/* format: 171115135628Z or 20171115135628 */
	tstr = (char *)atime->data;
	i = 0; /* mark as start */

	if (atime->type == V_ASN1_UTCTIME) { /* two digit year */
		t.tm_year = (tstr[i++] - '0') * 10;
		t.tm_year += (tstr[i++] - '0');
		if (t.tm_year < 70)
			t.tm_year += 100;
	} else if (atime->type == V_ASN1_GENERALIZEDTIME) { /* four digit year */
		t.tm_year = (tstr[i++] - '0') * 1000;
		t.tm_year += (tstr[i++] - '0') * 100;
		t.tm_year += (tstr[i++] - '0') * 10;
		t.tm_year += (tstr[i++] - '0');
		t.tm_year -= 1900;
	} else {
		dlog(DLOG_ERROR, "invalid ASN1_TIME.type <%x>\n", atime->type);
		return -1;
	}

	t.tm_mon = (tstr[i++] - '0') * 10;
	t.tm_mon += (tstr[i++] - '0') - 1; /* NOTE - Jan is 0 not 1 */
	t.tm_mday = (tstr[i++] - '0') * 10;
	t.tm_mday += (tstr[i++] - '0');
	t.tm_hour = (tstr[i++] - '0') * 10;
	t.tm_hour += (tstr[i++] - '0');
	t.tm_min = (tstr[i++] - '0') * 10;
	t.tm_min += (tstr[i++] - '0');
	t.tm_sec = (tstr[i++] - '0') * 10;
	t.tm_sec += (tstr[i++] - '0');

	return mktime(&t);
}
#endif // CHECK_EXIPRATION

/* move to config -S- */
long valid_days = 365; /* default for AIK (of PoC) */
char *pca_key_file = NULL; /* private key of PCA */
char *pca_cert_file = NULL; /* cert of PCA */
/* move to config -F- */

/**
 * @brief make a cert contains AIK public key, and sign with PCA's private key
 *  - NOTE: Certificate's default format is DER
 * @param proof pointer to identity proof
 * @return pointer of signed cert on success, otherwise NULL
 */
X509 *make_sign_cert(TPM_IDENTITY_PROOF *proof)
{
	X509 *newcert = NULL, *cacert = NULL;
	EVP_PKEY *ca_privkey = NULL, *aik_pubkey = NULL;
	ASN1_INTEGER *serial_asn = NULL;
	X509_NAME *name = NULL;
	FILE *rfp = NULL;
	long tlong;
	int tlen, ok = 0; /* default: failed */
	RSA *rsa = NULL;
	unsigned char exp_def[3] = { 0x01, 0x00, 0x01 }; // 65537

	/* load signing CA Certificate */
	if (pca_cert_file) {
		rfp = fopen(pca_cert_file, "r");
		if (rfp == NULL) {
			dlog(DLOG_ERROR, "fail to fopen(%s): %m\n", pca_cert_file);
			goto err_out;
		}
	} else {
		dlog(DLOG_ERROR, "No PCA cert file\n");
		goto err_out;
	}

	cacert = PEM_read_X509(rfp, NULL, NULL, NULL);
	if (cacert == NULL) {
		dlog(DLOG_ERROR, "fail to PEM_read_X509\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	fclose(rfp); rfp = NULL;

	/* load CA private key */
	if (pca_key_file) {
		rfp = fopen(pca_key_file, "r");
		if (rfp == NULL) {
			dlog(DLOG_ERROR, "fail to fopen(%s): %m\n", pca_key_file);
			goto err_out;
		}
	} else {
		dlog(DLOG_ERROR, "No PCA Key file\n");
		goto err_out;
	}

	ca_privkey = PEM_read_PrivateKey(rfp, NULL, NULL, NULL);
	if (ca_privkey == NULL) {
		dlog(DLOG_ERROR, "fail to PEM_read_PrivateKey\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	fclose(rfp); rfp = NULL;

	/* create Certificate for AIK
	 * Set { Subject/Issuer - privacy CA // PubKey - AIK // Serial - New }
	 * NOTE. No Hint for the relation between AIK and TPM
	 */
	newcert = X509_new();
	if (newcert == NULL) {
		dlog(DLOG_ERROR, "fail to X509_new\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* NOTE. 2 - version 3, 0 - version 1
	 * Version 3 is set for using extension
	 */
	if (X509_set_version(newcert, 2) != 1) {
		dlog(DLOG_ERROR, "fail to X509_set_version(v3)\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* serial: default is 0 */
	serial_asn = M_ASN1_INTEGER_new();
	// return 1 on success - fail only when memory error
	tlong = random();
	ASN1_INTEGER_set(serial_asn, tlong);
	// return 1 on success
	if (X509_set_serialNumber(newcert, serial_asn) == 0) {
		dlog(DLOG_ERROR, "fail to X509_set_serialNumber(%ld)\n", tlong);
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* NOTE - No Hint for the relation between AIK and TPM
	 * -> use info. of CA's instead of that of TPM's Request
	 */

	/* get subject from cacert, set it as issuer of newcert */
	name = X509_get_subject_name(cacert);
	if (name == NULL) {
		dlog(DLOG_ERROR, "fail to get subject name from CA's Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	if (X509_set_issuer_name(newcert, name) != 1) {
		dlog(DLOG_ERROR, "fail to set issuer name to new Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	//free(name); // don't do this, otherwise [newcert] would be corrupted

	/* NOTE - subject name of newcert should be empty */

	/* get AIKpub from IdentityProof, set it as pubkey of newcert */
	dlog(DLOG_DEBUG, "Dump pubKey\n");
	hex_dump_dlog(DLOG_DEBUG, (char *)proof->identityKey.pubKey.key, proof->identityKey.pubKey.keyLength);

	rsa = RSA_new();
	/* NOTE - rsa->n/e should be updated with result */
	rsa->n = BN_bin2bn(proof->identityKey.pubKey.key, proof->identityKey.pubKey.keyLength, rsa->n);
	rsa->e = BN_bin2bn(exp_def, sizeof(exp_def), rsa->e);

	aik_pubkey = EVP_PKEY_new();
	if (aik_pubkey == NULL) {
		dlog(DLOG_ERROR, "fail to get new EVP_PKEY for public key (AIKpub)\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	if (EVP_PKEY_set1_RSA(aik_pubkey, rsa) == 0) {
		dlog(DLOG_ERROR, "fail to get set public key (AIKpub) to EVP_PKEY\n");
		ERR_print_errors_dlog();
		//RSA_free(rsa);
		goto err_out;
	}

	if (X509_set_pubkey(newcert, aik_pubkey) != 1) {
		dlog(DLOG_ERROR, "fail to set public key (AIKpub) to new Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* adjust time of start and expire */
	if (X509_gmtime_adj(X509_get_notBefore(newcert), 0) == 0) {
		dlog(DLOG_ERROR, "fail to set start time to new Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	tlong = valid_days * 24 * 60 * 60; /* default, in seconds */
#ifdef	CHECK_EXIPRATION
	// use ASN1_TIME_diff, for openssl 1.0.2 +
	time_t nb_t = ASN1_TIME_to_timeT(X509_get_notBefore(newcert));
	time_t ca_t = ASN1_TIME_to_timeT(X509_get_notAfter(cacert));

	if (nb_t + tlong > ca_t) {
		tlong = ca_t - nb_t;
	}
#endif // CHECK_EXIPRATION

	if (X509_gmtime_adj(X509_get_notAfter(newcert), tlong) == 0) {
		dlog(DLOG_ERROR, "fail to set expire time to new Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* sign with issuer(CA) private key and SHA1 */
	tlen = X509_sign(newcert, ca_privkey, EVP_sha1());
	dlog(DLOG_DEBUG, "result of X509_sign <%d>\n", tlen); /* debug */

	if (tlen == 0) {
		dlog(DLOG_ERROR, "fail to sign new Cert\n");
		ERR_print_errors_dlog();
		goto err_out;
	}

	/* now all is done */
	ok = 1; /* Success */

	/* debug: dump newcert into file */
	X509_print_dlog(DLOG_DEBUG, newcert);

	if (do_dump_file) {
#define	AIK_CERT_FILE	HOMEDIR"output/aik_sign.crt"
		rfp = fopen(AIK_CERT_FILE, "w"); // reuse rfp for writing
		if (rfp) {
			PEM_write_X509(rfp, newcert);
			fclose(rfp); rfp = NULL;
		} else {
			dlog(DLOG_ERROR, "fail to fopen(%s): %m\n", AIK_CERT_FILE);
		}
	}

err_out:
	if (rfp) fclose(rfp);
	if (cacert) X509_free(cacert);
	if (ca_privkey) EVP_PKEY_free(ca_privkey);
	if (aik_pubkey) EVP_PKEY_free(aik_pubkey);
	if (!ok && newcert) {
		X509_free(newcert);
		newcert = NULL;
	}

	return newcert;
}


/**
 * @brief build Asym part and Sym part, and write them to stdout
 *  - Asym part contains the key for encrypted part of Sym
 *  - Asym part is encrypted by EKpub of TPM
 * @param aik_cert cert for AIKpub signed with PCApriv
 * @param proof pointer to identity proof data
 * @return 0 on success, otherwise -1
 */
int pack_response(X509 *aik_cert, TPM_IDENTITY_PROOF *proof)
{
	BYTE		key[128/8];
	BYTE		iv[16];
	BYTE		asymPlain[8 + sizeof(key) + SHA_DIGEST_LENGTH];
	unsigned char oaepPad[4] = "TCPA";
	BYTE		*asymPadded;
	UINT32		asymPaddedLength;
	BYTE		*asymEnc;
	UINT32		asymEncLength;
	BYTE		*credEnc = NULL;
	UINT32		credEncLength;
	BYTE		*symAttest;
	UINT32		symAttestLength;
	EVP_CIPHER_CTX ctx;
	X509		*ekX509;
	RSA			*ekRsa;
	int			trousersIVMode = 1; /* NOTE */
	int			out1, out2;
	unsigned char *cred_plain = NULL;
	int cplen = 0;
	unsigned char *end_cred = NULL;

	if ((aik_cert == NULL) || (proof == NULL)) {
		dlog(DLOG_ERROR, "Invalid aik_cert<%p> or proof<%p>\n", aik_cert, proof);
		return -1;
	}

	/* load certificate for EK */
	end_cred = (unsigned char *)proof->endorsementCredential;
	/* NOTE - [end_cred] would be updated */
	ekX509 = d2i_X509(NULL, (unsigned char const **)&end_cred, proof->endorsementSize);
	if (ekX509 == NULL) {
		dlog(DLOG_ERROR, "fail to d2i_X509 for proof<%p>\n", proof);
		ERR_print_errors_dlog();
		return -1;
	}

	if (do_dump_file) {
#define	EKCERTFILE	HOMEDIR"output/ekcert.der"
		dump_file((char *)proof->endorsementCredential, proof->endorsementSize, EKCERTFILE);
	}

	/* OpenSSL can't parse EK key due to OAEP OID - fix it */
	{
		X509_PUBKEY *pk = X509_get_X509_PUBKEY(ekX509);
		int algbufLen = i2d_X509_ALGOR(pk->algor, NULL);
		unsigned char *algbuf = malloc(algbufLen);
		unsigned char *algbufPtr = algbuf;
		i2d_X509_ALGOR(pk->algor, &algbufPtr);
		if (algbuf[12] == 7)
			algbuf[12] = 1;
		algbufPtr = algbuf;
		d2i_X509_ALGOR(&pk->algor, (void *)&algbufPtr, algbufLen);
		free(algbuf);
	}

	/* get RSA public key for cert. of EK */
	{
		EVP_PKEY	*ekPkey = NULL;
		if ((ekPkey = X509_get_pubkey(ekX509)) == NULL) {
			dlog(DLOG_ERROR, "fail to X509_get_pubkey(ekX509)\n");
			ERR_print_errors_dlog();
			return -1;
		}

		if ((ekRsa = EVP_PKEY_get1_RSA(ekPkey)) == NULL) {
			dlog(DLOG_ERROR, "fail to EVP_PKEY_get1_RSA(ekPkey)\n");
			ERR_print_errors_dlog();
			return -1;
		}
	}

	// NOTE - for variable length... refer d2i_X509(3SSL)
#if OPENSSL_VERSION_NUMBER < 0x0090700fL
#error "check the version of OpenSSL (0.9.7 or later)"
#endif // OPENSSL_VERSION_NUMBER

	cred_plain = NULL; /* redundent */
	cplen = i2d_X509(aik_cert, &cred_plain);
	dlog(DLOG_DEBUG, "i2d_X509_bio(aik_cert) = cplen<%d>cred_plain<%p>\n", cplen, cred_plain);

	/* NOTE - [cred_plain] is allocated (0.9.7 or later) */
	if (do_dump_file) {
#define	AIKCERTFILE	HOMEDIR"output/aik_cert.der"
		dump_file((char *)cred_plain, cplen, AIKCERTFILE); // debug
	}

	/* Construct encrypted output challenge */
	RAND_bytes(key, sizeof(key));
	RAND_bytes(iv, sizeof(iv));

	/* Encrypt credential (for SYM part) with key */
	credEnc = malloc(cplen + sizeof(iv));
	EVP_CIPHER_CTX_init(&ctx);
	EVP_EncryptInit(&ctx, EVP_aes_128_cbc(), key, iv); /* NOTE */
	EVP_EncryptUpdate(&ctx, credEnc, &out1, cred_plain, cplen);
	EVP_EncryptFinal_ex(&ctx, credEnc+out1, &out2);
	EVP_CIPHER_CTX_cleanup(&ctx);
	credEncLength = out1 + out2;

	dlog(DLOG_DEBUG, "After encryption: credEncSize<%zu>credEncLength<%u>\n", cplen + sizeof(iv), credEncLength);

	/* Create TPM_SYM_CA_ATTESTATION struct to hold encrypted cert */
	symAttestLength = 28 + sizeof(iv) + credEncLength;
	symAttest = malloc(symAttestLength);
	((UINT32 *)symAttest)[0] = htonl(credEncLength);
	((UINT32 *)symAttest)[1] = htonl(TPM_ALG_AES);
	((UINT16 *)symAttest)[4] = htons(TPM_ES_SYM_CBC_PKCS5PAD);
	((UINT16 *)symAttest)[5] = htons(TPM_SS_NONE);
	((UINT32 *)symAttest)[3] = htonl(12+sizeof(iv));
	((UINT32 *)symAttest)[4] = htonl(128);		/* Key length in bits */
	((UINT32 *)symAttest)[5] = htonl(sizeof(iv));	/* Block size in bytes */
	((UINT32 *)symAttest)[6] = htonl(sizeof(iv));	/* IV size in bytes */
	memcpy(symAttest+28, iv, sizeof(iv));
	memcpy(symAttest+28+sizeof(iv), credEnc, credEncLength);
	if (trousersIVMode) {
		((UINT32 *)symAttest)[0] = htonl(credEncLength + sizeof(iv));
		((UINT32 *)symAttest)[3] = htonl(12);	/* Take IV to be start of credEnc */
		((UINT32 *)symAttest)[6] = htonl(0);	/* IV size in bytes */
	}

	/* Prepare buffer to be RSA encrypted to EK */
	((UINT32 *)asymPlain)[0] = htonl(TPM_ALG_AES);
	((UINT16 *)asymPlain)[2] = htons(TPM_ES_SYM_CBC_PKCS5PAD);
	((UINT16 *)asymPlain)[3] = htons(sizeof(key));
	memcpy(asymPlain+8, key, sizeof(key));
	// NOTE: idDigest for AIK (in plain)
	{
		BYTE *aik = NULL, *aik2 = NULL;
		int alen = 0;
		TPM_PUBKEY *pPub = NULL;

		pPub = (TPM_PUBKEY *)&(proof->identityKey);
		alen = 8 + 4 + pPub->algorithmParms.parmSize; /* algorithmParms */
		alen += 4 + pPub->pubKey.keyLength; /* pubKey */
		dlog(DLOG_DEBUG, "[AIK] TPM_PUBKEY<%d>\n", alen);
		aik = (BYTE *)malloc(alen);
		if (aik == NULL) {
			dlog(DLOG_ERROR, "[AIK] malloc failed: %m\n");
			return -1; // CHECK - add error-handler?
		}
		((UINT32 *)aik)[0] = htonl(pPub->algorithmParms.algorithmID);
		((UINT16 *)aik)[2] = htons(pPub->algorithmParms.encScheme);
		((UINT16 *)aik)[3] = htons(pPub->algorithmParms.sigScheme);
		((UINT32 *)aik)[2] = htonl(pPub->algorithmParms.parmSize);
		memcpy(aik+12, pPub->algorithmParms.parms, pPub->algorithmParms.parmSize);
		aik2 = aik + 12 + pPub->algorithmParms.parmSize;
		((UINT32 *)aik2)[0] = htonl(pPub->pubKey.keyLength);
		memcpy(aik2+4, pPub->pubKey.key, pPub->pubKey.keyLength);
		SHA1(aik, alen, asymPlain + 8 + sizeof(key));
	}
	free(credEnc);
	credEnc = NULL;

	/* Encrypt with EKpub */
	/* Must use custom padding for TPM to decrypt it */
	asymPaddedLength = asymEncLength = RSA_size(ekRsa);
	asymPadded = malloc(asymPaddedLength);
	asymEnc = malloc(asymEncLength);
	RSA_padding_add_PKCS1_OAEP(asymPadded, asymPaddedLength, asymPlain,
					sizeof(asymPlain), oaepPad, sizeof(oaepPad));
	RSA_public_encrypt(asymPaddedLength, asymPadded, asymEnc, ekRsa, RSA_NO_PADDING);
	free(asymPadded);
	asymPadded = NULL;

	// write result as output of CGI
	dlog(DLOG_DEBUG, "print out headers\n");
	fprintf(stdout, "Content-Type: application/octet-stream\r\n");
	fprintf(stdout, "Content-Transfer-Encoding: binary\r\n");
	fprintf(stdout, "Content-Length: %u\r\n", asymEncLength + symAttestLength);
	fprintf(stdout, "Expect: 100-continue\r\n\r\n");

	/* ASYM part(256 bytes) + SYM part(total_length - 256) */
	if (fwrite(asymEnc, 1, asymEncLength, stdout) != asymEncLength) {
		dlog(DLOG_ERROR, "fail to write ASYM part to stdio: %m\n");
		return -1; /* CHECK - clear something? */
	}
	if (fwrite(symAttest, 1, symAttestLength, stdout) != symAttestLength) {
		dlog(DLOG_ERROR, "fail to write SYM part to stdio: %m\n");
		return -1; /* CHECK - clear something? */
	}

	if (do_dump_file) {
#define	ASYM_SENT	HOMEDIR"output/asym.sent"
		dump_file((char *)asymEnc, asymEncLength, ASYM_SENT);
#define	SYM_SENT	HOMEDIR"output/sym.sent"
		dump_file((char *)symAttest, symAttestLength, SYM_SENT);
	}

	free(symAttest);

	dlog(DLOG_DEBUG, "Success!\n");
	dlog(DLOG_DEBUG, "Total <%u/%u> written\n", asymEncLength, symAttestLength);

	return 0;
}


/* read and parse config's from environment given by web server
 * (see /etc/apache2/sites-enabled/pca.conf)
 * - PCA_CERT / PCA_KEY / VALID_TERM - for management
 * - DEBUG_MODE / DEBUG_DUMP - for debugging
 * return 0 on success
 */
int conf_from_env(void)
{
	char *pch = NULL;

	// NOTE - fprintf(stderr, ...) writes mesg to error log of httpd
	pch = getenv("DEBUG_MODE");
	fprintf(stderr, "DEBUG_MODE<%s>\n", pch ? pch : "NONE");

	if ((NULL == pch) || !strcasecmp("release", pch)) {
		/* only default log */
		init_dlog(NULL);
		set_level_dlog(NULL, DLOG_ERROR);
	} else {
#define	OUT_DIR	HOMEDIR"output"
		if (mkdir(OUT_DIR, 0755) && (errno != EEXIST)) {
			fprintf(stderr, "fail to mkdir(%s): %m\n", OUT_DIR);
			return -1;
		}

#define	LOG_FNAME	HOMEDIR"output/do_parse_sign.log"
		init_dlog(LOG_FNAME);
		/* [DLOG_NOTI] is default for non-release */
		set_level_dlog(pch, DLOG_NOTI);
	}

	pch = getenv("DEBUG_DUMP");
	dlog(DLOG_DEBUG, "DEBUG_DUMP<%s>\n", pch ? pch : "NONE");

	if (pch && strcasecmp("no", pch) && strcasecmp("none", pch)) {
		do_dump_file = 1; /* Do dump_file */
	} else {
		do_dump_file = 0; /* skip dump_file */
	}
	dlog(DLOG_DEBUG, "do_dump_file<%d>\n", do_dump_file);

#define	PCA_KEY_FILE	HOMEDIR"key/pca.key"	/* private - default */
#define	PCA_CERT_FILE	HOMEDIR"cert/pca.crt"	/* cert - default */
	pch = getenv("PCA_CERT");
	dlog(DLOG_DEBUG, "PCA_CERT<%s>\n", pch ? pch : "NONE");

	if (pca_cert_file) {
		free(pca_cert_file);
		pca_cert_file = NULL;
	}
	if (pch) {
		pca_cert_file = strdup(pch);
	} else {
		pca_cert_file = strdup(PCA_CERT_FILE);
	}
	dlog(DLOG_DEBUG, "pca_cert_file<%s>\n", pca_cert_file ? pca_cert_file : "NONE");

	pch = getenv("PCA_KEY");
	dlog(DLOG_DEBUG, "PCA_KEY<%s>\n", pch ? pch : "NONE");

	if (pca_key_file) {
		free(pca_key_file);
		pca_key_file = NULL;
	}
	if (pch) {
		pca_key_file = strdup(pch);
	} else {
		pca_key_file = strdup(PCA_KEY_FILE);
	}
	dlog(DLOG_DEBUG, "pca_key_file<%s>\n", pca_key_file ? pca_key_file : "NONE");

	pch = getenv("VALID_DAYS");
	dlog(DLOG_DEBUG, "VALID_DAYS<%s>\n", pch ? pch : "NONE");

	if (pch) {
		valid_days = atoi(pch);
	} else {
		valid_days = 365; /* default: 1-year */
	}
	dlog(DLOG_DEBUG, "valid_days<%ld>\n", valid_days);

	return 0; /* OK */
}


char mpbuf[2048]; // multi-purpose buffer

#define	MAX_CLEN	4096

/**
 * @brief main of CGI program
 * - read and parse data containing identity request from stdin
 * - sign AIK, and make response data
 * - write to stdout
 */
int main(int argc, char *argv[])
{
	int ret = -1;
	char *pch = NULL, *pir = NULL;
	TPM_IDENTITY_REQ req_parsed;
	TSS_RESULT result;
	const unsigned int kDefaultTpmRsaKeyBits = 2048;
	unsigned char key_buffer[kDefaultTpmRsaKeyBits / 8];
	int clen = 0, len = 0;
	TPM_SYMMETRIC_KEY symmetric_key;
	UINT64 offset = 0;
	TPM_IDENTITY_PROOF proof;
	X509 *sign_cert = NULL;
	RSA *pca_prikey = NULL;
	unsigned char *cred_plain = NULL;

	openlog("PCA", LOG_PID, LOG_USER);
	syslog(LOG_DEBUG, "Signing AIK started\n");

	// read and parse config's from environment given by web server
	if (conf_from_env()) {
		//fprintf(stderr, "fail to setup config\n");
		syslog(LOG_ERR, "fail to setup config\n");
		return -1;
	}

	// get size of POST data
	//putenv("CONTENT_LENGTH=0"); // for testing
	pch = getenv("CONTENT_LENGTH");
	if ((pch == NULL) || (sscanf(pch, "%d", &clen) != 1) || (clen > MAX_CLEN)) {
		dlog(DLOG_ERROR, "Invalid Content: %p %d\n", pch, clen);
		syslog(LOG_ERR, "Invalid Content: %p %d\n", pch, clen);
		do_clear();
		exit(1);
	}

	dlog(DLOG_DEBUG, "Content Length - STR <%s/%p>\n", pch, pch);
	dlog(DLOG_DEBUG, "Content Length - LEN <%d>\n", clen);

	// read in POST data
	pir = get_content(clen, stdin);
	dlog(DLOG_DEBUG, "Post Data <%p>\n", pir);

	if (NULL == pir) {
		dlog(DLOG_ERROR, "Fail to get content\n");
		syslog(LOG_ERR, "Fail to get content\n");
		do_clear();
		exit(1);
	}

	/* dump post field (original input data) */
	if (do_dump_file) {
#define	DUMP_PIR	HOMEDIR"output/post_it"
		len = dump_file(pir, clen, DUMP_PIR);
		dlog(DLOG_DEBUG, "Dump <%d> byte%s\n", len, len > 1 ? "s" : "");
	}

	/* parsing identiy request */
	if (parse_identity_request(pir, &req_parsed)) {
		dlog(DLOG_ERROR, "error in parse_identity_request\n");
		syslog(LOG_ERR, "error in parse_identity_request\n");
		goto err_out;
	}

	show_TPM_IDENTITY_REQ(&req_parsed, DLOG_DEBUG); /* debug */

	srandom(time(NULL)); /* NOTE */

	/* load Private Key of CA from Key(PEM) file */
	pca_prikey = load_RSA_key_PEM(pca_key_file, 1);
	if (pca_prikey == NULL) {
		dlog(DLOG_ERROR, "error in load_RSA_key_PEM(%s, 1)\n", pca_key_file);
		syslog(LOG_ERR, "error in load_RSA_key_PEM(%s, 1)\n", pca_key_file);
		goto err_out;
	}

	dlog(DLOG_DEBUG, "Dump caPrivKey(%p)--------------------------\n", pca_prikey);
	RSA_print_dlog(DLOG_DEBUG, pca_prikey, 8); /* debug: heading 8 spaces */
	dlog(DLOG_DEBUG, "-------------------------------------------------\n");

	/* Decrypt the symmetric key */
	len = RSA_private_decrypt(req_parsed.asymSize, req_parsed.asymBlob, key_buffer, pca_prikey, RSA_PKCS1_PADDING);
	if (len < 0) {
		dlog(DLOG_ERROR, "Failed to decrypt identity request key.\n");
		syslog(LOG_ERR, "Failed to decrypt identity request key.\n");
		goto err_out;
	}

	/* convert the symmetric key into TPM_SYMMETRIC_KEY */
	result = Trspi_UnloadBlob_SYMMETRIC_KEY(&offset, key_buffer, &symmetric_key);
	dlog(DLOG_DEBUG, "after decrypting symmetric key: offset<%ju>\n", offset);
	if (TPM_ERROR(result)) {
		dlog(DLOG_ERROR, "Failed to parse symmetric key.\n");
		syslog(LOG_ERR, "Failed to parse symmetric key.\n");
		goto err_out;
	}

	show_TPM_SYMMETRIC_KEY(&symmetric_key, DLOG_DEBUG); // debug

	len = 0; // for credential_len (reusing variable)
	/* Decrypt the request (Sym part) with the symmetric key */
	result = Trspi_SymDecrypt(symmetric_key.algId, TR_SYM_MODE_CBC,
								symmetric_key.data, NULL,
								req_parsed.symBlob, req_parsed.symSize,
								(BYTE *)mpbuf, (UINT32 *)&len);
	dlog(DLOG_DEBUG, "after decrypting request(Sym): credential_len<%u>\n", (UINT32)len);
	if (TPM_ERROR(result)) {
		dlog(DLOG_ERROR, "Failed to decrypt request with symmetric key.\n");
		syslog(LOG_ERR, "Failed to decrypt request with symmetric key.\n");
		goto err_out;
	}

	/* dump credential data decrypted */
	if (do_dump_file) {
#define	REQ_CLEAN	HOMEDIR"output/request.clean"
		if (dump_file(mpbuf, len, REQ_CLEAN) < len) {
			dlog(DLOG_ERROR, "error in dump_file\n");
			goto err_out;
		}
	}

	/* Parse the serialized TPM_IDENTITY_PROOF structure */
	offset = 0;
	result = Trspi_UnloadBlob_IDENTITY_PROOF(&offset, (BYTE *)mpbuf, &proof);
	dlog(DLOG_DEBUG, "after parsing proof: offset<%ju>\n", offset);
	if (TPM_ERROR(result)) {
		dlog(DLOG_ERROR, "Failed to parse proof.\n");
		syslog(LOG_ERR, "Failed to parse proof.\n");
		goto err_out;
	}

	show_TPM_IDENTITY_PROOF(&proof, DLOG_DEBUG); // debug

	/* dump credential data parsed */
	if (do_dump_file) {
#define	DUMP_EKCRT	HOMEDIR"output/cred.out"
		FILE *oo = fopen(DUMP_EKCRT, "wb");
		int ooo = fwrite(proof.endorsementCredential, 1, proof.endorsementSize, oo);
		if (ooo != proof.endorsementSize) {
			dlog(DLOG_ERROR, "error in fwrite <%d/%d>: %m\n", ooo, proof.endorsementSize);
		}
		fclose(oo);
	}

	/* dump public key of PCA into log file */
	if (DLOG_DEBUG >= get_level_dlog()) {
		RSA *pca_pubkey = NULL;

		/* load Public Key of CA from Cert(PEM) file */
		pca_pubkey = load_RSA_key_PEM(pca_cert_file, 0);
		if (pca_pubkey == NULL) {
			dlog(DLOG_ERROR, "error in load_RSA_key_PEM(%s, 0)\n", pca_cert_file);
		} else {
			dlog(DLOG_DEBUG, "Dump caPubKey(%p)-----------------------\n", pca_pubkey);
			RSA_print_dlog(DLOG_DEBUG, pca_pubkey, 8); /* debug: heading 8 spaces */
			dlog(DLOG_DEBUG, "-------------------------------------------------\n");
		}

		RSA_free(pca_pubkey);
	}

	// NOW Make Response with parsed data
	// Skip following routines - it's just TOY-system
	// FIXME routines for checking validity of credential should be added here
	// FIXME routines for querying DB should be added here

	// [identity.c] expects...
	// [asymBuf(ASYM_CA_CONTENTS): 2048/8 bytes] + [symBuf(SYM_CA_ATTESTATION): total - 2048/8 bytes]

	// make cert(DER): TPM_IDENTITY_PROOF + caPrivKey + caPubCert + validityDays + 0(level)
	sign_cert = make_sign_cert(&proof);

	if (sign_cert == NULL) {
		dlog(DLOG_ERROR, "fail to make Cert for signing\n");
		syslog(LOG_ERR, "fail to make Cert for signing\n");
		goto err_out;
	}

	if (pack_response(sign_cert, &proof)) {
		dlog(DLOG_ERROR, "fail to packing response with signed result\n");
		syslog(LOG_ERR, "fail to packing response with signed result\n");
		goto err_out;
	}

	/* destroy TPM_IDENTITY_REQ */

	ret = 0; /* OK */

err_out:
	if (pca_prikey) {
		RSA_free(pca_prikey); /* CHECK HERE */
	}

	if (pir) {
		free(pir);
	}

	if (sign_cert) {
		X509_free(sign_cert);
	}

	if (cred_plain) {
		free(cred_plain);
	}

	do_clear();

	return ret;
}
