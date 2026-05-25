#!/bin/bash

CPP_COMPILER="${CPP_COMPILER:-g++}"
CPP_OPTIONS="${CPP_OPTIONS:-}"
CPP_TARGET="${CPP_TARGET:-release}"
CPP_MODEL="${CPP_MODEL:-native}"

# Parsing build target and model
if [[ "$1" =~ ^(debug|beta|release)$ ]]; then
	CPP_TARGET="$1"
	shift
fi

if [[ "$1" =~ ^(x64|x86|arm64|native|fat)$ ]]; then
	CPP_MODEL="$1"
	shift
fi

# Setting compilation options based on the target
case "$CPP_TARGET" in
	debug) CPP_OPTIONS="-O0 -DDEBUG -g $CPP_OPTIONS" ;;
	beta) CPP_OPTIONS="-Os -DDEBUG -g $CPP_OPTIONS" ;;
	release) CPP_OPTIONS="-Os -DNDEBUG $CPP_OPTIONS" ;;
	*) echo "Unrecognized CPP_TARGET: $CPP_TARGET"; exit 1 ;;
esac

# Setting compilation options based on the model and platform
unameOut="$(uname -s)"
macFlags() {
	case "$1" in
		x64) echo "-m64 -arch x86_64 -target x86_64-apple-macos" ;;
		x86) echo "-m32 -arch i386 -target i386-apple-macos" ;;
		arm64) echo "-m64 -arch arm64 -target arm64-apple-macos" ;;
		fat) echo "-m64 -arch x86_64 -arch arm64" ;;
	esac
}
case "$CPP_MODEL" in
	x64|x86|arm64|fat)
		if [[ "$unameOut" == "Darwin" ]]; then
			CPP_OPTIONS="$(macFlags "$CPP_MODEL") $CPP_OPTIONS"
		else
			[[ "$CPP_MODEL" == "x86" ]] && CPP_OPTIONS="-m32 $CPP_OPTIONS" || CPP_OPTIONS="-m64 $CPP_OPTIONS"
		fi ;;
	native) ;; # No flags
	*) echo "Unrecognized CPP_MODEL: $CPP_MODEL"; exit 1 ;;
esac

CPP_OPTIONS="-fvisibility=hidden -fvisibility-inlines-hidden -Wno-trigraphs -Wreturn-type -Wunused-variable $CPP_OPTIONS"

if [ $# -lt 2 ]; then
	echo "BuildCpp.sh [debug|beta|release*] [x86|x64|arm64|native*|fat] <output> <source files and other compiler arguments>"
	echo "You can also use the environment variables: CPP_COMPILER, CPP_TARGET, CPP_MODEL and CPP_OPTIONS"
	exit 1
fi

output="$1"
shift

args=()
for arg in "$@"; do
	if [[ "$arg" == *.c ]]; then
		args+=(-x c "$arg" -x none)
	else
		args+=("$arg")
	fi
done

echo "Compiling $output $CPP_TARGET $CPP_MODEL using $CPP_COMPILER"
echo "$CPP_OPTIONS -o $output ${args[*]}"

if ! $CPP_COMPILER -pipe $CPP_OPTIONS -o "$output" "${args[@]}" 2>&1; then
	echo "Compilation of $output failed"
	exit 1
else
	echo "Compiled $output successfully"
	exit 0
fi
