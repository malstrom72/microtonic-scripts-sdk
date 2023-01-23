#include <sstream>
#include <iostream>
#include <string>
#include <set>
#include <algorithm>
#include <fstream>
#include "Makaron.h"

/*
	Roadmap:
 
	. 	@calc <arithmetic expression>
	. 	some kind of loop support, @repeat / @loop / @for or something
			suggestion: @for(n=1..13) blahblah @endfor
	. 	allow arguments to be passed by name? E.g. @specialKnob(bounds:{1,2,3}, caption:"qwerty")
	. 	make OffsetMap also for defined strings (store together with the string and offset and include during string expansion)
	. 	exceptions thrown have current parsing point as error offset while offsetMap more sensibly will point to the
		beginning of the current @ expression etc... should we skip including offset in exception entirely and
		build an exception with a "stack trace" outermost in String process(const String& source)?

	Done:
	. 	@include <filename> (would need to efficiently include source file in OffsetMapEntry)
		(but not in MakaronCmd)

*/

namespace Makaron {

const char* Exception::what() const throw() {
	try {
		if (errorWithLine.empty()) {
			std::ostringstream ss;
			ss << "Makaron error: " << error << " (" << file << (file.empty() ? "line " : " line ") << line
					<< ", column " << column << ")";
			errorWithLine = ss.str();
		}
		return errorWithLine.c_str();
	}
	catch (...) {
		assert(0);
		return "exception in Makaron::Exception::what()";
	}
}

std::pair<int, int> calculateLineAndColumn(const String& text, size_t offset) {
	int line = 1;
	int column = 1;
	const StringIt b = text.begin();
	const StringIt e = b + offset;
	for (StringIt p = b; p != e; ++p) {
		assert(p != text.end());
		if (*p == '\n') {
			++line;
			column = 0;
		}
		++column;
	}
	return std::make_pair(line, column);
}

Span::Span(const String& sourceCode, const String& fileName)
	: source(std::make_shared<const String>(sourceCode))
	, file(std::make_shared<const String>(fileName))
	, begin(source->begin())
	, end(source->end()) {
}

void Context::error(const std::string error) {
	size_t offset = p - processing.source->begin();
	std::pair<int, int> lineAndColumn = calculateLineAndColumn(*processing.source, offset);
	throw Exception(error, *processing.file, offset, lineAndColumn.first, lineAndColumn.second);
}

bool Context::isWhite(const Char c) {
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool Context::isLeadingIdentifierChar(const Char c) {
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '$' || c == '_');
}

bool Context::isIdentifierChar(const Char c) {
	return isLeadingIdentifierChar(c) || (c >= '0' && c <= '9');
}

bool Context::eof() const {
	assert(p <= processing.end);
	return p == processing.end;
}

void Context::skipWhite() {
	while (!eof() && isWhite(*p)) {
		++p;
	}
}

void Context::skipHorizontalWhite() {
	while (!eof() && (*p == ' ' || *p == '\t')) {
		++p;
	}
}

void Context::optionalLineBreak() {
	while (!eof() && (*p == ' ' || *p == '\t' || *p == '\r')) {
		++p;
	}
	if (!eof() && *p == '\n') {
		++p;
	}
}

bool Context::parseToken(const char* token) {
	const size_t n = strlen(token);
	if (processing.end - p >= static_cast<ptrdiff_t>(n) && std::equal(p, p + n, token)) {
		if (!isIdentifierChar(token[n - 1]) || (p + n == processing.end || !isIdentifierChar(*(p + n)))) {
			p += n;
			return true;
		}
	}
	return false;
}

/* Built with http://nuedge.net/StringHashMaker */
static int findReservedKeyword(size_t n /* string length */, const Char* s /* string (zero termination not required) */) {
	static const char* STRINGS[8] = {
		"begin", "define", "end", "endif", "elif", "else", "if", "redefine"
	};
	static const int HASH_TABLE[16] = {
		5, -1, -1, 4, -1, 2, -1, 7, -1, 6, -1, 0, 3, 1, -1, -1
	};
	if (n < 2 || n > 8) {
		return -1;
	}
	const int stringIndex = HASH_TABLE[((3 < n ? s[3] : 0) ^ s[0]) & 15];
	return (stringIndex >= 0 && strncmp(s, STRINGS[stringIndex], n) == 0
			&& STRINGS[stringIndex][n] == 0) ? stringIndex : -1;
}

String Context::parseIdentifier() {
	const StringIt b = p;
	if (!eof() && isLeadingIdentifierChar(*p)) {
		++p;
		while (!eof() && isIdentifierChar(*p)) {
			++p;
		}
	}
	String identifier(b, p);
	if (findReservedKeyword(identifier.size(), identifier.data()) >= 0) {
		error(std::string("Illegal use of \"") + identifier + "\"");
	}
	return identifier;
}

String Context::parseSymbol() {
	String name;
	if (!eof() && *p == '(') {
		++p;
		skipWhite();
		name = parseExpression(")");
		skipWhite();
		if (eof() || *p != ')') {
			error("Expected )");
		}
		++p;
	} else {
		name = parseIdentifier();
	}
	if (name.empty()) {
		error("Missing / invalid name");
	}
	return name;
}

void Context::parseParameterNames(std::vector<String>& parameterNames) {
	std::set<String> usedNames;
	if (!eof() && *p == '(') {
		++p;
		skipWhite();
		const String name = parseIdentifier();
		if (!name.empty()) {
			parameterNames.push_back(name);
			usedNames.insert(name);
			skipWhite();
			while (!eof() && *p == ',') {
				++p;
				skipWhite();
				const String name = parseIdentifier();
				if (name.empty()) {
					error("Expected parameter name");
				} else {
					parameterNames.push_back(name);
					if (!usedNames.insert(name).second) {
						error(std::string("Duplicate parameter name \"") + name + "\"");
					}
					skipWhite();
				}
			}
		}
		if (eof() || *p != ')') {
			error("Expected parameter name or )");
		}
		++p;
	}
}

StringIt Context::skipNested(const char* open, const char* close, bool skipLeadingWhite) {
	StringIt e = p;
	int blockCount = 1;
	while (!eof() && blockCount > 0) {
		e = p;
		if (skipLeadingWhite) {
			skipHorizontalWhite();
			if (eof()) {
				break;
			}
		}
		if (parseToken(open)) {
			++blockCount;
		} else if (parseToken(close)) {
			--blockCount;
		} else if (!parseToken("@@")) {
			assert(!eof());
			++p;
		}
	}
	if (blockCount > 0) {
		error(std::string("Missing ") + close);
	}
	return e;
}

Span Context::parseNested(const char* open, const char* close, bool skipLeadingWhite) {
	const StringIt b = p;
	return Span(processing, b, skipNested(open, close, skipLeadingWhite));
}

void Context::skipBracketsAndStrings(int depth) {
	if (depth >= DEFAULT_RECURSION_DEPTH_LIMIT) {
		error("Recursion depth limit reached");
	}
	assert(!eof());
	Char endChar = 0;
	switch (*p) {
		case '(': endChar = ')'; break;
		case '[': endChar = ']'; break;
		case '{': endChar = '}'; break;
	}
	if (endChar != 0) {
		++p;
		while (!eof() && *p != endChar) {
			skipBracketsAndStrings(depth + 1);
			++p;
		}
	} else {
		switch (*p) {
			case '\'': endChar = '\''; break;
			case '"': endChar = '"'; break;
		}
		if (endChar != 0) {
			++p;
			while (!eof() && *p != endChar) {
				if (*p == '\\') {
					++p;
				}
				if (!eof()) {
					++p;
				}
			}
		}
	}
	if (eof()) {
		error(std::string("Missing ") + endChar);
	}
}

String Context::parseExpression(const char* terminators) {
	const size_t terminatorCount = strlen(terminators);
	Span span;
	if (parseToken("@<")) {
		span = parseNested("@<", "@>", false);
	} else {
		const StringIt b = p;
		StringIt e = p;
		while (!eof() && std::find(terminators, terminators + terminatorCount, *p) == terminators + terminatorCount) {
			skipBracketsAndStrings(0);
			assert(!eof());
			if (!isWhite(*p)) {
				e = p + 1;
			}
			++p;
		}
		span = Span(processing, b, e);
	}
	String result;
	Context(depthLimiter - 1, this).process(span, result, 0);
	return result;
}

void Context::parseArgumentList(std::vector<String>& arguments) {
	if (!eof() && *p == '(') {
		do {
			++p;
			skipWhite();
			arguments.push_back(parseExpression(",)"));
			skipWhite();
		} while (!eof() && *p == ',');
		if (eof() || *p != ')') {
			error("Expected , or )");
		}
		++p;
	}
}

static bool standardIncludeLoader(const String& fileName, String& contents) {
	std::ifstream fileStream(fileName);
	if (!fileStream.good()) {
		return false;
	}
	fileStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
	std::istreambuf_iterator<Char> it(fileStream);
	std::istreambuf_iterator<Char> end;
	contents = String(it, end);
	return true;
}

Context::Context(int depthLimiter, Context* parentContext)
		: parentContext(parentContext), depthLimiter(depthLimiter), processed(0), offsets(0)
		, loader(parentContext != 0 ? parentContext->loader : standardIncludeLoader) {
}

void Context::stringDefinition(bool redefine) {
	skipWhite();
	const String name = parseIdentifier();
	if (name.empty()) {
		error("Missing / invalid name");
	}
	skipHorizontalWhite();
	if (eof() || *p != '=') {
		error("Expected =");
	}
	++p;
	skipHorizontalWhite();
	const String value = parseExpression("\n\r");
	optionalLineBreak();

	if (redefine) {
		Context* currentContext = this;
		do {
			std::map<String, Macro>::const_iterator macroIt = currentContext->macros.find(name);
			if (macroIt != currentContext->macros.end()) {
				error("Cannot redefine macro " + std::string("\"") + name + "\"");
			}
			std::map<String, String>::iterator stringIt = currentContext->strings.find(name);
			if (stringIt != currentContext->strings.end()) {
				stringIt->second = value;
				return;
			}
			currentContext = currentContext->parentContext;
		} while (currentContext != 0);
		error(std::string("Cannot redefine undefined \"") + name + "\"");
	} else {
		if (!defineString(name, value)) {
			error(std::string("\"") + name + "\" is already defined");
		}
	}
}

void Context::macroDefinition() {
	skipWhite();
	const String name = parseIdentifier();
	if (name.empty()) {
		error("Missing / invalid name");
	}
	std::vector<String> parameterNames;
	parseParameterNames(parameterNames);
	optionalLineBreak();
	if (!defineMacro(name, parameterNames, parseNested("@begin", "@end", true), this)) {
		error(std::string("\"") + name + "\" is already defined");
	}
	optionalLineBreak();
}

bool Context::testCondition() {
	skipWhite();
	if (eof() || *p != '(') {
		error("Expected (");
	}
	++p;
	skipWhite();
	const String left = parseExpression("=!");
	skipWhite();
	bool isEqual = parseToken("==");
	bool isNotEqual = (!isEqual && parseToken("!="));
	if (!isEqual && !isNotEqual) {
		error("Expected == or !=");
	}
	skipWhite();
	const String right = parseExpression(")");
	skipWhite();
	if (eof() || *p != ')') {
		error("Expected )");
	}
	++p;
	optionalLineBreak();
	return ((left == right) == isEqual);
}

void Context::ifStatement() {
	bool success = testCondition();

	bool finishedSpan = false;
	bool gotElse = false;
	Span span(processing, p, p);
	skipHorizontalWhite();
	while (!parseToken("@endif")) {
		if (eof()) {
			error("Missing @endif");
		} else if (parseToken("@if")) {
			skipNested("@if", "@endif", true);
		} else if (parseToken("@elif")) {
			if (gotElse) {
				error("Illegal use of \"elif\"");
			}
			if (!success) {
				success = testCondition();
				span.begin = p;
			} else {
				finishedSpan = true;
			}
		} else if (parseToken("@else")) {
			gotElse = true;
			if (!success) {
				optionalLineBreak();
				success = true;
				span.begin = p;
			} else {
				finishedSpan = true;
			}
		} else if (!parseToken("@@")) {
			++p;
		}
		if (!finishedSpan) {
			span.end = p;
			skipHorizontalWhite();
		}
	}
	optionalLineBreak();
	if (success) {
		assert(processed != 0);
		Context(depthLimiter - 1, this).process(span, *processed, offsets);
	}
}

void Context::invokeMacro() {
	const String name = parseSymbol();

	std::vector<String> arguments;
	parseArgumentList(arguments);
	
	const Macro* foundMacro = 0;
	const String* foundDefinition = 0;
	Context* currentContext = this;
	do {
		std::map<String, Macro>::const_iterator macroIt = currentContext->macros.find(name);
		if (macroIt != currentContext->macros.end()) {
			foundMacro = &macroIt->second;
			assert(foundMacro != 0);
			break;
		}
		std::map<String, String>::const_iterator stringIt = currentContext->strings.find(name);
		if (stringIt != currentContext->strings.end()) {
			foundDefinition = &stringIt->second;
			assert(foundDefinition != 0);
			break;
		}
		currentContext = currentContext->parentContext;
	} while (currentContext != 0);

	if (currentContext == 0) {
		error(std::string("\"") + name + "\" is undefined");
	} else if (foundDefinition != 0) {
		assert(processed != 0);
		(*processed) += *foundDefinition;
		if (arguments.size() != 0) {
			error(std::string("Incorrect number of arguments for ") + name);
		}
	} else {
		assert(foundMacro != 0);
		Context subContext(depthLimiter - 1, foundMacro->context);
		if (arguments.size() != foundMacro->params.size()) {
			error(std::string("Incorrect number of arguments for ") + name);
		}
		std::vector<String>::const_iterator argIt = arguments.begin();
		std::vector<String>::const_iterator paramIt = foundMacro->params.begin();
		while (argIt != arguments.end()) {
			subContext.defineString(*paramIt, *argIt);
			++paramIt;
			++argIt;
		}
		assert(paramIt == foundMacro->params.end());
		assert(processed != 0);
		subContext.process(foundMacro->span, *processed, offsets);
	}
}

bool Context::defineMacro(const String& name, const std::vector<String>& parameterNames, const Span& span, Context* context) {
	if (strings.find(name) != strings.end()) {
		return false;
	} else {
		Macro newMacro;
		newMacro.params = parameterNames;
		newMacro.span = span;
		newMacro.context = context;
		return macros.insert(std::make_pair(name, newMacro)).second;
	}
}

bool Context::defineString(const String& name, const String& definition) {
	if (macros.find(name) != macros.end()) {
		return false;
	} else {
		return strings.insert(std::make_pair(name, definition)).second;
	}
}

bool Context::redefineString(const String& name, const String& definition) {
	if (macros.find(name) != macros.end()) {
		return false;
	}
	std::map<String, String>::iterator it = strings.find(name);
	if (it == strings.end()) {
		return false;
	}
	it->second = definition;
	return true;
}

void Context::produce(const StringIt& b, const StringIt& e) {
	assert(processed != 0);
	if (e != b && offsets != 0) {
		OffsetMapEntry entry;
		entry.file = processing.file;
		entry.outputPoint = processed->size();
		entry.outputStretch = e - b;
		entry.inputFrom = processing.sourceOffset(b);
		entry.inputLength = 0;
		offsets->push_back(entry);
	}
	processed->append(b, e);
}

enum Instruction {
	LITERAL_AT, DEFINE_MACRO, DEFINE_STRING, REDEFINE_STRING, IF_STATEMENT, INCLUDE_STATEMENT, INVOKE_MACRO
};

static const char* INSTRUCTIONS[6] = {
	"@@", "@begin", "@define", "@redefine", "@if", "@include"
};

void Context::includeFile() {
	skipWhite();
	const String fileName = parseExpression("\n\r");
	optionalLineBreak();
	String source;
	if (!loader(fileName, source)) {
		error(std::string("Could not load include file: ") + fileName);
	}

	const Span previousProcessing = processing;
	const StringIt previousP = p;
	--depthLimiter;
	try {
		process(Span(source, fileName), *processed, offsets);
	}
	catch (...) {
		++depthLimiter;
		p = previousP;
		processing = previousProcessing;
		throw;
	}
	++depthLimiter;
	p = previousP;
	processing = previousProcessing;
}

void Context::process(const Span& input, String& output, std::vector<OffsetMapEntry>* offsetMap) {
	processing = input;
	processed = &output;
	offsets = offsetMap;
	p = input.begin;

	if (depthLimiter == 0) {
		error("Recursion depth limit reached");
	}
	
	while (!eof()) {
		StringIt textBegin = p;
		StringIt textEnd = p;
		while (!eof() && *p != '@') {
			if (*p == ' ' || *p == '\t') {
				skipHorizontalWhite();
			} else {
				++p;
				textEnd = p;
			}
		}
		if (eof()) {
			textEnd = p;
		}
		produce(textBegin, textEnd);
		if (!eof()) {
			const StringIt instructionBegin = p;
			
			Instruction instruction = LITERAL_AT;
			while (instruction < INVOKE_MACRO && !parseToken(INSTRUCTIONS[instruction])) {
				instruction = static_cast<Instruction>(instruction + 1);
			}
			
			if (instruction == LITERAL_AT || instruction == INVOKE_MACRO) {
				produce(textEnd, instructionBegin);
			}
			
			OffsetMapEntry mapEntry;
			size_t offsetsIndex;
			const bool hasOffsets = (offsets != 0);
			if (hasOffsets) {
				mapEntry.inputFrom = processing.sourceOffset(instructionBegin);
				assert(processed != 0);
				mapEntry.outputPoint = processed->size();
				offsetsIndex = offsets->size();
				offsets->push_back(mapEntry);
			}

			try {
				switch (instruction) {
					case LITERAL_AT: (*processed) += '@'; break;
					case DEFINE_MACRO: macroDefinition(); break;
					case DEFINE_STRING: stringDefinition(false); break;
					case REDEFINE_STRING: stringDefinition(true); break;
					case IF_STATEMENT: ifStatement(); break;
					case INVOKE_MACRO: ++p; invokeMacro(); break;
					case INCLUDE_STATEMENT: includeFile(); break;
				}
			}
			catch (const Exception&) {	 // we want the offsetMap to be as complete as possible
				if (hasOffsets) {
					mapEntry.inputLength = processing.sourceOffset(p) - mapEntry.inputFrom;
					assert(processed != 0);
					mapEntry.outputStretch = processed->size() - mapEntry.outputPoint + 1;
					(*offsets)[offsetsIndex] = mapEntry;
				}
				throw;
			}
			
			if (hasOffsets) {
				mapEntry.inputLength = processing.sourceOffset(p) - mapEntry.inputFrom;
				assert(processed != 0);
				mapEntry.outputStretch = processed->size() - mapEntry.outputPoint;
				(*offsets)[offsetsIndex] = mapEntry;
			}
		}
	}
}

void Context::setIncludeLoader(const LoaderFunction& loaderFunction) { loader = loaderFunction; }

String process(const String& source, const String& fileName) {
	String output;
	Context(DEFAULT_RECURSION_DEPTH_LIMIT).process(Span(source, fileName), output, 0);
	return output;
}

RangeVector findInputRanges(const std::vector<OffsetMapEntry>& offsetMap, size_t outputOffset) {
	std::vector<OffsetMapEntry>::const_iterator it = offsetMap.begin();
	// TODO: binary search maybe?
	while (it != offsetMap.end() && outputOffset < it->outputPoint) {
		++it;
	}
	RangeVector v;
	while (it != offsetMap.end() && outputOffset >= it->outputPoint) {
		if (outputOffset < it->outputPoint + it->outputStretch) {
			if (it->inputLength == 0) {
				const size_t inputOffset = it->inputFrom + (outputOffset - it->outputPoint);
				v.push_back(std::make_pair(inputOffset, inputOffset + 1));
			} else {
				v.push_back(std::make_pair(it->inputFrom, it->inputFrom + it->inputLength));
			}
		}
		++it;
	}
	return v;
}

static bool checkExpected(const char* source, const char* expected) {
	try {
		const String processed = process(source, "unit test");
		if (processed != expected) {
			return false;
		}
	}
	catch (const Exception& x) {
		(void)x;
		return false;
	}
	return true;
}

static bool checkError(const char* source, const char* expectedError, size_t expectedOffset, int expectedLine
		, int expectedColumn) {
	try {
		process(source, "unit test");
	}
	catch (const Exception& x) {
		if (x.getError() != expectedError) {
			return false;
		}
		if (x.getOffset() != expectedOffset) {
			return false;
		}
		if (x.getLineNumber() != expectedLine) {
			return false;
		}
		if (x.getColumnNumber() != expectedColumn) {
			return false;
		}
		return true;
	}
	return false;
}

bool unitTest() {
	assert(checkExpected(
			"abcd"
			,
			"abcd"));
	
	assert(checkExpected(
			"  abcd  "
			,
			"  abcd  "));

	assert(checkExpected(
			"  abcd  "
			,
			"  abcd  "));

	assert(checkExpected(
			"\n  abcd  \n\n"
			,
			"\n  abcd  \n\n"));

	assert(checkExpected(
			"@@abcd"
			,
			"@abcd"));
		
	assert(checkExpected(
			"  @@abcd  "
			,
			"  @abcd  "));
		
	assert(checkExpected(
			"@define abcd=\n"
			"<@abcd>"
			,
			"<>"));

	assert(checkExpected(
			"@define abcd = ghij\n"
			"@abcd"
			,
			"ghij"));
	
	assert(checkExpected(
			"@define abcd = ghij\n"
			"@(abcd)"
			,
			"ghij"));

	assert(checkExpected(
			"@define abcd=@<\n"
			"@>\n"
			"<@abcd>"
			,
			"<\n"
			">"));

	assert(checkExpected(
			"@define abcd=@<  \n"
			"  @>\n"
			"<@abcd>"
			,
			"<  \n"
			"  >"));

	assert(checkExpected(
			"@begin abcd ghij @end\n"
			"_@(abcd)_"
			,
			"_ghij_"));
	
	assert(checkExpected(
			"@begin concat(a,b) @(a)@(b) @end\n"
			"@concat(mon,ster)"
			,
			"monster"));
	
	assert(checkExpected(
			"@begin concat(a,b) @(a)@(b) @end\n"
			"@concat(\"mon\",\"ster\")"
			,
			"\"mon\"\"ster\""));
	
	assert(checkExpected(
			"@begin concat(a,b) @(a)@(b) @end\n"
			"@concat(\"mon\\\",\",ster)"
			,
			"\"mon\\\",\"ster"));
	
	assert(checkExpected(
			"@begin concat(a,b) @(a)@(b) @end\n"
			"@concat(@<cookie, the@>,@< monster@>)"
			,
			"cookie, the monster"));
	
	assert(checkExpected(
			"@begin   a   @begin   local x@end   @global@local   @end  @define global=y\n"
			"@a"
			,
			"yx"));
	
	assert(checkExpected(
			"@begin repeat2(t) @t@t @end\n"
			"@begin repeat4(t) @repeat2(@t)@repeat2(@t) @end\n"
			"@repeat4(@<@begin r(x) __@(x)__ @end\n"
			"@r(@<@@<1,2@@>@>)\n"
			"@>)"
			,
			"__@<1,2@>__\n"
			"__@<1,2@>__\n"
			"__@<1,2@>__\n"
			"__@<1,2@>__\n"));
	
	assert(checkExpected(
			"@begin f(a,b) @a+@b @end\n"
			"@f(@f(x,y),z)\n"
			"@f( @f( x , y ) , z)\n"
			"@f( @f( x , \"y)\" ) , z)\n"
			"@f(@f({[(x,'y)}]\"')]},_),z)\n"
			,
			"x+y+z\n"
			"x+y+z\n"
			"x+\"y)\"+z\n"
			"{[(x,'y)}]\"')]}+_+z\n"));
	
	assert(checkExpected(
			"@begin\n"
			"with2params(  x   ,   y )\n"
			"@begin test(y)\n"
			"@define x=UUU\n"
			"@begin z @x @end\n"
			"@(y)@(z)\n"
			"@end\n"
			"@test(\"@@x\")smurf@y\n"
			"@end\n"
			"@(with2params)(1,\" ,\")bye\n"
			,
			"\"@x\"UUU\n"
			"smurf\" ,\"\n"
			"bye\n"));
	
	assert(checkExpected(
			"@begin x abc @end @if ( @x == abc ) hello @endif"
			,
			"hello"));
	
	assert(checkExpected(
			"<<<@if (1==1)    yes 1    @endif>>>"
			"<<<@if( @<check me@>  == check me   )yes 2@endif>>>"
			,
			"<<<yes 1>>>"
			"<<<yes 2>>>"));

	assert(checkExpected(
			"@begin fruitColor(fruit)\n"
			"\t@if ( @fruit == apple ) @if (@preferRedApples == yes) red @else green @endif\n"
			"\t@elif ( @fruit == banana ) yellow\n"
			"\t@elif ( @fruit == pear ) green\n"
			"\t@else unknown\n"
			"\t@endif\n"
			"@end\n"
			"@define preferRedApples = yes\n"
			"[@fruitColor(apple)]\n"
			"[@fruitColor(banana)]\n"
			"[@fruitColor(pear)]\n"
			"[@fruitColor(rubberduck)]\n"
			"@redefine preferRedApples = no\n"
			"[@fruitColor(apple)]\n"
			,
			"[red]\n"
			"[yellow\n]\n"
			"[green\n]\n"
			"[unknown\n]\n"
			"[green]\n"));

	assert(checkExpected(
			"@define define123=123\n"
			"@define123\n"
			,
			"123\n"));

	assert(checkExpected(
			"@define sushi =   @<raw@>       \n"
			"@sushi"
			,
			"raw"));

	assert(checkExpected(
			"@define zyz=3\n"
			"@define a_3=66\n"
			"@(  a_3  )\n"
			"@(a_@zyz)\n"
			"@(  @<a_3@>  )\n"
			,
			"66\n"
			"66\n"
			"66\n"
			));

	assert(checkExpected(
			"@define xyzzy=plugh\n"
			"@xyzzy"
			,
			"plugh"));

	assert(checkExpected(
			"@define x=3\n"
			"@if (@x==3) @redefine x=11 @endif\n"
			"@x\n"
			,
			"11\n"));

	assert(checkExpected(
			"@begin dacro(x)@@a=@(@x)@end\n"
			"@begin macro(a)@dacro(a)\n"
			"\t@begin zacro(x)@@a=@(@x)@end\n"
			"\t@zacro(a)\n"
			"\t@a@a@a\n"
			"@end\n"
			"abcd@@ghi\n"
			"@define defg=23\n"
			"@define a=AAA\n"
			"@define xorx=polgh\n"
			"@if (0==0) @macro(@< @xorx @>) @endif\n"
			,
			"abcd@ghi\n"
			"@a=AAA\n"
			"\t@a= polgh \n"
			"\t polgh  polgh  polgh \n"));

	assert(checkError(
			"@end",
			"Illegal use of \"end\"", 4, 1, 5));
	
	assert(checkError(
			"@define else=3",
			"Illegal use of \"else\"", 12, 1, 13));
	
	assert(checkError(
			"@begin x(endif) @end",
			"Illegal use of \"endif\"", 14, 1, 15));
	
	assert(checkError(
			"@(define) x=y",
			"\"define\" is undefined", 9, 1, 10));
	
	assert(checkError(
			"@define",
			"Missing / invalid name", 7, 1, 8));
	
	assert(checkError(
			"@define 3x=y",
			"Missing / invalid name", 8, 1, 9));
	
	assert(checkError(
			"@begin x(",
			"Expected parameter name or )", 9, 1, 10));
	
	assert(checkError(
			"@begin x(5a",
			"Expected parameter name or )", 9, 1, 10));
	
	assert(checkError(
			"@begin x(a,b) z @end @x",
			"Incorrect number of arguments for x", 23, 1, 24));
	
	assert(checkError(
			"@begin x z @end @x()",
			"Incorrect number of arguments for x", 20, 1, 21));
	
	assert(checkError(
			"@begin x(a,b) z @end @x(",
			"Expected , or )", 24, 1, 25));

	assert(checkError(
			"@begin x",
			"Missing @end", 8, 1, 9));
	
	assert(checkError(
			"@begin x @begin y @end",
			"Missing @end", 22, 1, 23));

	assert(checkError(
			"@begin x @endx",
			"Missing @end", 14, 1, 15));

	assert(checkError(
			"@macro('qwerty)",
			"Missing '", 15, 1, 16));

	assert(checkError(
			"@define x y"
			, "Expected =", 10, 1, 11));

	assert(checkError(
			"@begin x @x @end\n"
			"@x"
			, "Recursion depth limit reached", 9, 1, 10));
	
	assert(checkError(
			"@begin a @begin (local)x@end @global@local @end@define global=y\n"
			"@local"
			, "\"local\" is undefined", 70, 2, 7));

	assert(checkError(
			"@define noIndirection=blah @begin weird((@noIndirection)) @end"
			, "Expected parameter name or )", 40, 1, 41));

	assert(checkError(
			"@define () = blah"
			, "Missing / invalid name", 8, 1, 9));

	assert(checkError(
			"@define (  ) = blah"
			, "Missing / invalid name", 8, 1, 9));

	assert(checkError(
			"@if (0 == 1) @else @elif (1 == 1) @endif"
			, "Illegal use of \"elif\"", 24, 1, 25));

	assert(checkError(
			"@define x=23\n"
			"@define x=11\n"
			, "\"x\" is already defined", 26, 3, 1));

	assert(checkError(
			"@redefine x=23"
			, "Cannot redefine undefined \"x\"", 14, 1, 15));

	assert(checkError(
			"@begin xyzzy @end @redefine xyzzy=55"
			, "Cannot redefine macro \"xyzzy\"", 36, 1, 37));

	assert(checkError(
			"@begin dupl(qwer,c,def,qwer,asdf) @end"
			, "Duplicate parameter name \"qwer\"", 27, 1, 28));

	return true;
}

} // namespace Makaron

#ifdef REGISTER_UNIT_TEST
REGISTER_UNIT_TEST(Makaron::unitTest)
#endif
