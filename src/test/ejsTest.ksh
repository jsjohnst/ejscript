#!/bin/bash
#
#	ejsTest.ksh -- EJS unit tests
#
################################################################################
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#	The latest version of this code is available at http://www.embedthis.com
#
#	This software is open source; you can redistribute it and/or modify it 
#	under the terms of the GNU General Public License as published by the 
#	Free Software Foundation; either version 2 of the License, or (at your 
#	option) any later version.
#
#	This program is distributed WITHOUT ANY WARRANTY; without even the 
#	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#	See the GNU General Public License for more details at:
#	http://www.embedthis.com/downloads/gplLicense.html
#	
#	This General Public License does NOT permit incorporating this software 
#	into proprietary programs. If you are unable to comply with the GPL, a 
#	commercial license for this software and support services are available
#	from Embedthis Software at http://www.embedthis.com
#
################################################################################

VALGRIND_CMD="valgrind --tool=memcheck --error-exitcode=1 --suppressions=ejs.supp"

. ${BLD_TOP}/buildConfig.sh

export LD_LIBRARY_PATH=${BLD_LIB_DIR}:$LD_LIBRARY_PATH

################################################################################

USAGE="
ejsTest [options...] files ...
    [--args args]       Args to pass to the command 
    [--compile]         Run ec to compile the files
	[--doc]             Generate documentation
    [--list]            Compile and use ejsmod to generate listings
    [--run]             Run using ejs
    [--runvm]           Compile and run using the ejs virtual machine
	[--slots]           Generate slot files
    [--testName name]   Name the test run
    [--valgrind]        Run under valgrind
    [--verbose ]        Use verbose mode
"

#
#	Compile (no run)
#
compile()
{
	local files=$*

	CMD="ejsc${BLD_EXE_FOR_BUILD} ${ARGS} $files"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	[ "$VERBOSE" -gt 0 ] && echo "  ${CMD}"
	eval $CMD 2>/tmp/ec$$.log
	status=$?
	if [ $status != 0 ]
	then
		echo "  # FAILED test $f"
		cat /tmp/ec$$.log
		rm -f /tmp/ec$$.log
		exit 255
	fi
	rm -f /tmp/ec$$.log
}


#
#	Generate doc
#
doc()
{
	local files=$*

	local saveARGS="$ARGS"
	ARGS="--doc $ARGS"

	compile $files

	ARGS="$saveARGS"

	CMD="ejsmod${BLD_EXE_FOR_BUILD} ${SEARCH} --html doc *.mod"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	if ls *.mod >/dev/null 2>&1 ; then
		if [ "$VERBOSE" -gt 0 ] ; then
			echo -n "  "
			eval echo ${CMD}
		fi
		eval $CMD 2>/tmp/ejsc$$.log
		status=$?
        if [ $status != 0 ] ; then
			echo "  # FAILED test $f"
			cat /tmp/ejsc$$.log
			rm -f /tmp/ejsc$$.log
			exit 255
		fi
		rm -f /tmp/ejsc$$.log
	fi
}


#
#	Do listing
#
list()
{
	local files=$*

	compile $files

	CMD="ejsmod${BLD_EXE_FOR_BUILD} ${SEARCH} --listing *.mod"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	if ls *.mod >/dev/null 2>&1 ; then
		if [ "$VERBOSE" -gt 0 ] ; then
			echo -n "  "
			eval echo ${CMD}
		fi
		eval $CMD 2>/tmp/ejsc$$.log
		status=$?
		if [ $status != 0 ]
		then
			echo "  # FAILED test $f"
			cat /tmp/ejsc$$.log
			rm -f /tmp/ejsc$$.log
			exit 255
		fi
		rm -f /tmp/ejsc$$.log
	fi
}


#
#	Run using ejs
#
run()
{
	local files=$*

	CMD="ejs${BLD_EXE_FOR_BUILD} ${ARGS} $files"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	[ "$VERBOSE" -gt 0 ] && echo "  ${CMD}"

	eval $CMD 2>/tmp/ejsc$$.log
	status=$?
    if [ $status != 0 ] ; then
		echo "  # FAILED test $f"
		cat /tmp/ejsc$$.log
		rm -f /tmp/ejsc$$.log
		exit 255
	fi
	rm -f /tmp/ejsc$$.log
}


#
#	Compile then run in stand-alone ejs VM
#
runvm()
{
	local files=$*

	compile $files

	CMD="ejsvm${BLD_EXE_FOR_BUILD} ${SEARCH} *.mod"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	if ls *.mod >/dev/null 2>&1 ; then
		if [ "$VERBOSE" -gt 0 ] ; then
			echo -n "  "
			eval echo ${CMD}
		fi
		eval $CMD 2>/tmp/ejsc$$.log
		status=$?
		if [ $status != 0 ]
		then
			echo "  # FAILED test $f"
			cat /tmp/ejsc$$.log
			rm -f /tmp/ejsc$$.log
			exit 255
		fi
		rm -f /tmp/ejsc$$.log
	fi
}


#
#	Generate slot files
#
slots()
{
	local files=$*

	compile $files

	CMD="ejsmod${BLD_EXE_FOR_BUILD} ${SEARCH} --cslots --jslots *.mod"
	if [ "$VALGRIND" = 1 ] ; then
		CMD="$VALGRIND_CMD $CMD"
	fi
	if ls *.mod >/dev/null 2>&1 ; then
		if [ "$VERBOSE" -gt 0 ] ; then
			echo -n "  "
			eval echo ${CMD}
		fi
		eval $CMD 2>/tmp/ejsc$$.log
		status=$?
		if [ $status != 0 ]
		then
			echo "  # FAILED test $f"
			cat /tmp/ejsc$$.log
			rm -f /tmp/ejsc$$.log
			exit 255
		fi
		rm -f /tmp/ejsc$$.log
	fi
}


################################################################################
#
#	Main. Parse args
#

ARGS=""
COMPILE=0
DOC=0
MODE=
LIST=0
RUN=0
RUN_VM=0
SLOTS=0
TEST_NAME=test
VALGRIND=0
VERBOSE=0
SEARCH=

echo -e "\n  # Running unit tests for ${BLD_PRODUCT}.$TEST_NAME"

while [ "$1" != "" ]
do
	case "$1" in
	--args)
		ARGS="$2"
		shift ; shift
		;;
	--compile)
		COMPILE=1
		shift
		;;
	--doc)
		DOC=1
		shift
		;;
	--list)
		LIST=1
		shift
		;;
	--testName)
		TEST_NAME=$2
		shift ; shift
		;;
	--run)
		RUN=1
		shift
		;;
	--runvm)
		RUN_VM=1
		shift
		;;
	--valgrind)
		VALGRIND=1
		shift
		;;
	--verbose)
		VERBOSE=`expr $VERBOSE + 1`
		shift
		;;
	--search)
		SEARCH="--searchpath \"$2\""
		shift; shift
		;;
	--slots)
		SLOTS=1
		shift
		;;
	-v)
		VERBOSE=`expr $VERBOSE + 1`
		shift
		;;
	-*)
		echo "Unknown arg \"$1\"" 
		echo "$USAGE"
		exit 255
		;;
	*)
		break
		;;
	esac
done

TESTS=$*

if [ "$BLD_FEATURE_FLOATING_POINT" = 0 ] ; then
	TESTS=`echo $TESTS | sed 's/float.ejs//'`
fi

if [ "$SEARCH" != "" ] ; then
	ARGS="$ARGS $SEARCH"
fi
if [ "$MODE" != "" ] ; then
	ARGS="$ARGS $MODE"
fi


for test in $TESTS
do
	rm -f *.mod *.lst
	base=${test%.es}
	if ls ${base}-*.es2 >/dev/null 2>&1 ; then
		files="$test ${base}-*.es2"
	else
		files="$test"
	fi

	if [ "$COMPILE" = 1 ] ; then
		compile $files
	fi

	if [ "$DOC" = 1 ] ; then
		doc $files
	fi

	if [ "$LIST" = 1 ] ; then
		list $files
	fi

	if [ "$RUN" = 1 ] ; then
		run $files
	fi

	if [ "$RUN_VM" = 1 ] ; then
		runvm $files
	fi

	if [ "$SLOTS" = 1 ] ; then
		slots $files
	fi

    [ "$VERBOSE" -gt 0 ] && echo
done

echo "  # PASSED all tests for \"${TEST_NAME}\""
exit 0

