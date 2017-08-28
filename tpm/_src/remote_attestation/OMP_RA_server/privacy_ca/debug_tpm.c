#include <stdio.h>
#include <trousers/tss.h>

#include "debug_log.h"


void show_TPM_IDENTITY_REQ(TPM_IDENTITY_REQ *pReq, int level)
{
	int i, imax;

	dlog(level, "Dump TPM_IDENTITY_REQ(%p)----------------------\n", pReq);
	if (pReq == NULL) return;

	dlog(level, "asymSize: %u\n", pReq->asymSize);
	dlog(level, "symSize: %u\n", pReq->symSize);

	dlog(level, "asymAlgorithm\n");
	dlog(level, "   algorithmID: %04x\n", pReq->asymAlgorithm.algorithmID);
	dlog(level, "   encScheme: %02x\n", pReq->asymAlgorithm.encScheme);
	dlog(level, "   sigScheme: %02x\n", pReq->asymAlgorithm.sigScheme);
	dlog(level, "   parmSize: %u\n", pReq->asymAlgorithm.parmSize);
	dlog(level, "   parms<%p> = ", pReq->asymAlgorithm.parms);
	imax = pReq->asymAlgorithm.parmSize;
	for (i = 0; i < imax; i++) {
		dlog(level, "%02x ", pReq->asymAlgorithm.parms[i]);
	}
	dlog(level, "\n");

	dlog(level, "symAlgorithm\n");
	dlog(level, "   algorithmID: %04x\n", pReq->symAlgorithm.algorithmID);
	dlog(level, "   encScheme: %02x\n", pReq->symAlgorithm.encScheme);
	dlog(level, "   sigScheme: %02x\n", pReq->symAlgorithm.sigScheme);
	dlog(level, "   parmSize: %u\n", pReq->symAlgorithm.parmSize);
	dlog(level, "   parms<%p> = ", pReq->symAlgorithm.parms);
	imax = pReq->symAlgorithm.parmSize;
	for (i = 0; i < imax; i++) {
		dlog(level, "%02x ", pReq->symAlgorithm.parms[i]);
	}
	dlog(level, "\n");

	dlog(level, "asymBlob:");
	imax = pReq->asymSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", pReq->asymBlob[i]);
	}
	dlog(level, "\n");

	dlog(level, "symBlob:");
	imax = pReq->symSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", pReq->symBlob[i]);
	}
	dlog(level, "\n");
	dlog(level, "-------------------------------------------------\n");
}


void show_TPM_SYMMETRIC_KEY(TPM_SYMMETRIC_KEY *pSkey, int level)
{
	int i, imax;

	dlog(level, "Dump TPM_SYMMETRIC_KEY(%p)-------------------\n", pSkey);
	if (pSkey == NULL) return;

	dlog(level, "algorithmID: %04x\n", pSkey->algId);
	dlog(level, "encScheme: %02x\n", pSkey->encScheme);
	dlog(level, "size: %hu\n", pSkey->size);
	dlog(level, "data<%p>:", pSkey->data);
	imax = pSkey->size;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", pSkey->data[i]);
	}
	dlog(level, "\n");
	dlog(level, "-------------------------------------------------\n");
}


void show_TPM_IDENTITY_PROOF(TPM_IDENTITY_PROOF *proof, int level)
{
	int i, imax;

	dlog(level, "Dump TPM_IDENTITY_PROOF(%p)-------------------\n", proof);
	if (proof == NULL) return;

	/* TPM_STRUCT_VER */
	dlog(level, "version:\n");
	dlog(level, "   major: %02x\n", proof->ver.major);
	dlog(level, "   minor: %02x\n", proof->ver.minor);
	dlog(level, "   revMajor: %02x\n", proof->ver.revMajor);
	dlog(level, "   revMinor: %02x\n", proof->ver.revMinor);

	dlog(level, "labelSize: %u\n", proof->labelSize);
	dlog(level, "identityBindingSize: %u\n", proof->identityBindingSize);
	dlog(level, "endorsementSize: %u\n", proof->endorsementSize);
	dlog(level, "platformSize: %u\n", proof->platformSize);
	dlog(level, "conformanceSize: %u\n", proof->conformanceSize);

	/* TPM_PUBKEY */
	dlog(level, "identityKey.algorithmParms\n");
	dlog(level, "   algorithmID: %04x\n", proof->identityKey.algorithmParms.algorithmID);
	dlog(level, "   encScheme: %02x\n", proof->identityKey.algorithmParms.encScheme);
	dlog(level, "   sigScheme: %02x\n", proof->identityKey.algorithmParms.sigScheme);
	dlog(level, "   parmSize: %u\n", proof->identityKey.algorithmParms.parmSize);
	dlog(level, "   parms<%p> = ", proof->identityKey.algorithmParms.parms);
	imax = proof->identityKey.algorithmParms.parmSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->identityKey.algorithmParms.parms[i]);
	}
	dlog(level, "\n");

	dlog(level, "identityKey.pubKey\n");
	dlog(level, "   keyLength: %u", proof->identityKey.pubKey.keyLength);
	imax = proof->identityKey.pubKey.keyLength;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->identityKey.pubKey.key[i]);
	}
	dlog(level, "\n");

	dlog(level, "label:");
	imax = proof->labelSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->labelArea[i]);
	}
	dlog(level, "\n");

	dlog(level, "identityBinding:");
	imax = proof->identityBindingSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->identityBinding[i]);
	}
	dlog(level, "\n");

	dlog(level, "endorsement:");
	imax = proof->endorsementSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->endorsementCredential[i]);
	}
	dlog(level, "\n");

	dlog(level, "platform:");
	imax = proof->platformSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->platformCredential[i]);
	}
	dlog(level, "\n");

	dlog(level, "conformance:");
	imax = proof->conformanceSize;
	for (i = 0; i < imax; i++) {
		if ((i % 20) == 0)
			dlog(level, "\n   ");
		dlog(level, "%02x ", proof->conformanceCredential[i]);
	}
	dlog(level, "\n");
	dlog(level, "-------------------------------------------------\n");
}

