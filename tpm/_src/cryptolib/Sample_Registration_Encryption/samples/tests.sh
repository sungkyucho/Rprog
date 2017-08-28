#!/bin/sh
./scroll_cipher
./scroll_cipher non-existing-file
./scroll_cipher file

echo "This is test for encryption" > msg.txt
echo ./sym_encrypt sym.key msg.txt msg.enc.bin 
./sym_encrypt sym.key msg.txt msg.enc.bin || exit 1
echo ./sym_decrypt sym.key msg.enc.bin msg.dec.txt || exit 1
./sym_decrypt sym.key msg.enc.bin msg.dec.txt || exit 1
echo diff msg.txt msg.dec.txt || exit 1
diff msg.txt msg.dec.txt || exit 1

echo ./encrypt_update sym.key msg.txt msg.encup.bin || exit 1
./encrypt_update sym.key msg.txt msg.encup.bin || exit 1
echo ./decrypt_update sym.key msg.encup.bin msg.decup.txt || exit 1
./decrypt_update sym.key msg.encup.bin msg.decup.txt || exit 1
echo diff msg.txt msg.decup.txt || exit 1
diff msg.txt msg.decup.txt || exit 1

echo ./gen_asym_key 1 || exit 1
./gen_asym_key 1 || exit 1
echo ./gen_csr 1_priv.pem 1.req || exit 1
./gen_csr 1_priv.pem 1.req || exit 1
echo ./gen_signature 1_priv.pem msg.txt msg.sig || exit 1
./gen_signature 1_priv.pem msg.txt msg.sig || exit 1
echo ./verify_signature 1_pub.pem msg.txt msg.sig || exit 1
./verify_signature 1_pub.pem msg.txt msg.sig || exit 1
