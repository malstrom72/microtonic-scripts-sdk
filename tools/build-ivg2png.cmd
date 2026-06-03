@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

CD /D "%~dp0.."

SET "target=%~1"
SET "model=%~2"
IF "%target%"=="" SET target=release
IF "%model%"=="" SET model=native

SET "output=tools\IVG2PNG\IVG2PNG.exe"
MKDIR "tools\IVG2PNG" >NUL 2>&1

SET sources=^
	IVG\tools\IVG2PNG.cpp ^
	IVG\src\IVG.cpp ^
	IVG\src\IMPD.cpp ^
	IVG\externals\NuX\NuXPixels.cpp ^
	IVG\externals\libpng\png.c ^
	IVG\externals\libpng\pngerror.c ^
	IVG\externals\libpng\pngget.c ^
	IVG\externals\libpng\pngmem.c ^
	IVG\externals\libpng\pngpread.c ^
	IVG\externals\libpng\pngread.c ^
	IVG\externals\libpng\pngrio.c ^
	IVG\externals\libpng\pngrtran.c ^
	IVG\externals\libpng\pngrutil.c ^
	IVG\externals\libpng\pngset.c ^
	IVG\externals\libpng\pngtrans.c ^
	IVG\externals\libpng\pngwio.c ^
	IVG\externals\libpng\pngwrite.c ^
	IVG\externals\libpng\pngwtran.c ^
	IVG\externals\libpng\pngwutil.c ^
	IVG\externals\zlib\adler32.c ^
	IVG\externals\zlib\compress.c ^
	IVG\externals\zlib\crc32.c ^
	IVG\externals\zlib\deflate.c ^
	IVG\externals\zlib\infback.c ^
	IVG\externals\zlib\inffast.c ^
	IVG\externals\zlib\inflate.c ^
	IVG\externals\zlib\inftrees.c ^
	IVG\externals\zlib\trees.c ^
	IVG\externals\zlib\uncompr.c ^
	IVG\externals\zlib\zutil.c

SET "CPP_OPTIONS=/DNUXPIXELS_SIMD=0 /UTARGET_OS_MAC /I IVG /I IVG\externals /I IVG\externals\libpng /I IVG\externals\zlib"

CALL CushyLint\support\BuildCpp.cmd %target% %model% "%output%" %sources% || EXIT /B 1
