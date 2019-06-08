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

# chi2 iteration NNLO RTOPT QCDNUM (default) [HERAPDF2.0 arXiv:1506.06042 ]
./tools/check22.sh def
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration NLO RTOPT QCDNUM [HERAPDF2.0 arXiv:1506.06042 ]
./tools/check22.sh defNLO
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration NLO FFABM QCDNUM [HERAPDF-HVQMASS arXiv:1804.01019]
./tools/check22.sh FFABM
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# chi2 iteration FONLL APFEL
./tools/check22.sh FONLL
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

# full fir ZMVFNS NNLO QCDNUM, with error bands (takes ~ 10 min)
#./tools/check22.sh ZMVFNS-fit
if [ `echo $?` -ne 0 ]; then flagAllFine=0; fi

#./tools/check22.sh def
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
