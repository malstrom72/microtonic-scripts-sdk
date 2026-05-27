/**
	IMPD is released under the BSD 2-Clause License.
	
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

#ifndef IMPD_h
#define IMPD_h

#include "assert.h" // Note: I always include assert.h like this so that you can override it with a "local" file.
#include <climits>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdint.h>

namespace IMPD {

template<typename T, typename U> T lossless_cast(U x) {
	assert(static_cast<U>(static_cast<T>(x)) == x);
	return static_cast<T>(x);
}

typedef char Char;
typedef wchar_t WideChar;																								// UTF16 or UTF32 depending on platform
typedef char32_t UniChar;																								// UTF32
typedef std::basic_string<Char> String;
typedef std::basic_string<WideChar> WideString;
typedef std::basic_string<UniChar> UniString;
typedef String::const_iterator StringIt;
typedef std::vector<String> StringVector;
typedef std::map<String, String> StringStringMap;

const int DEFAULT_STATEMENTS_LIMIT = 1000000;																			// To prevent endless loops. 1 million instructions can take quite a while, but better than crashing. If your data require more than a million statements to execute you are probably doing it wrong.
const int DEFAULT_RECURSION_LIMIT = 50;																					// To prevent stack overflow. If you can't describe your data without 50 times recursion, you are doing it wrong.
const int NUMBER_PRECISION_DIGITS = 13;
const double NUMBER_PRECISION_MAGNITUDE = 1e-13;

const int MATH_FUNCTION_COUNT = 17;
const int BUILT_IN_INSTRUCTION_COUNT = 12;
const int ESCAPE_CODE_COUNT = 7;

WideString convertUniToWideString(const UniString& s);
UniString convertWideToUniString(const WideString& s);

/**
	Helper representing a pair of iterators into a string.
**/
struct StringRange {
	StringRange(const StringIt& b, const StringIt& e) : b(b), e(e) { }
	StringRange(const String& s) : b(s.begin()), e(s.end()) { }
	operator String() const { return String(b, e); }
	operator String() { return String(b, e); }
	StringIt b;
	StringIt e;
};

/**
	Base class for all IMPD specific exceptions carrying an optional statement.
**/
class Exception : public std::exception {
	friend class Interpreter;
	protected:	Exception(const std::string& error) : error(error) { }
	protected:	Exception(const Exception& x, const String& statement) : error(x.error), statement(statement) { }
	protected:	Exception(const std::string& error, const String& statement) : error(error), statement(statement) { }
	public:		std::string getError() const throw() { return error; }
	public:		bool hasStatement() const throw() { return !statement.empty(); }
	public:		String getStatement() const throw() { return statement; }
	public:		virtual const char* what() const throw() { return error.c_str(); }
	public:		virtual ~Exception() throw() { }
	protected:	std::string error;
	protected:	String statement;
};

struct SyntaxException : public Exception { SyntaxException(const std::string& error) : Exception(error) { } };			///< SyntaxException is thrown when file data cannot be parsed properly.
struct RunTimeException : public Exception { RunTimeException(const std::string& error) : Exception(error) { } };		///< Run-time exceptions are caused by erraneous dynamic processing. E.g. variable contents is of wrong type for operation.
struct AbortedException : public Exception { AbortedException(const std::string& error) : Exception(error) { } };		///< AbortedException is thrown by a 'stop' intruction or when an Executor returns false from progress()
struct FormatException : public Exception { FormatException(const std::string& error) : Exception(error) { } };			///< FormatException is thrown when the 'format' instruction indicates that the data format is unsupported.

class Interpreter;

struct Argument {
	Argument() { }
	Argument(const String& label, const String& value) : label(label), value(value) { }
	String label;
	String value;
};
typedef std::vector<Argument> ArgumentVector;

/**
	Tracks format declarations for the current document scope.
**/
struct FormatInfo {
	FormatInfo() { }
	String formatId;
	std::set<String> uses;
	std::set<String> requires;
};

/**
	Stores parsed arguments and tracks which entries have been consumed.
	Fetching an argument marks it as used so missing or surplus parameters can be detected afterwards.
**/
class ArgumentsContainer {
	public:		static ArgumentsContainer parse(const Interpreter& interpreter, const StringRange& range);		///< Parse a raw argument string and normalize it for indexed and labeled lookups.
	public:		ArgumentsContainer(const Interpreter& interpreter, const ArgumentVector& arguments);			///< Construct a container from an already tokenized argument list.
	public:		const String* fetchOptional(int index, bool expand = true);										///< Fetch the N:th positional argument if present, returning 0 otherwise.
	public:		const String& fetchRequired(int index, bool expand = true);										///< Fetch the N:th positional argument or throw when it is missing.
	public:		const String* fetchOptional(const String& label, bool expand = true);							///< Fetch an optional labeled argument, returning 0 when the label is absent.
	public:		const String& fetchRequired(const String& label, bool expand = true);							///< Fetch a labeled argument or throw when the label was not provided.
	public:		void throwIfAnyUnfetched();																		///< Throw if unused arguments remain. Always call after consuming all expected inputs.
	public:		void throwIfNoneFetched();																		///< Throw if nothing was consumed. Unnecessary to call if you used fetchRequired() at least once.
	protected:	struct ArgumentExtra {
					ArgumentExtra() : hasFetched(false), hasExpanded(false) { }
					bool hasFetched;
					bool hasExpanded;
					String expanded;
				};
	protected:	void construct(const ArgumentVector& arguments);
	protected:	const String& fetch(int i, bool expand);
	protected:	const Interpreter& interpreter;
	protected:	ArgumentVector arguments;
	protected:	std::vector<int> indexed;
	protected:	std::map<String, int> labeled;
	protected:	std::vector<ArgumentExtra> extra;
	protected:	int unfetchedCount;
};

/**
	Interface representing a storage backend for interpreter variables.
**/
class Variables {
	public:		virtual bool declare(const String& var, const String& value) = 0;										///< Create a new variable and assign value. Variable must not already exist. Return false if it does exist.
	public:		virtual bool assign(const String& var, const String& value) = 0;										///< Assign value to an existing variable. Variable must exist. Return false if variable does not exist.
	public:		virtual bool lookup(const String& var, String& value) const = 0;										///< Load value of an existing variable into `value`. Return false (and do not touch `value`) if the variable does not exist.
	public:		virtual ~Variables() { }
};

/**
	Simple variable store implemented using an STL map.
**/
class STLMapVariables : public Variables {
	public:		virtual bool declare(const String& var, const String& value);
	public:		virtual bool assign(const String& var, const String& value);
	public:		virtual bool lookup(const String& var, String& value) const;
	protected:	StringStringMap vars;
};

/**
	Abstract interface for executing instructions and loading resources.
**/
class Executor {
	public:		virtual bool format(Interpreter& interpreter, const FormatInfo& formatInfo) = 0;						///< Return false to throw FormatException if the format is not supported. `formatInfo` contains the normalized identifier, declared `uses:` tokens, and the filtered `requires:` set.
	public:		virtual bool execute(Interpreter& interpreter, const String& instruction, const String& arguments) = 0; ///< Return false to throw SyntaxException if instruction is unrecognized. `instruction` is passed in lower case.
	public:		virtual bool progress(Interpreter& interpreter, int maxStatementsLeft) = 0;								///< Called before every statement is executed. Return false to stop processing and throw AbortedException.
	public:		virtual bool load(Interpreter& interpreter, const WideString& filename, String& contents) = 0;			///< Called by the INCLUDE instruction. Load contents of file into `contents`. Return false to throw a RunTimeException.
	public:		virtual void trace(Interpreter& interpreter, const WideString& s) = 0;									///< Used for debugging. Trace `s` to standard out, any log-files etc...
	public:		virtual bool meta(Interpreter& interpreter, const String& key, const String& arguments) = 0;			///< Used for passing meta-data from the IMPD script to the executor. `key` is passed in lower case (and will end with `-n` version number if declared in `format uses:`). `arguments` is the raw argument string (may be empty). Return false if the meta tag is unrecognized (not an error, but may trace a warning).
	public:		virtual ~Executor() { }
};

/**
	Executes IMPD scripts using an external Executor and variable store.
**/
class Interpreter {
	public:		static const String CURRENT_IMPD_REQUIRES_ID;
	public:		static const String YES_STRING;
	public:		static const String NO_STRING;

	public:		static void throwBadSyntax(const String& how);												///< Bad syntax should be thrown when file data cannot be parsed properly. E.g. missing arguments etc.
	public:		static void throwBadSyntax(const char* how);
	public:		static void throwRunTimeError(const String& how);											///< Run-time error should be thrown when dynamic processing fails. E.g. variable contents is of wrong type for operation.
	public:		static void throwRunTimeError(const char* how);

	public:		static String toString(bool b);																			///< Returns "yes" or "no".
	public:		static String toString(int32_t i, int radix = 10, int minLength = 1);									///< Converts integer to string with choosable `radix` (from 2 to 16). `minLength` should be between 0 and 8 * sizeof (int).
	public:		static String toString(double d, int precision = NUMBER_PRECISION_DIGITS);								///< Converts double to string in scientific e notation, e.g. -12.34e-3.

	public:		static bool toBool(const String& s);																	///< Tries to convert string (either "yes" or "no") to boolean. Throws a run-time error if the string isn't "yes" or "no".
	public:		static int toInt(const StringRange& r);																	///< Tries to convert a string range to signed integer. Throws a run-time error if the string range could not be fully converted. Notice that a StringRange can be implicitly constructed from String, so you can pass a String as the argument to this function as well. See also parseInt().
	public:		static double toDouble(const StringRange& r);															///< Tries to convert a string range to double. Throws a run-time error if the string could not be fully converted. Notice that a StringRange can be implicitly constructed from String, so you can pass a String as the argument to this function as well. See also parseDouble().
	public:		static String toLower(const StringRange& r);															///< Converts string to lower case. Only ASCII characters 'A' to 'Z' will be treated.
	public:		static UniString unescapeToUni(const StringRange& r);													///< Converts any escaped characters in a string to their unicode values. Special escape sequences are: @code \a \b \f \n \r \t \v \xHH \uHHHH \UHHHHHHHH \<decimal> @endcode. Any other escaped character is simply replaced by itself (without the backslash). Notice that a StringRange can be implicitly constructed from String, so you can pass a String as the argument to this function as well. Returned format is UTF32.
	public:		static WideString unescapeToWide(const StringRange& r);													///< Like unescapeToUni(), but returned type is std::wstring, which may be UTF32 or UTF16 depending on target platform.

	public:		static StringIt parseHex(StringIt p, const StringIt& e, uint32_t& i);									///< Parses and converts as much as possible of hexadecimal string starting at `p` and ending at `e` into an unsigned int. Returns an iterator pointing to the first character that could not be parsed.
	public:		static StringIt parseUnsignedInt(StringIt p, const StringIt& e, uint32_t& i);							///< Parses and converts as much as possible of decimal string starting at `p` and ending at `e` into an unsigned int (does not accept leading '+' or '-'). Returns an iterator pointing to the first character that could not be parsed.
	public:		static StringIt parseInt(StringIt p, const StringIt& e, int& i);										///< Parses and converts as much as possible of decimal string starting at `p` and ending at `e` into a signed int (accepts leading '+' or '-'). Returns an iterator pointing to the first character that could not be parsed.
	public:		static StringIt parseDouble(StringIt p, const StringIt& e, double& d);			/// Parses a floating point string starting at `p` and ending at `e` (supports scientific e notation). Returns an iterator pointing to the first character that could not be parsed. Returns `p` on failure.
		
	public:		Interpreter(Executor& executor, Variables& vars, FormatInfo& formatInfo
						, int statementsLimit = DEFAULT_STATEMENTS_LIMIT
						, int recursionLimit = DEFAULT_RECURSION_LIMIT);												///< Constructs a root interpreter. The root interpreter uses the global variables referenced to by `vars`.
	public:		Interpreter(Executor& executor, Variables& vars, Interpreter& callingFrame);
	public:		Interpreter(Executor& executor, Interpreter& enclosingInterpreter);
	public:		Interpreter(Executor& executor, Interpreter& enclosingInterpreter, FormatInfo& formatInfo);
	public:		Executor& getExecutor() const { return executor; }
	public:		Variables& getVariables() const { return vars; }
	public:		FormatInfo& getFormatInfo() { return formatInfo; }
	public:		const FormatInfo& getFormatInfo() const { return formatInfo; }
	public:		int mapArguments(const ArgumentVector& allArguments, StringStringMap& labeledArguments
						, StringVector& indexedArguments);																///< `labeledArguments` will map the labels converted to all lower case
	public:		void parseArguments(const StringRange& r, ArgumentVector& arguments) const;
	public:		int parseList(const StringRange& r, StringVector& elements, bool expandAll = false
						, bool removeEmpty = false, int minElements = 0, int maxElements = INT_MAX) const;
	public:		String expand(const StringRange& s) const;
	public:		void set(const String& name, const String& value);
	public:		String get(const String& name) const;
	public:		void run(const StringRange& r);

	protected:	class EvaluationValue;
	protected:	static bool isComment(StringIt p, const StringIt& e);
	protected:	static bool isSymbolLetter(Char c);																		///< _ is considered a letter in this context, . - and 0 to 9 are not
	protected:	static StringIt eatComment(StringIt p, const StringIt& e);
	protected:	static StringIt eatWhite(StringIt p, const StringIt& e);
	protected:	static StringIt eatEscape(StringIt p, const StringIt& e);
	protected:	static StringIt eatSymbol(StringIt p, const StringIt& e);
	protected:	static StringIt eatSymbolForAssignment(StringIt p, const StringIt& e);									///< Doesn't accept leading '-' or 0 to 9 to prevent common accidental errors like $x = 3 (where x is a numeric value) from going unnoticed.
	protected:	static StringIt eatBlock(StringIt p, const StringIt& e);
	protected:	static StringIt eatQuotedString(StringIt p, const StringIt& e);
	protected:	static StringIt eatArgumentValue(StringIt p, const StringIt& e);
	protected:	static StringIt eatListElement(StringIt p, const StringIt& e);
	protected:	static StringIt eatStatement(StringIt p, const StringIt& e);
	protected:	static StringIt unescapeChar(StringIt p, const StringIt& e, UniChar& c);
	protected:	enum Precedence {
					BRACKETS, CONDITIONAL, CONCAT, BOOLEAN, COMPARE, ADD_SUB, MUL_DIV_MOD
					, PREFIX, POSTFIX, POW, EXPAND, SPLICE, FUNCTION
				};
	protected:	String performExpansion(const StringRange& r) const;
	protected:	void runInstruction(const String& instruction, const StringRange& argumentsRange);
	protected:	void runStatement(const StringRange& r);
	protected:	bool evaluationValueToNumber(const EvaluationValue& v, double& d, String& s) const;
	protected:	StringIt numericOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
						, Char op, Precedence opPrecedence, bool dry) const;
	protected:	StringIt moduloPercentOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt concatOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, Precedence concatType, bool dry) const;
	protected:	StringIt comparisonOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt booleanOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt conditionalOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt substringOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt evaluateInner(StringIt b, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const;
	protected:	StringIt evaluateOuter(StringIt b, const StringIt& e, EvaluationValue& v, bool dry) const;
	protected:	Executor& executor;
	protected:	Variables& vars;
	protected:	FormatInfo& formatInfo;
	protected:	Interpreter* callingFrame;
	protected:	Interpreter& rootFrame;
	protected:	int statementsLimit;
	protected:	int recursionLimit;

	protected:	enum BuiltInInstruction {
					DEBUG_INSTRUCTION, CALL_INSTRUCTION, FOR_INSTRUCTION, FORMAT_INSTRUCTION, IF_INSTRUCTION
					, INCLUDE_INSTRUCTION, LOCAL_INSTRUCTION, META_INSTRUCTION, REPEAT_INSTRUCTION, RETURN_INSTRUCTION
					, STOP_INSTRUCTION, TRACE_INSTRUCTION
				};
	protected:	static const char* BUILT_IN_INSTRUCTION_STRINGS[BUILT_IN_INSTRUCTION_COUNT];
	protected:	static double (*MATH_FUNCTION_POINTERS[MATH_FUNCTION_COUNT])(double);
	protected:	static int findBuiltInInstruction(int n, const char* s);
	protected:	static int findFunction(int n, const char* s);
	protected:	static const Char ESCAPE_CHARS[ESCAPE_CODE_COUNT];
	protected:	static const Char ESCAPE_CODES[ESCAPE_CODE_COUNT];
};

} // namespace IMPD

#endif
