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
const size_t OUTPUT_LIMIT = 64 * 1024 * 1024;	// bytes a context and every context it creates may produce together

typedef char Char;
typedef wchar_t WideChar;
typedef std::basic_string<Char> String;
typedef std::basic_string<WideChar> WideString;
typedef String::const_iterator StringIt;

class Exception : public std::exception {
	public:		Exception(const std::string& error, const std::wstring& file, size_t offset, int line, int column)
						: error(error), file(file), offset(offset), line(line), column(column) { }
				std::string getError() const { return error; }
				std::wstring getFile() const { return file; }
				size_t getOffset() const { return offset; }	// To get recursive source ranges, use the length of the generated output with findInputRanges instead.
				int getLineNumber() const { return line; }
				int getColumnNumber() const { return column; }
				virtual const char* what() const throw();
				virtual ~Exception() throw() { }
	
	protected:	const std::string error;
				const std::wstring file;
				const size_t offset;
				const int line;
				const int column;
				mutable std::string errorWithLine;
};

class Span {
	friend class Context;
	public:		Span(const String& sourceCode, const WideString& fileName);
				Span(const Span& s, const StringIt& b, const StringIt& e) : source(s.source), file(s.file)
						, begin(b), end(e) {
					assert(begin >= source->begin() && end <= source->end());
				}
				operator String() const { return String(begin, end); }
				operator String() { return String(begin, end); }
				size_t sourceOffset(const StringIt& p) { return p - source->begin(); }

	protected:	Span() { }
				std::shared_ptr<const String> source;
				std::shared_ptr<const WideString> file;
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
	std::shared_ptr<const WideString> file;
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
	
	public:		typedef std::function<bool (const WideString& fileName, String& contents)> LoaderFunction;
	
				Context(int depthLimiter = DEFAULT_RECURSION_DEPTH_LIMIT,
						Context* parentContext = 0);		/// init with depth limit and parent
				bool defineMacro(const String& name, const std::vector<String>& parameterNames,
						const Span& span, Context* context);		/// register macro; false if name exists
				bool defineString(const String& name, const String& definiton);		/// define string constant; false if name used
				bool redefineString(const String& name, const String& definiton);		/// replace string; false if missing
				void process(const Span& input, String& output,
						std::vector<OffsetMapEntry>* offsetMap);		/// expand input; fill offsets if provided
				void setIncludeLoader(const LoaderFunction& loaderFunction);		/// set loader used by @include
	
	protected:	static bool isWhite(const Char c);		/// true if `c` is whitespace
				static bool isLeadingIdentifierChar(const Char c);		/// true if `c` can start identifier
				static bool isIdentifierChar(const Char c);		/// true if `c` is identifier char
				void error(const std::string error);		/// throw `Exception` at current location
				bool eof() const;		/// true when parser reached end
				void skipWhite();		/// skip all whitespace
				void skipHorizontalWhite();		/// skip spaces and tabs
				void optionalLineBreak();		/// skip optional line break
				void skipBracketsAndStrings(int depth);		/// skip bracketed or quoted blocks
				bool parseToken(const char* token);		/// consume token if present
				String parseIdentifier();		/// read identifier; empty if none
				String parseSymbol();		/// read identifier or `(expr)`
				StringIt skipNested(const char* open, const char* close,
						bool skipLeadingWhite);		/// skip nested pair and return end
				Span parseNested(const char* open, const char* close,
						bool skipLeadingWhite);		/// span inside nested pair
				String parseExpression(const char* terminators);		/// parse expression until terminator
				void parseArgumentList(std::vector<String>& arguments);		/// parse comma-separated args
				void parseParameterNames(std::vector<String>& parameterNames);		/// parse comma-separated names
				void stringDefinition(bool redefine);		/// handle string @define/@redefine
				void macroDefinition();		/// parse and store macro
				bool testCondition();		/// evaluate @if/@elif expression
				void ifStatement();		/// process @if...@endif
				void invokeMacro();		/// expand macro or string
				void includeFile();		/// handle @include directive
				void produce(const StringIt& b, const StringIt& e);		/// append source slice to output
				void spend(size_t byteCount);		// counts output toward the shared limit; throws when it is reached

				Context* const parentContext;
				Context* const rootContext;
				int depthLimiter;
				size_t outputLimiter;	// bytes left to produce, used in `rootContext` only
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
RangeVector findInputRanges(const std::vector<OffsetMapEntry>& offsetMap,
				size_t outputOffset);	/// map output offset to input ranges

std::pair<int, int> calculateLineAndColumn(const String& text, size_t offset);	/// get line and column for `offset`
String process(const String& source, const WideString& fileName);	/// convenience wrapper using default context
bool unitTest();	/// run built-in tests

} // namespace Makaron

#endif
