bin_PROGRAMS=scroll_cipher gen_asym_key gen_csr sym_encrypt sym_decrypt
bin_PROGRAMS+=encrypt_update decrypt_update gen_signature verify_signature

AM_CFLAGS = -I${top_srcdir}/src
AM_LDFLAGS = -L${top_srcdir}/src -lsettings -lcrypto

scroll_cipher_SOURCES=scroll_cipher.c
scroll_cipher_LDADD = -lcrypto -lsettings -ljson-c

gen_asym_key_SOURCES=gen_asym_key.c
gen_csr_SOURCES=gen_csr.c
sym_encrypt_SOURCES=sym_encrypt.c
sym_decrypt_SOURCES=sym_decrypt.c
encrypt_update_SOURCES=encrypt_update.c
decrypt_update_SOURCES=decrypt_update.c
gen_signature_SOURCES=gen_signature.c
verify_signature_SOURCES=verify_signature.c
