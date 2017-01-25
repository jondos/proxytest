#!/bin/bash
for mycert in *.cer ;
do
 openssl x509 -in $mycert -outform DER -out ./converted/$mycert && rm $mycert ;
done