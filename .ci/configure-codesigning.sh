#!/usr/bin/env bash

set -e
set -x

KEYCHAIN=amanuensis.keychain
PASSWD=travis

security create-keychain -p $PASSWD $KEYCHAIN

# Make the keychain the default so identities are found
security default-keychain -s $KEYCHAIN

# Unlock the keychain
security unlock-keychain -p $PASSWD $KEYCHAIN

# Set keychain locking timeout to 3600 seconds
security set-keychain-settings -t 3600 -u $KEYCHAIN

# Generate a random cert and install it to the new keychain
keygen/install_codesigning_cert.sh -k $KEYCHAIN -p $PASSWD

security set-key-partition-list -S apple-tool:,apple: -s -k $PASSWD $KEYCHAIN

# Now we need to configure the build to use the new cert
KEYHASH=$(sudo security find-certificate -c "Amanuensis Authors" -Z $KEY_CHAIN | awk '/^SHA-1/ { print $3}')
rm trusty-constants.pri
echo 'CERT_CN = "\"Amanuensis Authors\""' > trusty-constants.pri
echo 'CERT_OU = bendb.com' >> trusty-constants.pri
echo "CERTSHA1 = $KEYHASH" >> trusty-constants.pri
