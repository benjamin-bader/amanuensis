# Name of the application signing certificate
CERT_CN = "\"Amanuensis Authors\""

# Cert OU
CERT_OU = bendb.com

# Sha1 of the siging certificate
CERTSHA1 = 1234567890ABCDEFFEDCBA098765432112345678

DEFINES += kSigningCertCommonName=\\\"$${CERT_CN}\\\"


