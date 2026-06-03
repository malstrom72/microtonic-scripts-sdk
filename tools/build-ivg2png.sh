#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

target="${1:-release}"
model="${2:-native}"
simd="${3:-nosimd}"

case "$simd" in
	simd) simd_flag=1 ;;
	nosimd) simd_flag=0 ;;
	*)
		echo "Usage: tools/build-ivg2png.sh [debug|beta|release] [x86|x64|arm64|native|fat] [nosimd|simd]" >&2
		exit 1
		;;
esac

mkdir -p tools/IVG2PNG

c_sources=(
	IVG/externals/libpng/png.c
	IVG/externals/libpng/pngerror.c
	IVG/externals/libpng/pngget.c
	IVG/externals/libpng/pngmem.c
	IVG/externals/libpng/pngpread.c
	IVG/externals/libpng/pngread.c
	IVG/externals/libpng/pngrio.c
	IVG/externals/libpng/pngrtran.c
	IVG/externals/libpng/pngrutil.c
	IVG/externals/libpng/pngset.c
	IVG/externals/libpng/pngtrans.c
	IVG/externals/libpng/pngwio.c
	IVG/externals/libpng/pngwrite.c
	IVG/externals/libpng/pngwtran.c
	IVG/externals/libpng/pngwutil.c
	IVG/externals/zlib/adler32.c
	IVG/externals/zlib/compress.c
	IVG/externals/zlib/crc32.c
	IVG/externals/zlib/deflate.c
	IVG/externals/zlib/infback.c
	IVG/externals/zlib/inffast.c
	IVG/externals/zlib/inflate.c
	IVG/externals/zlib/inftrees.c
	IVG/externals/zlib/trees.c
	IVG/externals/zlib/uncompr.c
	IVG/externals/zlib/zutil.c
)

IVG/tools/BuildCpp.sh "$target" "$model" tools/IVG2PNG/IVG2PNG \
	-ffp-contract=off -UTARGET_OS_MAC IVG/tools/IVG2PNG.cpp -DNUXPIXELS_SIMD="$simd_flag" \
	-I IVG -I IVG/externals -I IVG/externals/libpng -I IVG/externals/zlib \
	IVG/src/IVG.cpp IVG/src/IMPD.cpp IVG/externals/NuX/NuXPixels.cpp \
	"${c_sources[@]}"

