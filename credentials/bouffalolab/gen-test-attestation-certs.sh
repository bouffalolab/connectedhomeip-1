#!/usr/bin/env bash

#
# Copyright (c) 2021-2022 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Script that can be used to generate CHIP device attestation certificates
# for testing purposes.
# The script expects the path to the chip-cert tool binary as an input argument.
#
# Usage example when the script is run from the CHIP SDK root directory:
#     ./credentials/test/gen-test-attestation-certs.sh ./out/debug/standalone/chip-cert
#
# The result will be stored in:
#     credentials/test/attestation
# If the intention is to re-generate a new set of attestation certificates that
# replace the already present ones then it is recommended to remove current certificates:
#     rm credentials/test/attestation/*
#
# Second example also generates C-Style file with those certificates/keys to be use by the SDK tests:
#     ./credentials/test/gen-test-attestation-certs.sh ./out/debug/standalone/chip-cert src/credentials/tests/CHIPAttCert_test_vectors
#
# In addition to the DER/PEM files this command also generates the following C-Style files:
#     src/credentials/tests/CHIPAttCert_test_vectors.cpp
#     src/credentials/tests/CHIPAttCert_test_vectors.h
#

set -e

here=${0%/*}

dest_dir="$here/attestation/"

rm -rf "$dest_dir"
mkdir -p "$dest_dir"

if [ $# == 1 ]; then
    chip_cert_tool=$1
else
    echo "Error: ./gen.sh chip_cert_command_path"
    exit
fi

VID=FFF1
PID=8005
 
cert_valid_from="2021-06-28 14:23:43"
cert_lifetime=4294967295

# Set #1:
#   - PAA Subject includes VID
#   - PAI Subject includes PID
{
    dac_ids=(0000 0001)

    #paa_key_file="../../credentials/test/attestation/Chip-Test-PAA-FFF1-Key"
    #paa_cert_file="../../credentials/test/attestation/Chip-Test-PAA-FFF1-Cert"

    paa_key_file="$dest_dir/Chip-Bouffalolab-PAA-$VID-Key"
    paa_cert_file="$dest_dir/Chip-Bouffalolab-PAA-$VID-Cert"

    "$chip_cert_tool" gen-att-cert --type a --subject-cn "Matter BouffaloLab PAA" --subject-vid "$VID" --valid-from "$cert_valid_from" --lifetime "$cert_lifetime" --out-key "$paa_key_file".pem --out "$paa_cert_file".pem

    pai_key_file="$dest_dir/Chip-Bouffalolab-PAI-$VID-Key"
    pai_cert_file="$dest_dir/Chip-Bouffalolab-PAI-$VID-Cert"

    "$chip_cert_tool" gen-att-cert --type i --subject-cn "Matter BouffaloLab PAI" --subject-vid "$VID" --valid-from "$cert_valid_from" --lifetime "$cert_lifetime" --ca-key "$paa_key_file".pem --ca-cert "$paa_cert_file".pem --out-key "$pai_key_file".pem --out "$pai_cert_file".pem

    for dac in "${dac_ids[@]}"; do
        dac_key_file="$dest_dir/Chip-Bouffalolab-DAC-$VID-$PID-$dac-Key"
        dac_cert_file="$dest_dir/Chip-Bouffalolab-DAC-$VID-$PID-$dac-Cert"

        "$chip_cert_tool" gen-att-cert --type d --subject-cn "Matter BouffaloLab DAC $dac" --subject-vid "$VID" --subject-pid "$PID" --valid-from "$cert_valid_from" --lifetime "$cert_lifetime" --ca-key "$pai_key_file".pem --ca-cert "$pai_cert_file".pem --out-key "$dac_key_file".pem --out "$dac_cert_file".pem
    done
}

# In addition to PEM format also create certificates in DER form.
for cert_file_pem in "$dest_dir"/*Cert.pem; do
    cert_file_der="${cert_file_pem/.pem/.der}"
    "$chip_cert_tool" convert-cert "$cert_file_pem" "$cert_file_der" --x509-der
done

# In addition to PEM format also create private key in DER form.
for key_file_pem in "$dest_dir"/*Key.pem; do
    key_file_der="${key_file_pem/.pem/.der}"
    "$chip_cert_tool" convert-key "$key_file_pem" "$key_file_der" --x509-der
done

type="Pai"
cert_file_der="${folder}/test-PAI-${VID}-cert.der"
cert_file_pem="${folder}/test-PAI-${VID}-cert.pem"
key_file_pem="${folder}/test-PAI-${VID}-key.pem"

print_start() {
  printf "namespace chip {\n"
  printf "namespace DevelopmentCerts {\n\n"
  if [ $type == "Dac" ]; then
    printf "#if CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID == 0x${PID}\n\n"
  fi
}

print_end() {
  if [ $type == "Dac" ]; then
    printf "#endif // CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID\n"
  fi
  printf "} // namespace DevelopmentCerts\n"
  printf "} // namespace chip\n"
}

printcert() {
  # convert cert to DER
  if [ -f "${cert_file_der}" ]; then
      rm "${cert_file_der}"
  fi
  "${chip_cert_tool}" convert-cert "${cert_file_pem}" "${cert_file_der}" --x509-der

  printf "// ------------------------------------------------------------ \n"
  printf "// ${type} CERTIFICATE ${cert_file_der} \n\n"

  printf "constexpr uint8_t ${type}_Cert_Array[] = {\n"
  less -f "${cert_file_der}" | od -t x1 -An | sed 's/\</0x/g' | sed 's/\>/,/g' | sed 's/^/   /g'
  printf "};\n\n"
  printf "ByteSpan k${type}Cert = ByteSpan(${type}_Cert_Array);\n\n"

  printf "// ${type} PUBLIC KEY FROM ${key_file_pem} \n\n"

  printf "constexpr uint8_t ${type}_PublicKey_Array[] = {\n"
  openssl ec -text -noout -in "${key_file_pem}" 2>/dev/null | sed '/ASN1 OID/d' | sed '/NIST CURVE/d' | sed -n '/pub:/,$p' | sed '/pub:/d' | sed 's/\([0-9a-fA-F][0-9a-fA-F]\)/0x\1/g' | sed 's/:/, /g'
  printf "};\n\n"
  printf "ByteSpan k${type}PublicKey = ByteSpan(${type}_PublicKey_Array);\n\n"

  printf "// ${type} PRIVATE KEY FROM ${key_file_pem} \n\n"

  printf "constexpr uint8_t ${type}_PrivateKey_Array[] = {\n"
  openssl ec -text -noout -in "${key_file_pem}" 2>/dev/null | sed '/read EC key/d' | sed '/Private-Key/d' | sed '/priv:/d' | sed '/pub:/,$d' | sed 's/\([0-9a-fA-F][0-9a-fA-F]\)/0x\1/g' | sed 's/:/, /g'
  printf "};\n\n"
  printf "ByteSpan k${type}PrivateKey = ByteSpan(${type}_PrivateKey_Array);\n\n"
}

type="Pai"
print_start
# Print generated certificate, keys, and parameters in C-Style to use in the SDK if the output file is proVIDed.
for cert_file_pem in "$dest_dir"/*PAI*Cert.pem;
do
    cert_file_der="${cert_file_pem/.pem/.der}"
    key_file_pem="${cert_file_pem/Cert.pem/Key.pem}"
        
    printcert

done
print_end

type="Dac"
print_start
i=1
for cert_file_pem in "$dest_dir"/*DAC*Cert.pem;
do
    printf "#if CHIP_CERT_TEST_DUT_ID == $i\n\n"
    cert_file_der="${cert_file_pem/.pem/.der}"
    key_file_pem="${cert_file_pem/Cert.pem/Key.pem}"
        
    printcert
    
    printf "#endif\n\n"
    i=$(($i + 1))
done
print_end
