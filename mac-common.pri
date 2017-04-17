# Name of the application signing certificate
APPCERT = "\"3rd Party Mac Developer Application: App Developer (XXXXXXXXXX)\""

# Cert OU
CERT_OU = XXXXXXXXXX

# Sha1 of the siging certificate
CERTSHA1 = 1234567890ABCDEFFEDCBA098765432112345678

DEFINES += kSigningCertCommonName=\\\"$${APPCERT}\\\"
