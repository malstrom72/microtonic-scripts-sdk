#!/bin/bash

CPP_COMPILER="${CPP_COMPILER-g++}"
CPP_OPTIONS="${CPP_OPTIONS-}"
CPP_TARGET="${CPP_TARGET-release}"
CPP_MODEL="${CPP_MODEL-64}"

if [ "$1" == "debug" ] || [ "$1" == "beta" ] || [ "$1" == "release" ]; then
	CPP_TARGET="$1"
	shift
fi

if [ "$1" == "64" ] || [ "$1" == "32" ]; then
	CPP_MODEL="$1"
	shift
fi

if [ "$CPP_TARGET" == "debug" ]; then
	CPP_OPTIONS="-O0 -DDEBUG -g $CPP_OPTIONS"
elif [ "$CPP_TARGET" == "beta" ]; then
	CPP_OPTIONS="-Os -DDEBUG -g $CPP_OPTIONS"
elif [ "$CPP_TARGET" == "release" ]; then
	CPP_OPTIONS="-Os -DNDEBUG $CPP_OPTIONS"
else
	echo Unrecognized CPP_TARGET "$CPP_TARGET"
	exit 1
fi

if [ "$CPP_MODEL" == "64" ]; then
	CPP_OPTIONS="-m64 $CPP_OPTIONS"
elif [ "$CPP_MODEL" == "32" ]; then
	CPP_OPTIONS="-m32 $CPP_OPTIONS"
else
	echo Unrecognized CPP_MODEL "$CPP_MODEL"
	exit 1
fi

CPP_OPTIONS="-fvisibility=hidden -fvisibility-inlines-hidden -Wno-trigraphs -Wreturn-type -Wunused-variable $CPP_OPTIONS"

if [ $# -lt 2 ]; then
	echo "BuildCpp.sh [debug|beta|release*] [32|64* (bit)] <output> <source files and other compiler arguments>"
	echo "You can also use the environment variables: CPP_COMPILER, CPP_TARGET, CPP_MODEL and CPP_OPTIONS"
	exit 1
fi

echo Compiling $1 $CPP_TARGET $CPP_MODEL using $CPP_COMPILER
echo $CPP_OPTIONS -o $@
logfile=$(mktemp)
$CPP_COMPILER -pipe $CPP_OPTIONS -o $@ >"$logfile" 2>&1
if [ $? -ne 0 ]; then
	cat "$logfile"
	rm "$logfile"
	echo Compilation of $1 failed
	exit 1
else
	rm "$logfile"
	echo Compiled $1
	exit 0
fi
