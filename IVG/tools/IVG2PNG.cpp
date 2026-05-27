/**
	IVG is released under the BSD 2-Clause License.

	Copyright (c) 2013-2025, Magnus Lidström

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include "src/IVG.h"
#include "png.h"
#include "zlib.h"

using namespace std;
using namespace IVG;
using namespace IMPD;
using namespace NuXPixels;

// Important! Make sure to compile with exceptions enabled for C code. E.g. in GCC -fexceptions (GCC_ENABLE_EXCEPTIONS)

static void PNGAPI myPNGErrorFunction(png_struct* png_ptr, png_const_charp error_msg) {
	throw std::runtime_error(std::string("Error writing PNG image : ") + std::string(static_cast<const char*>(error_msg)));
}

static bool isLittleEndian() {
	assert(sizeof (unsigned int) == 4);
	static const unsigned char bytes[4] = { 0x4A, 0x3B, 0x2C, 0x1D };
	if (*reinterpret_cast<const unsigned int*>(bytes) == 0x1D2C3B4A) {
		return true;
	} else {
		assert(*reinterpret_cast<const unsigned int*>(bytes) == 0x4A3B2C1D);
		return false;
	}
}

class IVGExecutorWithExternalFonts : public IVGExecutor {
	public:
		IVGExecutorWithExternalFonts(Canvas& canvas, const std::string& fontPath,
			    const AffineTransformation& xform = AffineTransformation())
			    : IVGExecutor(canvas, xform), fontPath(fontPath) {
		}
		virtual std::vector<const Font*> lookupFonts(IMPD::Interpreter& interpreter, const IMPD::WideString& fontName,
			    const IMPD::UniString& forString) {
			(void)interpreter;
			std::pair< FontMap::iterator, bool > insertResult = loadedFonts.insert(std::make_pair(fontName, Font()));
			if (insertResult.second) {
			    const std::string fontName8Bit(fontName.begin(), fontName.end());
			    String fontCode;
			    {
			        std::string path = fontPath.empty() ? (fontName8Bit + ".ivgfont") : (fontPath + "/" + fontName8Bit + ".ivgfont");
			        std::ifstream fileStream(path.c_str());
			        if (!fileStream.good()) {
			            return std::vector<const Font*>();
			        }
			        fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			        const std::istreambuf_iterator<Char> it(fileStream);
			        const std::istreambuf_iterator<Char> end;
			        fontCode = std::string(it, end);
			    }
			    std::wcerr << "parsing external font " << fontName << std::endl;
			    FontParser fontParser;
			    STLMapVariables vars;
			    FormatInfo formatInfo;
			    Interpreter impd(fontParser, vars, formatInfo);
			    impd.run(fontCode);
			    insertResult.first->second = fontParser.finalizeFont();
			}
			return std::vector<const Font*>(1, &insertResult.first->second);
		}
	protected:
		FontMap loadedFonts;
		std::string fontPath;
};


#ifdef LIBFUZZ
struct FuzzerExecutor : public IVGExecutor {
	FuzzerExecutor(Canvas& canvas, const NuXPixels::AffineTransformation& initialTransform = NuXPixels::AffineTransformation())
			: IVGExecutor(canvas, initialTransform) { }
	virtual void trace(IMPD::Interpreter& interpreter, const IMPD::WideString& s) { }
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	const std::string ivgSource(reinterpret_cast<const char*>(Data), reinterpret_cast<const char*>(Data) + Size);
	SelfContainedARGB32Canvas canvas;
	try {
		{
			STLMapVariables topVars;
			FuzzerExecutor ivgExecutor(canvas);
			FormatInfo formatInfo;
			Interpreter impd(ivgExecutor, topVars, formatInfo);
			impd.run(ivgSource);
		}
	}
	catch (IMPD::Exception&) {
		;
	}
	return 0;
}
#endif

#ifndef LIBFUZZ
int main(int argc, const char* argv[]) {
	try {
		const char* usage = "Usage: IVG2PNG [--fast] [--fonts <dir>] [--background <color>] <input.ivg> <output.png>\n\nVery simple!\n\n";
		const char* inputPath = 0;
		const char* outputPath = 0;
		ARGB32::Pixel background = 0;
		bool haveBackground = false;
		std::string fontPath;
		int compressionLevel = Z_BEST_COMPRESSION;
		bool fast = false;
		for (int i = 1; i < argc; ++i) {
			std::string arg(argv[i]);
			if (arg == "--fast") {
				fast = true;
				compressionLevel = Z_BEST_SPEED;
			} else if (arg == "--fonts") {
				if (++i == argc) { std::cerr << usage; return 1; }
				fontPath = argv[i];
			} else if (arg == "--background") {
				if (++i == argc) { std::cerr << usage; return 1; }
				background = parseColor(argv[i]);
				haveBackground = true;
			} else if (inputPath == 0) {
				inputPath = argv[i];
			} else if (outputPath == 0) {
				outputPath = argv[i];
			} else {
				std::cerr << usage;
				return 1;
			}
		}
		if (inputPath == 0 || outputPath == 0) {
			std::cerr << usage;
			return 1;
		}

		std::string ivgContents;
		{
			std::ifstream inStream(inputPath);
			if (!inStream.good()) throw std::runtime_error("Could not open input IVG file");
			ivgContents.assign(std::istreambuf_iterator<char>(inStream), std::istreambuf_iterator<char>());
			if (!inStream.good()) throw std::runtime_error("Could not read input IVG file");
			inStream.close();
		}
		std::cerr << "Read source IVG..." << std::endl;

		SelfContainedARGB32Canvas canvas;
		{
			STLMapVariables topVars;
			IVGExecutorWithExternalFonts ivgExecutor(canvas, fontPath);
			FormatInfo formatInfo;
			Interpreter impd(ivgExecutor, topVars, formatInfo);
			impd.run(ivgContents);
		}
		std::cerr << "Rasterized image..." << std::endl;

		SelfContainedRaster<ARGB32>* raster = canvas.accessRaster();
		if (raster == 0) throw std::runtime_error("IVG image is empty");
		IntRect bounds = raster->calcBounds();
		if (bounds.width <= 0 || bounds.height <= 0) throw std::runtime_error("IVG image is empty");

		if (haveBackground) {
			SelfContainedRaster<ARGB32> copy(*raster);
			(*raster) = Solid<ARGB32>(background) | copy;
		}
		std::vector<png_bytep> rowPointers(bounds.height);
		int imageStride = raster->getStride();
		ARGB32::Pixel* pixels = raster->getPixelPointer() + bounds.top * imageStride + bounds.left;
		for (int i = 0; i < bounds.height; ++i) {
			ARGB32::Pixel* p = pixels + i * imageStride;
			rowPointers[i] = reinterpret_cast<png_bytep>(p);
			for (int x = 0; x < bounds.width; ++x) {
				int a = (*p >> 24) & 0xFF;
				int r = (*p >> 16) & 0xFF;
				int g = (*p >> 8) & 0xFF;
				int b = (*p >> 0) & 0xFF;
				if (a != 0xFF && a != 0x00) {
					int m = 0xFFFF / a;
					r = (r * m) >> 8;
					g = (g * m) >> 8;
					b = (b * m) >> 8;
					assert(0 <= r && r < 0x100);
					assert(0 <= g && g < 0x100);
					assert(0 <= b && b < 0x100);
				}
				*p = (a << 24) | (r << 16) | (g << 8) | b;
				++p;
			}
		}
		std::cerr << "Converted to non-premultiplied alpha..." << std::endl;

		{
			FILE* f = NULL;
			png_structp png_ptr = 0;
			png_infop info_ptr = 0;
		
			try {
				f = fopen(outputPath, "wb");
				if (f == NULL) throw std::runtime_error("Could not open output PNG file");

				png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, myPNGErrorFunction, 0);
				if (png_ptr == 0) throw std::runtime_error("Error writing PNG image : could not initialize");

				info_ptr = png_create_info_struct(png_ptr);
				if (info_ptr == 0) throw std::runtime_error("Error writing PNG image : could not initialize");

				png_set_compression_level(png_ptr, compressionLevel);
				if (fast) png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
				png_init_io(png_ptr, f);

				png_set_IHDR(png_ptr, info_ptr, bounds.width, bounds.height, 8, PNG_COLOR_TYPE_RGB_ALPHA
						, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				
				png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr, PNG_sRGB_INTENT_ABSOLUTE);

				png_set_oFFs(png_ptr, info_ptr, bounds.left, bounds.top, PNG_OFFSET_PIXEL);

				png_set_rows(png_ptr, info_ptr, &rowPointers[0]);

				png_write_png(png_ptr, info_ptr, (isLittleEndian() ? PNG_TRANSFORM_BGR : PNG_TRANSFORM_SWAP_ALPHA), NULL);

				png_destroy_write_struct(&png_ptr, &info_ptr);
				fclose(f);
				f = NULL;
			}
			catch (...) {
				png_destroy_write_struct(&png_ptr, &info_ptr);
				if (f != NULL) {
					fclose(f);
					f = NULL;
				}
				throw;
			}
		}
		std::cerr << "Written to PNG." << std::endl;
	}
	catch (const IMPD::Exception& x) {
		std::cerr << "Exception: " << x.what() << std::endl;
		if (x.hasStatement()) std::cerr << "in statement: " << x.getStatement() << std::endl;
		return 1;
	}
	catch (const std::exception& x) {
		std::cerr << "Exception: " << x.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "General exception" << std::endl;
		return 1;
	}
	return 0;
}
#endif
