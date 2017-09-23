#!/usr/bin/env bash

set -e
set -x

openssl genrsa -out ama.key 2048
openssl req -x509 -new -config apple.conf -nodes -key ama.key -sha256 -out ama.crt

echo "Generating PKCS#12 wrapper; please choose a passphrase:"
openssl pkcs12 -export -inkey ama.key -in ama.crt -out ama.p12

open ama.p12

