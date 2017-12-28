#!/usr/bin/env bash

set -e
set -x

function show_usage() {
  cat <<- "EOF"
usage: install_codesigning_cert.sh [options]

Generates and installs a certificate suitable for signing builds of Amanuensis.

options:
  -k <keychain>   - specifies the keychain to which the cert should be installed.
                    If not given, the default keychain will be used.

  -p <passphrase> - the passphrase for the keychain to which the cert should be installed.
                    Required when specifying "-k".

  -h, --help      - show this help message and exit.
EOF
  exit 0
}

KEYCHAIN=
PASSWD=

OPTIND=1
while getopts "k:p:h-:" opt; do
  case "$opt" in
    k) KEYCHAIN=$OPTARG
       ;;
    p) PASSWD=$OPTARG
       ;;
    h) show_usage
       ;;
    -) case "$OPTARG" in
         help) show_usage
               ;;
       esac
  esac
done
shift "$((OPTIND-1))"

if [[ -n "$KEYCHAIN" && -z "$PASSWD" ]]; then
  echo "Password is required when using the -k option"
  exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$DIR"
trap popd EXIT

KEYCHAIN_OPTS=
PASSWD_OPENSSL_OPTS=
PASSWD_SECURITY_OPTS=

if [ -n "$PASSWD" ]; then
  PASSWD_OPENSSL_OPTS="-passout pass:$PASSWD"
  PASSWD_SECURITY_OPTS="-P $PASSWD"
fi

if [ -n "$KEYCHAIN" ]; then
  KEYCHAIN_OPTS="-k $KEYCHAIN"
fi

if [ -z "$PASSWD" ]; then
  echo "Generating PKCS#12 wrapper; please choose a passphrase:"
fi

openssl genrsa -out ama.key 2048
openssl req -x509 -new -config ama.conf -nodes -key ama.key -sha256 -out ama.crt
openssl pkcs12 -export -inkey ama.key -in ama.crt -out ama.p12 $PASSWD_OPENSSL_OPTS
security import ama.p12 -t agg -f pkcs12 $PASSWD_SECURITY_OPTS -T /usr/bin/codesign $KEYCHAIN_OPTS

