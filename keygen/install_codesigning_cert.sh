#!/usr/bin/env bash

set -e
set -x

# We need to automate keygen and key installation for Travis CI;
# part of this is providing a passphrase via stdin, then automatically
# installing it without any UI interaction.
#
# For normal development, we *always* prefer to go through the front door,
# so to speak.
PASSWD=
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  PASSWD=$(cat -)
fi

openssl genrsa -out ama.key 2048
openssl req -x509 -new -config ama.conf -nodes -key ama.key -sha256 -out ama.crt

if [[ "$PASSWD" == "" ]]; then
  echo "Generating PKCS#12 wrapper; please choose a passphrase:"
  openssl pkcs12 -export -inkey ama.key -in ama.crt -out ama.p12
  open ama.p12
else
  openssl pkcs12 -export -inkey ama.key -in ama.crt -out ama.p12 -passout pass:"$PASSWD"
  sudo security import ama.p12 -t agg -f pkcs12 -P "$PASSWD" -A
fi
