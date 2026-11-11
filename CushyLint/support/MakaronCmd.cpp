// FUTURE : allow multiple input files

#include <iostream>
#include <fstream>
#include <string>
#include <istream>
#include <ostream>
#include <iterator>
#include <memory>
#include <map>
#include <cstring>
#include <algorithm>
#include "Makaron.h"

#ifdef _WIN32
const char SEPARATOR_CHARACTER = '\\';
#else
const char SEPARATOR_CHARACTER = '/';
#endif

std::vector<std::string> includePaths;
std::map<std::string, std::string> loadedSources;																		// by file name, for line and column in error traces
std::vector<std::string> loadedPaths;																					// every included file by the path that opened it, in order

static std::string loadEntireStream(std::istream& stream) {
	std::istreambuf_iterator<Makaron::Char> it(stream);
	std::istreambuf_iterator<Makaron::Char> end;
	return std::string(it, end);
}


static bool openForWriting(std::ofstream& stream, const std::string& path, const char* what) {
	stream.open(path);
	if (!stream.good()) {
		std::cerr << "Could not open " << what << " file" << std::endl;
		return false;
	}
	stream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
	return true;
}

static std::string narrowString(const std::wstring& wide) {																// file names, which Makaron widens byte by byte
	std::string narrow;
	narrow.reserve(wide.size());
	for (std::wstring::const_iterator it = wide.begin(); it != wide.end(); ++it) {
		narrow += static_cast<char>(*it);
	}
	return narrow;
}

static bool myIncludeLoader(const Makaron::WideString& fileName, Makaron::String& contents) {
	const std::string narrowFileName = narrowString(fileName);
	for (std::vector<std::string>::const_iterator it = includePaths.begin(); it != includePaths.end(); ++it) {
		std::ifstream fileStream(*it + narrowFileName);
		if (fileStream.good()) {
			fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
			contents = loadEntireStream(fileStream);
			loadedSources[narrowFileName] = contents;
			if (std::find(loadedPaths.begin(), loadedPaths.end(), *it + narrowFileName) == loadedPaths.end()) {
				loadedPaths.push_back(*it + narrowFileName);
			}
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
		std::string dependencyPath;
		bool checkOnly = false;
		int argi = 1;
		while (argi < argc) {
			if (strcmp(argv[argi], "-c") == 0) {
				checkOnly = true;
				++argi;
			} else if (strcmp(argv[argi], "-M") == 0) {
				++argi;
				if (argi < argc) {
					dependencyPath = argv[argi];
					++argi;
				}
			} else if (strcmp(argv[argi], "-m") == 0) {
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
			std::cerr << "Makaron [-c] [-m <map file>] [-M <dependency file>] [-d <name>=<value> ...]"
					<< " [-i <additional include path>] <input file>|- [<output file>|-]" << std::endl;
			std::cerr << "-c checks only and writes no output; -M lists the input and every included file, one per line"
					<< std::endl
					<< "map lines: <output start>:<output end> (<input start>+|<span begin>:<span end>)"
					<< "[ <file, when not the input file>]" << std::endl;
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
			
			loadedSources[fileName] = source;
			context.process(Makaron::Span(source, Makaron::WideString(fileName.begin(), fileName.end())), processed
					, &offsetMap);
		}
		catch (const Makaron::Exception& x) {
			std::cerr << "!!!! Makaron error: " << x.getError() << std::endl
					<< "File: " << narrowString(x.getFile())
					<< ", line: " << x.getLineNumber() << ", column: " << x.getColumnNumber()
					<< " (@" << x.getOffset() << ")" << std::endl
					<< std::endl
					<< "Trace:" << std::endl;
			Makaron::RangeVector inputRanges = findInputRanges(offsetMap, processed.size());
			for (Makaron::RangeVector::const_iterator it = inputRanges.begin(); it != inputRanges.end(); ++it) {
				const std::string rangeFile = narrowString(*it->file);
				const std::map<std::string, std::string>::const_iterator loaded = loadedSources.find(rangeFile);
				std::cerr << "File: " << rangeFile;
				if (loaded != loadedSources.end()) {
					const std::pair<int, int> at = Makaron::calculateLineAndColumn(loaded->second, it->begin);
					std::cerr << ", line: " << at.first << ", column: " << at.second;
				}
				std::cerr << " (@" << it->begin;
				if (it->end > it->begin + 1) {
					std::cerr << ".." << it->end;
				}
				std::cerr << ')' << std::endl;
			}
			rc = 1;
		}

		if (!mapPath.empty()) {
			std::ofstream fileStream;
			if (!openForWriting(fileStream, mapPath, "map")) {
				return 1;
			}
			const Makaron::WideString inputFile(fileName.begin(), fileName.end());
			typedef std::vector<Makaron::OffsetMapEntry>::const_iterator EntryIt;
			for (EntryIt it = offsetMap.begin(); it != offsetMap.end(); ++it) {
				fileStream << it->outputPoint << ':' << (it->outputPoint + it->outputStretch) << ' ' << it->inputFrom;
				if (it->inputLength == 0) {
					fileStream << '+';
				} else {
					fileStream << ':' << (it->inputFrom + it->inputLength);
				}
				if (*it->file != inputFile) {																			// an included file is named, the input file is not
					fileStream << ' ' << narrowString(*it->file);
				}
				fileStream << std::endl;
			}
		}

		if (!dependencyPath.empty()) {
			std::ofstream fileStream;
			if (!openForWriting(fileStream, dependencyPath, "dependency")) {
				return 1;
			}
			fileStream << fileName << std::endl;
			for (std::vector<std::string>::const_iterator it = loadedPaths.begin(); it != loadedPaths.end(); ++it) {
				fileStream << *it << std::endl;
			}
		}

		if (!checkOnly && (outputPath.empty() || outputPath == "-")) {
			std::cout << processed;
		} else if (!checkOnly) {
			std::ofstream fileStream;
			if (!openForWriting(fileStream, outputPath, "output")) {
				return 1;
			}
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

