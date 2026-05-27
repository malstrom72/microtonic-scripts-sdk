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

#include <cmath>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#include "IMPD.h"

#if defined(_MSVC_LANG)
	#define CPP_STD _MSVC_LANG
#else
	#define CPP_STD __cplusplus
#endif

#define HAS_CPP11 (CPP_STD >= 201103L)

namespace IMPD {

// Undefine horrible, horrible Microsoft macros.
#undef min
#undef max

using namespace std;

static size_t calcUTF32ToUTF16Size(size_t utf32Size, const UniChar* utf32Chars) {
	size_t n = utf32Size;
	for (size_t i = 0; i < utf32Size; ++i) {
		n += ((utf32Chars[i] >> 16) != 0 ? 1 : 0);
	}
	return n;
}

static size_t convertUTF32ToUTF16(size_t utf32Size, const UniChar* utf32Chars, uint16_t* utf16Chars) {
	size_t outIndex = 0;
	for (size_t i = 0; i < utf32Size; ++i) {
		const UniChar c = utf32Chars[i];
		if ((c >> 16) == 0) {
			utf16Chars[outIndex] = static_cast<uint16_t>(c);
			++outIndex;
		} else {
			utf16Chars[outIndex] = static_cast<uint16_t>(((c - 0x10000) >> 10) + 0xD800);
			utf16Chars[outIndex + 1] = static_cast<uint16_t>(((c - 0x10000) & 0x3FF) + 0xDC00);
			outIndex += 2;
		}
	}
	return outIndex;
}

static size_t calcUTF16ToUTF32Size(size_t utf16Size, const uint16_t* utf16Chars) {
	size_t n = utf16Size;
	for (size_t i = 0; i < utf16Size; ++i) {
		const uint16_t c = utf16Chars[i];
		if (c >= 0xD800 && c <= 0xDBFF) {
			assert(i + 1 < utf16Size && utf16Chars[i + 1] >= 0xDC00 && utf16Chars[i + 1] < 0xE000);	 // Next should be low surrogate.
			++i;
			--n;
		}
	}
	return n;
}

static size_t convertUTF16ToUTF32(size_t utf16Size, const uint16_t* utf16Chars, UniChar* utf32Chars) {
	size_t outIndex = 0;
	for (size_t i = 0; i < utf16Size; ++i) {
		UniChar c = utf16Chars[i];
		if (c >= 0xD800 && c <= 0xDBFF) {
			assert(i + 1 < utf16Size);
			const uint16_t d = utf16Chars[i + 1];
			assert(d >= 0xDC00 && d < 0xE000);	// Next should be low surrogate.
			c = 0x10000 + ((c - 0xD800) << 10) + (d - 0xDC00);
			++i;
		}
		utf32Chars[outIndex] = c;
		++outIndex;
	}
	return outIndex;
}

WideString convertUniToWideString(const UniString& s) {
	if (s.empty()) {
		return WideString();
	} else if (sizeof (WideChar) == sizeof (UniChar)) {
		return WideString(s.begin(), s.end());
	} else if (sizeof (WideChar) == sizeof (uint16_t)) {
		const size_t requiredSize = calcUTF32ToUTF16Size(s.size(), s.data());
		WideString ws(requiredSize, L'\0');
		convertUTF32ToUTF16(s.size(), s.data(), reinterpret_cast<uint16_t*>(&ws[0]));
		return ws;
	} else {
		assert(0);
		return WideString();
	}
}

UniString convertWideToUniString(const WideString& s) {
	if (s.empty()) {
		return UniString();
	} else if (sizeof (WideChar) == sizeof (UniChar)) {
		return UniString(s.begin(), s.end());
	} else if (sizeof (WideChar) == sizeof (uint16_t)) {
		const size_t requiredSize = calcUTF16ToUTF32Size(s.size(), reinterpret_cast<const uint16_t*>(s.data()));
		UniString us(requiredSize, L'\0');
		convertUTF16ToUTF32(s.size(), reinterpret_cast<const uint16_t*>(s.data()), &us[0]);
		return us;
	} else {
		assert(0);
		return UniString();
	}
}

/* --- STLMapVariables --- */

bool STLMapVariables::declare(const String& var, const String& value) {
	return vars.insert(pair<String, String>(var, value)).second;
}

bool STLMapVariables::assign(const String& var, const String& value) {
	StringStringMap::iterator it = vars.find(var);
	if (it == vars.end()) return false;
	it->second = value;
	return true;
}

bool STLMapVariables::lookup(const String& var, String& value) const {
	StringStringMap::const_iterator it = vars.find(var);
	if (it == vars.end()) return false;
	value = it->second;
	return true;
}

/* --- ArgumentsContainer --- */

ArgumentsContainer::ArgumentsContainer(const Interpreter& interpreter, const ArgumentVector& arguments)
		: interpreter(interpreter), arguments(arguments), extra(arguments.size())
		, unfetchedCount(lossless_cast<int>(arguments.size())) {
	for (ArgumentVector::const_iterator it = arguments.begin(), e = arguments.end(); it != e; ++it) {
		if (it->label.empty()) indexed.push_back(lossless_cast<int>(it - arguments.begin()));
		else {
			pair<String, int> kv(interpreter.toLower(it->label), lossless_cast<int>(it - arguments.begin()));
			if (!labeled.insert(kv).second) interpreter.throwBadSyntax(String("Duplicate label: ") + kv.first);
		}
	}
}

ArgumentsContainer ArgumentsContainer::parse(const Interpreter& interpreter, const StringRange& range) {
	ArgumentVector arguments;
	interpreter.parseArguments(range, arguments);
	return ArgumentsContainer(interpreter, arguments);
}

const String& ArgumentsContainer::fetch(int i, bool expand) {
	assert(0 <= i && static_cast<size_t>(i) < arguments.size() && static_cast<size_t>(i) < extra.size());
	ArgumentExtra& x = extra[i];
	if (!x.hasFetched) {
		--unfetchedCount;
		assert(unfetchedCount >= 0);
		x.hasFetched = true;
	}
	if (!expand) return arguments[i].value;
	else {
		if (!x.hasExpanded) {
			x.expanded = interpreter.expand(arguments[i].value);
			x.hasExpanded = true;
		}
		return x.expanded;
	}
}

const String* ArgumentsContainer::fetchOptional(int index, bool expand) {
	assert(0 <= index);
	if (static_cast<size_t>(index) >= indexed.size()) return 0;
	return &fetch(indexed[index], expand);
}

const String& ArgumentsContainer::fetchRequired(int index, bool expand) {
	assert(0 <= index);
	if (static_cast<size_t>(index) >= indexed.size()) {
		interpreter.throwBadSyntax(String("Missing indexed argument ") + Interpreter::toString(index + 1));
	}
	return fetch(indexed[index], expand);
}

const String* ArgumentsContainer::fetchOptional(const String& label, bool expand) {
	map<String, int>::const_iterator it = labeled.find(label);
	if (it == labeled.end()) return 0;
	return &fetch(it->second, expand);
}

const String& ArgumentsContainer::fetchRequired(const String& label, bool expand) {
	map<String, int>::const_iterator it = labeled.find(label);
	if (it == labeled.end()) interpreter.throwBadSyntax(String("Missing argument: ") + label);
	return fetch(it->second, expand);
}

void ArgumentsContainer::throwIfNoneFetched() {
	if (unfetchedCount == lossless_cast<int>(arguments.size())) interpreter.throwBadSyntax("Missing argument(s)");
}

void ArgumentsContainer::throwIfAnyUnfetched() {
	if (unfetchedCount != 0) interpreter.throwBadSyntax("Unrecognized labels or too many arguments");
}

/* --- Interpreter --- */

const String Interpreter::CURRENT_IMPD_REQUIRES_ID("impd-1");
const String Interpreter::YES_STRING("yes");
const String Interpreter::NO_STRING("no");

static bool isNaN(double d) { return d != d; }
#if (HAS_CPP11)
static bool isFinite(double d) { return !isNaN(d) && std::isfinite(d); }
#else
static bool isFinite(double d) { return !isNaN(d) && fabs(d) != std::numeric_limits<double>::infinity(); }
#endif

static double checkedLog(double x) {
	if (x <= 0) {
		Interpreter::throwRunTimeError("Math error (log of 0 or less)");
	}
	return log(x);
}

static double checkedLog10(double x) {
	if (x <= 0) {
		Interpreter::throwRunTimeError("Math error (log10 of 0 or less)");
	}
	return log10(x);
}

static double checkedSqrt(double x) {
	if (x < 0) {
		Interpreter::throwRunTimeError("Math error (sqrt of negative)");
	}
	return sqrt(x);
}

double (*Interpreter::MATH_FUNCTION_POINTERS[MATH_FUNCTION_COUNT])(double) = {
	(double (*)(double))(fabs), (double (*)(double))(acos), (double (*)(double))(asin), (double (*)(double))(atan)
	, (double (*)(double))(ceil), (double (*)(double))(cos), (double (*)(double))(cosh), (double (*)(double))(exp)
	, (double (*)(double))(floor), (double (*)(double))(checkedLog), (double (*)(double))(checkedLog10)
	, (double (*)(double))(sin), (double (*)(double))(sinh), (double (*)(double))(checkedSqrt)
	, (double (*)(double))(tan), (double (*)(double))(tanh), (double (*)(double))(round)
};

const Char Interpreter::ESCAPE_CHARS[ESCAPE_CODE_COUNT] = {	 'a',  'b',	 'f',  'n',	 'r',  't',	 'v' };
const Char Interpreter::ESCAPE_CODES[ESCAPE_CODE_COUNT] = { '\a', '\b', '\f', '\n', '\r', '\t', '\v' };

/* Built with QuickHashGen */
int Interpreter::findFunction(int n /* string length */, const char* s /* string (zero terminated) */) {
	static const char* STRINGS[20] = {
		"abs", "acos", "asin", "atan", "ceil", "cos", "cosh", "exp", "floor", "log",
		"log10", "sin", "sinh", "sqrt", "tan", "tanh", "round", "pi", "len", "def"
	};
	static const int HASH_TABLE[64] = {
		-1, -1, -1, -1, 11, 15, -1, -1, -1, 12, 16, 17, -1, -1, -1, -1,
		-1, -1, 10, -1, -1, -1, 18, -1, -1, -1, 14, -1, -1, 4, -1, -1,
		-1, 0, -1, -1, -1, 8, 19, 1, -1, -1, 5, 6, 9, -1, 3, -1,
		-1, 13, -1, 7, -1, -1, -1, 2, -1, -1, -1, -1, -1, -1, -1, -1
	};
	if (n < 2 || n > 5) return -1;
	int stringIndex = HASH_TABLE[((n ^ s[1]) - s[0] ^ s[0]) & 63];
	return (stringIndex >= 0 && strncmp(s, STRINGS[stringIndex], n) == 0 && STRINGS[stringIndex][n] == 0) ? stringIndex : -1;
}

/* Built with QuickHashGen */
int Interpreter::findBuiltInInstruction(int n, const char* s) {
	static const char* STRINGS[12] = {
		"_debug", "call", "for", "format", "if", "include", "local", "meta", "repeat",
		"return", "stop", "trace"
	};
	static const int HASH_TABLE[32] = {
		8, -1, 3, -1, 9, -1, -1, -1, -1, 11, 2, 6, 1, -1, -1, 10,
		4, -1, -1, -1, 7, 0, -1, -1, -1, -1, -1, 5, -1, -1, -1, -1
	};
	const unsigned char* p = (const unsigned char*) s;
	assert(s[n] == '\0');
	if (n < 2 || n > 7) return -1;
	int stringIndex = HASH_TABLE[((static_cast<unsigned int>(n) << 3) ^ p[2]) & 31u];
	return (stringIndex >= 0 && strcmp(s, STRINGS[stringIndex]) == 0) ? stringIndex : -1;
}

class Interpreter::EvaluationValue {
	public:		enum Type { UNDEFINED, BOOLEAN, NUMERIC, STRING };														// Only for optimization of internal representation.
	public:		EvaluationValue();
	public:		EvaluationValue(bool b);
	public:		EvaluationValue(double v);
	public:		EvaluationValue(const String& s);
	public:		Type getType() const;
	public:		operator bool() const;
	public:		operator double() const;
	public:		operator String() const;
	protected:	Type type;
	protected:	String stringValue;
	protected:	double doubleValue;
};

Interpreter::EvaluationValue::EvaluationValue() : type(UNDEFINED), doubleValue(0.0) { }
Interpreter::EvaluationValue::EvaluationValue(bool b) : type(BOOLEAN), doubleValue(b ? 1.0 : 0.0) { }
Interpreter::EvaluationValue::EvaluationValue(double v) : type(NUMERIC), doubleValue(v) { }
Interpreter::EvaluationValue::EvaluationValue(const String& s) : type(STRING), stringValue(s), doubleValue(0.0) { }
Interpreter::EvaluationValue::Type Interpreter::EvaluationValue::getType() const { return type; }

Interpreter::EvaluationValue::operator bool() const {
	return (type != STRING ? (doubleValue != 0.0) : toBool(stringValue));
}

Interpreter::EvaluationValue::operator double() const {
	return (type != STRING ? doubleValue : toDouble(stringValue));
}

Interpreter::EvaluationValue::operator String() const {
	switch (type) {
		default: assert(0);
		case UNDEFINED: return String(); break;
		case BOOLEAN: return toString(doubleValue != 0.0); break;
		case NUMERIC: return toString(doubleValue); break;
		case STRING: return stringValue; break;
	}
}

Interpreter::Interpreter(Executor& executor, Variables& vars, FormatInfo& formatInfo, int statementsLimit, int recursionLimit)
		: executor(executor), vars(vars), formatInfo(formatInfo), callingFrame(0), rootFrame(*this)
		, statementsLimit(statementsLimit), recursionLimit(recursionLimit) { }

Interpreter::Interpreter(Executor& executor, Variables& vars, Interpreter& callingFrame)
		: executor(executor), vars(vars), formatInfo(callingFrame.formatInfo), callingFrame(&callingFrame), rootFrame(callingFrame.rootFrame)
		, statementsLimit(callingFrame.statementsLimit), recursionLimit(callingFrame.recursionLimit) { }

Interpreter::Interpreter(Executor& executor, Interpreter& enclosingInterpreter)
		: executor(executor), vars(enclosingInterpreter.vars), formatInfo(enclosingInterpreter.formatInfo)
		, callingFrame(enclosingInterpreter.callingFrame), rootFrame(enclosingInterpreter.rootFrame)
		, statementsLimit(enclosingInterpreter.statementsLimit), recursionLimit(enclosingInterpreter.recursionLimit) { }

Interpreter::Interpreter(Executor& executor, Interpreter& enclosingInterpreter, FormatInfo& formatInfo)
		: executor(executor), vars(enclosingInterpreter.vars), formatInfo(formatInfo), callingFrame(enclosingInterpreter.callingFrame)
		, rootFrame(enclosingInterpreter.rootFrame), statementsLimit(enclosingInterpreter.statementsLimit)
		, recursionLimit(enclosingInterpreter.recursionLimit) { }

void Interpreter::throwBadSyntax(const String& how) { throw SyntaxException(how); }
void Interpreter::throwRunTimeError(const String& how) { throw RunTimeException(how); }
void Interpreter::throwBadSyntax(const char* how) { throwBadSyntax(String(how)); }
void Interpreter::throwRunTimeError(const char* how) { throwRunTimeError(String(how)); }

bool Interpreter::isComment(StringIt p, const StringIt& e) {
	return (p != e && p + 1 != e && *p == '/' && (p[1] == '/' || p[1] == '*'));
}

StringIt Interpreter::eatComment(StringIt p, const StringIt& e) {
	assert(isComment(p, e));
	if (p[1] == '/') {
		static const Char END_CHARS[] = { '\r', '\n' };
		return find_first_of(p += 2, e, END_CHARS, END_CHARS + 2);
	} else {
		static const Char END_CHARS[] = { '*', '/' };
		p = search(p += 2, e, END_CHARS, END_CHARS + 2);
		if (p == e) throwBadSyntax("Missing */");
		return p + 2;
	}
}

StringIt Interpreter::eatWhite(StringIt p, const StringIt& e) {
	while (p != e) {
		switch (*p) {
			case ' ': case '\t': ++p; break;
			case '\r': if (p + 1 != e && p[1] == '\n') ++p;
			case '\n': {																								// Eat leading .. on a new line
				while (++p != e && (*p == ' ' || *p == '\t')) { }
			if (e - p >= 2 && p[0] == '.' && p[1] == '.') p += 2;
			break;
			}
			case '/': if (isComment(p, e)) { p = eatComment(p, e); break; }
			/* else continue */
			default: return p;
		}
	}
	return p;
}

StringIt Interpreter::eatEscape(StringIt p, const StringIt& e) {
	assert(p != e && *p == '\\');
	++p;
	if (p != e) {
		if (*p == '\r' && p + 1 != e && p[1] == '\n') ++p;
		++p;
	}
	return p;
}

bool Interpreter::isSymbolLetter(Char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

StringIt Interpreter::eatSymbol(StringIt p, const StringIt& e) {
	while (p != e && (isSymbolLetter(*p) || *p == '.' || *p == '-' || (*p >= '0' && *p <= '9'))) ++p;
	return p;
}

StringIt Interpreter::eatSymbolForAssignment(StringIt p, const StringIt& e) {
	return (p != e && isSymbolLetter(*p) ? eatSymbol(p + 1, e) : p);
}

StringIt Interpreter::eatBlock(StringIt p, const StringIt& e) {															// FIX : <- termination char argument is better solution
	assert(p != e && (*p == '{' || *p == '['));
	const Char c = *p;
	++p;
	while (p != e) {
		switch (*p) {
			case '\\': p = eatEscape(p, e); break;
			case '"': p = eatQuotedString(p, e); break;
			case '{': case '[': p = eatBlock(p, e); break;
			case '}': ++p; if (c == '{') return p; break;
			case ']': ++p; if (c == '[') return p; break;
			case '/': if (isComment(p, e)) { p = eatComment(p, e); break; }
			/* else continue */
			default: ++p; break;
		}
	}
	if (p == e) throwBadSyntax(c == '[' ? "Missing ]" : "Missing }");
				
	return p;
}

StringIt Interpreter::eatQuotedString(StringIt p, const StringIt& e) {
	assert(p != e && *p == '"');
	++p;
	while (p != e && *p != '"') {
		if (*p == '\\') {
			p = eatEscape(p, e);
		} else {
			++p;
		}
	}
	if (p == e) throwBadSyntax("Missing \""); 
	return ++p;
}

StringIt Interpreter::eatArgumentValue(StringIt p, const StringIt& e) {
	while (p != e) {
		switch (*p) {
			case '\\': p = eatEscape(p, e); break;
			case '"': p = eatQuotedString(p, e); break;
			case '{': case '[': p = eatBlock(p, e); break;
			case ' ': case '\t': case '\r': case '\n': case ':': return p;
			case '/': if (isComment(p, e)) return p;
			/* else continue */
			default: ++p; break;
		}
	}
	return p;
}

void Interpreter::parseArguments(const StringRange& r, ArgumentVector& arguments) const {
	StringIt p = eatWhite(r.b, r.e);
	while (p != r.e) {
		StringRange lastRange(p, p);
		StringRange range(lastRange);
		do {																											// Try to eat label(s) first, if it turns out not to be a label, continue and use it as a value 
			range.b = p;
			bool haveQuotes = (p != r.e && *p == '"');
			p = (haveQuotes ? eatQuotedString(p, r.e) : eatSymbol(p, r.e));
			range.e = p;
			if (p == r.e || *p != ':') break;
			
			if (lastRange.b != lastRange.e) arguments.push_back(Argument(lastRange, String()));
			lastRange = (haveQuotes ? StringRange(range.b + 1, range.e - 1) : range);
			if (lastRange.b == lastRange.e) throwBadSyntax("Label cannot be empty");
			StringIt q = eatWhite(++p, r.e);
			if (p == q) { range.b = range.e = p; break; }															   // If no space after ':' we go to value directly.

			p = q;
		} while (p != r.e);
		
		p = eatArgumentValue(range.e, r.e);																			   // Beginning of a label is valid beginning of argument, so continue from end-point
		range.e = p;
		arguments.push_back(Argument(lastRange, range));
		StringIt q = eatWhite(p, r.e);
		if (p == q && p != r.e) throwBadSyntax("Syntax error");
		p = q;
	}
}

void Interpreter::set(const String& name, const String& value) {
	Interpreter* frame = this;
	for (; !frame->vars.assign(name, value); frame = frame->callingFrame) {
		if (frame->callingFrame == 0) {
			if (!frame->vars.declare(name, value)) {
				throwRunTimeError(String("Could not set variable ") + name);
			}
			return;
		}
	}
}

String Interpreter::get(const String& name) const {
	String value;
	const Interpreter* f = this;
	for (; f != 0 && !f->vars.lookup(name, value); f = f->callingFrame) { }
	if (f == 0) throwRunTimeError(String("Variable ") + name + " does not exist");
	return value;
}

StringIt Interpreter::eatListElement(StringIt p, const StringIt& e) {
	while (p != e) {
		switch (*p) {
			case '\\': p = eatEscape(p, e); break;
			case '"': p = eatQuotedString(p, e); break;
			case '{': case '[': p = eatBlock(p, e); break;
			case ' ': case '\t': case '\r': case '\n': case ',': return p;
			case '/': if (isComment(p, e)) return p;
			/* else continue */
			default: ++p; break;
		}
	}
	return p;
}

int Interpreter::parseList(const StringRange& r, StringVector& elements, bool expandAll, bool removeEmpty
		, int minElements, int maxElements) const {
	assert(0 <= minElements && minElements <= maxElements);
	elements.reserve(elements.size() + minElements);
	StringIt p = eatWhite(r.b, r.e);
	bool first = true;
	while (p != r.e) {
		if (!first && *p == ',') p = eatWhite(p + 1, r.e);
		StringIt q = eatListElement(p, r.e);
		String v = (expandAll ? expand(StringRange(p, q)) : String(p, q));
		if (!removeEmpty || !v.empty()) {
			elements.push_back(v);
		}
		p = eatWhite(q, r.e);
		first = false;
	}
	if (lossless_cast<int>(elements.size()) > maxElements) {
		throwBadSyntax(String("Too many list elements (got " + toString(lossless_cast<int>(elements.size()))
				+ ", expected at most ") + toString(maxElements) + ")");
	}
	if (lossless_cast<int>(elements.size()) < minElements) {
		throwBadSyntax(String("Too few list elements (got " + toString(lossless_cast<int>(elements.size()))
				+ ", expected at least ") + toString(minElements) + ")");
	}
	return lossless_cast<int>(elements.size());
}

StringIt Interpreter::eatStatement(StringIt p, const StringIt& e) {
	if (p != e && *p == '[') return eatBlock(p, e);
	while (p != e) {
		switch (*p) {
			case '\\': p = eatEscape(p, e); break;
			case '\r':
			case '\n': {
				StringIt q = p;
				if (*q == '\r' && q + 1 != e && q[1] == '\n') ++q;
				while (++q != e && (*q == ' ' || *q == '\t')) { }
				if (e - q >= 2 && q[0] == '.' && q[1] == '.') { p = q + 2; break; }
			}
			/* else continue */
			case ';': return p;
			case '"': p = eatQuotedString(p, e); break;
			case '{': case '[': p = eatBlock(p, e); break;
			case '/': if (isComment(p, e)) { p = eatComment(p, e); break; }
			/* else continue */
			default: ++p; break;
		}
	}
	return p;
}

void Interpreter::runStatement(const StringRange& r) {
	if (r.e - r.b >= 2 && *r.b == '[' && r.e[-1] == ']') run(StringRange(r.b + 1, r.e - 1));
	else if (r.b != r.e) {
		StringIt p = eatSymbolForAssignment(r.b, r.e);
		if (p == r.b) throwBadSyntax("Invalid instruction");
		StringRange leftRange(r.b, p);
		StringIt q = eatWhite(p, r.e);
		if (q != r.e && *q == '=') set(leftRange, StringRange(eatWhite(q + 1, r.e), r.e));
		else runInstruction(toLower(leftRange), StringRange(q, r.e));
	}
}

void Interpreter::run(const StringRange& r) {
	if (recursionLimit == 0) throwRunTimeError("Recursion limit reached");
	--recursionLimit;
	StringRange activeRange = r;
	String expanded;
	try {
		StringIt p = r.b;
		do {
			p = eatWhite(p, r.e);
			activeRange = StringRange(p, r.e);
			StringIt q = eatStatement(p, r.e);
			activeRange.e = p = q;
			if (rootFrame.statementsLimit == 0) throwRunTimeError("Statements limit reached");
			if (!executor.progress(*this, rootFrame.statementsLimit)) throw AbortedException("Aborted");
			--rootFrame.statementsLimit;
			expanded = performExpansion(activeRange);
			activeRange = expanded;
			runStatement(activeRange);
			if (p != r.e && *p == ';') ++p;
		} while (p != r.e);
	}
	catch (Exception& x) {
		++recursionLimit;
		if (!x.hasStatement()) x.statement = activeRange;
		throw;
	}
	catch (...) {
		++recursionLimit;
		throw;
	}
	++recursionLimit;
}

String Interpreter::toString(bool b) { return (b ? YES_STRING : NO_STRING); }

// Based on code from PikaScript
String Interpreter::toString(int32_t i, int radix, int minLength) {
	assert(2 <= radix && radix <= 16);
	assert(0 <= minLength && minLength <= static_cast<int>(sizeof (int) * 8));
	Char buffer[sizeof (int) * 8 + 1], * p = buffer + sizeof (int) * 8 + 1, * e = p - minLength;
	for (int32_t x = i; p > e || x != 0; x /= radix) {
		assert(p >= buffer + 2);
		*--p = "fedcba9876543210123456789abcdef"[15 + x % radix];														// Mirrored hex string to handle negative x.
	}
	if (i < 0) *--p = '-';
	return String(p, buffer + sizeof (i) * 8 + 1 - p);
}

// Based on code from PikaScript
String Interpreter::toString(double d, int precision) {
	assert(1 <= precision && precision <= 24);
	assert(isFinite(d));

	const double EPSILON = 1.0e-300;
	const double SMALL = 1.0e-5;
	const double LARGE = 1.0e+10;

	double x = fabs(d);
	if (x <= EPSILON) return "0";

	Char buffer[32];
	Char* bp = buffer + 2;
	Char* dp = bp;
	Char* pp = dp + 1;
	Char* ep = pp + precision;
	
	double y = x;	
	for (; x >= 10.0 && pp < ep; x *= 0.1) ++pp;																		// Normalize values > 10 and move period position.

	if (pp >= ep || y <= SMALL || y >= LARGE) {																		   // Exponential treatment of very small or large values.
		double e = floor(log10(y) + 1.0e-10);
		String exps(e >= 0 ? "e+" : "e");
		exps += toString(static_cast<int>(e));
		int maxp = 15;																									// Limit precision because of rounding errors in log10 etc
		for (double f = fabs(e); f >= 8; f /= 10) --maxp;
		return (toString(d * pow(0.1, e), min(maxp, precision)) += exps);
	}
	for (; x < 1.0 && dp < buffer + 32; ++ep, x *= 10.0) {																// For values < 1, spit out leading 0's and increase precision.
		*dp++ = '0';
		if (dp == pp) *dp++ = '9';																					   // Hop over period position (set to 9 to avoid when eliminating 9's).
	}
	for (; dp < ep; ) {																									// Exhaust all remaining digits of mantissa into buffer.
		uint32_t ix = static_cast<uint32_t>(x);
		*dp++ = ix + '0';
		if (dp == pp) *dp++ = '9';																					   // Hop over period position (set to 9 to avoid when eliminating 9's).
		x = (x - ix) * 10.0;
	}
	if (x >= 5) {																									   // If remainder is >= 5, increment trailing 9's...
		while (dp[-1] == '9') *--dp = '0';
		if (dp == bp) *--bp = '1';																					   // If we are at spare position, set to '1' and include, otherwise, increment last non-9.
		else dp[-1]++;
	}
	*pp = '.';
	if (ep > pp) while (ep[-1] == '0') --ep;
	if (ep - 1 == pp) --ep;
	if (d < 0) *--bp = '-'; 
	return String(bp, ep - bp);
}

StringIt Interpreter::parseHex(StringIt p, const StringIt& e, uint32_t& i) {
	for (i = 0; p != e && ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')); ++p)
		i = (i << 4) + (*p <= '9' ? *p - '0' : (*p & ~0x20) - ('A' - 10));
	return p;
}

StringIt Interpreter::parseUnsignedInt(StringIt p, const StringIt& e, uint32_t& i) {
	for (i = 0; p != e && *p >= '0' && *p <= '9'; ++p) i = i * 10 + (*p - '0');
	return p;
}

StringIt Interpreter::parseInt(StringIt p, const StringIt& e, int32_t& i) {
	bool negative = (e - p >= 2 && ((*p == '+' || *p == '-') && p[1] >= '0' && p[1] <= '9') ? (*p++ == '-') : false);
	uint32_t ui;
	p = parseUnsignedInt(p, e, ui);
	i = (negative ? -static_cast<int>(ui) : ui);
	return p;
}

StringIt Interpreter::parseDouble(StringIt p, const StringIt& e, double& v) {
	assert(p <= e);
	v = 0.0;
	double d = 0;
	StringIt q = p;
	double sign = (e - q > 1 && (*q == '+' || *q == '-') ? (*q++ == '-' ? -1.0 : 1.0) : 1.0);
	if (q == e || (*q != '.' && (*q < '0' || *q > '9'))) return p;
	StringIt b = q;
	while (q != e && *q >= '0' && *q <= '9') d = d * 10.0 + (*q++ - '0');
	if (q != e && *q == '.') {
		double f = 1.0;
		while (++q != e && *q >= '0' && *q <= '9') d += (*q - '0') * (f *= 0.1);
		if (q == b + 1) return p;
	}
	if (q != e && (*q == 'E' || *q == 'e')) {
		int32_t i;
		StringIt t = parseInt(q + 1, e, i);
		if (t != q + 1) { d *= pow(10, static_cast<double>(i)); q = t; }
	}
	v = d * sign;
	return q;
}

int Interpreter::toInt(const StringRange& r) {
	int32_t i;
	StringIt p = parseInt(r.b, r.e, i);
	if (p == r.b || p != r.e) throwRunTimeError(String("Invalid integer: ") + String(r.b, r.e));
	return i;
}

double Interpreter::toDouble(const StringRange& r) {
	double v;
	StringIt q = parseDouble(r.b, r.e, v);
	if (q == r.b || q != r.e) {
		throwRunTimeError(String("Invalid number: ") + String(r.b, r.e));
	}
	if (!isFinite(v)) {
		throwRunTimeError(String("Number overflow: ") + String(r.b, r.e));
	}
	return v;
}

String Interpreter::toLower(const StringRange& r) {
	String d(r.e - r.b, 0);
	String::iterator q = d.begin();
	for (String::const_iterator p = r.b; p != r.e; ++p, ++q) {
		Char c = *p;
		*q = ((c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c);
	}
	return d;
}

bool Interpreter::toBool(const String& s) {
	const String lower = toLower(s);
	if (lower == YES_STRING) {
		return true;
	} else if (lower != NO_STRING) {
		throwRunTimeError(String("Invalid boolean (should be 'yes' or 'no'): ") + s);
	}
	return false;
}

StringIt Interpreter::numericOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
		, Char op, Precedence opPrecedence, bool dry) const {
	if (precedence < opPrecedence) {
		double l = v;
		StringIt q = evaluateInner(p += (op == '^' ? 2 : 1), e, v, opPrecedence, dry);
		if (q == p) throwBadSyntax("Syntax error");
		p = q;
		if (!dry) {
			double r = v;
			switch (op) {
				case '+': l += r; break;
				case '-': l -= r; break;
				case '*': l *= r; break;
				case '/': if (r == 0.0) throwRunTimeError("Division by zero"); else l /= r; break;
				case '^': { errno = 0; l = pow(l, r); if (errno != 0) throwRunTimeError("Math error"); break; }
				default: assert(0); break;
			}
			if (!isFinite(l)) throwRunTimeError("Number overflow");
			v = l;
		}
	}
	return p;
}

StringIt Interpreter::moduloPercentOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const {
	if (precedence < POSTFIX) {
		EvaluationValue rv;
		StringIt q = evaluateInner(p + 1, e, rv, MUL_DIV_MOD, dry);
		if (q == p + 1) {
			++p;
			if (!dry) {
				v = fabs(static_cast<double>(v) / 100.0);
			}
		} else if (precedence < MUL_DIV_MOD) {
			p = q;
			if (!dry) {
				double r = static_cast<double>(rv);
				if (r == 0.0) throwRunTimeError("Modulo by zero");
				else v = fmod(static_cast<double>(v), r);
			}
		}
	}
	return p;
}

StringIt Interpreter::concatOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
		, Precedence concatType, bool dry) const {
	if (precedence < concatType) {
		EvaluationValue r;
		StringIt q = evaluateInner(p, e, r, concatType, dry);
		if (q != p) {
			p = q;
			if (!dry) {
				v = static_cast<String>(v) + static_cast<String>(r);
			}
		}
	}
	return p;
}

StringIt Interpreter::booleanOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
		, bool dry) const {
	if (precedence < BOOLEAN) {
		bool l = v;
		Char op = *p;
		if (p + 1 != e && p[1] == op)  {
			StringIt q = evaluateInner(p += 2, e, v, BOOLEAN, dry);
			if (q == p) throwBadSyntax("Syntax error");
			p = q;
			if (!dry) {
				bool r = v;
				switch (op) {
					case '&': l = l && r; break;
					case '|': l = l || r; break;
					default: assert(0);
				}
				v = l;
			}
		}
	}
	return p;
}

bool Interpreter::evaluationValueToNumber(const EvaluationValue& v, double& d, String& s) const {
	bool isNumeric = (v.getType() == EvaluationValue::NUMERIC);
	if (isNumeric) {
		d = v;
		if (!isFinite(d)) throwRunTimeError("Number overflow");
	} else {
		s = static_cast<String>(v);
		StringIt q = parseDouble(s.begin(), s.end(), d);
		if (q != s.begin() && q == s.end()) {
			if (!isFinite(d)) throwRunTimeError("Number overflow");
			isNumeric = true;
		}
	}
	return isNumeric;
}
StringIt Interpreter::comparisonOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const {
	if (precedence < COMPARE) {
		EvaluationValue r;
		Char op0 = *p++;
		Char op1 = (p != e && *p == '=' ? *p++ : 0);
		if ((op0 == '!' || op0 == '=') && op1 == 0) throwBadSyntax("Syntax error");
		StringIt q = evaluateInner(p, e, r, COMPARE, dry);
		if (q == p) throwBadSyntax("Syntax error");
		p = q;
		
		if (!dry) {
			double ld;
			String ls;
			bool leftIsNum = evaluationValueToNumber(v, ld, ls);

			double rd;
			String rs;
			bool rightIsNum = evaluationValueToNumber(r, rd, rs);

			int comparison;
			if (leftIsNum != rightIsNum) comparison = (leftIsNum ? -1 : 1);
			else if (!leftIsNum) comparison = (ls == rs ? 0 : (ls < rs ? -1 : 1));
			else {
				double accuracy = min(fabs(ld), fabs(rd)) * NUMBER_PRECISION_MAGNITUDE;
				comparison = (fabs(ld - rd) <= accuracy ? 0 : (ld < rd ? -1 : 1));
			}
			
			bool b;
			switch (op0) {
				default: assert(0);
				case '=': b = (comparison == 0); break;
				case '!': b = (comparison != 0); break;
				case '<': b = (op1 == 0 ? comparison < 0 : comparison <= 0); break;
				case '>': b = (op1 == 0 ? comparison > 0 : comparison >= 0); break;
			}
			v = b;
		}
	}
	return p;
}

StringIt Interpreter::conditionalOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
		, bool dry) const {
	if (precedence <= CONDITIONAL) {
		const bool isTrue = (!dry && static_cast<bool>(v));
		EvaluationValue l;
		StringIt q = eatWhite(evaluateInner(++p, e, l, CONDITIONAL, dry || !isTrue), e);
		if (q == e || *q != ':') throwBadSyntax("Expected :");
		EvaluationValue r;
		q = eatWhite(evaluateInner(++q, e, r, CONDITIONAL, dry || isTrue), e);
		p = q;
		if (!dry) {
			v = (isTrue ? l : r);
		}
	}
	return p;
}

StringIt Interpreter::substringOperation(StringIt p, const StringIt& e, EvaluationValue& v, Precedence precedence
		, bool dry) const {
	if (precedence <= POSTFIX) {
		EvaluationValue offset(0.0);
		EvaluationValue length(1.0);
		StringIt q = eatWhite(evaluateInner(++p, e, offset, CONDITIONAL, dry), e);
		const bool gotOffset = (q != p);
		bool gotLength = true;
		if (q != e && *q == ':') {
			const StringIt t = ++q;
			q = eatWhite(evaluateInner(t, e, length, CONDITIONAL, dry), e);
			gotLength = (t != q);
			if (!gotLength && !gotOffset) {
				throwBadSyntax("Syntax error");
			}
		} else if (!gotOffset) {
			throwBadSyntax("Syntax error");
		}
		q = eatWhite(q, e);
		if (q == e || *q != '}') throwBadSyntax("Missing }");
		++q;
		p = q;

		if (!dry) {
			const String source = static_cast<String>(v);
			const long sourceLength = lossless_cast<long>(source.size());

			const long intOffset = (gotOffset ? static_cast<long>(floor(static_cast<double>(offset))) : 0L);
			const long intLength = (gotLength ? static_cast<long>(floor(static_cast<double>(length))) : 0L);

			long start = (gotOffset ? (intOffset < 0 ? sourceLength + intOffset : intOffset) : (intLength < 0 ? sourceLength : 0));
			long end = (gotLength ? start + intLength : sourceLength);
			start = min(max(start, 0L), sourceLength);
			end = min(max(end, 0L), sourceLength);
			if (end < start) {
				v = String(source.rbegin() + (sourceLength - start), source.rbegin() + (sourceLength - end));
			} else {
				v = String(source.begin() + start, source.begin() + end);
			}
		}
	}
	return p;
}

// FIX : the naming of these two functions make no sense any more, even the splitting into two functions is pointless

StringIt Interpreter::evaluateInner(StringIt b, const StringIt& e, EvaluationValue& v, Precedence precedence, bool dry) const {
	StringIt p = evaluateOuter(b, e, v, dry);
	while (p != b) {
		b = p;
		StringIt t = eatWhite(b, e);
		if (t != e) {
			StringIt q;
			switch (*t) {
				case '+': case '-': q = numericOperation(t, e, v, precedence, *t, ADD_SUB, dry); break;
				case '*': if (t + 1 != e && t[1] == '*') { q = numericOperation(t, e, v, precedence, '^', POW, dry); break; }
				/* else continue */
				case '/': q = numericOperation(t, e, v, precedence, *t, MUL_DIV_MOD, dry); break;
				case '%': q = moduloPercentOperation(t, e, v, precedence, dry); break;
				case '<': case '>': case '=': case '!': q = comparisonOperation(t, e, v, precedence, dry); break;
				case '&': case '|': q = booleanOperation(t, e, v, precedence, dry); break;
				case '?': q = conditionalOperation(t, e, v, precedence, dry); break;
				case '{': q = substringOperation(t, e, v, precedence, dry); break;
				default: q = concatOperation(t, e, v, precedence, (t == b ? SPLICE : CONCAT), dry); break;
			}
			if (q != t) p = q;
		}
	}
	return p;
}

StringIt Interpreter::evaluateOuter(StringIt b, const StringIt& e, EvaluationValue& v, bool dry) const {
	StringIt p = b;
	StringIt t = eatWhite(p, e);
	p = t;
	if (p == e) throwBadSyntax("Unexpected end");
	switch (*p) {
			case '[': {
				StringIt q = eatBlock(p, e);
				if (!dry) {
					v = performExpansion(StringRange(p + 1, q - 1));
				}
				p = q;
				break;
			}
		
		case '"': {
			StringIt q = eatQuotedString(p, e);
			if (!dry) {
				v = String(p + 1, q - 1);
			}
			p = q;
			break;
		}
		
		case '$': { p = evaluateInner(p + 1, e, v, EXPAND, dry); if (!dry) v = get(v); break; }
		case '!': { p = evaluateInner(p + 1, e, v, PREFIX, dry); if (!dry) v = !static_cast<bool>(v); break; }
		case '-': { p = evaluateInner(p + 1, e, v, PREFIX, dry); if (!dry) v = -static_cast<double>(v); break; }
		case '+': { p = evaluateInner(p + 1, e, v, PREFIX, dry); if (!dry) v = static_cast<double>(v); break; }

		case '(': {
			p = eatWhite(evaluateInner(p + 1, e, v, BRACKETS, dry), e);
			if (p == e || *p != ')') throwBadSyntax("Missing )");
			++p;
			break;
		}
		
		case '\\': {
			if (p + 1 != e) {
				UniChar c;
				p = unescapeChar(++p, e, c);
				if (static_cast<Char>(c) != c) {
					throwBadSyntax("Invalid character escape code inside { } expression");
				}
				if (!dry) {
					v = String(1, static_cast<Char>(c));
				}
			}
			break;
		}
		
		case '.': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
			double d;
			StringIt q = parseDouble(p, e, d);
			if (q != p) {
				if (!isFinite(d)) throwRunTimeError("Number overflow");
				p = q;
				if (!dry) {
					v = d;
				}
				break;
			}
		}
		/* else continue */
		
		default: {
			StringIt q = p;
			while (q != e && (isSymbolLetter(*q) || *q == '.' || (*q >= '0' && *q <= '9'))) ++q;
			String sym(p, q);
			int funcIndex = findFunction(lossless_cast<int>(sym.size()), sym.c_str());
			if (funcIndex >= 0 && funcIndex < MATH_FUNCTION_COUNT) {
				errno = 0;
				q = evaluateInner(q, e, v, FUNCTION, dry);
				if (!dry) {
					v = MATH_FUNCTION_POINTERS[funcIndex](v);
					if (errno != 0) throwRunTimeError("Math error");
					if (!isFinite(v)) throwRunTimeError("Number overflow");
				}
			} else if (funcIndex == MATH_FUNCTION_COUNT) {			// pi
				v = 3.1415926535897932384626433;
			} else if (funcIndex == MATH_FUNCTION_COUNT + 1) {		// len
				q = evaluateInner(q, e, v, FUNCTION, dry);
				if (!dry) {
					v = static_cast<double>(static_cast<String>(v).size());
				}
			} else if (funcIndex == MATH_FUNCTION_COUNT + 2) {		// def
				q = evaluateInner(q, e, v, FUNCTION, dry);
				if (!dry) {
					String dummyValue;
					const String name = v;
					const Interpreter* f = this;
					for (; f != 0 && !f->vars.lookup(name, dummyValue); f = f->callingFrame) { }
					v = (f != 0);
				}
			} else if (!dry) {
				v = sym;
			}
			p = q;
			break;
		}
	}
	if (p == t) p = b;
	return p;
}

StringIt Interpreter::unescapeChar(StringIt p, const StringIt& e, UniChar& c) {
	const Char* f = find(ESCAPE_CHARS, ESCAPE_CHARS + ESCAPE_CODE_COUNT, *p);
	uint32_t i;
	if (f != ESCAPE_CHARS + ESCAPE_CODE_COUNT) { ++p; i = ESCAPE_CODES[f - ESCAPE_CHARS]; }
	else if (*p == 'x') { ++p; p = parseHex(p, (e - p >= 2 ? p + 2 : e), i); }
	else if (*p == 'u') { ++p; p = parseHex(p, (e - p >= 4 ? p + 4 : e), i); }
	else if (*p == 'U') { ++p; p = parseHex(p, (e - p >= 8 ? p + 8 : e), i); }
	else {
		StringIt q = parseUnsignedInt(p, e, i);
		if (q != p) p = q;
		else i = *p++;
	}
	c = static_cast<UniChar>(i);
	return p;
}

UniString Interpreter::unescapeToUni(const StringRange& r) {
	UniString processed;
	StringIt b = r.b;
	StringIt p = b;
	while (p != r.e) {
		if (*p == '\\') {
			processed.append(b, p);
			if (++p != r.e) {
				UniChar c;
				p = unescapeChar(p, r.e, c);
				processed += c;
			}
			b = p;
		} else {
			++p;
		}
	}
	processed.append(b, p);
	return processed;
}

WideString Interpreter::unescapeToWide(const StringRange& r) {
	WideString processed;
	StringIt b = r.b;
	StringIt p = b;
	while (p != r.e) {
		if (*p == '\\') {
			processed.append(b, p);
			if (++p != r.e) {
				UniChar c;
				p = unescapeChar(p, r.e, c);
				if (sizeof (WideString::value_type) == sizeof (uint16_t)) {
					if ((c >> 16) == 0) {
						processed += c;
					} else {
						processed += static_cast<uint16_t>(((c - 0x10000) >> 10) + 0xD800);
						processed += static_cast<uint16_t>(((c - 0x10000) & 0x3FF) + 0xDC00);
					}
				} else {
					assert(sizeof (WideString::value_type) == sizeof (UniChar));
					processed += c;
				}
			}
			b = p;
		} else {
			++p;
		}
	}
	processed.append(b, p);
	return processed;
}

String Interpreter::performExpansion(const StringRange& r) const {
	StringIt b = r.b;
	const StringIt& e = r.e;
	bool pendingSpace = false;
	String processed;
	StringIt p = b;
	while (p != e) {
		switch (*p) {
			case '\\': p = eatEscape(p, e); break;
			case '"': p = eatQuotedString(p, e); break;
			case '[': p = eatBlock(p, e); break;

			case '$': case '{': {
				if (pendingSpace) {
					processed.append(" ");
					pendingSpace = false;
				}
				processed.append(b, p);
				
				if (*p++ == '$') {
					StringIt q = eatSymbol(p, e);
					if (q == p) throwBadSyntax("Syntax error");
					processed.append(get(String(p, q)));
					p = q;
				} else {
					EvaluationValue v;
					p = eatWhite(evaluateInner(p, e, v, BRACKETS, false), e);
					if (p == e || *p != '}') throwBadSyntax("Syntax error");
					++p;
					processed.append(v);
				}

				b = p;
				break;
			}
			
			case '/': if (!isComment(p, e)) { ++p; break; }
			/* else continue */
					
			case ' ': case '\t': case '\r': case '\n': {
				if (pendingSpace) processed.append(" ");
				processed.append(b, p);
				pendingSpace = !processed.empty();
				p = eatWhite(p, e);
				b = p;
				break;
			}
			
			default: ++p; break;
		}
	}
	if (pendingSpace && b != p) processed.append(" ");
	processed.append(b, p);
	return processed;
}

String Interpreter::expand(const StringRange& r) const {
	if (r.e - r.b >= 2 && ((*r.b == '[' && r.e[-1] == ']') || (*r.b == '"' && r.e[-1] == '"'))) {
		StringRange newRange = StringRange(r.b + 1, r.e - 1);
		return (*r.b == '[' ? performExpansion(newRange) : String(newRange));
	} else {
		return String(r);
	}
}

int Interpreter::mapArguments(const ArgumentVector& allArguments, StringStringMap& labeledArguments
		, StringVector& indexedArguments) {
	for (ArgumentVector::const_iterator it = allArguments.begin(), e = allArguments.end(); it != e; ++it) {
		if (it->label.empty()) indexedArguments.push_back(it->value);
		else labeledArguments[toLower(it->label)] = it->value;
	}
	return lossless_cast<int>(indexedArguments.size());
}

void Interpreter::runInstruction(const String& instructionString, const StringRange& argumentsRange) {
	int foundIndex = findBuiltInInstruction(lossless_cast<int>(instructionString.size()), instructionString.c_str());
	if (foundIndex < 0) {
		if (executor.execute(*this, instructionString, argumentsRange)) return;
		else throwBadSyntax(String("Unrecognized instruction: ") + instructionString);
}

	BuiltInInstruction instruction = static_cast<BuiltInInstruction>(foundIndex);
	switch (instruction) {
		case STOP_INSTRUCTION: throw AbortedException("Encountered STOP instruction");
		case TRACE_INSTRUCTION: {
			executor.trace(*this, unescapeToWide(argumentsRange));
			break;
		}

		case FORMAT_INSTRUCTION: {
			if (!formatInfo.formatId.empty()) {
				throwBadSyntax("Duplicate format instruction");
			}
			ArgumentsContainer args(ArgumentsContainer::parse(*this, argumentsRange));
			const String& formatId = args.fetchRequired(0);
			if (formatId.empty()) {
				throwBadSyntax("Empty format identifier");
			}
			StringVector usesList;
			const String* s = args.fetchOptional("uses");
			if (s != 0) {
				parseList(*s, usesList, true, true);
				transform(usesList.begin(), usesList.end(), usesList.begin(), toLower);
				for (StringVector::const_iterator it = usesList.begin(); it != usesList.end(); ++it) {
					formatInfo.uses.insert(*it);
				}
			}
			StringVector requiresList;
			s = args.fetchOptional("requires");
			if (s != 0) {
				parseList(*s, requiresList, true, true);
				transform(requiresList.begin(), requiresList.end(), requiresList.begin(), toLower);
				requiresList.erase(remove(requiresList.begin(), requiresList.end(), CURRENT_IMPD_REQUIRES_ID)
						, requiresList.end());
				for (StringVector::const_iterator it = requiresList.begin(); it != requiresList.end(); ++it) {
					formatInfo.requires.insert(*it);
				}
			}
			args.throwIfAnyUnfetched();
			formatInfo.formatId = toLower(formatId);
			if (!executor.format(*this, formatInfo)) {
				throw FormatException("Unsupported data format");
			}
			break;
		}

		case META_INSTRUCTION: {
			if (formatInfo.formatId.empty()) {
				throwBadSyntax("Meta instruction requires a preceding format declaration");
			}
			const StringIt b = eatWhite(argumentsRange.b, argumentsRange.e);
			const StringIt q = eatSymbol(b, argumentsRange.e);
			if (q == b) {
				throwBadSyntax("Missing / invalid meta identifier");
			}
			const String metaToken(b, q);
			const String metaLower = toLower(metaToken);
			const FormatInfo& info = formatInfo;
			String resolvedMeta;
			if (info.uses.find(metaLower) != info.uses.end()) {
				resolvedMeta = metaLower;
			} else {
				const String prefix(metaLower + "-");
				uint32_t bestVersion = 0;
				for (std::set<String>::const_iterator it = info.uses.lower_bound(prefix)
						; it != info.uses.end() && it->compare(0, prefix.size(), prefix) == 0; ++it) {
					uint32_t parsedVersion;
					const StringIt b = it->begin() + prefix.size();
					const StringIt e = parseUnsignedInt(b, it->end(), parsedVersion);
					if (e != b && e == it->end() && parsedVersion >= bestVersion) {
						bestVersion = parsedVersion;
						resolvedMeta = *it;
					}
				}
				if (resolvedMeta.empty()) {
					throwBadSyntax(String("Undeclared meta tag: ") + metaToken);
				}
			}
			const bool success = executor.meta(*this, resolvedMeta, String(eatWhite(q, argumentsRange.e), argumentsRange.e));
			(void)success;
			return;
		}
		
		case LOCAL_INSTRUCTION:
		case RETURN_INSTRUCTION: {
			if (argumentsRange.b == argumentsRange.e) {
				throwBadSyntax("Missing variable name");
			}
			StringIt p = eatSymbolForAssignment(argumentsRange.b, argumentsRange.e);
			if (p == argumentsRange.b) {
				throwBadSyntax("Invalid variable name");
			}
			String varName(argumentsRange.b, p);
			StringIt q = eatWhite(p, argumentsRange.e);
			bool emptyAssignment = (q == argumentsRange.e);
			if (!emptyAssignment) {
				if (*q != '=') throwBadSyntax("Expected =");
				q = eatWhite(q + 1, argumentsRange.e);
			}
			String varValue(q, argumentsRange.e);
			if (instruction == RETURN_INSTRUCTION) {
				if (callingFrame == 0) throwRunTimeError("Cannot return in global frame");
				callingFrame->set(varName, (emptyAssignment ? get(varName) : varValue));
			} else if (!vars.declare(varName, varValue)) throwRunTimeError(String("Variable ") + varName + " already declared");
			break;
		}
		
		case IF_INSTRUCTION: {
			ArgumentsContainer args(ArgumentsContainer::parse(*this, argumentsRange));
			const String& condition = args.fetchRequired(0);
			const String& thenBlock = args.fetchRequired(1, false);
			const String* elseBlock = args.fetchOptional("else", false);
			args.throwIfAnyUnfetched();
			
			if (toBool(condition)) run(thenBlock);
			else if (elseBlock != 0) run(*elseBlock);
			break;
		}

		case REPEAT_INSTRUCTION: {
			ArgumentsContainer args(ArgumentsContainer::parse(*this, argumentsRange));
			int count = toInt(args.fetchRequired(0));
			const String& repeatBlock = args.fetchRequired(1, false);
			const String* condition = args.fetchOptional("while", false);
			args.throwIfAnyUnfetched();

			if (condition == 0) {
				for (int i = 0; i < count; ++i) run(repeatBlock);
			} else {
				if ((*condition)[0] != '[') throwBadSyntax("'while:' condition has to be enclosed in [ ]");
				for (int i = 0; i < count; ++i) {
					if (!toBool(expand(*condition))) break;
					run(repeatBlock);
				}
			}
			break;
		}

		case FOR_INSTRUCTION: {
			ArgumentsContainer args(ArgumentsContainer::parse(*this, argumentsRange));
			const String& indexVar = args.fetchRequired(0);
			const String& doBlock = args.fetchRequired(1, false);
			const String* inWhat = args.fetchOptional("in");
			
			if (inWhat != 0) {
				bool reverse = false;
				const String* reverseArg = args.fetchOptional("reverse");
				if (reverseArg != 0) reverse = toBool(*reverseArg);
				args.throwIfAnyUnfetched();
				StringVector list;
				parseList(*inWhat, list, false, false);
				if (reverse) std::reverse(list.begin(), list.end());
				for (StringVector::const_iterator it = list.begin(), e = list.end(); it != e; ++it) {
					set(indexVar, *it);
					run(doBlock);
				}
			} else {
				const double from = toDouble(args.fetchRequired("from"));
				const double to = toDouble(args.fetchRequired("to"));
				const String* stepArg = args.fetchOptional("step");
				const double step = (stepArg != 0 ? toDouble(*stepArg) : (from < to ? 1.0 : -1.0));
				args.throwIfAnyUnfetched();
				const double low = min(from, to);
				const double high = max(from, to);
				for (double i = from; i >= low && i <= high; i += step) {
					set(indexVar, toString(i));
					run(doBlock);
				}
			}
			break;
		}
		
		case CALL_INSTRUCTION:
		case INCLUDE_INSTRUCTION: {
			ArgumentVector allArguments;
			parseArguments(argumentsRange, allArguments);		
			if (allArguments.size() < 1) {
				throwBadSyntax("Missing argument(s)");
			}
			STLMapVariables newVars;
			String runThis;
			int counter = 0;
			for (ArgumentVector::const_iterator it = allArguments.begin(), e = allArguments.end(); it != e; ++it) {
				if (!runThis.empty())
					newVars.declare((it->label.empty() ? Interpreter::toString(counter++) : it->label), it->value);
				else if (it->label.empty()) runThis = it->value;
			}
			if (instruction == INCLUDE_INSTRUCTION) {
				const WideString file = unescapeToWide(expand(runThis));
				if (!executor.load(*this, file, runThis)) {
					throwRunTimeError(String("Could not include file: ") + String(file.begin(), file.end()));
				}
			}
			newVars.declare("n", toString(counter));
			Interpreter newFrame(executor, newVars, *this);
			newFrame.run(runThis);
			break;
		}

		case DEBUG_INSTRUCTION: {
			ArgumentVector allArguments;
			parseArguments(argumentsRange, allArguments);		
			WideString line = L"|";
			StringStringMap labeledArguments;
			StringVector indexedArguments;
			mapArguments(allArguments, labeledArguments, indexedArguments);
			bool doExpand = false;
			StringStringMap::const_iterator it = labeledArguments.find("expand");
			if (it != labeledArguments.end()) doExpand = toBool(expand(it->second));
			for (ArgumentVector::const_iterator it = allArguments.begin(), e = allArguments.end(); it != e; ++it) {
				if (doExpand) line += unescapeToWide(it->label) + L"=" + unescapeToWide(expand(it->value));
				else line += unescapeToWide(it->label) + L"=" + WideString(it->value.begin(), it->value.end());
				line += L'|';
			}
			executor.trace(*this, line);
			break;
		}
		
		default: assert(0);
	}
}

} // namespace IMPD
