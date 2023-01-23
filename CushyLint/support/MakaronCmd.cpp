// FUTURE : allow multiple input files, but we need Span to also keep track of which file it is so we get proper error messages and also the map output file needs to include filenames

#include <iostream>
#include <fstream>
#include <string>
#include <istream>
#include <ostream>
#include <iterator>
#include <memory>
#include "Makaron.h"

#ifdef _WIN32
	const char SEPARATOR_CHARACTER = '\\';
#else
	const char SEPARATOR_CHARACTER = '/';
#endif

std::vector<std::string> includePaths;

static std::string loadEntireStream(std::istream& stream) {
	std::istreambuf_iterator<Makaron::Char> it(stream);
	std::istreambuf_iterator<Makaron::Char> end;
	return std::string(it, end);
}

static bool myIncludeLoader(const Makaron::String& fileName, Makaron::String& contents) {
	for (std::vector<std::string>::const_iterator it = includePaths.begin(); it != includePaths.end(); ++it) {
		std::ifstream fileStream(*it + fileName);
		if (fileStream.good()) {
			fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			contents = loadEntireStream(fileStream);
			return true;
		} else if (!fileName.empty() && fileName.front() == SEPARATOR_CHARACTER) { // only use empty path if leading /
			assert(it->empty());
			return false;
		}
	}
	return false;
}


#ifdef LIBFUZZ
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	std::vector<Makaron::OffsetMapEntry> offsetMap;
	Makaron::String processed;
    try {
		Makaron::String source = Makaron::String(reinterpret_cast<const char*>(Data)
				, reinterpret_cast<const char*>(Data) + Size);
		Makaron::Context context;
		context.process(source, processed, &offsetMap);
	}
	catch (const Makaron::Exception& x) {
		Makaron::RangeVector inputRanges = findInputRanges(offsetMap, processed.size());
	}
	return 0;
}
#endif

#ifndef LIBFUZZ
int main(int argc, const char* argv[]) {
	assert(Makaron::unitTest());

	int rc = 0;
	try {
		includePaths.push_back(std::string());
		Makaron::Context context;
		context.setIncludeLoader(myIncludeLoader);

		std::string mapPath;
		int argi = 1;
		while (argi < argc) {
			if (strcmp(argv[argi], "-m") == 0) {
				++argi;
				if (argi < argc) {
					mapPath = argv[argi];
					++argi;
				}
			} else if (strncmp(argv[argi], "-d", 2) == 0) {
				const char* def = &argv[argi][2];
				if (*def == 0) {
					++argi;
					if (argi < argc) {
						def = argv[argi];
					}
				}
				const char* p = def;
				while (*p != 0 && *p != '=') {
					++p;
				}
				const std::string name(def, p);
				std::string value;
				if (*p != 0) {
					++p;
					value = p;
				}
				context.defineString(name, value);
				++argi;
			} else if (strncmp(argv[argi], "-i", 2) == 0) {
				++argi;
				if (argi < argc) {
					std::string path = argv[argi];
					if (path.empty() || path.back() != SEPARATOR_CHARACTER) {
						path += SEPARATOR_CHARACTER;
					}
					includePaths.push_back(path);
					++argi;
				}
			} else {
				break;
			}
		}
		
		if (argi >= argc) {
			std::cerr << "Makaron [-m <map file>] [-d <name>=<value> ...] [-i <additional include path>] <input file>|- [<output file>|-]" << std::endl;
			std::cerr << "map lines: <output start>:<output end> (<input start>+|<span begin>:<span end>)" << std::endl;
			return 1;
		}
		
		std::string inputPath = argv[argi];
		++argi;
		
		std::string outputPath;
		if (argi < argc) {
			outputPath = argv[argi];
			++argi;
		}

		std::vector<Makaron::OffsetMapEntry> offsetMap;
		Makaron::String source;
		Makaron::String fileName;
		Makaron::String processed;
		try {
			if (inputPath.empty() || inputPath == "-") {
				source = loadEntireStream(std::cin);
				fileName = "stdin";
			} else {
				std::ifstream fileStream(inputPath);
				if (!fileStream.good()) {
					std::cerr << "Could not open input file" << std::endl;
					return 1;
				}
				fileName = inputPath;
				fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
				source = loadEntireStream(fileStream);
			}
			
			context.process(Makaron::Span(source, fileName), processed, &offsetMap);
		}
		catch (const Makaron::Exception& x) {
			std::cerr << "!!!! Makaron error: " << x.getError() << std::endl
					<< "File: " << x.getFile() << ", line: " << x.getLineNumber() << ", column: " << x.getColumnNumber()
					<< " (@" << x.getOffset() << ")" << std::endl
					<< std::endl
					<< "Trace:" << std::endl;
			Makaron::RangeVector inputRanges = findInputRanges(offsetMap, processed.size());
			for (Makaron::RangeVector::const_iterator it = inputRanges.begin(); it != inputRanges.end(); ++it) {
				std::pair<int, int> lineAndColumn = Makaron::calculateLineAndColumn(source, it->first);
				std::cerr << "Line: " << lineAndColumn.first << ", column: " << lineAndColumn.second;
				if (it->second > it->first + 1) {
					std::cerr << " (@" << it->first << ".." << it->second << ')' << std::endl;
				} else {
					std::cerr << " (@" << it->first << ')' << std::endl;
				}
			}
			rc = 1;
		}

		if (!mapPath.empty()) {
			std::ofstream fileStream(mapPath);
			if (!fileStream.good()) {
				std::cerr << "Could not open map file" << std::endl;
				return 1;
			}
			fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			for (std::vector<Makaron::OffsetMapEntry>::const_iterator it = offsetMap.begin(); it != offsetMap.end(); ++it) {
				fileStream << it->outputPoint << ':' << (it->outputPoint + it->outputStretch) << ' ' << it->inputFrom;
				if (it->inputLength == 0) {
					fileStream << '+' << std::endl;
				} else {
					fileStream << ':' << (it->inputFrom + it->inputLength) << std::endl;
				}
			}
		}

		if (outputPath.empty() || outputPath == "-") {
			std::cout << processed;
		} else {
			std::ofstream fileStream(outputPath);
			if (!fileStream.good()) {
				std::cerr << "Could not open output file" << std::endl;
				return 1;
			}
			fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			fileStream << processed;
		}
	}
	catch (const std::exception& x) {
		std::cerr << "!!!! Exception: " << x.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "!!!! General exception" << std::endl;
		return 1;
	}
	return rc;
}
#endif

