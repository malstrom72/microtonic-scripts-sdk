#ifndef Makaron_h
#define Makaron_h

#include "assert.h" // Note: I always include assert.h like this so that you can override it with a "local" file.
#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>

namespace Makaron {

const int DEFAULT_RECURSION_DEPTH_LIMIT = 20;

typedef char Char;
typedef std::basic_string<Char> String;
typedef String::const_iterator StringIt;

class Exception : public std::exception {
	public:		Exception(const std::string& error, const std::string& file, size_t offset, int line, int column)
						: error(error), file(file), offset(offset), line(line), column(column) { }
				std::string getError() const { return error; }
				std::string getFile() const { return file; }
				size_t getOffset() const { return offset; }	// To get recursive source ranges, use the length of the generated output with findInputRanges instead.
				int getLineNumber() const { return line; }
				int getColumnNumber() const { return column; }
				virtual const char* what() const throw();
				virtual ~Exception() throw() { }
	
	protected:	const std::string error;
				const std::string file;
				const size_t offset;
				const int line;
				const int column;
				mutable std::string errorWithLine;
};

class Span {
	friend class Context;
	public:		Span(const String& sourceCode, const String& fileName);
				Span(const Span& s, const StringIt& b, const StringIt& e) : source(s.source), file(s.file)
						, begin(b), end(e) {
					assert(begin >= source->begin() && end <= source->end());
				}
				operator String() const { return String(begin, end); }
				operator String() { return String(begin, end); }
				size_t sourceOffset(const StringIt& p) { return p - source->begin(); }

	protected:	Span() { }
				std::shared_ptr<const String> source;
				std::shared_ptr<const String> file;
				StringIt begin;
				StringIt end;
};

/**
	It is ok for a stretch from `outputPoint` to (`outputPoint`+`outputStretch`) to go one character past the generated
	output length. This means an error occured during output generation when processing this entry's input range.
	This entry should be included in the error diagnostics.
 
	If `inputLength` == 0 for passed text: inputOffset = inputFrom + (outputPoint - outputPoint)
	If `inputLength` > 0 for macro invokation: inputRange = inputFrom .. inputFrom + inputLength
 
	Generated offset map vectors will always be sorted by `outputPoint` ascending. With overlapping entries, the
	earliest element is the "outermost" (e.g. the first macro call) and the last element is the "innermost".
*/
struct OffsetMapEntry {
	std::shared_ptr<const String> file;
	size_t outputPoint;
	size_t outputStretch;
	size_t inputFrom;
	size_t inputLength;
};

class Context {
	protected:	struct Macro {
					std::vector<String> params;
					Span span;
					Context* context;
				};
	
	public:		typedef std::function<bool (const String& fileName, String& contents)> LoaderFunction;
	
				Context(int depthLimiter = DEFAULT_RECURSION_DEPTH_LIMIT, Context* parentContext = 0);
				bool defineMacro(const String& name, const std::vector<String>& parameterNames, const Span& span, Context* context);
				bool defineString(const String& name, const String& definiton);
				bool redefineString(const String& name, const String& definiton);
				void process(const Span& input, String& output, std::vector<OffsetMapEntry>* offsetMap);
				void setIncludeLoader(const LoaderFunction& loaderFunction);
	
	protected:	static bool isWhite(const Char c);
				static bool isLeadingIdentifierChar(const Char c);
				static bool isIdentifierChar(const Char c);
				void error(const std::string error);
				bool eof() const;
				void skipWhite();
				void skipHorizontalWhite();
				void optionalLineBreak();
				void skipBracketsAndStrings(int depth);
				bool parseToken(const char* token);
				String parseIdentifier();
				String parseSymbol();
				StringIt skipNested(const char* open, const char* close, bool skipLeadingWhite);
				Span parseNested(const char* open, const char* close, bool skipLeadingWhite);
				String parseExpression(const char* terminators);
				void parseArgumentList(std::vector<String>& arguments);
				void parseParameterNames(std::vector<String>& parameterNames);
				void stringDefinition(bool redefine);
				void macroDefinition();
				bool testCondition();
				void ifStatement();
				void invokeMacro();
				void includeFile();
				void produce(const StringIt& b, const StringIt& e);

				Context* const parentContext;
				int depthLimiter;
				LoaderFunction loader;
				std::map<String, Macro> macros;
				std::map<String, String> strings;
				Span processing;
				String* processed;
				std::vector<OffsetMapEntry>* offsets;
				StringIt p;
};

// FIX : RangeVector should include file name too
typedef std::vector< std::pair<size_t, size_t> > RangeVector;

/**
	Returns ranges in input text as absolute [begin, end) offsets for an offset in output text. In case of macro calls,
	first range returned is outermost and last is innermost.
 
	Notice that you can use this routine when an error occurs to get a full "call stack" of source ranges. Just pass the
	length of the generated output to `outputOffset`.
*/
RangeVector findInputRanges(const std::vector<OffsetMapEntry>& offsetMap, size_t outputOffset);

std::pair<int, int> calculateLineAndColumn(const String& text, size_t offset);
String process(const String& source, const String& fileName);
bool unitTest();

} // namespace Makaron

#endif
