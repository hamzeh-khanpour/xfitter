#!/bin/bash

# list of tests to omit (if commented out, no tests are omitted)
#omitTests=('ZMVFNS-fit' 'profilerLHAPDF') # these are two slow tests, skipping them will save ~15min

# xfitter binary
xfitter=`pwd`/'bin/xfitter'

# log file for xfitter output
xflogfile='xfitter.log'

# log file for test output
testlogfile='test.log'

# output messages
FAILED="\e[31m\e[1mFAIL\e[0m" # bold red
PASSED="\e[32m\e[1mPASS\e[0m" # bold green

# function to check if element $1 is in array $2
# code from https://stackoverflow.com/questions/3685970/check-if-a-bash-array-contains-a-value
containsElement () {
  local e match="$1"
  shift
  for e; do [[ "$e" == "$match" ]] && return 0; done
  return 1
}

# function to check if $1 and $2 are identical
checkFile()
{
  printf "diff $1 $2 ... "x
  diff $1 $2 > /dev/null
  exitcode=$?

  if [ $exitcode = 0 ]; then
  echo "PASSED"
  else
  echo "FAILED"
  fi 
}

runTest()
{
  # TESTNAME is the name of directory in examples/
  TESTNAME=$1
  TESTNAMEDEF='defaultNNLO'
  if [ -z $TESTNAME ]; then
    TESTNAME=$TESTNAMEDEF
  fi
  # directory where to run
  rundir=$2
  # Optionally copy results and make them reference
  COPYRESULTS=$3
  #rm -rf $rundir
  #mkdir -p $rundir

  echo "========================================"
  echo "Running check: $TESTNAME"
  echo "========================================"
  if [ $COPYRESULTS -eq 0 ]; then
    echo "This is validation test:"
    echo "OK if code runs properly"
    echo "FAILED if code fails to reproduce expected results"
    echo "========================================"
  fi

  INPUTDIR="examples/$TESTNAME"
  EXAMPLEDIR="examples/$TESTNAME/output"
  if [ $COPYRESULTS -eq 1 ]; then
    echo "Results will be stored as reference in $EXAMPLEDIR"
  fi
  echo "Running in temp/$TESTNAME"
  echo "Log file stored in temp/$TESTNAME/$xflogfile"

  if [ ! -d $INPUTDIR ]; then
    echo "Failed to find input files for test \"$TESTNAME\""
    echo "expected directory $INPUTDIR"
    return 1
  fi
  echo "Using input files from ${INPUTDIR}"
  echo "========================================"

  cp ${INPUTDIR}/steering.txt $rundir
  cp ${INPUTDIR}/parameters.yaml $rundir
  cp ${INPUTDIR}/constants.yaml $rundir
  ln -s `pwd`/datafiles $rundir/datafiles

  cd $rundir
  ${xfitter} >& ${xflogfile}
  cd - > /dev/null

  flagBAD=0
  # check chi2 in Results.txt ("After minimisation ...")
  if [ $COPYRESULTS -eq 0 ]; then
    # some tests do not call 'fcn 3' and there is no chi2 stored in Results.txt
    grep  'After' ${EXAMPLEDIR}/Results.txt > temp/def.txt
    exitcode=$?
    if [ $exitcode = 0 ]; then
      grep  'After' $rundir/output/Results.txt > temp/out.txt
      exitcode=$?
      if [ $exitcode = 0 ]; then
        cat temp/out.txt
        diff temp/out.txt temp/def.txt 
        exitcode=$?
        if [ $exitcode = 0 ]; then
          echo "========================================"
          echo "Check of chi^2 is PASSED"
          echo "========================================"
        else
          echo "========================================"
          echo "FAILED validation with default steering"
          echo "========================================"
          flagBAD=1
        fi
      else
          echo "========================================"
          echo "FAILED no chi2 calculated: check temp/$TESTNAME/$xflogfile"
          echo "========================================"
      fi
    fi
    rm -f temp/out.txt temp/def.txt
  fi

  # check all output files
  if [ $COPYRESULTS -eq 0 ]; then
    echo "Checking all output files ..."
    for targetfile in `find ${EXAMPLEDIR} -type f`; do
      file=`echo ${targetfile} | sed -e s@${EXAMPLEDIR}@${rundir}/output@`
      if [ ! -f $file ]; then
        echo "No expected file $file"
        flagBAD=1
        continue
      fi
      out=`checkFile $file $targetfile`
      echo $out
      echo $out | grep FAILED > /dev/null
      exitcode=$?
      if [ $exitcode == 0 ]; then
        flagBAD=1
      fi
    done
    echo "========================================"
    if [ $flagBAD == 0 ]; then
      echo "Everything is PASSED"
    else
      echo "Something FAILED: see above for details"
    fi
    echo "========================================"
  else
    rm -rf $EXAMPLEDIR
    cp -r $rundir/output $EXAMPLEDIR
    echo "Output copied"
    echo "========================================"
  fi

  return $flagBAD
}

##########################
# the script starts here #
##########################
if [ $# -ne 0 ] && [ "${@: -1}" = "--help" ]; then
  echo "Usage: test.sh <TEST1> <TEST2> ... [OPTION]"
  echo "OPTION could be:"
  echo "  --copy to copy results and make them reference"
  echo "  --help to see this message"
  echo "If no test names are provided, all tests in directory examples/ will run, except those specified in 'omitTests' list"
  exit 1
fi

COPY=0
if [ $# -ne 0 ] && [ "${@: -1}" = "--copy" ]; then
  COPY=1
  echo "==========================================================================="
  echo "Running in COPY mode: output of tests will be copied and saved as reference"
  echo "==========================================================================="
fi

listOfTests=""
for arg in "$@"; do
  if [ "${arg:0:2}" != "--" ]; then
    listOfTests="$listOfTests $arg"
  fi
done
if [ -z "$listOfTests" ]; then
  for dir in `ls -1d examples/*`; do
    dir=`basename $dir`;
    containsElement $dir "${omitTests[@]}"
    if [ `echo $?` -eq 1 ]; then
      listOfTests="$listOfTests $dir"
    fi
  done
fi
if [ -z "$listOfTests" ]; then
  echo "No tests to run"
fi

testsPassed=0
testsFailed=0
for arg in `echo $listOfTests`; do
  dir=temp/$arg
  rm -rf $dir
  mkdir -p $dir
  log=$dir/$testlogfile
  printf "Testing $arg ... "
  rm -rf 
  runTest $arg $dir $COPY >& $log
  exitcode=$?
  if [ $COPY -eq 1 ]; then
    if [ $exitcode == 0 ]; then
      echo copied [details in $log]
      testsPassed=$[$testsPassed+1]
    else
      echo not copied [details in $log]
      testsFailed=$[$testsFailed+1]
    fi
  else
    if [ $exitcode == 0 ]; then
      echo -e $PASSED [details in $log]
      testsPassed=$[$testsPassed+1]
    else
      echo -e $FAILED [details in $log]
      testsFailed=$[$testsFailed+1]
    fi
  fi
done

if [ $COPY -eq 1 ]; then
  if [ $testsPassed -gt 0 ]; then
    echo -e "-> $testsPassed test(s) copied"
  fi
  if [ $testsFailed -gt 0 ]; then
    echo -e "-> $testsFailed test(s) not copied"
  fi
else
  if [ $testsPassed -gt 0 ]; then
    echo -e "-> $testsPassed test(s) $PASSED"
  fi
  if [ $testsFailed -gt 0 ]; then
    echo -e "-> $testsFailed test(s) $FAILED"
  fi
fi

exit $flagBAD
