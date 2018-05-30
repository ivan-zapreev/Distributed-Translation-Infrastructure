#!/bin/bash

#Prints the program usage
# ${0} - the program name
function usage(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <key-length> <life-time> <auth-name> <client-name> <dh-size>"
  echo "    <key-length>  - the key length, in bytes, to be used when generating the authority and client private keys"
  echo "    <life-time>   - the life-time, in days, of the authority and client certificates"
  echo "    <auth-name>   - the name of the authority private key and certificate files"
  echo "    <client-name> - the name of the client private key and certificate files"
  echo "    <dh-size>     - the DH (Diffie-Hellman) parameter file size, in bytes"
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This script allows to generate self-signed certificates for"
  echo "   the TLS mode of the distributed translation infrastructure."
  echo ""
  usage ${0}
  echo ""
  echo "--------"
  echo "PURPOSE:"
  echo "--------"
  echo "   The main purpose of the script is to simplify and automate the"
  echo "   required TLS parameter (keys, certificates and etc) generation"
  echo ""
  echo "--------"
  echo "RESULTS:"
  echo "--------"
  echo "The next produced files are to be used in the client/server configuration files: "
  echo "   1. <client-name><key-length>.crt - SSL certificate file, to be used for 'tls_crt_file'"
  echo "   2. <client-name><key-length>.key - SSL private key file, to be used for 'tls_key_file'"
  echo "   3. dh<dh-size>.pem - the temporary DH (Diffie-Hellman) parameter file, to be used for 'tls_tmp_dh_file'"
  echo "   4. Any other generated files may also be used for e.g. installingcertificates manually and etc."
  echo ""
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   1. The script assumes OpenSSL to be installed and available in ${PATH}!"
  echo "   2. All of the produced files are in PEM format."
  echo "   3. Generating dh<dh-size>.pem can take a rather long time."
  echo "   4. Small-size (e.g. 512) DH parameter files will be negated by OpenSSL"
  echo "   5. Suggested DH parameter files sizes >= 1024 (2048 shall be generally accepted)"
  echo "   6. During the script running process you will be asked questions, please fill in the data!"
  echo "   7. Make sure that the 'Common Name' of the authority and client certificates are different!"
  echo ""
}

#Reports an error, does not exit
# ${0} - the script name
# ${1} - the error message
function error() {
   echo "ERROR in ${0}: ${1}"
}

#Allows the program to fail
function fail() {
   exit 1
}

#If the script is called with no arguments then show the info message
if [ "$#" -eq 0 ]; then
   error "Improper number of arguments!"
   info ${0}
   fail
fi

#Check if the key length is defined
if [ -z "${1}" ]; then
   error "<<key-length> is not defined"
   fail
fi

#Check if the life time is defined
if [ -z "${2}" ]; then
   error "<life-time> is not defined"
   fail
fi

#Check if the authority name is defined
if [ -z "${3}" ]; then
   error "<auth-name> is not defined"
   fail
fi

#Check if the client name is defined
if [ -z "${4}" ]; then
   error "<client-name> is not defined"
   fail
fi

#Check if the DH size is defined
if [ -z "${5}" ]; then
   error "<dh-size> is not defined"
   fail
fi

echo ""
echo "-------- STARTING (8 stage process) ---------"
echo ""

echo "----------------------------------------------"
echo "- 1. Generating the authority's private key -"
echo "----------------------------------------------"
openssl genrsa -out ${3}${1}.key ${1}

echo "----------------------------------------------"
echo "- 2. Generating the authority's certificate -"
echo "----------------------------------------------"
openssl req -x509 -new -nodes -key ${3}${1}.key -days ${2} -out ${3}${1}.crt

echo "-------------------------------------------"
echo "- 3. Generating the client's private key -"
echo "-------------------------------------------"
openssl genrsa -out ${4}${1}.key ${1}

echo "-------------------------------------------------"
echo "- 4. Generating the client's signature request -"
echo "------------------------------------------------"
openssl req -new -key ${4}${1}.key -out ${4}${1}.csr

echo "-----------------------------------------------"
echo "- 5. Signing client's request with authority -"
echo "-----------------------------------------------"
openssl x509 -req -in ${4}${1}.csr -CA ${3}${1}.crt -CAkey ${3}${1}.key -CAcreateserial -out ${4}${1}.crt -days ${2}

echo "-----------------------------------------"
echo "- 6. Verifying authority's certificate -"
echo "-----------------------------------------"
openssl verify -CAfile ${3}${1}.crt ${3}${1}.crt

echo "--------------------------------------"
echo "- 7. Verifying client's certificate -"
echo "--------------------------------------"
openssl verify -CAfile ${3}${1}.crt ${4}${1}.crt

echo "------------------------------------"
echo "- 8. Generating DH parameters file -"
echo "------------------------------------"
openssl dhparam -out dh${5}.pem ${5}

echo "------------------------------------"
echo "Generated: "
echo ""
ls -al ${4}${1}.* ${3}${1}.* dh${5}.pem
echo ""
echo "---------------DONE-----------------"
echo ""
