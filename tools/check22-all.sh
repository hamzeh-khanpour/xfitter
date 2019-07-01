#!/bin/bash
#
# Collection of all tests (slow tests are commented out)
#
# If output for some tests needs to be updated, add '--copy', e.g.:
# ./tools/check22.sh def --copy
#

echo "====================================================="
echo "Running all tests ... output will be stored in temp/"
echo "====================================================="

rm -rf temp/
flagAllFine=1
COPY=''
# uncomment if you want to copy results and make them reference
COPY=' --copy'

# chi2 iteration NNLO RTOPT QCDNUM (default) [HERAPDF2.0 arXiv:1506.06042 ]
# it tests also storing NNLO PDF from QCDNUM evolution in LHAPDF6 format
./tools/check22.sh defaultNNLO $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration NLO RTOPT QCDNUM [HERAPDF2.0 arXiv:1506.06042 ]
# it tests also storing NLO PDF from QCDNUM evolution in LHAPDF6 format
./tools/check22.sh defaultNLO $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration NLO FFABM QCDNUM [HERAPDF-HVQMASS arXiv:1804.01019]
./tools/check22.sh FFABM $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration FONLL APFEL
# it tests also storing PDF from APFEL evolution in LHAPDF6 format
./tools/check22.sh FONLL $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration for ALL data using NLO RTOPT LHAPDF=CT10nlo
# it tests also storing NNLO PDF from LHAPDF evolution in LHAPDF6 format
./tools/check22.sh ALLDATA $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration for AFB pseudodata, checks AFB (LO) and APPLgrid (NLO) reactions
./tools/check22.sh AFB $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration for LHeC pseudodata
./tools/check22.sh LHeC $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration for PROSA analysis 1503.04581 (absolute variant)
./tools/check22.sh HVQMNR-abs $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration for PROSA analysis 1503.04581 (normalised variant)
./tools/check22.sh HVQMNR-norm $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# full fit ZMVFNS NNLO QCDNUM, with error bands (takes ~ 10 min)
# it tests also storing PDF eigenvectors in LHAPDF6 format
./tools/check22.sh ZMVFNS-fit $COPY
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

echo "====================================================="
if [ $flagAllFine -eq 1 ]; then
  echo " -> All tests are fine"
else
  echo " -> Something failed: see above for details (logs are in /temp)"
fi
echo "====================================================="

if [ $flagAllFine = 0 ]; then
  exit 1
fi
