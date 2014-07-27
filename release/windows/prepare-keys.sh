#!/bin/bash
set -e
openssl pkcs12 -in authenticode.pfx -nocerts -nodes -out key.pem
openssl pkcs12 -in authenticode.pfx -nokeys -nodes -out cert.pem
openssl rsa -in key.pem -outform DER -out authenticode.key
openssl crl2pkcs7 -nocrl -certfile cert.pem -outform DER -out authenticode.spc
rm -f key.pem cert.pem
