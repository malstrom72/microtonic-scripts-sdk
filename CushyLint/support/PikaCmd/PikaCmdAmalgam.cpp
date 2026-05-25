/**
	\file PikaScript.h
	
	PikaScript is a high-level scripting language written in C++.
	
	\version
	
	Version 0.97
	
	\page Copyright
	
	PikaScript is released under the BSD 2-Clause License. http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PikaScript_h
#define PikaScript_h

#include "assert.h" // Note: I always include assert.h like this so that you can override it with a "local" file.
#include <vector>
#include <map>
#include <string>
#include <functional>

// These are defined as macros in the Windows headers and collide with some of our "proper" C++ definitions. Sorry, it
// just ain't right to use global macros in C++. I #undef them. Include PikaScript.h before the Windows headers if you
// need these macros (or push and pop their definitions before and after including this header with #pragma push_macro
// and pop_macro).

#undef VOID
#undef ERROR

/**
	The PikaScript namespace.
*/
namespace Pika {

#if (PIKA_UNICODE)
	#define STR(s) L##s
	#define PIKA_SCRIPT_VERSION L"0.97"
#else
	#define STR(x) x
	#define PIKA_SCRIPT_VERSION "0.97"
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

/**
	\name Conversion routines for string <-> other types.
	
	For some of these we could use stdlib implementations yes, but:
	
	-# Some of them (e.g. atof, strtod) behaves differently depending on global "locale" setting. We can't have that.
	-# The stdlib implementations can be slow (e.g. my double->string conversion is about 3 times faster than MSVC CRT).
	-# Pika requires high-precision string representation and proper handling of trailing 9's etc.
*/
//@{

template<class S> std::string toStdString(const S& s);																	///< Converts the string \p s to a standard C++ string. \details The default implementation is std::string(s.begin(), s.end()). You should specialize this template if necessary.
template<class S> ulong hexToLong(typename S::const_iterator& p, const typename S::const_iterator& e);					///< Converts a string in hexadecimal form to an ulong integer. \details \p p is updated on return to point to the first unparsed (e.g. invalid) character. If \p p == \p e, the full string was successfully converted.
template<class S> long stringToLong(typename S::const_iterator& p, const typename S::const_iterator& e);				///< Converts a string in decimal form to a signed long integer. \details \p p is updated on return to point to the first unparsed (e.g. invalid) character. If \p p == \p e, the full string was successfully converted.
template<class S, typename T> S intToString(T i, int radix = 10, int minLength = 1);									///< Converts the integer \p i to a string with a radix and minimum length of your choice. \details \p radix can be anything between 1 (binary) and 16 (hexadecimal).
template<class S> double stringToDouble(typename S::const_iterator& p, const typename S::const_iterator& e);			///< Converts a string in scientific e notation (e.g. -12.34e-3) to a double floating point value. \details Spaces before 'e' are not accepted. Uppercase 'E' is allowed. Positive and negative 'infinity' is supported (provided the compiler allows it).\p p is updated on return to point to the first unparsed (e.g. invalid) character. If \p p == \p e, the full string was successfully converted.
template<class S> bool stringToDouble(const S& s, double& d);															///< A convenient utility routine that tries to convert the entire string \p s (in scientific e notation) to a double, returning true on success or false if the string is not in valid syntax.
template<class S> S doubleToString(double d, int precision = 14);														///< Converts the double \p d to a string (in scientific e notation, e.g. -12.34e-3). \details \p precision can be between 1 and 24 and is the number of digits to include in the output string (not counting any exponent of course). Any trailing decimal zeroes will be trimmed and only significant digits will be included.
template<class S> S unescape(typename S::const_iterator& p, const typename S::const_iterator& e);						///< Converts a string that is either enclosed in single (' ') or double (" ") quotes. \details The routine expects the string beginning at \p p to be of one of these forms, but \p e can point beyond the terminating quote. If the single (' ') quote is used, the string between the quotes is simply extracted "as is" with the exception of pairs of apostrophes ('') that are used to represent single apostrophes. If the string is enclosed in double quotes (" ") it can use C-style escape sequences. The supported escape sequences are: @code \\ \" \' \a \b \f \n \r \t \v \xHH \uHHHH \<decimal> @endcode. On return, \p p will point to the first unparsed (e.g. invalid) character. If \p p == \p e, the full string was successfully converted.
template<class S> S escape(const S& s);																					///< Depending on the contents of the source string \p s it is encoded either in single (' ') or double (" ") quotes. \details If the string contains only printable ASCII chars (ASCII values between 32 and 126 inclusively) and no apostrophes (' '), it is enclosed in single quotes with no further processing. Otherwise it is enclosed in double quotes (" ") and any unprintable ASCII character, backslash (\) or quotation mark (\c ") is encoded using C-style escape sequences (e.g. \code "line1\nline2" \endcode).

//@}

/**
	bound_mem_fun_t is a member functor bound to a specific C++ object through a pointer. You may use this class instead
	of "manually" binding a std::mem_fun functor to an object. Besides being more convenient, this class solves a
	problem in some STL implementations that prevents you from having reference arguments in the functor.
	
	You would normally use the helper function bound_mem_fun() to automatically instantiate the correct template.
*/
template<class C, class A0, class R> class bound_mem_fun_t : public std::unary_function<A0, R> {
	public:		bound_mem_fun_t(R (C::*m)(A0), C* o) : m(m), o(o) { }
	public:		R operator()(A0 a) const { return (o->*m)(a); }
	protected:	R (C::*m)(A0);
	protected:	C* o;
};

/**
	bound_mem_fun creates a member functor bound to a specific C++ object through a pointer. You may use this function
	instead of "manually" binding a std::mem_fun functor to an object. For example the following code: \code
	std::bind1st(std::mem_fun(&Dancer::tapDance), fredAstaire))); \endcode can be replaced with \code
	bound_mem_fun(&Dancer::tapDance, fredAstaire); \endcode
	
	Furthermore, bound_mem_fun does not suffer from a problem that some STL implementations has which prevents you from
	using member functors with reference arguments.
	
	bound_mem_fun is used in PikaScript to directly bind a native function to a member function of a certain C++ object.
*/
template<class C, class A0, class R> inline bound_mem_fun_t<C, A0, R> bound_mem_fun(R (C::*m)(A0), C* o) {
	return bound_mem_fun_t<C, A0, R>(m, o);
}

/**
	We use this dummy class to specialize member functions for arbitrary types (including void, references etc).
*/
template<class T> class Dumb { };

/**
	The PikaScript exception class. It is based on \c std::exception and stores a simple error string of type \p S. The
	standard what()	is provided, and therefore conversion to a \c const \c char* string must be performed in case the
	\p S class is not directly compatible. Since what() is defined to return a pointer only, we need storage for the
	converted string within this class too (Exception::converted).
*/
template<class S> class Exception : public std::exception {
	public:		Exception(const S& error) : error(error) { }															///< Simply constructs the exception with error string \p error.
	public:		virtual S getError() const throw() { return error; }													///< Return the error string for this exception.
	public:		virtual const char *what() const throw() { return (converted = toStdString(error)).c_str(); }			///< Returns the error as a null-terminated char string. \details Use getError() if you can since it is faster and permits other character types than char.
	public:		virtual ~Exception() throw() { }																		// (GCC requires explicit destructor with one that has throw().)
	protected:	S error;																								///< The error string.
	protected:	mutable std::string converted;																			///< Since what() is defined to return a pointer only, we need storage for the converted string within this class too.
};

/**
	STLValue is the reference implementation of a PikaScript variable. Internally, all values are represented by an STL
	compliant string class \p S *. This class actually inherits from \p S (with public inheritance) for optimization
	reasons. This way we avoid a lot of unnecessary temporary objects when we cast to and from strings. (Unfortunately
	it is not possible to make this inheritance private and add conversion operators to the \p S class. Explicit
	conversion operators in C++ have lower priority than implicit base class conversions.)
	
	Although it may seem inefficient to store all variables in textual representation it makes PikaScript easy to
	interface with and debug for. With the custom value <-> text conversion routines in PikaScript the performance isn't
	too bad. It mainly depends on the performance of the string implementation which is the reason why this class is a
	template. The standard variant of STLValue uses std::string, but you may want to "plug in" a more efficient class.
		
	STLValue supports construction from and casting to the following C++ types:

	- \c bool
	- \c int
	- \c uint
	- \c long
	- \c ulong
	- \c float
	- \c double
	- the template string class \p S

	\note
	* PikaScript only requires a specific subset of the STL string features. Utility member functions (such as \c
	find_first_of) are never used (instead, the equivalent generic STL algorithms are utilized). Destructive functions
	such as \c insert and \c erase are not used either. Furthermore, PikaScript consider all string access through the
	subscript operator [] to be for reading only (therefore only a \c const function for this operator is required).
*/
template<class S> class STLValue : public S {

	/// \name Typedefs.
	//@{
	public:		typedef S String;																						///< The class to use for all strings (i.e. the super-class).
	public:		typedef typename S::value_type Char;																	///< The character type for all strings (e.g. char or wchar_t).
	//@}
	/// \name Constructors.
	//@{
	public:		STLValue() { }																							///< The default constructor initializes the value to the empty string (or "void").
	public:		STLValue(double d) : S(doubleToString<S>(d)) { }														///< Constructs a value representing the double precision floating point \p d.
	public:		STLValue(float f) : S(doubleToString<S>(f)) { }															///< Constructs a value representing the single precision floating point \p f.
	public:		STLValue(long i) : S(intToString<S, long>(i)) { }														///< Constructs a value representing the signed long integer \p l.
	public:		STLValue(ulong i) : S(intToString<S, ulong>(i)) { }														///< Constructs a value representing the ulong integer \p l.
	public:		STLValue(int i) : S(intToString<S, long>(i)) { }														///< Constructs a value representing the signed integer \p i.
	public:		STLValue(uint i) : S(intToString<S, ulong>(i)) { }														///< Constructs a value representing the unsigned integer \p i.
	public:		STLValue(bool b) : S(b ? S(STR("true")) : S(STR("false"))) { }											///< Constructs a value representing the boolean \p b.
	public:		template<class T> STLValue(const T& s) : S(s) { }														///< Pass other types of construction onwards to the super-class \p S.
	//@}
	/// \name Conversion to native C++ types.
	//@{
	public:		operator bool() const;																					///< Converts the value to a boolean. \details If the value isn't "true" or "false" an exception is thrown.
	public:		operator long() const;																					///< Converts the value to a signed long integer. \details If the value isn't in valid integer format an exception is thrown.
	public:		operator double() const;																				///< Converts the value to a double precision floating point. \details If the value isn't in valid floating point format an exception is thrown.
	public:		operator float() const { return float(double(*this)); }													///< Converts the value to a single precision floating point. \details If the value isn't in valid floating point format an exception is thrown.
	public:		operator ulong() const { return ulong(long(*this)); }													///< Converts the value to an ulong integer. \details If the value isn't in valid integer format an exception is thrown.
	public:		operator int() const { return int(long(*this)); }														///< Converts the value to a signed integer. \details If the value isn't in valid integer format an exception is thrown.
	public:		operator uint() const { return uint(int(*this)); }														///< Converts the value to an unsigned integer. \details If the value isn't in valid integer format an exception is thrown.
	//@}
	/// \name Overloaded operators (comparisons and subscript).
	//@{
	public:		bool operator<(const STLValue& r) const;																///< Less than comparison operator. \details Notice that numbers are compared numerically and a number is always considered less than any non-number string.
	public:		bool operator==(const STLValue& r) const;																///< Equality operator. \details Notice that numbers are compared numerically (e.g. '1.0' and '1' are considered identical) and strings are compared literally (character by character).
	public:		bool operator!=(const STLValue& r) const { return !(*this == r); }										///< Non-equality operator. \details Notice that numbers are compared numerically (e.g. '1.0' and '1' are considered identical) and strings are compared literally (character by character).
	public:		bool operator>(const STLValue& r) const { return r < (*this); }											///< Greater than comparison operator. \details Notice that numbers are compared numerically and a number is always considered less than any non-number string.
	public:		bool operator<=(const STLValue& r) const { return !(r < (*this)); }										///< Less than or equal to comparison operator. \details Notice that numbers are compared numerically and a number is always considered less than any non-number string.
	public:		bool operator>=(const STLValue& r) const { return !((*this) < r); }										///< Greater than or equal to comparison operator. \details Notice that numbers are compared numerically and a number is always considered less than any non-number string.
	public:		const STLValue operator[](const STLValue& i) const;														///< The subscript operator returns the concatenation of the value with the dot (.) separator (if necessary) and the value \p i. \details Use it on a reference value to create a reference to a subscript element of that reference.
	//@}
	/// \name Classification methods.
	//@{
	public:		bool isVoid() const { return S::empty(); }																///< Returns true if the value represents the empty string.
	//@}
	/// \name Helper templates to allow certain operations on any type that is convertible to a STLValue.
	//@{
	public:		template<class T> bool operator<(const T& r) const { return operator<(STLValue(r)); }
	public:		template<class T> bool operator==(const T& r) const { return operator==(STLValue(r)); }
	public:		template<class T> bool operator!=(const T& r) const { return operator!=(STLValue(r)); }
	public:		template<class T> bool operator>(const T& r) const { return operator>(STLValue(r)); }
	public:		template<class T> bool operator<=(const T& r) const { return operator<=(STLValue(r)); }
	public:		template<class T> bool operator>=(const T& r) const { return operator>=(STLValue(r)); }
	public:		template<class T> const STLValue operator[](const T& i) const { return operator[](STLValue(i)); }
	//@}
};

/**	Precedence levels are used both internally for the parser and externally for the tracing mechanism. */
enum Precedence {
	NO_TRACE = 0		///< used only for tracing with tick()
	, TRACE_ERROR = 1	///< used only for tracing with tick()
	, TRACE_CALL = 2	///< used only for tracing with tick()
	, TRACE_LOOP = 3	///< used only for tracing with tick()
	, STATEMENT = 4		///< x; y;
	, BODY = 5			///< if () x, for () x
	, ARGUMENT = 6		///< (x, y)
	, BRACKETS = 7		///< (x) [x]
	, ASSIGN = 8		///< x=y x*=y x/=y x\=y x%=y x+=y x-=y x<<=y x>>=y x#=y x&=y x^=y x|=y
	, LOGICAL_OR = 9	///< x||y
	, LOGICAL_AND = 10	///< x&&y
	, BIT_OR = 11		///< x|y
	, BIT_XOR = 12		///< x^y
	, BIT_AND = 13		///< x&y
	, EQUALITY = 14		///< x===y x==y x!==y x!=y
	, COMPARE = 15		///< x<y x<=y x>y x>=y
	, CONCAT = 16		///< x#y
	, SHIFT = 17		///< x<<y x\>>y
	, ADD_SUB = 18		///< x+y x-y
	, MUL_DIV = 19		///< x*y x/y x\y x%y
	, PREFIX = 20 		///< \@x !x ~x +x -x ++x --x
	, POSTFIX = 21		///< x() x.y x[y] x{y} x++ x--
	, DEFINITION = 22	///< function { }
};

/**
	Script is a meta-class that groups all the core classes of the PikaScript interpreter together (except for the value
	class). The benefit of having a class like this is that we can declare types that are common to all sub-classes.
	
	The class is a template that takes another meta-class for configuring PikaScript. The configuration class should
	contain the following typedefs:
	
	-# \c Value			(use this class for all PikaScript values, e.g. STLValue<std::string>)
	-# \c Locals		(when a function call occurs, this sub-class of Variables will be instantiated for the callee)
	-# \c Globals		(this sub-class of Variables is used for the FullRoot class)
*/
template<class Config> struct Script {

	typedef typename Config::Value Value;																				///< The class used for all values and variables (defined by the configuration meta-class). E.g. STLValue.
	typedef typename Value::String String;																				///< The class used for strings (defined by the string class). E.g. \c std::string.
	typedef typename String::value_type Char;																			///< The character type for all strings (defined by the string class). E.g. \c char.
	typedef typename String::size_type SizeType;																		///< The length type for all strings (defined by the string class). E.g. \c size_t.
	typedef typename String::const_iterator StringIt;																	///< The const_iterator of the string is used so frequently it deserves its own typedef.
	typedef Exception<String> Xception;																					///< The exception type.
	
	class Native;
	class Root;
	
	/**
		Variables is an abstract base class which implements the interface to the variable space that a Frame works on.
		In the configuration meta-class class (Script::Config) two typedefs exist that determines which sub-classes of
		Variables should be used for the "root frame" (= Globals) and subsequently for the "sub-frames" (= Locals).
		
		A standard Variables class is supplied in this header file (STLVariables). Custom sub-classes are useful for
		optimization and special integration needs.
		
		Notice that the separation of Frames and Variables makes it possible to have more than one Frame referencing
		the same variable space. This could be useful for example in a threaded situation where several concurrent
		threads running PikaScript should share global variables. In this case each thread should still have a distinct
		"root frame" and you need to implement a sub-class of Variables that accesses its data in a thread-safe manner.
	*/
	class Variables {
		public:		typedef Script ForScript;
		public:		typedef std::vector< std::pair<String, Value> > VarList;
		public:		virtual bool lookup(const String& symbol, Value& result) = 0;										///< Lookup \p symbol. \details If found, store the found value in \p result and return true, otherwise return false.
		public:		virtual bool assign(const String& symbol, const Value& value) = 0;									///< Assign \p value to \p symbol and return true if the assignment succeeded. \details If false is returned, the calling Frame::set() will throw an exception.
		public:		virtual bool erase(const String& symbol) = 0;														///< Erase \p symbol. Return true if the symbol existed and was successfully erased.
		public:		virtual void list(const String& key, VarList& list) = 0;											///< Iterate all symbols that begins with \p key and push back names and values to \p list. \details There are no requirements on the order of the listed elements. You should not erase the list at the beginning.
		public:		virtual Native* lookupNative(const String& identifier) = 0;											///< Lookup the native function (or object) with \p identifier. \details Return 0 if the native could not be found. In this case, the caller will throw an exception.
		public:		virtual bool assignNative(const String& identifier, Native* native) = 0;							///< Assign the native function (or object) \p native to \p identifier, replacing any already existing definition. \details Once assigned, the native is considered "owned" by this variable space. This class is responsible for deleting its natives on destruction and also delete the existing definition when an identifier is being reassigned.
		public:		virtual ~Variables();																				///< Destructor. \details Don't forget to delete all registered natives.
	};
	
	/**
		The execution context and interpreter for PikaScript.
		
		This is where the magic happens. A Frame represents an execution context for a PikaScript function and it
		contains the source code interpreter. Normally you do not create instances of Frame yourself. They are created
		on stack whenever a function call is made. Notice that this implementation of PikaScript does not run in a
		virtual machine, instead it is interpreted directly and it shares calling stack etc with your C++ application.
	*/
	class Frame {

		/// \name Construction.
		//@{
		public:		Frame(Variables& vars, Root& root, Frame* previous);												///< Constructs the Frame and associates it with the variable space \p vars. \details All frames on the calling stack have direct access to the "root frame" which is designated by \p root (will be = \c *this for the Root). \p previous should point to the caller Frame (or 0 for the Root). The "frame label" of a root frame is always \c :: . Root::generateLabel() is called to create unique labels for other frames.
		//@}
		/// \name Properties.
		//@{
		public:		Variables& getVariables() const throw() { return vars; }											///< Returns a reference to the Variable instance associated with this Frame. Simple as that.
		public:		Root& getRoot() const throw() { return root; }														///< Returns a reference to the "root frame" for this Frame. (No brainer.)		
		public:		Frame& getPrevious() const throw() { assert(previous != 0); return *previous; }						///< Returns a reference to the previous frame (i.e. the frame of the caller of this frame). Must not be called on the root frame (will assert).
		//@}
		/// \name Getting, setting and referencing variables.
		//@{
		public:		Value get(const String& identifier, bool fallback = false) const;									///< Gets a variable value. \details If \p identifier is prefixed with a "frame identifier" (e.g. a "frame label" or \c ^), it will be "resolved" and used for retrieving the variable. Otherwise the variable space associated with this Frame instance will be checked and if the variable cannot be found and \p fallback is true, the global variable space will also be checked. If the variable cannot be found in any of the checked locations, an exception will be thrown.
		public:		Value getOptional(const String& identifier, const Value& defaultValue = Value()) const;				///< Tries to get the variable value as with get() (but never "falls back"). \details If the variable cannot be found, \p defaultValue will be returned instead.
		public:		const Value& set(const String& identifier, const Value& v);											///< Sets a variable value. \details Just as with get(), \p identifier may be prefixed with a "frame identifier" to address a different Frame.
		public:		Value reference(const String& identifier) const;													///< Creates a reference to the variable identified by \p identifier by prefixing it with a "frame label". \details If the identifier is already prefixed with a "frame identifier" (such as \c ^) it will be resolved to determine the frame. Otherwise, the label of this Frame instance is used.
		public:		std::pair< Frame*, String > resolveFrame(const String& identifier) const;							///< Resolves the frame for \p identifier and returns it together with \p identifier stripped of any prefixed "frame identifier". \details The rules are as follows: 1) If the identifier has a leading \c ::, the "root frame" is returned. 2) If the identifier begins with an existing frame label, this frame is used for resolving the rest of the identifier. (If a frame label cannot be found an exception is thrown.) 3) For each leading '^' the previous frame is used for resolving the rest of the identifier. 4) Finally, if the identifier does not begin with the '$' character, the "closure" of the current frame is returned, otherwise the current frame is returned.
		//@}
		/// \name Calling functions and evaluating source code.
		//@{
		public:		Value call(const String& callee, const Value& body, long argc, const Value* argv = 0);				///< Calls a Pika function (by setting up a new "sub-frame" and executing the function body). \details You may pass \c Value() (the \c void value) for \p callee or \p body. If only \p callee is specified, it will be used to retrieve the function body (through get()). If only \p body is specified, the called function will not have a \c $callee variable (\c $callee is used for debugging and object-oriented solutions). If both are present, the \c $callee variable will be set to \p callee, and \p body will be executed. \p argc is the number of arguments and if this is not zero, the \p argv parameter should point to an array of arguments (of at least \p argc elements in size). The return value is that of the PikaScript function.
		public:		Value execute(const String& body);																	///< A low-level function that executes \p body directly on the Frame instance. \details This means that unlike call(), you need to setup a "sub-frame" yourself, populate it with argument variables and then use this function. The return value is that of the PikaScript function.
		public:		Value evaluate(const String source);																///< Evaluates the PikaScript expression in \p source directly on this Frame. \details This differs from execute() in that \p source is not expected to be in the format of a "function body" of an "ordinary", "lambda" or "native" function. (Notice that we are not passing a reference to the \p source string here so that we are safe in case the PikaScript code manipulates the very string it is running on.) The return value is that of the evaluated expression.
		public:		StringIt parse(const StringIt& begin, const StringIt& end, bool literal);							///< Parses a PikaScript expression or literal (without evaluating it) and returns an iterator pointing at the end of the expression.
		//@}
		/// \name Registering native functions (or objects).
		//@{
		public:		void registerNative(const String& identifier, Native* native);										///< Registers the native function (or object) \p native with \p identifier in the appropriate variable space (determined by any "frame identifier" present in \p identifier). \details Once registered, the native is considered "owned" by the variable space. In other words, all registered natives will be deleted by the Variables destructor. Also, if you register a new native on an already used identifier, the old native for that identifier will be deleted automatically. Besides assigning the native with Variables::assignNative() this method also sets the variable \p identifier to \c <identifier> (unless \p native is a null-pointer).
		public:		template<class A0, class R> void registerNative(const String& i, R (*f)(A0)) {
						registerNative(i, newUnaryFunctor(std::ptr_fun(f)));
					}																									///< Helper template for easily registering a unary C++ function. \details The C++ function should take a single argument of either Frame& or any of the native types that are convertible from Script::Value. It should return a value of any type that is convertible to Script::Value or void.
		public:		template<class A0, class A1, class R> void registerNative(const String& i, R (*f)(A0, A1)) {
						registerNative(i, newBinaryFunctor(std::ptr_fun(f)));
					}																									///< Helper template for easily registering a binary C++ function. \details The C++ function should take two arguments of any of the native types that are convertible from Script::Value. It should return a value of any type that is convertible to Script::Value or void.
		public:		template<class C, class A0, class R> void registerNative(const String& i, C* o, R (C::*m)(A0)) {
						registerNative(i, newUnaryFunctor(bound_mem_fun(m, o)));
					}																									///< Helper template for easily registering a unary C++ member function in the C++ object pointed to by \p o. \details The C++ member function should take a single argument of either Frame& or any of the native types that are convertible from Script::Value. It should return a value of any type that is convertible to Script::Value or void. Normally, this registration technique is used for bridging Pika function calls to methods of a C++ object which is guaranteed to live as long as the target Frame.
		public:		void unregisterNative(const String& identifier) { registerNative(identifier, (Native*)(0)); }		///< Helper function for unregistering a native function / object. \details Unregistering a native is the same as registering a null-pointer to the identifier. Any PikaScript variable referring to \c <identifier> will still do so (including the one created automatically by registerNative()). However, performing a function call on such a variable will generate a run-time exception.
		//@}
		/// \name Destruction.
		//@{
		public:		virtual ~Frame() { }																				///< The default destructor does nothing, but it is always good practice to have a virtual destructor.
		//@}
		protected:	typedef std::pair<bool, Value> XValue;																///< The XValue differentiates lvalues and rvalues and is used internally in the interpreter. \details first = lvalue or not, second = symbol (for lvalue) or actual value (for rvalue).
		protected:	void tick(const StringIt& p, const XValue& v, Precedence thres, bool exit);
		protected:	Value rvalue(const XValue& v, bool fallback = true);
		protected:	const Value& lvalue(const XValue& v);
		protected:	void white(StringIt& p, const StringIt& e);
		protected:	bool token(StringIt& p, const StringIt& e, const Char* token);
		protected:	Frame* resolveFrame(StringIt& p, const StringIt& e) const;
		protected:	template<class F> bool binaryOp(StringIt& p, const StringIt& e, XValue& v, bool dry
							, Precedence thres, int hop, Precedence prec, F op);
		protected:	template<class F> bool assignableOp(StringIt& p, const StringIt& e, XValue& v, bool dry
							, Precedence thres, int hop, Precedence prec, F op);
		protected:	template<class F> bool addSubOp(StringIt& p, const StringIt& e, XValue& v, bool dry
							, Precedence thres, const F& f);
		protected:	template<class E, class I, class S> bool lgtOp(StringIt& p, const StringIt& e, XValue& v, bool dry
							, Precedence thres, const E& excl, const I& incl, S shift);
		protected:	bool pre(StringIt& p, const StringIt& e, XValue& v, bool dry);
		protected:	bool post(StringIt& p, const StringIt& e, XValue& v, bool dry, Precedence thres);
		protected:	bool expr(StringIt& p, const StringIt& e, XValue& v, bool emptyOk, bool dry, Precedence thres);
		protected:	bool termExpr(StringIt& p, const StringIt& e, XValue& v, bool emptyOk, bool dry, Precedence thres
							, Char term);
		protected:	static long intDiv(long x, long y);

		protected:	Variables& vars;
		protected:	Root& root;
		protected:	Frame* const previous;
		protected:	Frame* closure;
		protected:	const String* source;
		protected:	const String label;
		
		private:	Frame(const Frame& copy); // N/A
		private:	Frame& operator=(const Frame& copy); // N/A
	};
	
	/**
		The Root is the first Frame you instantiate. It is the starting point for the execution of PikaScript code. Its
		variables can be accessed from any frame with the special "frame identifier" \c ::. Furthermore, its variable
		space is often checked as a "backup" for symbols that cannot be retrieved from local "sub-frames".
		
		The class also offers a few functions out of which you may overload trace() and setTracer() if you want to
		customize the tracing mechanism in PikaScript. The default implementation calls a PikaScript function that you
		can designate with the standard library function "trace".
		
		In case you use PikaScript concurrently in different threads, you need a Root for every thread, but you could
		implement and share a sub-class of Variables that accesses shared data in a thread-safe manner.
		
		If you just want to use the standard Root implementation with a standard variable space you may want to use
		FullRoot instead.
	*/
	class Root : public Frame {
		public:		Root(Variables& vars);
		public:		virtual void trace(Frame& frame, const String& source, SizeType offset, bool lvalue
							, const Value& value, Precedence level, bool exit);											///< Overload this member function if you want to customize the tracing mechanism in PikaScript. \details The default implementation calls the PikaScript function Root::tracerFunction that you can assign with the standard library function "trace". See the standard library documentation on "trace" for more information on the arguments to this member function.
		public:		virtual void setTracer(Precedence traceLevel, const Value& tracerFunction) throw();					///< Called by the standard library function "trace" to assign a PikaScript tracer function and a trace level. (Also called by the standard trace() on exceptions.) \details You may want to overload this member function if you change the tracing mechanism and need control over the trace level for example.
		public:		bool doTrace(Precedence level) const throw() { return level <= traceLevel; }						///< \details This function is called *a lot*. For performance reasons it is good if it becomes inlined, so we are not declaring it virtual. If you want to customize which events that will be traced, try cleverly implementing your own trace() and setTracer() member functions instead.
		public:		String generateLabel();																				///< Each "sub-frame" requires a unique "frame label". \details This function creates it by "incrementing" Root::autoLabel, character by character, using '0' to '9' and upper and lower case 'a' to 'z', growing the string when necessary.
		protected:	Precedence traceLevel;																				///< Calls to trace() will only happen when the "precedence level" is less or equal to this. \details E.g. if traceLevel is CALL, only function calls and caught exceptions will be traced.
		protected:	Value tracerFunction;																				///< Pika-script tracer function (used by the default trace() implementation).
		protected:	bool isInsideTracer;																				///< Set to prevent recursive calling of tracer (used by the default trace() implementation).
		protected:	Char autoLabel[32];																					///< The last generated frame label (padded with leading ':').
		protected:	Char* autoLabelStart;																				///< The first character of the last generated frame label (begins at autoLabel + 30 and slowly moves backwards when necessary).
	};
	
	/**
		FullRoot inherits from both Root and Config::Globals (which should be a descendant to Variable). Its
		constructor adds the natives of the standard library. This means that by instantiating this class you will get
		a full execution environment for PikaScript ready to go.
	*/
	class FullRoot : public Root, public Config::Globals {
		public:		FullRoot(bool includeIONatives = true) : Root(*(typename Config::Globals*)(this)) {					///< If \p includeIO is false, 'load', 'save', 'input', 'print' and 'system' will not be registered.
						addLibraryNatives(*this, includeIONatives);
					}
	};
	
	/**
		STLVariables is the reference implementation of a variable space. It simply uses two std::map's for the
		PikaScript variables and the natives respectively. All registered natives are deleted on the destruction of this
		class.
		
		See Variables for descriptions on the overloaded member functions in this class.
	*/
	class STLVariables : public Variables {
		public:		typedef std::map<const String, Value> VariableMap;
		public:		typedef std::map<const String, Native* > NativeMap;
		public:		virtual bool lookup(const String& symbol, Value& result);
		public:		virtual bool assign(const String& symbol, const Value& value) { vars[symbol] = value; return true; }
		public:		virtual bool erase(const String& symbol) { return (vars.erase(symbol) > 0); }
		public:		virtual void list(const String& key, typename Variables::VarList& list);
		public:		virtual Native* lookupNative(const String& name);
		public:		virtual bool assignNative(const String& name, Native* native);
		public:		virtual ~STLVariables();
		public:		VariableMap vars;
		public:		NativeMap natives;
	};
	
	/**
		Native is the base class for the native functions and objects that can be accessed from PikaScript. It has a
		single virtual member function which should process a call to the native. Since natives are owned by the
		variable space once they are registered (and destroyed when the variable space destructs), they often act as
		simple bridges to other C++ functions and objects.
		
		The easiest way to register a native is by calling one of the Frame::registerNative() member functions
		(typically on the "root frame"). You will find a couple of template functions there that allows you to register
		functors directly. They will create the necessary Native classes for you in the background.
	*/
	class Native {
		public:		virtual Value pikaCall(Frame& /*frame*/) { throw Xception(STR("Not callable")); }					///< Process the PikaScript call. \details Arguments can be retrieved by getting \c $0, \c $1 etc from \p frame (via Frame::get() or Frame::getOptional()). If the function should not return a value, return \c Value() (which constructs a \c void value).
		public:		virtual ~Native();
	};

	/**
		We provide two Native template classes for bridging PikaScript calls to C++ "functors". One that takes a single
		argument (UnaryFunctor) and one that takes two arguments (BinaryFunctor). A "functor" is either a class that
		has an overloaded operator() or a C / C++ function. It is a concept introduced to C++ with STL so please refer
		to your STL documentation of choice for more info. (For example: http://www.sgi.com/tech/stl/functors.html )
		
		Thanks to some clever template tricks, these classes are very flexible when it comes to what type of arguments
		your functor can take and what type it may return. Here are your options:
		
		- Any argument can be of a type that is convertible from Script::Value (e.g., \c bool, \c long, \c double etc).
		- Likewise, the functor can return any type that is convertible to a Script::Value.
		- You can also use a functor with \c void result type.
		- The functor may take a single argument type of Script::Frame&. You can then retrieve all the arguments for
		the call by reading \c $0, \c $1, \c $2 etc from the Frame (via Frame::get() or Frame::getOptional()).
		
		In Frame you will find a template function (Frame::registerNative()) that allows you to register a native C++
		function directly through a functor. It will construct the proper functor instance for you "in the background".

		If you need use examples, please see the standard library implementation in PikaScriptImpl.h.
	*/
	template<class F, class A0 = typename F::argument_type, class R = typename F::result_type> class UnaryFunctor
			: public Native {
		public:		UnaryFunctor(const F& functor) : func(functor) { }
		protected:	template<class T> const Value arg(Frame& f, const T&) { return f.get(STR("$0")); }					///< Get the first argument from the frame \p f.
		protected:	Frame& arg(Frame& f, const Dumb<Frame&>&) { return f; }												///< Overloaded arg() that returns the actual frame instead of getting the argument value if the functor argument references a Script::Frame& type.
		protected:	const Frame& arg(Frame& f, const Dumb<const Frame&>&) { return f; }									///< Overloaded arg() that returns the actual frame instead of getting the argument value if the functor argument references a const Script::Frame& type.
		protected:	template<class A, class T> Value call(A& a, const Dumb<T>&) { return func(a); }						///< Call the functor with the argument \p a.
		protected:	template<class A> Value call(A& a, const Dumb<void>&) { func(a); return Value(); }					///< Overloaded to return the \c void value for functors that returns the \c void type.
		public:		virtual Value pikaCall(Frame& f) { return call(arg(f, Dumb<A0>()), Dumb<R>()); }
		protected:	F func;
	};
	
	/**
		See UnaryFunctor for documentation.
	*/
	template<class F, class A0 = typename F::first_argument_type, class A1 = typename F::second_argument_type
			, class R = typename F::result_type> class BinaryFunctor : public Native {
		public:		BinaryFunctor(const F& functor) : func(functor) { }
		protected:	template<class T> Value call(const Value& a0, const Value& a1, const T&) { return func(a0, a1); }	///< Call the functor with the arguments \p a0 and \p a1.
		protected:	Value call(const Value& a0, const Value& a1, const Dumb<void>&) { func(a0, a1); return Value(); }	///< Overloaded to return the \c void value for functors that returns the \c void type.
		public:		virtual Value pikaCall(Frame& f) { return call(f.get(STR("$0")), f.get(STR("$1")), Dumb<R>()); }
		protected:	F func;
	};
	
	template<class F> static UnaryFunctor<F>* newUnaryFunctor(const F& f) { return new UnaryFunctor<F>(f); }			///< Helper function to create a UnaryFunctor class with correct template parameters.
	template<class F> static BinaryFunctor<F>* newBinaryFunctor(const F& f) { return new BinaryFunctor<F>(f); }			///< Helper function to create a BinaryFunctor class with correct template parameters.

	/**
		getThisAndMethod splits the \c $callee variable of \p frame into object ("this") and method. The returned value
		is a pair, where the \c first value ("this") is a reference to the object and the \c second value is the
		"method" name as a string.
		
		\details
		Notice that if the $callee variable does not begin with a "frame specifier", it is assumed that the object
		belongs to the previous frame (e.g. the caller of the method). This holds true even if the method is actually
		defined in the root frame. For example \code function { obj.meth() } \endcode would trigger an error even if
		\c ::obj is defined since \c obj isn't defined in our function. While \code function { ::obj.meth() } \endcode
		works.
		
		One common use for this function is in a PikaScript object constructor for extracting the "this" reference
		that should be constructed. Another situation where this routine is useful is if you use the "elevate" function
		to aggregate various methods into a single C++ function. You may then use this function to extract the method
		name.
	*/
	static std::pair<Value, String> getThisAndMethod(Frame& frame);
	static Value getThis(Frame& frame) { return getThisAndMethod(frame).first; }										///< Returns only the "this" value as descripted in getThisAndMethod().
	static Value getMethod(Frame& frame) { return getThisAndMethod(frame).second; }										///< Returns only the "method" value as descripted in getThisAndMethod().

	struct lib {
		static Value elevate(Frame& frame);																				///< Used to "aggregate" different method calls into a single function call.
		static String character(double d);
		static bool deleter(const Frame& frame);
		static Value evaluate(const Frame& frame);
		static bool exists(const Frame& frame);
		static ulong find(const String& a, const String& b);
		static void foreach(Frame& frame);
		static String input(const String& prompt);
		static Value invoke(Frame& frame);
		static ulong length(const String& s);
		static String load(const String& file);
		static String lower(String s);
		static ulong mismatch(const String& a, const String& b);
		static uint ordinal(const String& s);
		static ulong parse(Frame& frame);
		static String precision(const Frame& frame);
		static void print(const String& s);
		static String radix(const Frame& frame);
		static double random(double m);
		static String reverse(String s);
		static void save(const String& file, const String& chars);
		static int system(const String& command);
		static ulong search(const String& a, const String& b);
		static ulong span(const String& a, const String& b);
		static void thrower(const String& s);
		static Value time(const Frame&);
		static void trace(const Frame& frame);
		static Value tryer(Frame& frame);
		static String upper(String s);
	};
	
	static void addLibraryNatives(Frame& frame, bool includeIO = true);													///< Registers the standard library native functions to \p frame. If \p includeIO is false, 'load', 'save', 'input', 'print' and 'system' will not be registered. Please, refer to the PikaScript standard library reference guide for more info on individual native functions.

}; // struct Script

/**
	StdConfig is a configuration class for Script that uses the reference implementations of STLValue and
	Script::STLVariables.
*/
struct StdConfig {
	typedef STLValue<std::string> Value;
	typedef Script<StdConfig>::STLVariables Locals;
	typedef Locals Globals;
};

/**
	This typedef exist for your convenience. If you wish to use the reference implementation of PikaScript you can now
	simply instantiate Pika::StdScript::FullRoot and off you go.
*/
typedef Script<StdConfig> StdScript;

#undef STR

} // namespace Pika

#endif
/**
	\file PikaScriptImpl.h
	
	PikaScriptImpl contains the template definitions for PikaScript.
	
	You only need to include this file if you want to instantiate a customization on the reference implementation on
	PikaScript. If you are satisfied with the reference implementation (StdScript), you only need to include
	PikaScript.h and add PikaScript.cpp to your project.
	                                                                           
	\version
	
	Version 0.97
	
	\page Copyright
	
	PikaScript is released under the BSD 2-Clause License. http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PikaScriptImpl_h
#define PikaScriptImpl_h

#include <math.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <limits>
#include <locale>
#if !defined(PikaScript_h)
#include "PikaScript.h"
#endif

namespace Pika {

template<typename T> inline T mini(T a, T b) { return (a < b) ? a : b; }
template<typename T> inline T maxi(T a, T b) { return (a < b) ? b : a; }

// Usually I am pretty militant against macros, but sorry, the following ones are just too handy.

// FIX : is there *some* way to solve this without ugly macros? 
#if (PIKA_UNICODE)
	#define STR(s) L##s
#else
	#define STR(x) x
#endif
#define TMPL template<class CFG>
#define T_TYPE(x) typename Script<CFG>::x

/* --- Utility Routines --- */

inline ushort ushortChar(char c) { return uchar(c); }
inline ushort ushortChar(wchar_t c) { return ushort(c); }
inline ulong ulongChar(char c) { return static_cast<uchar>(c); }
inline ulong ulongChar(wchar_t c) { return static_cast<ulong>(c); }
template<class C> std::basic_ostream<C>& xcout();
template<class C> std::basic_istream<C>& xcin();
template<> inline std::basic_ostream<char>& xcout() { return std::cout; }
template<> inline std::basic_ostream<wchar_t>& xcout() { return std::wcout; }
template<> inline std::basic_istream<char>& xcin() { return std::cin; }
template<> inline std::basic_istream<wchar_t>& xcin() { return std::wcin; }
template<> inline std::string toStdString(const std::string& s) { return s; }

inline ulong shiftRight(ulong l, int r) { return l >> r; }
inline ulong shiftLeft(ulong l, int r) { return l << r; }
inline ulong bitAnd(ulong l, ulong r) { return l & r; }
inline ulong bitOr(ulong l, ulong r) { return l | r; }
inline ulong bitXor(ulong l, ulong r) { return l ^ r; }
inline double modulo(double x, double y) { return fmod(x, y); }

template<class C> inline bool isSymbolChar(C c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
}

template<class C> inline bool maybeWhite(C c) { return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '/'); }

template<class S> std::string toStdString(const S& s) { return std::string(s.begin(), s.end()); }
template<> std::string toStdString(const std::string& s);
inline std::string& toStdString(std::string& s) { return s; }

template<class S> ulong hexToLong(typename S::const_iterator& p, const typename S::const_iterator& e) {
	assert(p <= e);
	ulong l = 0;
	for (; p < e && ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')); ++p)
		l = (l << 4) + (*p <= '9' ? *p - '0' : (*p & ~0x20) - ('A' - 10));
	return l;
}

template<class S> long stringToLong(typename S::const_iterator& p, const typename S::const_iterator& e) {
	assert(p <= e);
	bool negative = (e - p > 1 && ((*p == '+' || *p == '-') && p[1] >= '0' && p[1] <= '9') ? (*p++ == '-') : false);
	long l = 0;
	for (; p < e && *p >= '0' && *p <= '9'; ++p) l = l * 10 + (*p - '0');
	return negative ? -l : l;
}

template<class S, class T> S intToString(T i, int radix, int minLength) {
	assert(2 <= radix && radix <= 16);
	assert(0 <= minLength && minLength <= static_cast<int>(sizeof (T) * 8));
	typename S::value_type buffer[sizeof (T) * 8 + 1], * p = buffer + sizeof (T) * 8 + 1, * e = p - minLength;
	for (T x = i; p > e || x != 0; x /= radix) {
		assert(p >= buffer + 2);
		*--p = STR("fedcba9876543210123456789abcdef")[15 + x % radix];													// Mirrored hex string to handle negative x.
	}
	if (std::numeric_limits<T>::is_signed && i < 0) *--p = '-';
	return S(p, buffer + sizeof (T) * 8 + 1 - p);
}

template<class S> double stringToDouble(typename S::const_iterator& p, const typename S::const_iterator& e) {
	assert(p <= e);
	double d = 0, sign = (e - p > 1 && (*p == '+' || *p == '-') ? (*p++ == '-' ? -1.0 : 1.0) : 1.0);
	if (std::numeric_limits<double>::has_infinity && e - p >= 8 && equal(p, p + 8, STR("infinity"))) {
		p += 8;
		d = std::numeric_limits<double>::infinity();
	} else if (p < e && *p >= '0' && *p <= '9') {
		if (*p == '0') ++p; else do { d = d * 10.0 + (*p - '0'); } while (++p < e && *p >= '0' && *p <= '9');
		if (e - p > 1 && *p == '.' && p[1] >= '0' && p[1] <= '9') {
			++p;
			double f = 1.0;
			do { d += (*p - '0') * (f *= 0.1); } while (++p < e && *p >= '0' && *p <= '9');
		}
		if (e - p > 1 && (*p == 'E' || *p == 'e')) {
			typename S::const_iterator b = p;
			d *= pow(10, double(stringToLong<S>(++p, e)));
			if (p == b + 1) p = b;
		}
	}
	return d * sign;
}

template<class S> bool stringToDouble(const S& s, double& d) {
	typename S::const_iterator b = s.begin(), e = s.end(), p = b;
	d = stringToDouble<S>(p, e);
	return (p != b && p >= e);
}

template<class S> S doubleToString(double d, int precision) {
	assert(1 <= precision && precision <= 24);
	const double EPSILON = 1.0e-300, SMALL = 1.0e-5, LARGE = 1.0e+10;	
	double x = fabs(d), y = x;
	if (y <= EPSILON) return S(STR("0"));
	else if (precision >= 12 && y < LARGE && long(d) == d) return intToString<S, long>(long(d));
	else if (std::numeric_limits<double>::has_infinity && x == std::numeric_limits<double>::infinity())
		return d < 0 ? S(STR("-infinity")) : S(STR("+infinity"));
	typename S::value_type buffer[32], * bp = buffer + 2, * dp = bp, * pp = dp + 1, * ep = pp + precision;
	for (; x >= 10.0 && pp < ep; x *= 0.1) ++pp;																		// Normalize values > 10 and move period position.
	if (pp >= ep || y <= SMALL || y >= LARGE) {																			// Exponential treatment of very small or large values.
		double e = floor(log10(y) + 1.0e-10);
		S exps(e >= 0 ? S(STR("e+")) : S(STR("e")));
		exps += intToString<S, int>(int(e));
		int maxp = 15;																									// Limit precision because of rounding errors in log10 etc
		for (double f = fabs(e); f >= 8; f /= 10) --maxp;
		return (doubleToString<S>(d * pow(0.1, e), mini(maxp, precision)) += exps);
	}
	for (; x < 1.0 && dp < buffer + 32; ++ep, x *= 10.0) {																// For values < 1, spit out leading 0's and increase precision.
		*dp++ = '0';
		if (dp == pp) *dp++ = '9';																						// Hop over period position (set to 9 to avoid when eliminating 9's).
	}
	for (; dp < ep; ) {																									// Exhaust all remaining digits of mantissa into buffer.
		uint ix = uint(x);
		*dp++ = ix + '0';
		if (dp == pp) *dp++ = '9';																						// Hop over period position (set to 9 to avoid when eliminating 9's).
		x = (x - ix) * 10.0;
	}
	if (x >= 5) {																										// If remainder is >= 5, increment trailing 9's...
		while (dp[-1] == '9') *--dp = '0';
		if (dp == bp) *--bp = '1'; else dp[-1]++;																		// If we are at spare position, set to '1' and include, otherwise, increment last non-9.
	}
	*pp = '.';
	if (ep > pp) while (ep[-1] == '0') --ep;
	if (ep - 1 == pp) --ep;
	if (d < 0) *--bp = '-';	
	return S(bp, ep - bp);
}

const int ESCAPE_CODE_COUNT = 10;

template<class S> S unescape(typename S::const_iterator& p, const typename S::const_iterator& e) {
	assert(p <= e);
	typedef typename S::value_type CHAR;
	static const CHAR ESCAPE_CHARS[ESCAPE_CODE_COUNT] = { '\\', '\"', '\'',  'a',  'b',  'f',  'n',  'r',  't',  'v' };
	static const CHAR ESCAPE_CODES[ESCAPE_CODE_COUNT] = { '\\', '\"', '\'', '\a', '\b', '\f', '\n', '\r', '\t', '\v' };
	if (p >= e || (*p != '"' && *p != '\'')) throw Exception<S>(STR("Invalid string literal"));
	S d;
	typename S::const_iterator b = ++p;
	if (p[-1] == '\'') while (e - (p = std::find(p, e, '\'')) > 1 && p[1] == '\'') { d += S(b, ++p); b = ++p; }
	else while (p < e && *p != '\"') {
		if (*p == '\\' && e - p > 1) {
			d += S(b, p);
			const CHAR* f = std::find(ESCAPE_CHARS, ESCAPE_CHARS + ESCAPE_CODE_COUNT, *++p);
			ulong l;
			if (f != ESCAPE_CHARS + ESCAPE_CODE_COUNT) { ++p; l = ESCAPE_CODES[f - ESCAPE_CHARS]; }
			else if (*p == 'x') { b = ++p; l = hexToLong<S>(p, (e - p > 2 ? p + 2 : e)); }
			else if (*p == 'u') { b = ++p; l = hexToLong<S>(p, (e - p > 4 ? p + 4 : e)); }
			else if (*p == 'U') { b = ++p; l = hexToLong<S>(p, (e - p > 8 ? p + 8 : e)); }
			else { b = p; l = stringToLong<S>(p, e); }
			if (p == b) throw Exception<S>(STR("Invalid escape character"));
			b = p;
			d += CHAR(l);
		} else ++p;
	}
	if (p >= e) throw Exception<S>(STR("Unterminated string"));
	return (d += S(b, p++));
}

template<class S> S escape(const S& s) {
	typedef typename S::value_type CHAR;
	static const CHAR ESCAPE_CHARS[ESCAPE_CODE_COUNT] = { '\\', '\"', '\'',  'a',  'b',  'f',  'n',  'r',  't',  'v' };
	static const CHAR ESCAPE_CODES[ESCAPE_CODE_COUNT] = { '\\', '\"', '\'', '\a', '\b', '\f', '\n', '\r', '\t', '\v' };
	typename S::const_iterator b = s.begin(), e = s.end();
	bool needToBackUp = false;																								// If we need to re-escape with " " and string contained " or \ we need to start over from the beginning.
	for (; b < e && *b >= 32 && *b <= 126 && *b != '\''; ++b) needToBackUp = needToBackUp || (*b == '\\' || *b == '\"');
	if (b >= e) return ((S(STR("'")) += s) += '\'');
	if (needToBackUp) b = s.begin();
	typename S::const_iterator l = s.begin();
	S d = S(STR("\""));
	while (true) {
		while (b < e && *b >= 32 && *b <= 126 && *b != '\\' && *b != '\"') ++b;
		d += S(l, b);
		if (b >= e) break;
		const CHAR* f = std::find(ESCAPE_CODES, ESCAPE_CODES + ESCAPE_CODE_COUNT, *b);
		if (f != ESCAPE_CODES + ESCAPE_CODE_COUNT) (d += '\\') += ESCAPE_CHARS[f - ESCAPE_CODES];
		else if (uchar(*b) == ushortChar(*b)) (d += STR("\\x")) += intToString<S>(uchar(*b), 16, 2);
		else if (ushortChar(*b) == ulongChar(*b)) (d += STR("\\u")) += intToString<S>(ushortChar(*b), 16, 4);
		else (d += STR("\\U")) += intToString<S>(ulongChar(*b), 16, 8);
		l = ++b;
	}
	return (d += '\"');
}

/* --- STLValue --- */

template<class S> STLValue<S>::operator bool() const {
	if (S(*this) == STR("false")) return false;
	else if (S(*this) == STR("true")) return true;
	else throw Exception<S>(S(STR("Invalid boolean: ")) += escape(S(*this)));
}

template<class S> STLValue<S>::operator long() const {
	typename S::const_iterator p = S::begin();
	long y = stringToLong<S>(p, S::end());
	if (p == S::begin() || p < S::end()) throw Exception<S>(S(STR("Invalid integer: ")) += escape(S(*this)));
	return y;
}

template<class S> STLValue<S>::operator double() const {
	double d;
	if (!stringToDouble(*this, d)) throw Exception<S>(S(STR("Invalid number: ")) += escape(S(*this)));
	return d;
}

template<class S> bool STLValue<S>::operator<(const STLValue& r) const {
	double lv, rv;
	bool lnum = stringToDouble(*this, lv), rnum = stringToDouble(r, rv);
	return (lnum == rnum ? (lnum ? lv < rv : (const S&)(*this) < (const S&)(r)) : lnum);
}

template<class S> bool STLValue<S>::operator==(const STLValue& r) const {
	double lv, rv;
	bool lnum = stringToDouble(*this, lv), rnum = stringToDouble(r, rv);
	return (lnum == rnum && (lnum ? lv == rv : (const S&)(*this) == (const S&)(r)));
}

template<class S> const STLValue<S> STLValue<S>::operator[](const STLValue& i) const {
	typename S::const_iterator b = S::begin(), p = S::end();
	if (p > b) switch (*(p - 1)) {
		case '$':	if (--p == b) break; /* else continue */
		case '^':	while (p > b && *(p - 1) == '^') --p; if (p == b) break; /* else continue */
		case ':':	if (p - 1 > b && *(p - 1) == ':' && *b == ':' && std::find(b + 1, p, ':') == p - 1) p = b; break;
	}
	return (p == b) ? S(*this) + S(i) : (S(*this) + STR('.')) += S(i);
}

/* --- Frame --- */

TMPL Script<CFG>::Frame::Frame(Variables& vars, Root& root, Frame* previous) : vars(vars), root(root)
		, previous(previous), closure(this), label(previous == 0 ? String(STR("::")) : root.generateLabel()) { }

TMPL T_TYPE(Value) Script<CFG>::Frame::rvalue(const XValue& v, bool fallback) {
	return !v.first ? v.second : get(v.second, fallback);
}

TMPL const T_TYPE(Value)& Script<CFG>::Frame::lvalue(const XValue& v) {
	if (!v.first) throw Xception(STR("Invalid lvalue"));
	return v.second;
}

TMPL T_TYPE(Frame*) Script<CFG>::Frame::resolveFrame(StringIt& p, const StringIt& e) const {
	assert(p <= e);
	Frame* f = const_cast<Frame*>(this);
	if (p < e && *p == ':') {
		StringIt n = std::find(p + 1, e, ':');
		if (n >= e) throw Xception(String(STR("Invalid identifier: ")) += escape(String(p, e)));
		if (n - p > 1) {
			String s(p, n + 1);
			while (f->label != s)
				if ((f = f->previous) == 0) throw Xception(String(STR("Frame does not exist: ")) += escape(s));
		} else f = &root;
		p = n + 1;
	}
	for (; p < e && *p == '^'; ++p)
		if ((f = f->previous) == 0) throw Xception(STR("Frame does not exist"));
	if (p >= e || *p != '$') f = f->closure;
	return f;
}

TMPL std::pair< T_TYPE(Frame*), T_TYPE(String) > Script<CFG>::Frame::resolveFrame(const String& identifier) const {
	switch (identifier[0]) {
		default:	return std::pair<Frame*, String>(closure, identifier);
		case '$':	return std::pair<Frame*, String>(const_cast<Frame*>(this), identifier);
		case ':': case '^': {
			StringIt b = const_cast<const String&>(identifier).begin(), e = const_cast<const String&>(identifier).end();
			Frame* frame = resolveFrame(b, e);
			return std::pair<Frame*, String>(frame, String(b, e));
		}
	}
}

TMPL T_TYPE(Value) Script<CFG>::Frame::get(const String& identifier, bool fallback) const {
	Value temp;
	std::pair<Frame*, String> fs = resolveFrame(identifier);
	if (fs.first->vars.lookup(fs.second, temp)) return temp;
	else if (fallback && identifier[0] != '$' && isSymbolChar(identifier[0]) && root.vars.lookup(fs.second, temp))
		return temp;
	else throw Xception(String(STR("Undefined: ")) += escape(identifier));
}

TMPL T_TYPE(Value) Script<CFG>::Frame::getOptional(const String& identifier, const Value& defaultValue) const {
	std::pair<Frame*, String> fs = resolveFrame(identifier);
	Value temp;
	return fs.first->vars.lookup(fs.second, temp) ? temp : defaultValue;
}

TMPL const T_TYPE(Value)& Script<CFG>::Frame::set(const String& identifier, const Value& v) {
	std::pair<Frame*, String> fs = resolveFrame(identifier);
	if (!fs.first->vars.assign(fs.second, v)) throw Xception(String(STR("Cannot modify: ")) += escape(identifier));
	return v;
}

TMPL T_TYPE(Value) Script<CFG>::Frame::reference(const String& identifier) const {
	std::pair<Frame*, String> fs = resolveFrame(identifier);
	return fs.first->label + fs.second;
}

TMPL T_TYPE(Value) Script<CFG>::Frame::execute(const String& body) {
	StringIt b = body.begin(), e = body.end();
	switch (b < e ? *b : 0) {
		case '{':	return evaluate(body);																				// <-- function
		case '>':	closure = resolveFrame(++b, e);																		// <-- lambda
					return evaluate(String(b, e));
		case '<':	if (--e - ++b > 0) {																				// <-- native
						Frame* nativeFrame = (*b == ':' ? resolveFrame(b, e) : &root);
						Native* native = nativeFrame->vars.lookupNative(String(b, e));
						if (native != 0) return native->pikaCall(*this);
					}
					throw Xception(String(STR("Unknown native function: ")) += escape(body));
		default:	throw Xception(String(STR("Illegal call on: ")) + escape(body));
	}
}

TMPL void Script<CFG>::Frame::white(StringIt& p, const StringIt& e) {
	assert(p <= e);
	while (p < e) {
		switch (*p) {
			case ' ': case '\t': case '\r': case '\n': ++p; break;
			case '/':	if (e - p > 1 && p[1] == '/') {
							static const Char END_CHARS[] = { '\r', '\n' };
							p = find_first_of(p += 2, e, END_CHARS, END_CHARS + 2);
							break;
						} else if (e - p > 1 && p[1] == '*') {
							static const Char END_CHARS[] = { '*', '/' };
							p = search(p += 2, e, END_CHARS, END_CHARS + 2);
							if (p >= e) throw Xception(STR("Missing '*/'"));
							p += 2;
							break;
						} /* else continue */
			default:	return;
		}
	}
}

TMPL bool Script<CFG>::Frame::token(StringIt& p, const StringIt& e, const Char* token) {
	assert(p <= e);
	StringIt t = p + 1;
	while (*token != 0 && t < e && *t == *token) { ++t; ++token; }
	if (*token == 0 && (t >= e || !isSymbolChar(*t))) {
		if ((p = t) < e && maybeWhite(*p)) white(p, e);
		return true;
	} else return false;
}

TMPL template<class F> bool Script<CFG>::Frame::binaryOp(StringIt& p, const StringIt& e, XValue& v, bool dry
		, Precedence thres, int hop, Precedence prec, F op) {
	assert(p <= e);
	assert(hop >= 0);
	if (thres >= prec) return false;
	XValue r;
	expr(p += hop, e, r, false, dry, prec);
	if (!dry) v = XValue(false, op(rvalue(v), rvalue(r)));
	return true;
}

TMPL template<class F> bool Script<CFG>::Frame::assignableOp(StringIt& p, const StringIt& e, XValue& v, bool dry
		, Precedence thres, int hop, Precedence prec, F op) {
	assert(p <= e);
	assert(hop >= 0);
	if (p + hop >= e || p[hop] != '=') return binaryOp(p, e, v, dry, thres, hop, prec, op);
	if (thres > ASSIGN) return false;
	XValue r;																											// <-- operate and assign
	expr(p += hop + 1, e, r, false, dry, ASSIGN);
	if (!dry) v = XValue(false, set(lvalue(v), op(rvalue(v, false), rvalue(r))));
	return true;
}

TMPL template<class F> bool Script<CFG>::Frame::addSubOp(StringIt& p, const StringIt& e, XValue& v, bool dry
		, Precedence thres, const F& f) {
	assert(p <= e);
	if (p + 1 >= e || p[1] != *p) return assignableOp(p, e, v, dry, thres, 1, ADD_SUB, f);
	else if (thres >= POSTFIX) return false;
	else if (!dry) {
		Value r = rvalue(v, false);																						// <-- post inc/dec
		set(lvalue(v), f(long(r), 1));
		v = XValue(false, r);
	}
	p += 2;
	return true;
}

TMPL template<class E, class I, class S> bool Script<CFG>::Frame::lgtOp(StringIt& p, const StringIt& e, XValue& v
		, bool dry, Precedence thres, const E& excl, const I& incl, S shift) {
	assert(p <= e);
	if (e - p > 1 && p[1] == *p) return assignableOp(p, e, v, dry, thres, 2, SHIFT, shift);								// <-- shift
	else if (e - p > 1 && p[1] == '=') return binaryOp(p, e, v, dry, thres, 2, COMPARE, incl);							// <-- less/greater or equal
	else return binaryOp(p, e, v, dry, thres, 1, COMPARE, excl);														// <-- less/greater
}

TMPL bool Script<CFG>::Frame::pre(StringIt& p, const StringIt& e, XValue& v, bool dry) {
	assert(p <= e);
	StringIt b = p;	
	switch (p < e ? *p : 0) {
		case 0:		return false;
		case '!':	expr(++p, e, v, false, dry, PREFIX); if (!dry) v = XValue(false, !rvalue(v)); return true;			// <-- logical not
		case '~':	expr(++p, e, v, false, dry, PREFIX); if (!dry) v = XValue(false, ~ulong(rvalue(v))); return true;	// <-- bitwise not
		case '(':	termExpr(++p, e, v, false, dry, BRACKETS, ')'); return true;										// <-- parenthesis
		case ':':	if (e - p > 1 && p[1] == ':') p += 2; break;														// <-- root
		case '^':	while (++p < e && *p == '^'); break;																// <-- frame peek
		case '@':	expr(++p, e, v, false, dry, PREFIX); if (!dry) v = XValue(false, reference(lvalue(v))); return true;// <-- reference
		case '[':	termExpr(++p, e, v, false, dry, BRACKETS, ']'); if (!dry) v = XValue(true, rvalue(v)); return true;	// <-- indirection
		case '<':	p = std::find(p, e, '>'); if (p < e) ++p; if (!dry) v = XValue(false, String(b, p)); return true;	// <-- native literal
		case '\'': case '"': { Value s(unescape<String>(p, e)); if (!dry) v = XValue(false, s); } return true;			// <-- string literal
		case 'e':	if (token(p, e, STR("lse"))) throw Xception(STR("Unexpected 'else' (preceded by ';'?)")); break;	// <-- error on unexpected else
		case 't':	if (token(p, e, STR("rue"))) { if (!dry) v = XValue(false, true); return true; } break;				// <-- true literal
		case 'v':	if (token(p, e, STR("oid"))) { if (!dry) v = XValue(false, Value()); return true; } break;			// <-- void literal
		
		case '>':	if (++p < e && maybeWhite(*p)) white(p, e);															// <-- lambda
					b = p;
					expr(p, e, v, false, true, STATEMENT);
					if (!dry) v = XValue(false, (String(STR(">")) += closure->label) += String(b, p));
					return true;

		case '{':	do { expr(++p, e, v, true, dry, STATEMENT); } while (p < e && *p == ';');							// <-- compound
					if (p >= e) throw Xception(STR("Missing '}'"));
					if (*p != '}') throw Xception(STR("Syntax error (missing ';')?"));
					++p;
					return true;
		
		case '+': case '-':
					if (token(p, e, STR("infinity"))) p = b + 1; /* and continue to stringToDouble */					// <-- infinity literal
					else if (++p >= e) return false;
					else if (*p == *b) {
						expr(++p, e, v, false, dry, PREFIX);															// <-- pre inc/dec
						if (!dry) v = XValue(false, set(lvalue(v), long(rvalue(v, false)) + (*b == '-' ? -1 : 1)));
						return true;
					} else if (*p < '0' || *p > '9') {
						expr(p, e, v, false, dry, PREFIX);																// <-- positive / negative
						if (!dry) v = XValue(false, *b == '-' ? -double(rvalue(v)) : double(rvalue(v)));
						return true;
					} /* else continue */
							
		case '0':	if (e - p > 1 && p[1] == 'x') {
						ulong l = hexToLong<String>(p += 2, e);															// <-- hexadecimal literal
						if (p == b + 2) throw Xception(STR("Invalid hexadecimal number"));
						if (!dry) v = XValue(false, *b == '-' ? -long(l) : l);
						return true;
					} /* else continue */
		
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {						// <-- numeric literal
						double d = stringToDouble<String>(p, e);
						if (!dry) v = XValue(false, *b == '-' ? -d : d);
					}
					return true;
					
		case 'f': 	if (token(p, e, STR("alse"))) { if (!dry) v = XValue(false, false); return true; }					// <-- false literal
					else if (token(p, e, STR("or"))) {
						if (p >= e || *p != '(') throw Xception(STR("Expected '('"));									// <-- for
						XValue xv;
						termExpr(++p, e, xv, true, dry, ARGUMENT, ';');
						StringIt cp = p;
						termExpr(p, e, xv, true, dry, ARGUMENT, ';');
						StringIt ip = p;
						termExpr(p, e, xv, true, true, ARGUMENT, ')');
						StringIt bp = p;
						bool cb = !dry && rvalue(xv);
						do {
							expr(p = bp, e, v, true, !cb, BODY);
							if (cb) {
								if (root.doTrace(TRACE_LOOP)) tick(p, v, TRACE_LOOP, true);
								StringIt ep = p;
								expr(p = ip, e, xv, true, false, ARGUMENT);
								expr(p = cp, e, xv, true, false, ARGUMENT);
								p = ep;
								cb = rvalue(xv);
							}
						} while (cb);
						if (!dry && root.doTrace(TRACE_LOOP)) tick(p, v, TRACE_LOOP, false);
						return true;
					} else if (token(p, e, STR("unction"))) {
						if (p >= e || *p != '{') throw Xception(STR("Expected '{'"));
						b = p;
						expr(p, e, v, false, true, DEFINITION);															// <-- function
						if (!dry) v = XValue(false, String(b, p));
						return true;
					}
					break;

		case 'i':	if (e - p > 1 && token(p, e, STR("f"))) {
						if (p >= e || *p != '(') throw Xception(STR("Expected '('"));									// <-- if
						XValue c;
						termExpr(++p, e, c, false, dry, ARGUMENT, ')');
						bool b = dry || rvalue(c);
						expr(p, e, v, false, dry || !b, BODY);
						if (p < e && *p == 'e' && token(p, e, STR("lse"))) expr(p, e, v, false, dry || b, BODY);		// <-- else
						return true;
					}
					break;
	}
	while (p < e && isSymbolChar(*p)) ++p;
	if (b != p && !dry) v = XValue(true, String(b, p));
	return (b != p);
}

TMPL long Script<CFG>::Frame::intDiv(long x, long y) {
	if (y == 0) {
		throw Xception(STR("Division by zero"));
	}
	return x / y;
}

TMPL bool Script<CFG>::Frame::post(StringIt& p, const StringIt& e, XValue& v, bool dry, Precedence thres) {
	assert(p <= e);
	switch (p < e ? *p : 0) {
		case 0:		return false;
		case ' ': case '\t': case '\r': case '\n':
					if (thres < DEFINITION) { Char c = *p; while (++p < e && *p == c) { } return true; } break;			// <-- white spaces
		case '/':	if (thres < DEFINITION && e - p > 1 && (p[1] == '/' || p[1] == '*')) { white(p, e); return true; }	// <-- comment
					return assignableOp(p, e, v, dry, thres, 1, MUL_DIV, std::divides<double>());						// <-- divide
		case '+':	return addSubOp(p, e, v, dry, thres, std::plus<double>());											// <-- add
		case '-':	return addSubOp(p, e, v, dry, thres, std::minus<double>());											// <-- subtract
		case '#':	return assignableOp(p, e, v, dry, thres, 1, CONCAT, std::plus<String>());							// <-- concat
		case '*':	return assignableOp(p, e, v, dry, thres, 1, MUL_DIV, std::multiplies<double>());					// <-- multipy
		case '\\':	return assignableOp(p, e, v, dry, thres, 1, MUL_DIV, intDiv);										// <-- integer division
		case '%':	return assignableOp(p, e, v, dry, thres, 1, MUL_DIV, modulo);										// <-- modulus
		case '^':	return assignableOp(p, e, v, dry, thres, 1, BIT_XOR, bitXor);										// <-- xor
		case '<':	return lgtOp(p, e, v, dry, thres, std::less<Value>(), std::less_equal<Value>(), shiftLeft);			// <-- shift left
		case '>':	return lgtOp(p, e, v, dry, thres, std::greater<Value>(), std::greater_equal<Value>(), shiftRight);	// <-- shift right
		
		case '!':	if (e - p > 2 && p[2] == '=' && p[1] == '=')
						return binaryOp(p, e, v, dry, thres, 3, EQUALITY, std::not_equal_to<String>());					// <-- literal not equals
					else if (e - p > 1 && p[1] == '=')
						return binaryOp(p, e, v, dry, thres, 2, EQUALITY, std::not_equal_to<Value>());					// <-- not equals
					break;

		case '=':	if (e - p > 2 && p[2] == '=' && p[1] == '=')
						return binaryOp(p, e, v, dry, thres, 3, EQUALITY, std::equal_to<String>());						// <-- literal equals
					else if (e - p > 1 && p[1] == '=')
						return binaryOp(p, e, v, dry, thres, 2, EQUALITY, std::equal_to<Value>());						// <-- equals
					else if (thres <= ASSIGN) {
						XValue r;																						// <-- assign
						expr(++p, e, r, false, dry, ASSIGN);
						if (!dry) v = XValue(false, set(lvalue(v), rvalue(r)));
						return true;
					}
					break;

		case '&':	if (e - p < 2 || p[1] != '&') return assignableOp(p, e, v, dry, thres, 1, BIT_AND, bitAnd);			// <-- bitwise and
					else if (thres < LOGICAL_AND) {
						bool l = !dry && rvalue(v);																		// <-- logical and
						expr(p += 2, e, v, false, !l, LOGICAL_AND);
						if (!dry) v = XValue(false, l && rvalue(v));
						return true;
					}
					break;
		
		case '|': 	if (e - p < 2 || p[1] != '|') return assignableOp(p, e, v, dry, thres, 1, BIT_OR, bitOr);			// <-- bitwise or
					else if (thres < LOGICAL_OR) {
						bool l = dry || rvalue(v);																		// <-- logical or
						expr(p += 2, e, v, false, l, LOGICAL_OR);
						if (!dry) v = XValue(false, l || rvalue(v));
						return true;
					}
					break;
		
		case '.':	{																									// <-- member
						if (++p < e && maybeWhite(*p)) white(p, e);
						StringIt b = p;
						while (p < e && isSymbolChar(*p)) ++p;
						if (!dry) v = XValue(true, lvalue(v)[String(b, p)]);
						return true;
					}
					
		case '[':	if (thres < POSTFIX) {																				// <-- subscript
						XValue element;
						termExpr(++p, e, element, false, dry, BRACKETS, ']');
						if (!dry) v = XValue(true, lvalue(v)[rvalue(element)]);
						return true;
					}
					break;
					
		case '{':	if (thres < POSTFIX) {																				// <-- substring
						XValue index;
						bool gotIndex = expr(++p, e, index, true, dry, BRACKETS);
						if (p >= e || (*p != ':' && *p != '}')) throw Xception(STR("Expected '}' or ':'"));
						if (*p++ == ':') {
							XValue count;
							bool gotCount = termExpr(p, e, count, true, dry, BRACKETS, '}');
							if (!dry) {
								String s = rvalue(v);
								long i = !gotIndex ? 0L : long(rvalue(index));
								long n = gotCount ? long(rvalue(count)) + mini(i, 0L) : String::npos;
								v = XValue(false, i <= long(s.size()) && (!gotCount || n >= 0L)
										? Value(s.substr(maxi(i, 0L), n)) : Value());
							}
						} else if (gotIndex && !dry) {
							String s = rvalue(v);
							long i = long(rvalue(index));
							v = XValue(false, i >= 0L && i <= long(s.size()) ? Value(s.substr(i, 1)) : Value());
						} else if (!dry) throw Xception(STR("Syntax error"));
						return true;
					}
					break;

		case '(': 	if (thres < POSTFIX) {																				// <-- call
						typename CFG::Locals locals;
						Frame calleeFrame(locals, root, this);
						long n = 0;
						do {
							if (++p < e && maybeWhite(*p)) white(p, e);
							if (p < e && *p == ')' && n == 0) break;
							XValue arg;
							if (expr(p, e, arg, true, dry, ARGUMENT) && !dry)
								locals.assign(String(STR("$")) += intToString<String>(n), rvalue(arg));
							++n;
						} while (p < e && *p == ',');
						if (p >= e || *p != ')') throw Xception(STR("Expected ',' or ')'"));
						++p;
						if (!dry) {
							locals.assign(STR("$n"), n);
							if (v.first) locals.assign(STR("$callee"), v.second);
							v = XValue(false, calleeFrame.execute(rvalue(v)));
						}
						return true;
					}
					break;
	}
	return false;
}

TMPL void Script<CFG>::Frame::tick(const StringIt& p, const XValue& v, Precedence thres, bool exit) {
	root.trace(*this, *source, p - source->begin(), v.first, v.second, thres, exit);
}

TMPL bool Script<CFG>::Frame::expr(StringIt& p, const StringIt& e, XValue& v, bool emptyOk, bool dry, Precedence thres) {
	assert(p <= e);
	if (p < e && maybeWhite(*p)) white(p, e);
	if (!dry && root.doTrace(thres)) tick(p, v, thres, false);
	if (pre(p, e, v, dry)) {
		while (post(p, e, v, dry, thres));
		if (!dry && root.doTrace(thres)) tick(p, v, thres, true);
		return true;
	} else if (!emptyOk) throw Xception(STR("Syntax error"));
	return false;
}

TMPL bool Script<CFG>::Frame::termExpr(StringIt& p, const StringIt& e, XValue& v, bool emptyOk, bool dry
		, Precedence thres, Char term) {
	assert(p <= e);
	bool nonEmpty = expr(p, e, v, emptyOk, dry, thres);
	if (p >= e || *p != term) throw Xception((String(STR("Missing '")) += String(&term, 1)) += '\'');
	++p;
	return nonEmpty;
}

TMPL T_TYPE(Value) Script<CFG>::Frame::evaluate(const String source) {
	XValue v;
	const String* oldSource = this->source;
	this->source = &source;
	try {
		StringIt p = source.begin(), e = source.end();
		if (root.doTrace(TRACE_CALL)) tick(p, v, TRACE_CALL, false);
		try {
			try {
				while (p < e) {
					expr(p, e, v, true, false, STATEMENT);
					if (p < e) {
						if (*p != ';') throw Xception(STR("Syntax error"));
						++p;
					}
				}
				v = XValue(false, rvalue(v));
			} catch (const Xception& x) {
				if (root.doTrace(TRACE_ERROR)) tick(p, XValue(false, x.getError()), TRACE_ERROR, previous == 0);
				throw;
			} catch (const std::exception& x) {
				if (root.doTrace(TRACE_ERROR)) {
					const char* s = x.what();
					String err = String(std::basic_string<Char>(s, s + strlen(s)));
					tick(p, XValue(false, err), TRACE_ERROR, previous == 0);
				}
				throw;
			} catch (...) {
				if (root.doTrace(TRACE_ERROR))
					tick(p, XValue(false, STR("Unknown exception")), TRACE_ERROR, previous == 0);
				throw;
			}
		} catch (...) {
			if (root.doTrace(TRACE_CALL)) tick(p, v, TRACE_CALL, true);
			throw;
		}
		if (root.doTrace(TRACE_CALL)) tick(p, v, TRACE_CALL, true);
	} catch (...) {
		this->source = oldSource;
		throw;
	}
	this->source = oldSource;
	return v.second;
}

TMPL T_TYPE(StringIt) Script<CFG>::Frame::parse(const StringIt& begin, const StringIt& end, bool literal) {
	assert(begin <= end);
	StringIt p = begin, e = end;
	XValue dummy;
	if (!literal) expr(p, end, dummy, true, true, STATEMENT);
	else switch (p < e ? *p : 0) {
		case 'f': if (!token(p, e, STR("alse")) && token(p, e, STR("unction"))) pre(p = begin, e, dummy, true); break;
		case 't': token(p, e, STR("rue")); break;
		case 'v': token(p, e, STR("oid")); break;
		case '+': case '-': if (token(p, e, STR("infinity")) || p + 1 >= e || p[1] < '0' || p[1] > '9') break;
		case '<': case '>': case '0': case '\'': case '"': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9': pre(p, e, dummy, true); break;		
	}
	return p;
}

TMPL T_TYPE(Value) Script<CFG>::Frame::call(const String& callee, const Value& body, long argc, const Value* argv) {
	assert(argc >= 0);
	typename CFG::Locals locals;
	Frame calleeFrame(locals, root, this);
	locals.assign(STR("$n"), argc);
	for (long i = 0; i < argc; ++i) locals.assign(String(STR("$")) += intToString<String>(i), argv[i]);
	if (!callee.empty()) locals.assign(STR("$callee"), callee);
	return calleeFrame.execute(body.empty() ? get(callee, true) : body);
}

TMPL void Script<CFG>::Frame::registerNative(const String& identifier, Native* native) {
	std::pair<Frame*, String> p = resolveFrame(identifier);
	if (!p.first->vars.assignNative(p.second, native))
		throw Xception(String(STR("Cannot register native: ")) += escape(identifier));
	if (native != 0)
		p.first->set(p.second, (String(STR("<")) += (p.first == &root ? p.second : p.first->label + p.second)) += '>');
}

/* --- Root --- */

TMPL Script<CFG>::Root::Root(Variables& vars) : Frame(vars, *this, 0), traceLevel(NO_TRACE), isInsideTracer(false)
		, autoLabelStart(autoLabel + 29) {
	std::fill_n(autoLabel, 32, ':');
}

TMPL T_TYPE(String) Script<CFG>::Root::generateLabel() {
	Char* b = autoLabelStart, * p = autoLabel + 30;
	while (*--p == 'z');
	switch (*p) {
		case ':':	*p = '1'; *--b = ':'; autoLabelStart = b; break;
		case '9':	*p = 'A'; break;
		case 'Z':	*p = 'a'; break;
		default:	(*p)++; break;
	}
	for (++p; *p != ':'; ++p) *p = '0';
	return String(const_cast<const Char*>(b), autoLabel + 31 - b);
}

TMPL void Script<CFG>::Root::setTracer(Precedence traceLevel, const Value& tracerFunction) throw() {
	this->traceLevel = traceLevel;
	this->tracerFunction = tracerFunction;
}

TMPL void Script<CFG>::Root::trace(Frame& frame, const String& source, SizeType offset, bool lvalue, const Value& value
		, Precedence level, bool exit) {
	if (!tracerFunction.isVoid() && !isInsideTracer) {
		try {
			isInsideTracer = true;
			Value argv[6] = { source, static_cast<ulong>(offset), lvalue, value, int(level), exit };
			frame.call(String(), tracerFunction, 6, argv);
			isInsideTracer = false;
		} catch (...) {
			isInsideTracer = false;
			setTracer(NO_TRACE, Value());																				// Turn off tracing on uncaught exceptions.
			throw;
		}
	}
}

/* --- STLVariables --- */

TMPL bool Script<CFG>::STLVariables::lookup(const String& symbol, Value& result) {
	typename VariableMap::const_iterator it = vars.find(symbol);
	if (it == vars.end()) return false;
	result = it->second;
	return true;
}

TMPL void Script<CFG>::STLVariables::list(const String& key, typename Variables::VarList& list) {
	for (typename VariableMap::const_iterator it = vars.lower_bound(key)
			; it != vars.end() && it->first.substr(0, key.size()) == key; ++it) list.push_back(*it);
}

TMPL T_TYPE(Native*) Script<CFG>::STLVariables::lookupNative(const String& identifier) {
	typename NativeMap::iterator it = natives.find(identifier);
	return (it == natives.end() ? 0 : it->second);
}

TMPL bool Script<CFG>::STLVariables::assignNative(const String& identifier, Native* native) {
	typename NativeMap::iterator it = natives.insert(typename NativeMap::value_type(identifier, (Native*)(0))).first;
	if (it->second != native) delete it->second;
	it->second = native;
	return true;
}

TMPL Script<CFG>::STLVariables::~STLVariables() {
	for (typename NativeMap::iterator it = natives.begin(); it != natives.end(); ++it) delete it->second;
}

TMPL std::pair<T_TYPE(Value), T_TYPE(String)> Script<CFG>::getThisAndMethod(Frame& f) {
	const String fn = f.get(STR("$callee"));
	StringIt it = std::find(fn.rbegin(), fn.rend(), '.').base();
	if (it <= fn.begin()) throw Xception(STR("Non-method call"));
	return std::pair<Value, String>(f.getPrevious().reference(String(fn.begin(), it - 1)), String(it, fn.end()));
}

/* --- Standard Library --- */

TMPL T_TYPE(Value) Script<CFG>::lib::elevate(Frame& f) { return f.execute(f.get(getThisAndMethod(f).first, true)); }
TMPL ulong Script<CFG>::lib::length(const String& s) { return static_cast<ulong>(s.size()); }
TMPL void Script<CFG>::lib::print(const String& s) { xcout<Char>() << std::basic_string<Char>(s) << std::endl; }
TMPL double Script<CFG>::lib::random(double m) { return m * rand() / double(RAND_MAX); }
TMPL T_TYPE(String) Script<CFG>::lib::reverse(String s) { std::reverse(s.begin(), s.end()); return s; }
TMPL void Script<CFG>::lib::thrower(const String& s) { throw Xception(s); }
TMPL T_TYPE(Value) Script<CFG>::lib::time(const Frame&) { return double(::time(0)); }

TMPL T_TYPE(String) Script<CFG>::lib::upper(String s) {
	transform(s.begin(), s.end(), s.begin(), std::bind2nd(std::ptr_fun(std::toupper<Char>), std::locale::classic()));
	return s;
}

TMPL T_TYPE(String) Script<CFG>::lib::lower(String s) {
	transform(s.begin(), s.end(), s.begin(), std::bind2nd(std::ptr_fun(std::tolower<Char>), std::locale::classic()));
	return s;
}

TMPL T_TYPE(String) Script<CFG>::lib::character(double d) {
	if (ushortChar(Char(d)) != d) throw Xception(String(STR("Illegal character code: ")) += doubleToString<String>(d));
	return String(1, Char(d));
}

TMPL uint Script<CFG>::lib::ordinal(const String& s) {
	if (s.size() != 1) throw Xception(String(STR("Value is not single character: ")) += escape(s));
	return ushortChar(s[0]);
}

TMPL bool Script<CFG>::lib::deleter(const Frame& f) {
	Value x = f.get(STR("$0"));
	std::pair<Frame*, String> fs = f.getPrevious().resolveFrame(x);
	return fs.first->getVariables().erase(fs.second);
}

TMPL T_TYPE(Value) Script<CFG>::lib::evaluate(const Frame& f) {
	return f.resolveFrame(f.getOptional(STR("$1"))).first->evaluate(f.get(STR("$0")));
}

TMPL bool Script<CFG>::lib::exists(const Frame& f) {
	Value x = f.get(STR("$0"));
	Value result;
	std::pair<Frame*, String> fs = f.getPrevious().resolveFrame(x);
	return fs.first->getVariables().lookup(fs.second, result);
}

TMPL ulong Script<CFG>::lib::find(const String& a, const String& b) {
	return static_cast<ulong>(find_first_of(a.begin(), a.end(), b.begin(), b.end()) - a.begin());
}

TMPL void Script<CFG>::lib::foreach(Frame& f) {
	Value arg1 = f.get(STR("$1"));
	std::pair<Frame*, String> fs = f.getPrevious().resolveFrame(f.get(STR("$0"))[Value()]); 
	typename Variables::VarList list;
	fs.first->getVariables().list(fs.second, list);
	for (typename Variables::VarList::const_iterator it = list.begin(); it != list.end(); ++it) {
		Value argv[3] = { fs.first->reference(it->first), it->first.substr(fs.second.size()), it->second };
		f.call(String(), arg1, 3, argv);
	}
}

TMPL T_TYPE(String) Script<CFG>::lib::input(const String& prompt) {
	xcout<Char>() << std::basic_string<Char>(prompt);
	std::basic_string<Char> s;
	std::basic_istream<Char>& instream = xcin<Char>();
	if (instream.eof()) throw Xception(STR("Unexpected end of input file"));
	if (!instream.good()) throw Xception(STR("Input file error"));
	getline(instream, s);
	return s;
}

TMPL T_TYPE(Value) Script<CFG>::lib::invoke(Frame& f) {
	Value source = f.get(STR("$2")), arg4 = f.getOptional(STR("$4"));
	long offset = long(f.getOptional(STR("$3"), 0));
	long n = arg4.isVoid() ? long(f.get(source[String(STR("n"))])) - offset : long(arg4);
	if (n < 0) throw Xception(STR("Too few array elements"));
	std::vector<Value> a(n);
	for (long i = 0; i < long(a.size()); ++i) a[i] = f.get(source[i + offset]);
	return f.call(f.getOptional(STR("$0")), f.getOptional(STR("$1")), long(a.size()), a.empty() ? 0 : &a[0]);
}

TMPL T_TYPE(String) Script<CFG>::lib::load(const String& file) {
	std::basic_ifstream<Char> instream(toStdString(file).c_str());															// Sorry, can't pass a wchar_t filename. MSVC supports it, but it is non-standard. So we convert to a std::string to be on the safe side.
	if (!instream.good()) throw Xception(String(STR("Cannot open file for reading: ")) += escape(file));
	String chars;
	while (!instream.eof()) {
		if (!instream.good()) throw Xception(String(STR("Error reading from file: ")) += escape(file));
		Char buffer[4096];
		instream.read(buffer, 4096);
		chars += String(buffer, static_cast<typename String::size_type>(instream.gcount()));
	}
	return chars;
}

TMPL ulong Script<CFG>::lib::mismatch(const String& a, const String& b) {
	if (a.size() > b.size()) return static_cast<ulong>(std::mismatch(b.begin(), b.end(), a.begin()).first - b.begin());
	else return static_cast<ulong>(std::mismatch(a.begin(), a.end(), b.begin()).first - a.begin());
}

TMPL ulong Script<CFG>::lib::parse(Frame& f) {
	const String source = f.get(STR("$0"));
	return static_cast<ulong>(f.parse(source.begin(), source.end(), f.get(STR("$1"))) - source.begin());
}

TMPL T_TYPE(String) Script<CFG>::lib::radix(const Frame& f) {
	int radix = f.get(STR("$1"));
	if (radix < 2 || radix > 16) throw Xception(String(STR("Radix out of range: ")) += intToString<String>(radix));
	int minLength = f.getOptional(STR("$2"), 1);
	if (minLength < 0 || minLength > int(sizeof (int) * 8))
		throw Xception(String(STR("Minimum length out of range: ")) += intToString<String>(minLength));
	return intToString<String, ulong>(f.get(STR("$0")), f.get(STR("$1")), minLength);
}

TMPL void Script<CFG>::lib::save(const String& file, const String& chars) {
	std::basic_ofstream<Char> outstream(toStdString(file).c_str());															// Sorry, can't pass a wchar_t filename. MSVC supports it, but it is non-standard. So we convert to a std::string to be on the safe side.
	if (!outstream.good()) throw Xception(String(STR("Cannot open file for writing: ")) += escape(file));
	outstream.write(chars.data(), chars.size());
	if (!outstream.good()) throw Xception(String(STR("Error writing to file: ")) += escape(file));
}

TMPL ulong Script<CFG>::lib::search(const String& a, const String& b) {
	return static_cast<ulong>(std::search(a.begin(), a.end(), b.begin(), b.end()) - a.begin());
}

TMPL ulong Script<CFG>::lib::span(const String& a, const String& b) {
	typename String::const_iterator it;
	for (it = a.begin(); it != a.end() && std::find(b.begin(), b.end(), *it) != b.end(); ++it);
	return static_cast<ulong>(it - a.begin());
}

TMPL T_TYPE(String) Script<CFG>::lib::precision(const Frame& f) {
	return doubleToString<String>(f.get(STR("$0")), mini(maxi(int(f.get(STR("$1"))), 1), 16));
}

TMPL int Script<CFG>::lib::system(const String& command) {
	int xc = (command.empty() ? -1 : ::system(toStdString(command).c_str()));
	if (xc < 0) throw Xception(String(STR("Error executing system command: ")) += escape(command));
	return xc;
}

TMPL void Script<CFG>::lib::trace(const Frame& f) {
	f.getRoot().setTracer(Precedence(int(f.getOptional(STR("$1"), int(TRACE_CALL)))), f.getOptional(STR("$0")));
}

TMPL T_TYPE(Value) Script<CFG>::lib::tryer(Frame& f) {
	try { f.call(String(), f.get(STR("$0")), 0); } catch (const Xception& x) { return x.getError(); }
	return Value();
}

TMPL void Script<CFG>::addLibraryNatives(Frame& f, bool includeIO) {
	f.set(STR("VERSION"), PIKA_SCRIPT_VERSION);
	f.set(STR("run"), STR(">::evaluate((>{ $s = load($0); if ($s{:2} == '#!') $s{find($s, \"\\n\"):} })($0), @$)"));	// Note: we need this as a "bootstrap" to include 'stdlib.pika'.
	f.registerNative(STR("abs"), (double (*)(double))(fabs));
	f.registerNative(STR("acos"), (double (*)(double))(acos));
	f.registerNative(STR("asin"), (double (*)(double))(asin));
	f.registerNative(STR("atan"), (double (*)(double))(atan));
	f.registerNative(STR("atan2"), (double (*)(double, double))(atan2));
	f.registerNative(STR("ceil"), (double (*)(double))(ceil));
	f.registerNative(STR("char"), lib::character);
	f.registerNative(STR("cos"), (double (*)(double))(cos));
	f.registerNative(STR("cosh"), (double (*)(double))(cosh));
	f.registerNative(STR("delete"), lib::deleter);
	f.registerNative(STR("escape"), (String (*)(const String&))(escape));
	f.registerNative(STR("exists"), lib::exists);
	f.registerNative(STR("elevate"), lib::elevate);
	f.registerNative(STR("evaluate"), lib::evaluate);
	f.registerNative(STR("exp"), (double (*)(double))(exp));
	f.registerNative(STR("find"), lib::find);
	f.registerNative(STR("floor"), (double (*)(double))(floor));
	f.registerNative(STR("foreach"), lib::foreach);
	f.set(STR("include"), STR(">::if (!exists(@::included[$0])) { invoke('run',, @$); ::included[$0] = true }"));		// Note: we need this as a "bootstrap" to include 'stdlib.pika'.
	if (includeIO) f.registerNative(STR("input"), lib::input);
	f.registerNative(STR("invoke"), lib::invoke);
	f.registerNative(STR("length"), lib::length);
	f.registerNative(STR("log"), (double (*)(double))(log));
	f.registerNative(STR("log10"), (double (*)(double))(log10));
	if (includeIO) f.registerNative(STR("load"), lib::load);
	f.registerNative(STR("lower"), lib::lower);
	f.registerNative(STR("mismatch"), lib::mismatch);
	f.registerNative(STR("ordinal"), lib::ordinal);
	f.registerNative(STR("pow"), (double (*)(double, double))(pow));
	f.registerNative(STR("parse"), lib::parse);
	f.registerNative(STR("precision"), lib::precision);
	if (includeIO) f.registerNative(STR("print"), lib::print);
	f.registerNative(STR("radix"), lib::radix);
	f.registerNative(STR("random"), lib::random);
	f.registerNative(STR("reverse"), lib::reverse);
	f.registerNative(STR("sin"), (double (*)(double))(sin));
	f.registerNative(STR("sinh"), (double (*)(double))(sinh));
	if (includeIO) f.registerNative(STR("save"), lib::save);
	f.registerNative(STR("search"), lib::search);
	f.registerNative(STR("span"), lib::span);
	f.registerNative(STR("sqrt"), (double (*)(double))(sqrt));
	if (includeIO) f.registerNative(STR("system"), lib::system);
	f.registerNative(STR("tan"), (double (*)(double))(tan));
	f.registerNative(STR("tanh"), (double (*)(double))(tanh));
	f.registerNative(STR("time"), lib::time);
	f.registerNative(STR("throw"), lib::thrower);
	f.registerNative(STR("trace"), lib::trace);
	f.registerNative(STR("try"), lib::tryer);
	f.registerNative(STR("upper"), lib::upper);
}

TMPL Script<CFG>::Native::~Native() { }
TMPL Script<CFG>::Variables::~Variables() { }

#undef TMPL
#undef T_TYPE
#undef STR

} // namespace Pika

#endif
/**
	\file QStrings.h
	
	QStrings is a high performance string class inspired by std::string.
	
	QStrings implements a subset of std::string and can be used as a much optimized string implementation for
	PikaScript. It achieves its great performance by:
	
	1) Maintaining a separate memory pool for smaller string buffers instead of using the slower standard heap.
	2) Sharing string buffers not only for full strings, but even for substrings.
	
	\warning
	
	WARNING! The current implementation of the memory pool is *not* thread-safe due to unprotected use of shared global
	data. You must only use QStrings in single-threaded applications or in the case of a multi-threaded application you
	must only use QStrings from a single thread at a time!

	\version
	
	Version 0.97
	
	\page Copyright
	
	PikaScript is released under the BSD 2-Clause License. http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef QStrings_h
#define QStrings_h

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <wchar.h>
#include <string>
#include <algorithm>
#include <limits>
#include "assert.h"

namespace QStrings {

/* --- Declaration --- */

// TODO : documentation
template<typename C, size_t PS = (64 - 12)> class QString {
	public:		enum { npos = 0x7FFFFFFF };
	public:		typedef size_t size_type;
	public:		typedef C value_type;
	public:		template<typename E, class Q> class _iterator;
	public:		typedef _iterator<C, QString<C, PS> > iterator;
	public:		typedef _iterator<const C, const QString<C, PS> > const_iterator;
	public:		class Buffer;
	public:		typedef std::reverse_iterator<iterator> reverse_iterator;
	public:		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	public:		static void deinit() throw();
	public:		template<size_t N> QString(const C (&s)[N]);
	public:		QString();
	public:		QString(const QString& copy);
	public:		QString(C* s);
	public:		QString(size_type n, C c);
	public:		QString(const C* b, size_type l, size_type extra = 0);
	public:		QString(const_iterator b, const_iterator e);
	public:		QString(const C* b, const C* e);
	public:		QString(const std::basic_string<C>& copy);
	public:		QString(const QString& copy, size_type offset, size_type count);
	public:		QString& operator=(const QString& copy);
	public:		C operator[](size_type index) const;
	public:		QString& operator+=(const QString& s);
	public:		QString operator+(const QString& s) const;
	public:		template<size_t N> QString& operator+=(const C (&s)[N]);
	public:		template<size_t N> QString operator+(const C (&s)[N]) const;
	public:		QString& operator+=(C c);
	public:		QString operator+(C c) const;
	public:		size_type size() const;
	public:		bool empty() const;
	public:		const C* data() const;
	public:		const C* c_str() const;
	public:		QString substr(size_type offset, size_type count = npos) const;
	public:		~QString() throw();
	public:		bool operator<(const QString& r) const;
	public:		bool operator<=(const QString& r) const;
	public:		bool operator==(const QString& r) const;
	public:		bool operator!=(const QString& r) const;
	public:		bool operator>=(const QString& r) const;
	public:		bool operator>(const QString& r) const;
	public:		const_iterator begin() const { return const_iterator(pointer, this); }
	public:		const_iterator end() const { return const_iterator(pointer + length, this); }
	public:		iterator begin() { unshare(); return iterator(pointer, this); }
	public:		iterator end() { unshare(); return iterator(pointer + length, this); }
	public:		const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	public:		const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	public:		reverse_iterator rbegin() { unshare(); return reverse_iterator(end()); }
	public:		reverse_iterator rend() { unshare(); return reverse_iterator(begin()); }
	public:		operator std::basic_string<C>() const { return std::basic_string<C>(pointer, length); }
	protected:	void newBuffer(size_type l, const C* b, size_type extra = 0);
	protected:	QString& append(const C* p, typename QString<C, PS>::size_type l);
	protected:	QString concat(const C* p, typename QString<C, PS>::size_type l) const;
	protected:	void unshare() const;
	protected:	void retain() const throw();
	protected:	void release() throw();
	protected:	static void copychars(C* d, const C* s, size_t l);
	protected:	C* pointer;
	protected:	size_type length;
	protected:	Buffer* buffer;
	protected:	static Buffer* obtainDummyBuffer();
};

/* --- Implementation --- */

template<> inline void QString<char>::copychars(char* d, const char* s, size_t l) { memcpy(d, s, l); }
template<> inline void QString<wchar_t>::copychars(wchar_t* d, const wchar_t* s, size_t l) { wmemcpy(d, s, l); }

template<typename C, size_t PS> class QString<C, PS>::Buffer {
	friend class QString<C, PS>;
	protected:	static Buffer*& obtainPoolHead() { static Buffer* poolHead; return poolHead; }

	public:		Buffer(size_type n) : rc(1), size(n > PS ? n : PS), chars(n > PS ? new C[n] : internal) { };
	public:		~Buffer() { if (chars != internal) delete [] chars; }
	
	public:		static void* operator new(size_t count) {
					assert(count == sizeof (Buffer));
					Buffer* h = obtainPoolHead();
					if (h == 0) h = reinterpret_cast<Buffer*>(::operator new(count));
					else obtainPoolHead() = obtainPoolHead()->next;
					return h;
				}
				
	public:		static void operator delete(void* pointer) throw() {
					Buffer* h = reinterpret_cast<Buffer*>(pointer);
					h->next = obtainPoolHead();
					obtainPoolHead() = h;
				}
				
	public:		static void cleanPool() throw() {
					while (obtainPoolHead() != 0) {
						Buffer* h = obtainPoolHead();
						obtainPoolHead() = h->next;
						::operator delete(h);
					}
				}
				
	protected:	union {
					struct {
						C internal[PS];
						int rc;
						size_type size;
						C* chars;
					};
					Buffer* next;
				};
};

template<typename C, size_t PS> void QString<C, PS>::retain() const throw() { ++buffer->rc; }
template<typename C, size_t PS> void QString<C, PS>::deinit() throw() { Buffer::cleanPool(); }
template<typename C, size_t PS> typename QString<C, PS>::size_type QString<C, PS>::size() const { return length; }
template<typename C, size_t PS> bool QString<C, PS>::empty() const { return (length == 0); }
template<typename C, size_t PS> const C* QString<C, PS>::data() const { return pointer; }
	
template<typename C, size_t PS> template<typename E, class Q> class QString<C, PS>::_iterator
		: public std::iterator<std::random_access_iterator_tag, E> { // FIX : is there some good base-class for this in STL?
	friend class QString;
	public:		_iterator() : p(0), s(0) { }
	public:		template<typename E2, class Q2> _iterator(const _iterator<E2, Q2>& copy) : p(copy.p), s(copy.s) { }
	protected:	explicit _iterator(E* p, Q* s) : p(p), s(s) { }
	public:		friend _iterator operator+(const _iterator& it, ptrdiff_t d) { return _iterator(it.p + d, it.s); }
	public:		friend _iterator operator+(ptrdiff_t d, const _iterator& it) { return _iterator(it.p + d, it.s); }
	public:		friend _iterator operator-(const _iterator& it, ptrdiff_t d) { return _iterator(it.p - d, it.s); }
	public:		friend ptrdiff_t operator-(const _iterator& a, const _iterator& b) {
					assert(a.s == b.s);
					return a.p - b.p;
				}
	public:		_iterator& operator+=(ptrdiff_t d) { p += d; return (*this); }
	public:		_iterator& operator-=(ptrdiff_t d) { p -= d; return (*this); }
	public:		_iterator& operator++() { ++p; return (*this); }
	public:		_iterator& operator--() { --p; return (*this); }
	public:		const _iterator operator++(int) { _iterator copy(*this); ++p; return copy; }
	public:		const _iterator operator--(int) { _iterator copy(*this); --p; return copy; }
	public:		template<typename E2, class Q2> _iterator& operator=(const _iterator<E2, Q2>& copy) {
					p = copy.p;
					s = copy.s;
					return (*this);
				}
	public:		template<typename E2, class Q2> bool operator<(const _iterator<E2, Q2>& r) const {
					assert(r.s == s);
					return (p < r.p);
				}
	public:		template<typename E2, class Q2> bool operator<=(const _iterator<E2, Q2>& r) const {
					assert(r.s == s);
					return (p <= r.p);
				}
	public:		template<typename E2, class Q2> bool operator==(const _iterator<E2, Q2>& r) const { return (p == r.p); }
	public:		template<typename E2, class Q2> bool operator!=(const _iterator<E2, Q2>& r) const {	return (p != r.p); }
	public:		template<typename E2, class Q2> bool operator>=(const _iterator<E2, Q2>& r) const {
					assert(r.s == s);
					return (p >= r.p);
				}
	public:		template<typename E2, class Q2> bool operator>(const _iterator<E2, Q2>& r) const {
					assert(r.s == s);
					return (p > r.p);
				}
	public:		E& operator*() const { assert(s->data() <= p && p < s->data() + s->size()); return *p; }
	public:		E& operator[](ptrdiff_t d) const { return *((*this) + d); }
	public:		E* p;
	public:		Q* s;
};

template<typename C, size_t PS> template<size_t N> QString<C, PS>::QString(const C (&s)[N])
		: pointer(const_cast<C*>(s)), length(N - 1), buffer(obtainDummyBuffer()) {
	retain();
}

template<typename C, size_t PS> QString<C, PS>::QString() : length(0), buffer(obtainDummyBuffer()) {
	pointer = buffer->chars;
	retain();
}
template<typename C, size_t PS> typename QString<C, PS>::Buffer* QString<C, PS>::obtainDummyBuffer() {
	static typename QString<C, PS>::Buffer dummy(0);
	return &dummy;
}

template<typename C, size_t PS> void QString<C, PS>::release() throw() {
	assert(buffer->rc >= 1);
	if (--buffer->rc == 0) delete buffer;
}

template<typename C, size_t PS> QString<C, PS>::~QString() throw() { release(); }

template<typename C, size_t PS> void QString<C, PS>::newBuffer(size_type l, const C* b, size_type extra) {
	assert(l == 0 || b != 0);
	buffer = new Buffer(l + extra + 1);
	pointer = buffer->chars;
	length = l;
	copychars(pointer, b, l);
	pointer[length] = 0;
}

template<typename C, size_t PS> QString<C, PS>::QString(C* s) {
	assert(s != 0);
	newBuffer(std::char_traits<C>::length(s), s);
}

template<typename C, size_t PS> QString<C, PS>::QString(const C* b, size_type l, size_type extra) {
	assert(b != 0);
	newBuffer(l, b, extra);
}

template<typename C, size_t PS> QString<C, PS>::QString(size_type n, C c) {
	newBuffer(0, 0, n);
	std::fill_n(pointer, n, c);
	length = n;
}

template<typename C, size_t PS> QString<C, PS>::QString(const QString& copy)
		: pointer(copy.pointer), length(copy.length), buffer(copy.buffer) {
	retain();
}

template<typename C, size_t PS> QString<C, PS>::QString(const_iterator b, const_iterator e)
		: pointer(const_cast<C*>(b.p)), length(e.p - b.p), buffer(b.s->buffer) {
	assert(b.s->begin() <= b && b <= b.s->end());
	assert(b <= e && e <= b.s->end());
	retain();
}

template<typename C, size_t PS> QString<C, PS>::QString(const C* b, const C* e) {
	assert(b != 0);
	assert(e != 0);
	assert(b <= e);
	newBuffer(e - b, b);
}

template<typename C, size_t PS> QString<C, PS>::QString(const std::basic_string<C>& copy) {
	newBuffer(copy.size(), copy.data());
}

template<typename C, size_t PS> QString<C, PS>::QString(const QString& copy, size_type offset, size_type count)
		: pointer(copy.pointer + offset), length(count < copy.length - offset ? count : copy.length - offset), buffer(copy.buffer) {
	assert((!std::numeric_limits<size_type>::is_signed || offset >= 0) && offset <= copy.length);
	retain();
}

template<typename C, size_t PS> QString<C, PS>& QString<C, PS>::operator=(const QString<C, PS>& copy) {
	copy.retain();
	this->release();
	this->pointer = copy.pointer;
	this->length = copy.length;
	this->buffer = copy.buffer;
	return (*this);
}

template<typename C, size_t PS> QString<C, PS>& QString<C, PS>::append(const C* p, size_type l) {
	assert(p != 0);
	bool fit = (pointer >= buffer->chars && pointer + length + l < buffer->chars + buffer->size);
	if (buffer->rc != 1 && fit && memcmp(pointer + length, p, l) == 0) {
		length += l;
		return (*this);
	}
	if (buffer->rc != 1 || !fit)
		(*this) = QString(pointer, length, l + (length < 65536 ? length : 65536)); // FIX : constant
	
	copychars(pointer + length, p, l);
	length += l;
	assert(buffer->rc == 1);
	pointer[length] = 0;
	return (*this);
}

template<typename C, size_t PS> QString<C, PS> QString<C, PS>::concat(const C* p, size_type l) const {
	assert(p != 0);
	QString copy(pointer, length, l + (l < size_type(65536) ? l : size_type(65536)));
	copychars(copy.pointer + copy.length, p, l);
	copy.length += l;
	assert(copy.buffer->rc == 1);
	copy.pointer[copy.length] = 0;
	return copy;
}

template<typename C, size_t PS> QString<C, PS>& QString<C, PS>::operator+=(const QString<C, PS>& s) {
	return (length == 0) ? (*this = s) : append(s.pointer, s.length);
}

template<typename C, size_t PS> QString<C, PS> QString<C, PS>::operator+(const QString<C, PS>& s) const {
	return (length == 0) ? s : concat(s.pointer, s.length);
}

template<typename C, size_t PS> template<size_t N> QString<C, PS>& QString<C, PS>::operator+=(const C (&s)[N]) {
	return append(s, N - 1);
}

template<typename C, size_t PS> template<size_t N> QString<C, PS> QString<C, PS>::operator+(const C (&s)[N]) const {
	return concat(s, N - 1);
}

template<typename C, size_t PS> QString<C, PS>& QString<C, PS>::operator+=(C c) { return append(&c, 1); }
template<typename C, size_t PS> QString<C, PS> QString<C, PS>::operator+(C c) const { return concat(&c, 1); }

template<typename C, size_t PS> void QString<C, PS>::unshare() const {
	if (buffer->rc != 1) *const_cast<QString<C, PS>*>(this) = QString(pointer, length);
}

template<typename C, size_t PS> const C* QString<C, PS>::c_str() const {
	if (*(pointer + length) != 0) {
		unshare();
		*(pointer + length) = 0;
	}
	return pointer;
}

template<typename C, size_t PS> QString<C, PS> QString<C, PS>::substr(size_type offset, size_type count) const {
	return QString(*this, offset, count);
}

template<typename C, size_t PS> C QString<C, PS>::operator[](size_type index) const {
	assert((!std::numeric_limits<size_type>::is_signed || 0 <= index) && index <= length);
	return (index == length) ? 0 : pointer[index];
}

template<typename C, size_t PS> bool QString<C, PS>::operator<(const QString<C, PS>& r) const {
	int d = (this->pointer == r.pointer ? 0 : std::char_traits<C>::compare
			(this->pointer, r.pointer, (this->length < r.length ? this->length : r.length)));
	return (d == 0 ? (this->length < r.length) : (d < 0));
}

template<typename C, size_t PS> bool QString<C, PS>::operator<=(const QString<C, PS>& r) const {
	int d = (this->pointer == r.pointer ? 0 : std::char_traits<C>::compare
			(this->pointer, r.pointer, (this->length < r.length ? this->length : r.length)));
	return (d == 0 ? (this->length <= r.length) : (d < 0));
}

template<typename C, size_t PS> bool QString<C, PS>::operator==(const QString<C, PS>& r) const {
	return (this->length == r.length && (this->pointer == r.pointer
			|| std::char_traits<C>::compare(this->pointer, r.pointer, this->length) == 0));
}

template<typename C, size_t PS> bool QString<C, PS>::operator!=(const QString<C, PS>& r) const { return !(*this == r); }
template<typename C, size_t PS> bool QString<C, PS>::operator>=(const QString<C, PS>& r) const { return !(*this < r); }
template<typename C, size_t PS> bool QString<C, PS>::operator>(const QString<C, PS>& r) const { return !(*this <= r); }

bool unitTest();

}

#endif
/**
	\file QuickVars.h

	QuickVars is a (generally) faster version of the reference implementation's STLVariable.
	
	It achieves its better performance by caching the most recently used variables in a super-tiny hash table. The
	downside is that it uses more stack memory, especially if you have deep calling stacks with few local variables.
	
	\version

	Version 0.97
		
	\page Copyright

	PikaScript is released under the BSD 2-Clause License. http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef QuickVars_h
#define QuickVars_h

#if !defined(PikaScriptImpl_h)
#include "PikaScriptImpl.h"
#endif

namespace Pika {

// TODO : documentation with use example
// TODO : I think this one could be based on an arbitrary variables class and use assign, lookup etc of the super-class instead of accessing vars directly. The question is if it would affect performance?
template<class Super, unsigned int CACHE_SIZE = 11> class QuickVars : public Super {
	public:		typedef typename Super::ForScript::String String;
	public:		typedef typename Super::ForScript::Value Value;
	public:		typedef std::pair<String, Value> CacheEntry;
			
	public:		unsigned int hash(const String& s) {
					unsigned int l = static_cast<unsigned int>(s.size());
					if (s.size() == 1 && s[0] >= 'a' && s[0] <= 'z') return (s[0] - 'a') % CACHE_SIZE;
					if (s.size() == 2 && s[0] == '$' && s[1] >= '0' && s[1] <= '9') return s[1] - '0';
					return (s[0] * 1733 + s[l >> 2] * 2069 + s[l >> 1] * 2377 + s[l - 1] * 2851) % CACHE_SIZE;
				}

	public:		virtual bool assign(const String& identifier, const Value& value) {
					if (identifier.empty()) return false;
					unsigned int i = hash(identifier);
					if (cache[i].first == identifier) { cache[i] = CacheEntry(identifier, value); return true; }
					if (!cache[i].first.empty()) Super::vars[cache[i].first] = cache[i].second;
					cache[i] = CacheEntry(identifier, value);
					return true;
				}

	public:		virtual bool erase(const String& identifier) {
					bool erased = (Super::vars.erase(identifier) != 0);
					unsigned int i = hash(identifier);
					if (cache[i].first == identifier) { cache[i] = std::pair<const String, Value>(); erased = true; }
					return erased;
				}

	public:		virtual bool lookup(const String& identifier, Value& result) {
					if (identifier.empty()) return false;
					unsigned int i = hash(identifier);
					if (cache[i].first == identifier) { result = cache[i].second; return true; }
					typename Super::VariableMap::const_iterator it = Super::vars.find(identifier);
					if (it == Super::vars.end()) return false;
					if (!cache[i].first.empty()) Super::vars[cache[i].first] = cache[i].second;
					cache[i] = CacheEntry(identifier, it->second);
					result = it->second;
					return true;
				}

	public:		virtual void list(const String& key, typename Super::ForScript::Variables::VarList& list) {
					for (unsigned int i = 0; i < CACHE_SIZE; ++i)
						if (!cache[i].first.empty()) Super::vars[cache[i].first] = cache[i].second;
					for (typename Super::VariableMap::const_iterator it = Super::vars.lower_bound(key)
							; it != Super::vars.end() && it->first.substr(0, key.size()) == key; ++it)
						list.push_back(*it);
				}

	protected:	CacheEntry cache[CACHE_SIZE];
};

} // namespace Pika

#endif
/**
	\file PikaScript.cpp
	
	PikaScript is a high-level scripting language written in C++.
	
	\version
	
	Version 0.97
	
	\page Copyright
	
	PikaScript is released under the "New Simplified BSD License". http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	Neither the name of the NuEdge Development nor the names of its contributors may be used to endorse or promote
	products derived from this software without specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <time.h>
#if !defined(PikaScript_h)
#include "PikaScript.h"
#endif
#if !defined(PikaScriptImpl_h)
#include "PikaScriptImpl.h"
#endif

namespace Pika {

template struct Script<StdConfig>;

} // namespace Pika
/**
	\file QStrings.cpp
	
	QStrings is a high performance string class inspired by std::string.
	
	QStrings implements a subset of std::string and can be used as a much optimized string implementation for
	PikaScript. It achieves its great performance by:
	
	1) Maintaining a separate memory pool for smaller string buffers instead of using the slower standard heap.
	2) Sharing string buffers not only for full strings, but even for substrings.
	
	\warning
	
	WARNING! The current implementation of the memory pool is *not* thread-safe due to unprotected use of shared global
	data. You must only use QStrings in single-threaded applications or in the case of a multi-threaded application you
	must only use QStrings from a single thread at a time!

	\version
	
	Version 0.97
	
	\page Copyright
	
	PikaScript is released under the "New Simplified BSD License". http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	Neither the name of the NuEdge Development nor the names of its contributors may be used to endorse or promote
	products derived from this software without specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if !defined(QStrings_h)
#include "QStrings.h"
#endif
#include "assert.h"

namespace QStrings {

bool unitTest() {
	QString<char> s;
	s = "hej";
	QString<char> t = s;
	s += " du";
	s += " glade ";
	s += s.substr(4, 3);
	s += s;
	s = s + "ta en spade";
	assert(s == "hej du glade du hej du glade du ta en spade");
	assert(s < "hej du glade du hej du glade du ta en spadef");
	assert(s < "hej du glade du hej du glade du ta en spadf");
	assert(s <= "hej du glade du hej du glade du ta en spade");
	assert(s <= "hej du glade du hej du glade du ta en spadf");
	assert(s >= "hej du glade du hej du glade du ta en spadd");
	assert(s >= "hej du glade du hej du glade du ta en spade");
	assert(s > "hej du glade du hej du glade du ta en spadd");
	assert(s > "hej du glade du hej du glade du ta en spad");
	assert(s != "hej du glade du hej du glade du ta en spad");
	assert(s != "hej du glade du hej du glade du ta en spadd");
	t = "";
	const QString<char>& cs = s;
	for (QString<char>::const_iterator it = cs.begin(); it != cs.end(); ++it) {
		const char c[1] = { *it };
		t += QString<char>(c, 1);
	}
	assert(t == s);
	assert(static_cast<size_t>(cs.end() - cs.begin()) == cs.size());
	assert(cs.end() - cs.size() == cs.begin());
	assert(cs.begin() + cs.size() == cs.end());
	assert(QString<char>(cs.begin(), cs.begin() + 3) == "hej");
	assert(QString<char>(cs.begin() + 4, cs.begin() + 6) == "du");
	assert(QString<char>(cs.end() - 5, cs.end()) == "spade");
	
	s = "abc";
	t = s;
	const QString<char>& u = t;
	QString<char>::iterator it = t.begin();
	QString<char>::const_iterator it2 = u.begin();
	*it = 'c';
	*it = *it2;
	assert(it >= t.begin() && it <= t.end());
	assert(it2 >= u.begin() && it2 <= u.end());
	assert(s == "abc");
	
	return true;
}

}

#ifdef REGISTER_UNIT_TEST
REGISTER_UNIT_TEST(QStrings::unitTest)
#endif
const char* BUILT_IN_DEBUG =
	"/*\n"
	"\tdebug.pika v0.97\n"
	"\n"
	"\tPikaScript is released under the \"New Simplified BSD License\". http://www.opensource.org/licenses/bsd-license.php\n"
	"\t\n"
	"\tCopyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem\n"
	"\tAll rights reserved.\n"
	"*/\n"
	"\n"
	"include('stdlib.pika');\n"
	"\n"
	"CONSOLE_COLS = 120;\n"
	"CONSOLE_ROWS = 30;\n"
	"\n"
	"assert = function {\n"
	"\tif (!(if (classify($0) === 'function') $0() else $0)) throw('Assertion failure: ' # coalesce(@$1, @$0));\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"limitLength = function { if (length($0) <= $1) ( $0 ) else ( $0{:$1 - 3} # '...' ) };\n"
	"singleLine = function { replace($0, \"\\t\\r\\n\", ' ', find, 1) };\n"
	"\n"
	"// FIX : use \"more\" (or implement same functionality here)...\n"
	"dump = function {\n"
	"\tvargs(, @var); defaults(@var, @^);\n"
	"\tprint('Dumping ' # var # ' of ' # describeCall(var) # LF # repeat('=', 8));\n"
	"\tif (exists(var)) print(var # ' = ' # limitLength(singleLine(toSource([var])), CONSOLE_COLS - length(var) - 3));\n"
	"\tforeach(var, >print($1 # ' = ' # limitLength(singleLine(toSource($2)), CONSOLE_COLS - length($1) - 3)));\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"// FIX : shouldn't be required... we should build the \"more\" facility into interactive.pika.\n"
	"more = function {\n"
	"\targs(@s);\n"
	"\tlineOut => if (!silent) {\n"
	"\t\tprint($0);\n"
	"\t\tif (++lc >= CONSOLE_ROWS) {\n"
	"\t\t\tlc = 0;\n"
	"\t\t\tif (lower(input(\"---- more (yes)? \")){0} === 'n') silent = true;\n"
	"\t\t}\n"
	"\t};\n"
	"\tlc = 0;\n"
	"\tsilent = false;\n"
	"\ttokenize(s, >if (!silent) lineOut($0));\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"// FIX : overlaps with dump, and I don't like the name, too generic\n"
	"//show = function {\n"
	"//\targs(@var);\n"
	"//\tmore(var # if (classify(var) === 'reference' && (s = sourceFor(var, ' ')) !== '')\n"
	"//\t\t\t(LF # repeat('=', 8) # LF # s # repeat('=', 8)));\n"
	"//\t( void )\n"
	"//};\n"
	"\n"
	"describeCall = function {\n"
	"\ts = (if (exists(@[$0].$callee)) [$0].$callee else '(unknown)') # '(';\n"
	"\tif (exists(@[$0].$n)) {\n"
	"\t\tfor (i = 0; i < [$0].$n; ++i) {\n"
	"\t\t\tif (exists(@[$0].$[i])) s #= limitLength(singleLine(toSource([$0].$[i])), CONSOLE_COLS);\n"
	"\t\t\tif (i < [$0].$n - 1) s #= ', '\n"
	"\t\t}\n"
	"\t};\n"
	"\ts #= ')';\n"
	"\t( s )\n"
	"};\n"
	"\n"
	"callstack = function {\n"
	"\tvargs(, @frame, @prefix); defaults(@frame, @^, @prefix, '');\n"
	"\tfor (; { print(prefix # describeCall(frame)); frame !== '::'; }; frame = @[frame # '^']) ;\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"NO_TRACE_LEVEL = 0;\n"
	"ERROR_TRACE_LEVEL = 1;\n"
	"CALL_TRACE_LEVEL = 2;\n"
	"LOOP_TRACE_LEVEL = 3;\n"
	"STATEMENT_TRACE_LEVEL = 4;\n"
	"BODY_TRACE_LEVEL = 5;\n"
	"ARGUMENT_TRACE_LEVEL = 6;\n"
	"BRACKET_TRACE_LEVEL = 7;\n"
	"MAX_TRACE_LEVEL = 22;\n"
	"\n"
	"traceVerbose = function {\n"
	"\ttrace(function {\n"
	"\t\tif (!exists(@^$callee) || !exists(@::traceVerbose.filter[^$callee])) {\n"
	"\t\t\targs(@source, @offset, @lvalue, @result, @level, @exit);\n"
	"\t\t\tif (level == CALL_TRACE_LEVEL) {\n"
	"\t\t\t\t\t\t// FIX : subroutine\n"
	"\t\t\t\tif (exists(@^$callee)) source = ^$callee;\n"
	"\t\t\t\ts = (if (exit) '}-- ' else '--{ ') # describeCall(@^);\n"
	"\t\t\t\tif (exit) s #= ' = ' # limitLength(singleLine(toSource(result)), CONSOLE_COLS \\ 4);\n"
	"\t\t\t} else {\n"
	"\t\t\t// FIX : use repeat\n"
	"\t\t\t\tfixedWidth = function { $0 = $0{:$1}; for (; length($0) < $1; $0 #= ' ') ; $0 };\n"
	"\t\t\t\ts = ' ' # fixedWidth(singleLine(source{offset - 4:4}), 4) # ' <> ' # fixedWidth(singleLine(source{offset:}), CONSOLE_COLS \\ 4)\n"
	"\t\t\t\t\t\t# ' | (' # (if (exit) '<' else '>') # level # ') ' # (if (lvalue) result else limitLength(singleLine(toSource(result)), CONSOLE_COLS \\ 4));\n"
	"\t\t\t};\n"
	"\t\t\tprint(s);\n"
	"\t\t}\n"
	"\t}, coalesce(@$0, CALL_TRACE_LEVEL));\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"traceCalls = function {\n"
	"\t::traceCalls.depth = 0;\n"
	"\t::traceCalls.filter['args'] = true;\n"
	"\t::traceCalls.filter['vargs'] = true;\n"
	"\t::traceCalls.filter['defaults'] = true;\n"
	"\ttrace(function {\n"
	"\t\tif (!exists(@^$callee) || !exists(@::traceCalls.filter[^$callee])) {\n"
	"\t\t\targs(@source, @offset, @lvalue, @result, @level, @exit);\n"
	"\t\t\tif (level == ERROR_TRACE_LEVEL) print(' !!!! ' # result);\n"
	"\t\t\tif (level == CALL_TRACE_LEVEL) {\n"
	"\t\t\t\ts = describeCall(@^);\n"
	"\t\t\t\tif (!exit) ++::traceCalls.depth\n"
	"\t\t\t\telse {\n"
	"\t\t\t\t\ts #= ' = ' # limitLength(singleLine(toSource(result)), CONSOLE_COLS \\ 4);\n"
	"\t\t\t\t\t--::traceCalls.depth;\n"
	"\t\t\t\t};\n"
	"\t\t\t\tprint(repeat(' ', traceCalls.depth) # s);\n"
	"\t\t\t}\n"
	"\t\t}\n"
	"\t}, CALL_TRACE_LEVEL);\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"debug.tracer = function {\n"
	"// TODO : slimline\n"
	"\targs(@source, @offset, @lvalue, @result, @level, @exit);\n"
	"\tleave = false;\n"
	"\tif (level == ERROR_TRACE_LEVEL) {\n"
	"\t\texit = false; // for the error level, exit just indicates that it is the last catch in the chain\n"
	"\t\tprint(' !!!! ' # result);\n"
	"\t\t::debug.callDepth = 0;\n"
	"\t};\n"
	"\tif (exit) {\n"
	"\t\tif (level == CALL_TRACE_LEVEL) --::debug.callDepth;\n"
	"\t\tif (debug.callDepth <= 0) {\n"
	"\t\t\tif (!::debug.lastWasExit) print(' (' # (if (lvalue) result else replace(limitLength(toSource(result), CONSOLE_COLS \\ 4), \"\\n\", ' ')) # ')');\n"
	"\t\t\tif (debug.callDepth < 0 && level == CALL_TRACE_LEVEL) print(' <<<<');\n"
	"\t\t}\n"
	"\t} else {\n"
	"\t\tif (level == CALL_TRACE_LEVEL) ++::debug.callDepth;\n"
	"\t\tif (debug.callDepth <= 0) {\n"
	"\t\t\thelp = \"enter   step over\\n    >   step in\\n    <   step out\\n    ^   continue\\n    !   stop\\n    =   dump locals\\n    :   dump globals\\n    (   call-stack\\n    {   show function\\n\\nanything else evaluates expression\\n\";\n"
	"\t\t\tprompt = replace(source{offset:81}, \"\\t\", ' ');\n"
	"\t\t\tprompt = limitLength(prompt{:find(prompt, \"\\n\")}, CONSOLE_COLS);\n"
	"\t\t\tif (level != CALL_TRACE_LEVEL) prompt = ' >>>> ' # prompt\n"
	"\t\t\telse prompt = ' ===> ' # describeCall(@^);\n"
	"\t\t\tfor (; {\n"
	"\t\t\t\t\tprint(prompt);\n"
	"\t\t\t\t\ts = input('debug> ');\n"
	"\t\t\t\t\tif (s === '^') { leave = true; false; }\n"
	"\t\t\t\t\telse if (s === '<') { ::debug.callDepth = 1; false; }\n"
	"\t\t\t\t\telse if (s === '>') { ::debug.callDepth = -1; false; }\n"
	"\t\t\t\t\telse if (s === '') { ::debug.callDepth = 0; false; }\n"
	"\t\t\t\t\telse true;\n"
	"\t\t\t\t} ; ) {\n"
	"\t\t\t\tif (s === '?') print(help)\n"
	"\t\t\t\telse if (s === '!') throw('Debugger stop')\n"
	"\t\t\t\telse if (s === '=') dump(@^) // FIX : also list non-closure @^$ in case closure frame differs from $ frame\n"
	"\t\t\t\telse if (s === ':') dump(@::)\n"
	"\t\t\t\telse if (s === '{') print(source{:offset} # ' <####> ' # source{offset:}) // FIX : display func name first\n"
	"\t\t\t\telse if (s === '(') callstack(@^)\n"
	"\t\t\t\telse {\n"
	"\t\t\t\t\tx = try(>s = evaluate(s, @^^^$));\n"
	"\t\t\t\t\tprint(if (x !== void) x else s);\n"
	"\t\t\t\t}\n"
	"\t\t\t}\n"
	"\t\t}\n"
	"\t};\n"
	"\t::debug.lastWasExit = exit;\n"
	"\tif (leave) traceLevel = ERROR_TRACE_LEVEL\n"
	"\telse if (debug.callDepth > 0) traceLevel = CALL_TRACE_LEVEL\n"
	"\telse traceLevel = BODY_TRACE_LEVEL;\n"
	"\ttrace(debug.tracer, traceLevel);\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"debug = function {\n"
	"\t::debug.lastWasExit = false;\n"
	"\tif (exists(@$0)) {\n"
	"\t\t::debug.callDepth = -1;\n"
	"\t\ttrace(debug.tracer, CALL_TRACE_LEVEL);\n"
	"\t\t$0();\n"
	"\t} else {\n"
	"\t\t::debug.callDepth = 1;\n"
	"\t\ttrace(debug.tracer, CALL_TRACE_LEVEL);\n"
	"\t\tprint('*** DEBUGGER BREAK ***');\n"
	"\t\t( void );\n"
	"\t}\n"
	"};\n"
	"\n"
	"traceErrors = function {\n"
	"\ttrace(function {\n"
	"\t\targs(@source, @offset, @lvalue, @result, @level, @exit);\n"
	"\t\tCC4 = CONSOLE_COLS \\ 4;\n"
	"\t\ts = singleLine(source{offset - CC4:CC4} # ' <!!!!> ' # source{offset:CC4});\n"
	"\t\tif (offset > CC4) s = '... ' # s;\n"
	"\t\tif (offset + CC4 < length(source)) s #= ' ...';\n"
	"\t\tprint('   ! ' # describeCall(@^) # ' !   ' # s);\n"
	"//\t\tif (exit) print('!!!! ' # result);\n"
	"\t\tif (exit) print('');\n"
	"\t}, ERROR_TRACE_LEVEL);\n"
	"};\n"
	"\n"
	"profile = function {\n"
	"\t::profiler.dump = function { dump(@::profiler.counters); };\n"
	"\t::profiler.reset = function { prune(@::profiler.counters); };\n"
	"\t::profiler.tick = function { \n"
	"\t\t::profiler.ticker = ::profiler.skip;\n"
	"\t\tif ((t = time()) != ::profiler.last) {\n"
	"\t\t\t::profiler.last = t;\n"
	"\t\t\tif (exists(f = @^^$callee)) {\n"
	"\t\t\t\tprint('');\n"
	"\t\t\t\tc = @::profiler.counters[[f]];\n"
	"\t\t\t\tdefaults(c, 0);\n"
	"\t\t\t\t[c]++;\n"
	"\t\t\t\tprint('   @ ' # [f] # ': ' # [c]);\n"
	"args(@source, @offset);\n"
	"CC4 = CONSOLE_COLS \\ 2 - 16 - 5;\n"
	"s = singleLine(source{offset - CC4:CC4} # ' <####> ' # source{offset:CC4});\n"
	"if (offset > CC4) s = '... ' # s;\n"
	"if (offset + CC4 < length(source)) s #= ' ...';\n"
	"print('     ' # s);\n"
	"\t\t\t\tcallstack(@^^, '     ');\n"
	"\t\t\t\tprint('');\n"
	"\t\t\t}\n"
	"\t\t}\n"
	"\t};\n"
	"\n"
	"\t::profiler.ticker = 0;\n"
	"\ttrace(function { ++::profiler.ticker; }, STATEMENT_TRACE_LEVEL);\n"
	"\tprint('Calibrating...');\n"
	"\tfor (t = time(); (t2 = time()) == t;);\n"
	"\tfor (::profiler.ticker = 0; time() == t2;) wildmatch(random(1), '[0-8.]*');\n"
	"\t::profiler.skip = (::profiler.ticker \\= 4);\n"
	"\tprint(\"Profiling started.\\n\\nprofiler.dump() show accumulated call counts.\\nprofiler.reset() resets call counts.\\ntrace() stops profiling.\");\n"
	"\t::profiler.last = time();\n"
	"\ttrace(function { if (--::profiler.ticker <= 0) profiler.tick($0, $1); }, STATEMENT_TRACE_LEVEL);\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"clock = function {\n"
	"\targs(@func);\n"
	"\tprint('----- Clocking...');\n"
	"\tfor (t = time() + 1; time() < t;);\n"
	"\tipersec = { i = 0; for (++t; time() < t;) ++i; };\n"
	"\tfunc();\n"
	"\tj = 0; for (u = time() + 1; time() < u; ++j);\n"
	"\tprint('----- Clocked: ' # (u - t - min(j, ipersec) / ipersec) # ' secs');\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"traceErrors();\n"
	"\n"
	"// Define dummy help that loads the help.pika on demand.\n"
	"help = function { /****    Type help() to get started.    ****/        run('help.pika'); invoke('help', , @$) };\n"
	"\n"
	"void;\n";

const char* BUILT_IN_HELP =
	"/*\n"
	"\thelp.pika v0.97\n"
	"\n"
	"\tPikaScript is released under the \"New Simplified BSD License\". http://www.opensource.org/licenses/bsd-license.php\n"
	"\t\n"
	"\tCopyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem\n"
	"\tAll rights reserved.\n"
	"*/\n"
	"\n"
	"include('stdlib.pika');\n"
	"\n"
	"prune(@help);\n"
	"\n"
	"describe = function {\n"
	"\tvargs(@category, @page, @syntax,, @description, @examples, @seealso);\n"
	"\tif (exists(@::help[page])) throw('Help page ' # escape(page) # ' already described');\n"
	"\t::help._categories[category][page] = page;\n"
	"\t::help[page].category = category;\n"
	"\t::help[page].syntax = syntax;\n"
	"\t::help[page].description = coalesce(@description);\n"
	"\t::help[page].examples = coalesce(@examples);\n"
	"\t::help[page].seealso = coalesce(@seealso);\n"
	"\tfull = 'SYNTAX' # LF # syntax # LF # LF # 'CATEGORY' # LF # category;\n"
	"\tif (exists(@description)) full #= LF # LF # 'DESCRIPTION' # LF # description;\n"
	"\tif (exists(@examples)) full #= LF # LF # 'EXAMPLE(S)' # LF # examples;\n"
	"\tif (exists(@seealso)) full #= LF # LF # 'SEE ALSO' # LF # seealso;\n"
	"\t::help[page] = full;\n"
	"\t::help._isdirty = true;\n"
	"};\n"
	"\n"
	"help._isdirty = false;\n"
	"help._lookup = function {\n"
	"\tif (help._isdirty) {\n"
	"\t\t::help._isdirty = false;\n"
	"\t\t::help['#categories'] = 'Defined help categories:' # LF # LF;\n"
	"\t\tlastCategory = '';\n"
	"\t\tforeach(@::help._categories, >{\n"
	"\t\t\ti = find($1, '.');\n"
	"\t\t\tthisPage = $1{(i + 1):};\n"
	"\t\t\tthisCategory = $1{:i};\n"
	"\t\t\tif (lastCategory !== thisCategory) {\n"
	"\t\t\t\tif (lastCategory !== '') ::help[lastCategory] = pageList{:length(pageList) - 2};\n"
	"\t\t\t\tlastCategory = thisCategory;\n"
	"\t\t\t\tpageList = 'The category ' # thisCategory # ' contains the following help pages:' # LF # LF;\n"
	"\t\t\t\t::help['#categories'] #= thisCategory # ', ';\n"
	"\t\t\t};\n"
	"\t\t\tpageList #= thisPage # ', ';\n"
	"\t\t});\n"
	"\t\tif (lastCategory !== '') ::help[lastCategory] = pageList{:length(pageList) - 2};\n"
	"\t\t::help['#pages'] = 'Available help pages:' # LF # LF;\n"
	"\t\tforeach(@::help, >if ($1{0} !== '_' && find('.', $1) != 0) ::help['#pages'] #= $1 # ', ');\n"
	"\t\t::help['#categories'] = ::help['#categories']{:length(::help['#categories']) - 2};\n"
	"\t\t::help['#pages'] = ::help['#pages']{:length(::help['#pages']) - 2};\n"
	"\t};\n"
	"\tpage = coalesce(@$0, 'help');\n"
	"\tif (page{0} === '/') {\n"
	"\t\tsearchFor = lower(page{1:});\n"
	"\t\tsearchLen = length(searchFor);\n"
	"\t\tsurroundChars = max(30 - (searchLen >> 1), 10);\n"
	"\t\tfound = 0;\n"
	"\t\tforeach(@::help, >{\n"
	"\t\t\tif ($1{0} !== '_' && find('.', $1) != 0 && (i = search(lower($2), searchFor)) < length($2)) {\n"
	"\t\t\t\t$line = $2{i - surroundChars:surroundChars} # '--->' # $2{i:searchLen}\n"
	"\t\t\t\t\t\t# '<---' # $2{i + searchLen:surroundChars};\n"
	"\t\t\t\tif (found == 0) print(\"The following pages contains the search string (showing only the first match for each page):\\n\");\n"
	"\t\t\t\tprint($1 # repeat(' ', 12 - length($1)) # ':  ' # replace($line, \"\\t\\n\", ' ', find, 1));\n"
	"\t\t\t\t++found;\n"
	"\t\t\t}\n"
	"\t\t});\n"
	"\t\tif (found == 0) print(\"No page contained the search string\");\n"
	"\t} else if (exists(@::help[page])) print(repeat('=', 20) # LF # help[page] # LF # repeat('=', 20))\n"
	"\telse print('No help page called ' # escape(page) # \". Type help('#pages') to list all available pages.\");\n"
	"\t( void )\n"
	"};\n"
	"\n"
	"help = function { /****    Type help() to get started.    ****/        invoke('help._lookup', , @$) };\n"
	"\t\n"
	"describe('#containers', 'ascend',\t\"@parent = ascend(@child)\",\t\t\t\t\t\t\"Returns a reference to the \\\"parent container\\\" of @child (i.e. the variable or element that contains the sub-element referred to by @child). If @child is a top-level variable, void is returned.\", \"ascend(@x['3']) === @x\\nascend(@x.y.z) === @x.y\\nascend(@x) === void\");\n"
	"describe('#containers', 'clone',\t\"@target = clone(@source, @target)\",\t\t\t\"Makes a \\\"deep copy\\\" of the container @source to @target, meaning that all elements under the source and any sub-elements that they may have (and so on) will be copied to the target. If there is a variable directly at @source it will also be copied to @target. The returned value is the input @target reference.\\n\\nNotice that this function does not erase any existing elements under @target. You may want to consider calling prune() on @target first.\", \"clone(@originalData, @myCarbonCopy)\", 'copy, prune');\n"
	"describe('#containers', 'foreach',\t\"foreach(@map, >doThis)\",\t\t\t\t\t\t\"Traverses all elements under @map (and any sub-elements that they may have and so on) and calls >doThis once for every encountered element. (An alternative description of foreach() is that it calls >doThis for every found variable that begins with the value of @map # '.') Three arguments will be passed to >doThis:\\n\\n$0 will be the full reference to the found element (e.g. \\\"::zoo.elephant\\\")\\n$1 will be the name of the element (e.g. \\\"elephant\\\")\\n$2 will be the value of the element.\\n\\nThe order with which elements are processed is undefined and depends on the implementation of PikaScript. Any modifications to @map while running foreach() will not be reflected in the calls to >doThis. Notice that you normally would not use foreach() on arrays since it would also include the 'n' element (the element count). Use iterate() or a simple for-loop instead.\", \"foreach(map(@a, 'Asia', 4157, 'Africa', 1030, 'Americas', 929, 'Europe', 739, 'Oceania', 35), >print($1 # '=' # $2))\", 'iterate');\n"
	"describe('#containers', 'map',\t\t\"@map = map(@map, ['keys', <values>, ...])\",\t\"Creates a \\\"map\\\" under @map by assigning sub-elements to @map for each key / value pair passed to the function. The returned value is the input @map reference.\\n\\nNotice that this function does not erase any existing elements under @map so you may call this function repeatedly to incrementally build a map. Use prune on @map to delete it.\\n\\nA creative use of this function is to create efficient \\\"switch statements\\\" something that is otherwise missing in PikaScript by mapping keys to functions / lambda expressions. You could then execute the switch like this: mySwitch[theKey].\", \"map(@userInfo['magnus'], 'fullName','Magnus Lidstroem' , 'birthDate','31 march 1972' , 'favoriteColor','#3333FF')\\nmap(@actions, 'hi',>print('Good day sir!') , 'spin',>{ for (i = 0; i < 360; ++i) print('Spun ' # i # ' degrees') } , 'quit',>doQuit = true)\\n[map(@temp, 'zero',0 , 'one',1 , 'two',2 , 'three',3)]['two'] == 2\", 'foreach, prune, set');\n"
	"describe('#containers', 'prune',\t\"+count = prune(@reference)\",\t\t\t\t\t\"Deletes the variable referenced to by @reference as well as all its sub-elements. Use prune() to delete an entire \\\"plain old data\\\" container (e.g. an \\\"array\\\" or \\\"map\\\"). Use destruct() instead for deleting an \\\"object\\\" to have its destructor called before it is deleted. prune() returns the number of variables that were deleted.\", \"prune(@myArray)\", 'delete, destruct');\n"
	"describe('#containers', 'redotify',\t\"'zzz...' = redotify('zzz%2e%2e%2e')\",\t\t\t\"Decodes an \\\"undotified\\\" string as returned from undotify(). See help for \\\"undotify\\\" for further explanation and examples.\", \"redotify('nearly 50%25 use google%2ecom daily') === 'nearly 50% use google.com daily'\", 'undotify');\n"
	"describe('#containers', 'set',\t\t\"@set = set(@set, ['keys', ...])\",\t\t\t\t\"Creates a \\\"set\\\" under @set by assigning sub-elements with the value true for each key. The returned value is the input @set reference.\\n\\nNotice that this function does not erase any existing elements under @set so you may call this function repeatedly to incrementally build a set. Use prune() on @set to delete it.\\n\\nOne practical use for sets is to efficiently check if a value belongs to a group of values. Initially you create the group of values with this function and later you can call exists(@set[key]).\", \"set(@validColors, 'red', 'green', 'blue', 'yellow')\\nexists(@[set(@smallPrimes, 2, 3, 5, 7, 11, 13, 17, 19)][13])\", 'foreach, map, prune');\n"
	"describe('#containers', 'undotify',\t\"'zzz%2e%2e%2e' = undotify('zzz...')\",\t\t\t\"Simply replaces all '.' with '%2e' and all '%' with '%25'. The purpose of this function is to allow arbitrary strings to work as keys in multi-dimensional arrays / deep containers. Without \\\"undotifying\\\" keys, any '.' in the keys would be interpreted as separators. E.g. \\\"files['name.ext'].size\\\" is the same as \\\"files.name.ext.size\\\", which is probably not what you want. Instead use \\\"files[undotify('name.ext')].size\\\". To return the original key from an \\\"undotified\\\" key, use redotify().\", \"undotify('nearly 50% use google.com daily') === 'nearly 50%25 use google%2ecom daily'\\nredotify(undotify('a.b.c%d.e.f')) === 'a.b.c%d.e.f'\", 'redotify');\n"
	"\n"
	"describe('#arrays', 'append',\t\"@array = append(@array, [<elements>, ...])\",\t\t\"Appends <elements> to @array. If [@array].n does not exist it will be initialized to 0 and this routine will in practice work like compose(). Unlike compose() however, all element argument must be present. The returned value is the input @array reference.\\n\\nNotice that calling this function on a \\\"double-ended queue\\\" also works.\", \"append(@myArray, 5, 6, 7, 8)\\nequal(append(compose(@temp1, 'a', 'b', 'c'), 'd', 'e', 'f'), compose(@temp2, 'a', 'b', 'c', 'd', 'e', 'f')) == true\", 'compose, insert');\n"
	"describe('#arrays', 'compose',\t\"@array = compose(@array, [<elements>, ...])\",\t\t\"Creates an array of indexed elements under @array initialized with the values of the remaining arguments (<element>). The first element will have an index of zero (e.g. \\\"array[0]\\\"). The special element 'n' (e.g. \\\"array.n\\\") will contain the number of indexed elements in the array. If an element argument is omitted the corresponding element will not be initialized, possibly making the array \\\"non-contiguous\\\". The returned value is the input @array reference.\\n\\nNotice that this function does not erase any existing elements under @array. You may want to consider calling prune() on @array before composing a new array.\", \"compose(@myArray, 1, 2, 3, 4)\\ncompose(@holy, 'nextIsEmpty', , 'previousWasEmpty')\\n[compose(@temp, 'zero', 'one', 'two', 'three')][2] === 'two'\", 'append, decompose, map, prune');\n"
	"describe('#arrays', 'copy',\t\t\"@target = copy(@source, +offset, +count, @target, +index)\", \"Copies +count elements from the @source array beginning at +offset into @target at +index, replacing any already existing elements at the target indices. The element count of the @target array (i.e. [@target].n) may be incremented to fit the new elements.\\n\\nThe @source array must be contiguous. If the output +index is greater than [@target].n (or (+index) + (+count) < 0), the resulting array will become non-contiguous.\\n\\nOnly direct elements under the arrays will be affected. Any sub-elements that they in turn may have are ignored. @source and @target may reference the same array. The returned value is the input @target reference.\", \"copy(@myArray, 10, 5, @myArray, 13)\\nequal(copy(compose(@temp1, 'a', 'b', 'c', 'd'), 1, 2, compose(@temp2, 'e', 'f', 'g', 'h'), 3), compose(@temp3, 'e', 'f', 'g', 'b', 'c')) == true\", 'clone, inject, remove');\n"
	"describe('#arrays', 'decompose',\"decompose(@array, [@variables, ...])\",\t\t\t\t\"Decomposes an array by storing the indexed elements under @array one by one into the given references. If an argument is left empty, the corresponding element index will be skipped.\", \"decompose(@breakMe, @first, @second, @third, , @noFourthButFifth)\", 'compose');\n"
	"describe('#arrays', 'equal',\t\"?same = equal(@arrayA, @arrayB)\",\t\t\t\t\t\"Returns true if the arrays @arrayA and @arrayB are the same size and all their elements are identical. Both arrays must be contiguous (i.e. all their elements must be defined). Only direct elements under the arrays will be tested. Any sub-elements that they in turn may have are silently ignored.\", \"equal(@firstArray, @secondArray)\\nequal(compose(@temp1, 1, 10, 100, 'one thousand'), compose(@temp2, 1, 10, 100, 'one thousand')) == true\");\n"
	"describe('#arrays', 'fill',\t\t\"@array = fill(@array, +offset, +count, <value>)\",\t\"Fills a range of +count elements in @array with <value> starting at +offset, replacing any existing elements. If the target @array does not exist (i.e. [@array].n is not defined) it is created. The element count (i.e. [@array].n) may be incremented to fit the new elements.\\n\\nOnly direct elements under the arrays will be affected. The returned value is the input @array reference.\", \"equal(fill(@a, 0, 5, 'x'), compose(@b, 'x', 'x', 'x', 'x', 'x'))\", 'copy, inject, insert, remove');\n"
	"describe('#arrays', 'inject',\t\"@target = inject(@source, +offset, +count, @target, +index)\", \"Inserts +count elements from the @source array beginning at +offset into @target at +index, relocating any elements at and after +index to make room for the inserted elements. Both arrays must be contiguous and the target +index must be between 0 and [@target].n. Only direct elements under the arrays will be affected. Any sub-elements that they in turn may have are ignored. @source and @target should not reference the same array. The returned value is the input @target reference.\", \"inject(@myArray, 10, 5, @myArray, 13)\\nequal(inject(compose(@temp1, 'a', 'b', 'c', 'd'), 1, 2, compose(@temp2, 'e', 'f', 'g', 'h'), 3), compose(@temp3, 'e', 'f', 'g', 'b', 'c', 'h')) == true\", 'copy, fill, insert, remove');\n"
	"describe('#arrays', 'iterate',\t\"iterate(@array, >doThis)\",\t\t\t\t\t\t\t\"Iterates all elements in @array (as determined by [@array].n) and calls >doThis once for every encountered element in ascending index order. Three arguments will be passed to >doThis:\\n\\n$0 will be the full reference to the found element (e.g. \\\"::highscores.3\\\")\\n$1 will be the element index (e.g. 3)\\n$2 will be the value of the element.\\n\\niterate() is the equivalent to foreach() for arrays. Any change to [@array].n while running iterate() will not be accounted for. The array must be contiguous. Only direct elements under the array will be iterated.\", \"iterate(compose(@a, 0, 'one', 2, true), >print($1 # '=' # $2))\", 'foreach');\n"
	"describe('#arrays', 'insert',\t\"@array = insert(@array, +offset, [<elements>, ...])\",\t\"Inserts one or more elements into @array before the index +offset. The array must be contiguous. Only direct elements under the array will be moved to make room for the new elements. Any sub-elements that they in turn may have remain unaffected. +offset must be between 0 and the element count of @array (or an exception will be thrown). [@array].n must be defined prior to calling this routine. The returned value is the input @array reference.\", \"insert(@myArray, 10, 'insert', 'three', 'strings')\\nequal(insert(compose(@temp1, 'a', 'b', 'f'), 2, 'c', 'd', 'e'), compose(@temp2, 'a', 'b', 'c', 'd', 'e', 'f')) == true\", 'inject, remove');\n"
	"describe('#arrays', 'remove',\t\"@array = remove(@array, +offset, [+count = 1])\",\t\"Removes +count number of elements from @array beginning at +offset, relocating any elements after the removed elements so that the array remains contiguous. (Only direct elements under the array are moved. Any sub-elements under these elements will be left untouched.)\\n\\nIf +offset and / or +count are negative, this function still yields predictable results (e.g. an +offset of -3 and +count of 6 will remove the three first elements). Likewise, it is allowed to remove elements beyond the end of the array (but naturally it will have no effect). The returned value is the input @array reference.\", \"remove(@removeNumberThree, 3)\\nremove(@drop1and2, 1, 2)\\nequal(remove(compose(@temp1, 'a', 'b', 'c', 'd', 'e'), 1, 3), compose(@temp2, 'a', 'e')) == true\", 'copy, fill, inject, insert, prune');\n"
	"describe('#arrays', 'rsort',\t\"@array = rsort(@array)\",\t\t\t\t\t\t\t\"Sorts the elements of @array in descending order. The returned value is the input @array reference. To sort in ascending order, use sort(). If you need greater control over the sorting (e.g. how elements are compared), use the lower level function qsort() instead.\", \"rsort(@myArray)\\nequal(rsort(compose(@temp1, 1.1, -5, 1.5, 17, 0x10, 'xyz', 'a', 'def', 'a')), compose(@temp2, 'xyz', 'def', 'a', 'a', 17, 0x10, 1.5, 1.1, -5)) == true\", 'qsort, sort');\n"
	"describe('#arrays', 'qsort',\t\"qsort(+from, +to, >compare, >swap)\",\t\t\t\t\"This is an abstract implementation of the quicksort algorithm. qsort() handles the logic of the sorting algorithm (the bones) while you provide the functions >compare and >swap that carries out the concrete operations on the data being sorted (the meat).\\n\\n+from and +to defines the sorting range (+to is non-inclusive).\\n\\n>compare is called with two sorting indices and you should return a negative value if the data for the first index ($0) should be placed before the data for the second index ($1). Return a positive non-zero value for the opposite and return zero if the data is identical. (You can use the global ::compare function to easily implement this.)\\n\\n>swap is also called with two indices in $0 and $1 ($0 is always less than $1). The function should swap the data for the two indices. (You can use the global ::swap function to easily implement this.)\\n\\nThe functions sort() and rsort() use this function to implement sorting of entire arrays (ascending and descending respectively).  \", \"qsort(0, myArray.n, >myArray[$0] - myArray[$1], >swap(@myArray[$0], @myArray[$1]))\\nqsort(0, scrambleMe.n, >random(2) - 1, >swap(@scrambleMe[$0], @scrambleMe[$1]))\", 'compare, rsort, sort');\n"
	"describe('#arrays', 'sort',\t\t\"@array = sort(@array)\",\t\t\t\t\t\t\t\"Sorts the elements of @array in ascending order. The returned value is the input @array reference. To sort in descending order, use rsort(). If you need greater control over the sorting (e.g. how elements are compared), use the lower level function qsort() instead.\", \"sort(@myArray)\\nequal(sort(compose(@temp1, 1.1, -5, 1.5, 17, 0x10, 'xyz', 'a', 'def', 'a')), compose(@temp2, -5, 1.1, 1.5, 0x10, 17, 'a', 'a', 'def', 'xyz')) == true\", 'qsort, rsort');\n"
	"\n"
	"describe('#queues', 'resetQueue',\t\"resetQueue(@queue)\",\t\t\t\t\t\t\t\"Initializes the \\\"double-ended queue\\\" referenced to by @queue for use with pushBack(), popFront() etc by setting the sub-elements [@queue].m and [@queue].n to 0.\\n\\n[@queue].m is the index of the first element on the queue and it is decremented on pushFront() and incremented on popFront(). [@queue].n is one greater than the index of the last element on the queue and it is incremented on pushBack() and decremented on popBack(). (The 'm' and 'n' identifiers were chosen to make queue() compatible with some of the array functions such as append().)\", \"resetQueue(@dq)\", 'popBack, popFront, pushBack, pushFront, queueSize');\n"
	"describe('#queues', 'queueSize',\t\"+count = queueSize(@queue)\",\t\t\t\t\t\"Returns the count of elements in the double-ended queue @queue. The count is defined by [@queue].n - [@queue].m (see resetQueue() for an explanation). (@queue must have been initialized with resetQueue() prior to calling this routine.)\", \"count = queueSize(@dq)\", 'popBack, popFront, pushBack, pushFront, resetQueue');\n"
	"describe('#queues', 'popBack',\t\t\"<value> = popBack(@queue)\",\t\t\t\t\t\"Pops <value> from the back of the double-ended queue @queue. If @queue is empty an exception will be thrown. (@queue must have been initialized with resetQueue() prior to calling this routine.)\", \"lastIn = popBack(@dq)\", 'popFront, pushBack, pushFront, queueSize, resetQueue');\n"
	"describe('#queues', 'pushBack',\t\t\"@queue = pushBack(@queue, <value>)\",\t\t\t\"Pushes <value> to the back of the double-ended queue @queue. The returned value is the input @queue reference. (@queue must have been initialized with resetQueue() prior to calling this routine.)\\n\\nIt is also legal to call append() to push several elements at once to a queue.\", \"pushBack(@dq, 'lastIn')\", 'append, popBack, popFront, pushFront, queueSize, resetQueue');\n"
	"describe('#queues', 'popFront',\t\t\"<value> = popFront(@queue)\",\t\t\t\t\t\"Pops <value> from the front of the double-ended queue @queue. If @queue is empty an exception will be thrown. (@queue must have been initialized with resetQueue() prior to calling this routine.)\", \"firstOut = popFront(@dq)\", 'popBack, pushBack, pushFront, queueSize, resetQueue');\n"
	"describe('#queues', 'pushFront',\t\"@queue = pushFront(@queue, <value>)\",\t\t\t\"Pushes <value> to the front of the double-ended queue @queue. The returned value is the input @queue reference. (@queue must have been initialized with resetQueue() prior to calling this routine.)\", \"pushFront(@dq, 'firstOut')\", 'popBack, popFront, pushBack, queueSize, resetQueue');\n"
	"\n"
	"describe('#debug', 'assert',\t\"assert(?testResult|>testMe, ['description'])\",\t\t\"Asserts are used to check for programming errors in run-time. Until you run \\\"debug.pika\\\", asserts are disabled which means this function will do absolutely nothing (it is defined as an empty function in \\\"stdlib.pika\\\"). Running \\\"debug.pika\\\" will enable asserts by redefining this function. When enabled, it either checks the boolean ?testResult or executes >testMe and checks the result.\\n\\nTwo things differ depending on the choice of passing a boolean or a function / lambda expression. If you pass a boolean, e.g. assert(myfunc() == 3), and assertions are disabled, the argument will still be evaluated, i.e. myfunc() will still be called. Furthermore, if 'description' is omitted, the exception on an assertion failure will simply contain 'true' or 'false'. \\n\\nIf you pass a function / lambda expression, e.g. assert(>myfunc() == 3), the argument will only be evaluated if assertions are enabled and the exception will contain the full expression. In most cases you will want to use this variant.\", \"assert(>(0 <= $0 <= 100))\\nassert(alwaysCallMe(), 'alwaysCallMe() failed miserably')\");\n"
	"describe('#debug', 'trace',\t\t\"trace([>tracer], [+level = 2])\",\t\t\t\t\t\"Sets or resets the tracer function. The tracer function is called at various points in the PikaScript interpreter code. For example, you can use it for tracing caught exceptions, function calls and even implement debuggers and profilers.\\n\\n+level ranges from 0 (no tracing) to 21 (maximum tracing) and determines which events that will trigger a callback. The default trace level of 2 calls the trace function on each function entry and exit. Other useful levels are 1 for tracing caught errors and 3 to trace every interpreted statement. (Please see \\\"PikaScriptImpl.h\\\" for a complete listing of all levels.)\\n\\nThe >tracer function will be called with the following arguments:\\n\\n$0 = the source code being executed\\n$1 = the offset into the source code\\n$2 = whether $3 is an lvalue or an rvalue (lvalue = identifier, rvalue = actual value)\\n$3 = the result from the last operation\\n$4 = trace level\\n$5 = false for \\\"entry\\\", true for \\\"exit\\\" (e.g. function call entry / exit).\\n\\nCall trace() without arguments to stop all tracing.\", \"trace()\\ntrace(function { print((if ($5) '} ' else '{ ') # if (exists(@^$callee)) ^$callee else '????') })\");\n"
	"\n"
	"describe('#help', 'help',\t\t\"help('page'|'/search')\", \"Prints a help page or searches the help database for text. Function syntax is documented with the following conventions:\\n\\n- Arguments in [ ] brackets are optional and default values may be documented like this: [arg = value]\\n- Vertical bar | is used to separate alternatives\\n- ... means repeat 0 or more times\\n- \\\"Type classes\\\" can be identified as follows: ?boolean, +number, 'string', >function, @reference, <arbitrary>\\n\\nType help('#categories') for a list of valid categories. help('#pages') lists all available help pages. You can also search the entire help database for a string with help('/search')\", \"help('#categories')\\nhelp('#pages')\\nhelp('#utils')\\nhelp('args')\\nhelp('/delete')\");\n"
	"describe('#help', 'describe',\t\"describe('category', 'page', 'syntax', ['description'], ['examples'], ['seealso'])\", \"Adds a help page to the database.\", \"describe('#mine', 'countIf', '+count = countIf(@array, <value>)', 'Counts the number of items in @array that is equal to <value>.', \\\"n = countIf(@fruits, 'apple')\\\")\");\n"
	"\n"
	"describe('#math', 'abs',\t'+y = abs(+x)',\t\t\t\t'Returns the absolute value of +x.', \"abs(3.7) == 3.7\\nabs(-3.7) == 3.7\\nabs(-infinity) == +infinity\", 'sign');\n"
	"describe('#math', 'acos',\t'+y = acos(+x)',\t\t\t\"Returns the arccosine of +x (which should be in the range -1 to 1 or the result will be undefined). The returned value is in the range 0 to PI.\\n\\nInverse: cos().\", \"acos(0.0) == PI / 2\\nacos(0.73168886887382) == 0.75\\nacos(cos(0.6)) == 0.6\", 'asin, atan, cos');\n"
	"describe('#math', 'asin',\t'+y = asin(+x)',\t\t\t\"Returns the arcsine of +x (which should be in the range -1 to 1 or the result will be undefined). The returned value is in the range -PI / 2 to PI / 2.\\n\\nInverse: sin().\", \"asin(0.0) == 0.0\\nasin(0.69613523862736) == 0.77\\nasin(sin(0.5)) == 0.5\", 'acos, atan, sin');\n"
	"describe('#math', 'atan',\t'+y = atan(+x)',\t\t\t\"Returns the arctangent of +x. The returned value is in the range -PI / 2 to PI / 2.\\n\\nInverse: tan().\", \"atan(0.0) == 0.0\\natan(0.93159645994407) == 0.75\\natan(tan(0.5)) == 0.5\", 'acos, asin, atan2, tan');\n"
	"describe('#math', 'atan2',\t'+z = atan2(+y, +x)',\t\t'Returns the arctangent of +y/+x with proper handling of quadrants. The returned value is in the range -PI to PI.', \"atan2(0.0, 1.0) == 0.0\\natan2(1.0, 0.0) == PI / 2\\natan2(sin(0.5), cos(0.5)) == 0.5\", 'atan');\n"
	"describe('#math', 'cbrt',\t'+y = cbrt(+x)',\t\t\t\"Returns the cube root of +x.\\n\\nInverse: cube(+y).\", \"cbrt(0.0) == 0.0\\ncbrt(0.421875) == 0.75\\ncbrt(cube(-0.7)) == -0.7\", 'sqr');\n"
	"describe('#math', 'ceil',\t'+y = ceil(+x)',\t\t\t'Returns the ceiling of value +x. Ceil() rounds both positive and negative values upwards.', \"ceil(0.0) == 0.0\\nceil(-0.99999) == 0.0\\nceil(1000.00001) == 1001.0\", 'floor, round, trunc');\n"
	"describe('#math', 'cos',\t'+y = cos(+x)',\t\t\t\t\"Returns the cosine of +x. The returned value is in the range -1 to 1.\\n\\nInverse: acos().\", \"cos(0.0) == 1.0\\ncos(0.72273424781342) == 0.75\\ncos(acos(0.5)) == 0.5\", 'acos, sin, tan');\n"
	"describe('#math', 'cosh',\t'+y = cosh(+x)',\t\t\t'Returns the hyperbolic cosine of +x.', \"cosh(0.0) == 1.0\\ncosh(0.9624236501192069) == 1.5\", 'sinh, tanh');\n"
	"describe('#math', 'cube',\t'+y = cube(+x)',\t\t\t\"Returns the cube of +x.\\n\\nInverse: cbrt(+y).\", \"cube(0.0) == 0.0\\ncube(0.90856029641607) == 0.75\\ncube(cbrt(-0.7)) == -0.7\", 'cbrt, sqr');\n"
	"describe('#math', 'exp',\t'+y = exp(+x)',\t\t\t\t\"Returns the exponential of +x. I.e, the result is e to the power +x.\\n\\nInverse: log().\", \"exp(0.0) == 1.0\\nexp(1.0) == 2.718281828459\\nexp(-0.28768207245178) == 0.75\\nexp(log(0.6)) == 0.6\", 'log, log2, log10, pow');\n"
	"describe('#math', 'factorial',\t'+y = factorial(+x)',\t'Returns the factorial of value +x. +x should be an integer in the range of 1 to 170.', \"factorial(10) == 3628800\");\n"
	"describe('#math', 'floor',\t'+y = floor(+x)',\t\t\t'Returns the floor of value +x. Floor() rounds both positive and negative values downwards.', \"floor(0.0) == 0.0\\nfloor(-0.99999) == -1.0\\nfloor(1000.00001) == 1000.0\", 'ceil, round, trunc');\n"
	"describe('#math', 'log',\t'+y = log(+x)',\t\t\t\t\"Returns the natural logarithm of +x (i.e. the logarithm with base e). +x should be positive or the result will be undefined.\\n\\nInverse: exp().\", \"log(0.0) == -infinity\\nlog(1.0) == 0.0\\nlog(2.7182818284591) == 1.0\\nlog(exp(0.6)) == 0.6\", 'exp, log2, log10, logb, nroot, pow');\n"
	"describe('#math', 'log2',\t'+y = log2(+x)',\t\t\t\"Returns the base-2 logarithm of +x. +x should be positive or the result will be undefined.\\n\\nInverse: pow(2, +y).\", \"log2(0.0) == -infinity\\nlog2(1.0) == 0.0\\nlog2(65536.0) == 16.0\\nlog2(pow(2, 1.3)) == 1.3\", 'exp, log, log10, logb, nroot, pow');\n"
	"describe('#math', 'log10',\t'+y = log10(+x)',\t\t\t\"Returns the base-10 logarithm of +x. +x should be positive or the result will be undefined.\\n\\nInverse: pow(10, +y).\", \"log10(0.0) == -infinity\\nlog10(1.0) == 0.0\\nlog10(10000.0) == 4.0\\nlog10(pow(10, 0.6)) == 0.6\", 'exp, log, log2, logb, nroot, pow');\n"
	"describe('#math', 'logb',\t'+y = logb(+b, +x)',\t\t\"Returns the logarithm of +x with base +b. +x should be positive or the result will be undefined. Equivalent to log(+x) / log(+b).\\n\\nInverses: +x = pow(+b, +y), +b = nroot(+y, +x).\", \"logb(20, 1.0) == 0.0\\nlogb(20, 8000) == 3\", 'exp, log, log2, log10, nroot, pow');\n"
	"describe('#math', 'nroot',\t'+x = nroot(+y, +z)',\t\t\"Returns the nth (+y) root of +z.\\n\\nInverse: +z = pow(+x, +y).\", \"nroot(11, pow(17, 11)) == 17\", 'exp, log, log2, logb, log10, pow');\n"
	"describe('#math', 'pow',\t'+z = pow(+x, +y)',\t\t\t\"Returns +x raised to the power of +y.\\n\\nInverses: +y = log(+z) / log(+x) or logb(+x, +z), +x = pow(+z, 1.0 / +y) or nroot(+y, +z).\", \"pow(0.0, 0.0) == 1.0\\npow(10.0, 4.0) == 10000.0\\npow(10.0, log10(0.8)) == 0.8\\npow(pow(2.8, 9.8), 1.0 / 9.8) == 2.8\", 'exp, log, log2, logb, log10, nroot');\n"
	"describe('#math', 'random',\t'+y = random(+x)',\t\t\t'Returns a pseudo-random number between 0 and +x.', 'random(100.0)');\n"
	"describe('#math', 'round',\t'+y = round(+x)',\t\t\t'Rounds the value of +x to the nearest integer. If the decimal part of +x is exactly 0.5, the rounding will be upwards (e.g. -3.5 rounds to 3.0).', \"round(1.23456) == 1\\nround(-1.6789) == -2\\nround(3.5) == 4.0\\nround(-3.5) == -3.0\\nround(1000.499999) == 1000.0\", 'ceil, floor, trunc');\n"
	"describe('#math', 'sign',\t'+y = sign(+x)',\t\t\t\"Returns -1 if +x is negative, +1 if +x is positive or 0 if +x is zero.\", \"sign(0.0) == 0.0\\nsign(12.34) == 1\\nsign(-infinity) == -1\", 'abs');\n"
	"describe('#math', 'sin',\t'+y = sin(+x)',\t\t\t\t\"Returns the sine of +x. The returned value is in the range -1 to 1.\\n\\nInverse: asin().\", \"sin(0.0) == 0.0\\nsin(0.84806207898148) == 0.75\\nsin(asin(0.5)) == 0.5\", 'asin, cos, tan');\n"
	"describe('#math', 'sinh',\t'+y = sinh(+x)',\t\t\t'Returns the hyperbolic sine of +x.', \"sinh(0.0) == 0.0\\nsinh(0.61958958372348) == 0.66\", 'cosh, tanh');\n"
	"describe('#math', 'sqr',\t'+y = sqr(+x)',\t\t\t\t\"Returns the square of +x.\\n\\nInverse: sqrt(+y).\", \"sqr(0.0) == 0.0\\nsqr(0.86602540378444) == 0.75\\nsqr(sqrt(0.75)) == 0.75\", 'cube, sqrt');\n"
	"describe('#math', 'sqrt',\t'+y = sqrt(+x)',\t\t\t\"Returns the square root of +x. +x should be positive or the result will be undefined.\\n\\nInverse: sqr(+y).\", \"sqrt(0.0) == 0.0\\nsqrt(0.5625) == 0.75\\nsqrt(sqr(0.7)) == 0.7\", 'sqr');\n"
	"describe('#math', 'tan',\t'+y = tan(+x)',\t\t\t\t\"Returns the tangent of +x.\\n\\nInverse: atan(+y).\", \"tan(0.0) == 0.0\\ntan(0.603982978253) == 0.69\\ntan(atan(0.3)) == 0.3\", 'atan, cos, sin');\n"
	"describe('#math', 'tanh',\t'+y = tanh(+x)',\t\t\t'Returns the hyperbolic tangent of +x.', \"tanh(0.0) == 0.0\\ntanh(0.9729550745276566) == 0.75\", 'cosh, sinh');\n"
	"describe('#math', 'trunc',\t'+y = trunc(+x, [+n = 0])',\t'Truncates the value of +x leaving up to +n decimal places intact. If +n is omitted, all decimals are truncated. Truncation rounds positive values downwards and negative values upwards.', \"trunc(1.23456) == 1\\ntrunc(-1.23456) == -1\\ntrunc(1.23456, 2) == 1.23\\ntrunc(1.5, 10) == 1.5\", 'ceil, floor, precision, round');\n"
	"\n"
	"describe('#objects', 'construct',\t\t\"@object = construct(@object, >constructor, [<arguments>, ...])\",\t\t\"Constructs\", \"\", 'destruct, new, newLocal, this');\n"
	"describe('#objects', 'destruct',\t\t\"+count = destruct(@object)\",\t\t\t\t\t\t\t\t\t\t\t\"Destructs\", \"\", 'construct, delete, prune');\n"
	"describe('#objects', 'gc',\t\t\t\t\"+count = gc()\",\t\t\t\t\t\t\t\t\t\t\t\t\t\t\"Garbage collect.\", \"\", 'new, newLocal');\n"
	"describe('#objects', 'invokeMethod',\t\"<result> = invokeMethod(@object, 'method', @args, [+offset = 0], [+count])\",\t\"Like invoke() but for methods.\", \"\", 'invoke');\n"
	"describe('#objects', 'method',\t\t\t\"'method' = method()\",\t\t\t\t\t\t\t\t\t\t\t\t\t\"Method called.\", \"\", 'this');\n"
	"describe('#objects', 'new',\t\t\t\t\"@object = new(>constructor, [<arguments>, ...])\",\t\t\t\t\t\t\"Allocates and constructs.\", \"\", 'construct, gc, newLocal');\n"
	"describe('#objects', 'newLocal',\t\t\"@object = newLocal(>constructor, [<arguments>, ...])\",\t\t\t\t\t\"Allocates and constructs on local heap.\", \"\", 'construct, gc, new');\n"
	"describe('#objects', 'this',\t\t\t\"@object = this()\",\t\t\t\t\t\t\t\t\t\t\t\t\t\t\"Object called.\", \"\", 'method');\n"
	"\n"
	"describe('#strings', 'char',\t\t\"'character' = char(+code)\",\t\t\t\t\t\t\"Returns the character represented by +code as a string. +code is either an ASCII or Unicode value (depending on how PikaScript is configured). If +code is not a valid character code the exception 'Illegal character code: {code}' will be thrown.\\n\\nInverse: ordinal('character').\", \"char(65) === 'A'\\nchar(ordinal('\xc3\xa5')) === '\xc3\xa5'\", 'ordinal');\n"
	"describe('#strings', 'chop',\t\t\"'chopped' = chop('string', +count)\",\t\t\t\t\"Removes the last +count number of characters from 'string'. This function is equivalent to 'string'{:length('string') - +count}. If +count is zero or negative, the entire 'string' is returned. If +count is greater than the length of 'string', the empty string is returned. (There is no function for removing characters from the beginning of the string because you can easily use 'string'{+count:}.)\", \"chop('abcdefgh', 3) === 'abcde'\\nchop('abcdefgh', 42) === ''\", 'length, right, trim');\n"
	"describe('#strings', 'bake',\t\t\"'concrete' = bake('abstract', ['escape' = \\\"{\\\"], ['return' = \\\"}\\\"])\",\t\"Processes the 'abstract' string by interpreting any text bracketed by 'escape' and 'return' as PikaScript expressions and injecting the results from evaluating those expressions. The default brackets are '{' and '}'. The code is evaluated in the caller's frame. Thus you can inject local variables like this: '{myvar}'.\", \"bake('The result of 3+7 is {3+7}') === 'The result of 3+7 is 10'\\nbake('Welcome back {username}. It has been {days} days since your last visit.')\", 'evaluate');\n"
	"describe('#strings', 'escape',\t\t\"'escaped' = escape('raw')\",\t\t\t\t\t\t\"Depending on the contents of the source string 'raw' it is encoded either in single (') or double (\\\") quotes. If the string contains only printable ASCII chars (ASCII values between 32 and 126 inclusively) and no apostrophes ('), it is enclosed in single quotes with no further processing. Otherwise it is enclosed in double quotes (\\\") and any unprintable ASCII character, backslash (\\\\) or quotation mark (\\\") is encoded using C-style escape sequences (e.g. \\\"line1\\\\nline2\\\").\\n\\nYou can use unescape() to decode an escaped string.\", \"escape(\\\"trivial\\\") === \\\"'trivial'\\\"\\nescape(\\\"it's got an apostrophe\\\") === '\\\"it''s got an apostrophe\\\"'\\nescape(unescape('\\\"first line\\\\n\\\\xe2\\\\x00tail\\\"')) === '\\\"first line\\\\n\\\\xe2\\\\x00tail\\\"'\", 'unescape');\n"
	"describe('#strings', 'find',\t\t\"+offset = find('string', 'chars')\",\t\t\t\t\"Finds the first occurrence of any character of 'chars' in 'string' and returns the zero-based offset (i.e. 0 = first character). The search is case-sensitive. If no characters in 'chars' exist in 'string', the length of 'string' is returned. Use rfind() to find the last occurrence instead of the first. Use span() to find the first occurrence of any character not present in 'chars'. Use search() to find sub-strings instead of single characters.\", \"find('abcd', 'd') == 3\\nfind('abcdcba', 'dc') == 2\\nfind('nomatch', 'x') == 7\", 'mismatch, rfind, search, span');\n"
	"describe('#strings', 'length',\t\t\"+count = length('string')\",\t\t\t\t\t\t\"Returns the character count of 'string'.\", \"length('abcd') == 4\");\n"
	"describe('#strings', 'lower',\t\t\"'lowercase' = lower('string')\",\t\t\t\t\t\"Translates 'string' character by character to lower case. Notice that the standard implementation only works with characters having ASCII values between 32 and 126 inclusively.\", \"lower('aBcD') === 'abcd'\", 'upper');\n"
	"describe('#strings', 'mismatch',\t\"+offset = mismatch('first', 'second')\",\t\t\t\"Compares the 'first' and 'second' strings character by character and returns the zero-based offset of the first mismatch (e.g. 0 = first character). If the strings are identical in contents, the returned value is the length of the shortest string. As usual, the comparison is case sensitive.\", \"mismatch('abcd', 'abcd') == 4\\nmismatch('abc', 'abcd') == 3\\nmismatch('abCd', 'abcd') == 2\", 'find, search, span');\n"
	"describe('#strings', 'ordinal',\t\t\"+code = ordinal('character')\",\t\t\t\t\t\t\"Returns the ordinal (i.e. the character code) of the single character string 'character'. Depending on how PikaScript is configured, the character code is an ASCII or Unicode value. If 'character' cannot be converted to a character code the exception 'Value is not single character: {character}' will be thrown.\\n\\nInverse: char(+code).\", \"ordinal('A') == 65\\nordinal(char(211)) == 211\", 'char');\n"
	"describe('#strings', 'precision',\t\"'string' = precision(+value, +precision)\",\t\t\t\"Converts +value to a decimal number string (in scientific E notation if required). +precision is the maximum number of digits to include in the output. Scientific E notation (e.g. 1.3e+3) will be used if +precision is smaller than the number of digits required to express +value in decimal notation. The maximum number of characters returned is +precision plus 7 (for possible minus sign, decimal point and exponent).\", \"precision(12345, 3) === '1.23e+4'\\nprecision(9876, 8) === '9876'\\nprecision(9876.54321, 8) === '9876.5432'\\nprecision(-0.000000123456, 5) === '-1.2346e-7'\\nprecision(+infinity, 1) === '+infinity'\", \"radix, trunc\");\n"
	"describe('#strings', 'radix',\t\t\"'string' = radix(+value, +radix, [+minLength])\",\t\"Converts the integer +value to a string using a selectable radix between 2 (binary) and 16 (hexadecimal). If +minLength is specified and the string becomes shorter than this, it will be padded with leading zeroes. May throw 'Radix out of range: {radix}' or 'Minimum length out of range: {minLength}'.\", \"radix(0xaa, 2, 12) === '000010101010'\\nradix(3735928559, 16) === 'deadbeef'\\nradix(0x2710, 10) === 10000\", 'precision');\n"
	"describe('#strings', 'repeat',\t\t\"'repeated' = repeat('repeatme', +count)\",\t\t\t\"Concatenates 'repeatme' +count number of times.\", \"repeat(' ', 5) === '     '\\nrepeat('-#-', 10) === '-#--#--#--#--#--#--#--#--#--#-'\");\n"
	"describe('#strings', 'replace',\t\t\"'processed' = replace('source', 'what', 'with', [>findFunction = search], [+dropCount = length(what)], [>replaceFunction = >$1])\",\t\"Replaces all occurrences of 'what' with 'with' in the 'source' string.\\n\\nThe optional >findFunction allows you to modify how the function finds occurrences of 'what' and +dropCount determines how many characters are replaced on each occurrence. The default >findFunction is ::search (and +dropCount is the number of characters in 'what'), which means that 'what' represents a substring to substitute. If you want this function to substitute any occurrence of any character in 'what', you can let >findFunction be ::find and +dropCount be 1. Similarly, you may use ::span to substitute occurrences of all characters not present in 'what'.\\n\\nFinally, >replaceFunction lets you customize how substrings should be replaced. It will be called with two arguments, the source substring in $0 and 'with' in $1, and it is expected to return the replacement substring.\", \"replace('Barbazoo', 'zoo', 'bright') === 'Barbabright'\\nreplace('Barbalama', 'lm', 'p', find, 1) === 'Barbapapa'\\nreplace('Bqaxrbzzabypeillme', 'Bbarel', '', span, 1) === 'Barbabelle'\\nreplace('B03102020', '0123', 'abmr', find, 1, >$1{$0}) === 'Barbamama'\", \"bake, find, search, span\");\n"
	"describe('#strings', 'reverse',\t\t\"'backwards' = reverse('string')\",\t\t\t\t\t\"Returns 'string' reversed.\", \"reverse('stressed') === 'desserts'\");\n"
	"describe('#strings', 'rfind',\t\t\"+offset = rfind('string', 'chars')\",\t\t\t\t\"As find(), but finds the last occurrence of any character of 'chars' instead of the first. -1 is returned if no character was found (unlike find() which returns the length of 'string').\", \"rfind('abcd', 'd') == 3\\nrfind('abcdcba', 'dc') == 4\\nrfind('nomatch', 'xyz') == -1\", 'find, rsearch, rspan');\n"
	"describe('#strings', 'right',\t\t\"'ending' = right('string', +count)\",\t\t\t\t\"Returns the last +count number of characters from 'string'. This function is equivalent to 'string'{length('string') - +count:}. If +count is greater than the length of 'string', the entire 'string' is returned. If +count is zero or negative, the empty string is returned. (There is no \\\"left\\\" function because you can easily use 'string'{:+count}.)\", \"right('abcdefgh', 3) === 'fgh'\\nright('abcdefgh', 42) === 'abcdefgh'\", 'chop, length, trim');\n"
	"describe('#strings', 'rsearch',\t\t\"+offset = rsearch('string', 'substring')\",\t\t\t\"As search(), but finds the last occurrence of 'substring' in 'string' instead of the first. A negative value is returned if 'substring' was not found (unlike search() which returns the length of 'string').\", \"rsearch('abcdabcd', 'cd') == 6\\nrsearch('nomatch', 'xyz') == -3\", 'rfind, rspan, search');\n"
	"describe('#strings', 'rspan',\t\t\"+offset = rspan('string', 'chars')\",\t\t\t\t\"As span(), but finds the last occurrence of a character not present in 'chars' instead of the first. -1 is returned if the entire 'string' consists of characters in 'chars (unlike span() which returns the length of 'string').\", \"rspan('abcd', 'abc') == 3\\nrspan('abcdcba', 'ab') == 4\\nrspan('george bush', 'he bugs gore') == -1\", 'rfind, rsearch, span');\n"
	"describe('#strings', 'search',\t\t\"+offset = search('string', 'substring')\",\t\t\t\"Finds the first occurrence of 'substring' in 'string' and returns the zero-based offset (e.g. 0 = first character). The search is case-sensitive. If 'substring' does not exist in 'string', the length of 'string' is returned. Use rsearch() to find the last occurrence instead of the first. Use find() to find the first occurrence of any character in a set of characters instead of a sub-string.\", \"search('abcdabcd', 'cd') == 2\\nsearch('nomatch', 'x') == 7\", 'find, mismatch, rsearch, span');\n"
	"describe('#strings', 'span',\t\t\"+offset = span('string', 'chars')\",\t\t\t\t\"Finds the first occurrence of a character in 'string' that is not present in 'chars' and returns the zero-based offset (i.e. 0 = first character). The search is case-sensitive. If the entire 'string' consists of characters in 'chars', the length of 'string' is returned. Use rspan() to find the last occurrence instead of the first. Use find() to find the first occurrence of any character in 'chars'.\", \"span('abcd', 'abc') == 3\\nspan('abcdcba', 'ab') == 2\\nspan('george bush', 'he bugs gore') == 11\", 'find, mismatch, rspan, search');\n"
	"describe('#strings', 'tokenize',\t\"tokenize('source', >processor, ['delimiters' = \\\"\\\\n\\\"])\",\t\t\t\"Divides the 'source' string into tokens separated by any character in 'delimiters' (linefeed by default). For every extracted token, >processor is called, passing the token as the single argument $0 (not including the delimiter). The final delimiter at the end of the string is optional. For example, tokenize() can be useful for reading individual lines from a text file, parsing tab or comma-separated data and splitting sentences into separate words.\", \"tokenize(\\\"First line\\\\nSecond line\\\\nLast line\\\\n\\\", >append(@lines, $0))\\ntokenize('Eeny, meeny, miny, moe', >print(trim($0)), ',')\\ntokenize('Data is not information, information is not knowledge, knowledge is not understanding, understanding is not!wisdom.', >if ($0 !== '') append(@words, $0), \\\" \\\\t\\\\r\\\\n,.!?&\\\\\\\"/;:=-()[]{}\\\")\", \"parse, trim, wildmatch\");\n"
	"describe('#strings', 'trim',\t\t\"'trimmed' = trim('string', ['leading' = \\\" \\\\t\\\\r\\\\n\\\"], ['trailing' = \\\" \\\\t\\\\r\\\\n\\\"])\", \"Trims the source 'string' from leading and / or trailing characters of choice. The default characters are any white space character. If you pass void to 'leading' or 'trailing' you can prevent the routine from trimming leading respectively trailing characters.\", \"trim(\\\"  extractme\\\\t\\\") === 'extractme'\\ntrim(\\\"\\\\n    keep trailing spaces  \\\\n\\\", , void) === \\\"keep trailing spaces  \\\\n\\\"\\ntrim(\\\"--- keep me ---\\\", '-', '-') === ' keep me '\", \"replace\");\n"
	"describe('#strings', 'unescape',\t\"'raw' = unescape('escaped')\",\t\t\t\t\t\t\"Converts a string that is either enclosed in single (') or double (\\\") quotes. If the single (') quote is used, the string between the quotes is simply extracted \\\"as is\\\" with the exception of pairs of apostrophes ('') that are used to represent single apostrophes. If the string is enclosed in double quotes (\\\") it can use a subset of the C-style escape sequences. The supported sequences are: \\\\\\\\ \\\\\\\" \\\\' \\\\a \\\\b \\\\f \\\\n \\\\r \\\\t \\\\v \\\\xHH \\\\uHHHH \\\\<decimal>. If the string cannot be successfully converted an exception will be thrown.\\n\\nInverse: escape('raw').\", \"unescape(\\\"'trivial'\\\") == 'trivial'\\nunescape('\\\"it''s got an apostrophe\\\"') == \\\"it's got an apostrophe\\\"\\nunescape(escape(\\\"first line\\\\n\\\\xe2\\\\x00tail\\\")) == \\\"first line\\\\n\\\\xe2\\\\x00tail\\\"\", 'escape, evaluate');\n"
	"describe('#strings', 'upper',\t\t\"'uppercase' = upper('string')\",\t\t\t\t\t\"Translates 'string' character by character to upper case. Notice that the standard implementation only works with characters having ASCII values between 32 and 126 inclusively.\", \"upper('aBcD') === 'ABCD'\", 'lower');\n"
	"describe('#strings', 'wildfind',\t\"+offset|void = wildfind('source', 'pattern', +from, +to, @captureQueue)\",\t\"This is a low-level subroutine used by wildmatch() to match the full or partial 'pattern' in 'source' between the offsets +from and +to (inclusively). The returned value is either the offset where the first match was found or void if no match was found. @captureQueue should be initialized with resetQueue() prior to calling this routine. \\\"Captured ranges\\\" will be pushed to this \\\"queue\\\" as pairs of offsets and lengths. Pop these with popFront().\\n\\nSee the documentation for wildmatch() for a description of the pattern syntax and more.\", \"wildfind('abcdef', 'def', 0, 6, @c) == 3\\nwildfind('abcdef', '[def]', 0, 6, @c) == 5\\nwildfind('abcdef', '[def]*', 0, 6, @c) == 3\\nwildfind('abcdef', '[^def]', 4, 6, @c) == void\", \"popFront, resetQueue, wildmatch\");\n"
	"describe('#strings', 'wildmatch',\t\"?matched = wildmatch('source', 'pattern', [@captures, ...])\",\t\t\"Tries to match the 'source' string with 'pattern' (which may contain \\\"wild card\\\" patterns). true is returned if there is a match. You may also capture substrings from 'source' into the @captures variables. The pattern syntax is inspired by the \\\"glob\\\" standard (i.e. the syntax used for matching file names in most operating systems). However, a lot of additional features have been added, making the complexity of the syntax somewhere between glob and \\\"regular expressions\\\". It is easiest to describe with some examples:\\n\\n*           any string (including the empty string)\\n?           a single arbitrary character\\n~           an optional arbitrary character\\nsmurf       the string 'smurf' exactly (comparison is always case sensitive)\\n*smurf*     'smurf' anywhere in the source\\n????~~~~    between four and eight arbitrary characters\\n[a-zA-Z]    any single lower or upper case letter between 'a' and 'z'\\n[^a-zA-Z]   any single character that is not between 'a' and 'z' (case insensitive)\\n[*]         matches a single asterisk\\n[^]         a single ^ character only\\n[[]]        [ or ]\\n[]^]        ] or ^\\n[x-]        x or -\\n[0-9]*      a string consisting of zero or more digits\\n[0-9]????   exactly four digits\\n[0-9]?*     a string consisting of one or more digits\\n[0-9]??~~   between two and four digits\\n[0-9]?[]*   a single digit and then an arbitrary string\\n{*}smurf    captures everything before 'smurf' into the next @captures variable\\n\\nNotice that the * and ~ quantifiers are always non-greedy (i.e. they match as little as they possibly can). (This is a limitation of the current implementation, there are plans to let double ** mark a greedy match instead.) If you want to perform case insensitive matching for the entire pattern, use lower() or upper() on the source string. There is also a low-level routine called wildfind() if you need greater control over the matching.\", \"wildmatch('readme.txt', '*.txt')\\nwildmatch('myfile.with.extension', '{[^<>:\\\"/\\\\|?*]*}.{[^<>:\\\"/\\\\|?*.]*}', @filename, @extension) && filename === 'myfile.with' && extension === 'extension'\\nwildmatch(LF # \\\"skip line\\\\n\\\\n\\\\tmatch : me \\\\nthis:is the rest\\\" # LF, \\\"*\\\\n[ \\\\t]*{[^ \\\\t]?*}[ \\\\t]*:*{[^ \\\\t]?[]*}[ \\\\t]*\\\\n{*}\\\", @key, @value, @theRest) && key === 'match' && value === 'me'\", \"lower, tokenize, upper, wildfind\");\n"
	"\n"
	"describe('#utils', 'args',\t\t'args([@variables, ...])',\t\t\t\t\t\t\t'Assigns arguments to named variables. Pass a reference to a local variable for each argument your function expects. The caller of your function must pass exactly this number of arguments or an exception will be thrown.', 'args(@x, @y)', 'vargs');\n"
	"describe('#utils', 'classify',\t\"'class' = classify(<value>)\",\t\t\t\t\t\t\"Examines <value> and tries to determine what \\\"value class\\\" it belongs to:\\n\\n- 'void' (empty string)\\n- 'boolean' ('true' or 'false')\\n- 'number' (starts with a digit, '+' or '-' and is convertible to a number)\\n- 'reference' (starting with ':' and containing at least one more ':')\\n- 'function' (enclosed in '{ }' or begins with '>:' and contains one more ':')\\n- 'native' (enclosed in '< >')\\n- 'string' (if no other match)\\n\\nNotice that since there are no strict value types in PikaScript the result of this function should be considered a \\\"hint\\\" only of what type <value> is. For example, there is no guarantee that a value classified as a 'function' actually contains executable code.\", \"classify(void) === 'void'\\nclassify('false') === 'boolean'\\nclassify(123.456) === 'number'\\nclassify(@localvar) === 'reference'\\nclassify(function { }) === 'function'\\nclassify(>lambda) === 'function'\\nclassify('<print>') === 'native'\\nclassify('tumbleweed') === 'string'\");\n"
	"describe('#utils', 'coalesce',\t'<value> = coalesce(<values>|@variables, ...)',\t\t\"Returns the first <value> in the argument list that is non-void, or the contents of the first @variables that exists (whichever comes first). void is returned if everything else fails.\\n\\nA word of warning here, if a string value happens to look like a reference (e.g. '::') it will be interpreted as a such which may yield unexpected results. E.g. coalesce('::uhuh', 'oops') will not return 'oops' if a global variable named 'uhuh' exists.\", \"coalesce(@gimmeVoidIfNotDefined)\\ncoalesce(maybeVoid, perhapsThisAintVoid, 'nowIAmDefinitelyNotVoid')\", 'defaults, exists');\n"
	"describe('#utils', 'compare',\t'<diff> = compare(<a>, <b>)',\t\t\t\t\t\t'Returns 0 if <a> equals <b>, -1 if <a> is less than <b> and 1 if <a> is greater than <b>. This function is useful in sorting algorithms.', \"compare('abc', 'def') < 0\\ncompare('def', 'abc') > 0\\ncompare('abc', 'abc') == 0\", 'qsort, swap');\n"
	"describe('#utils', 'defaults',\t'defaults([@variable, <value>, ...])',\t\t\t\t'Assigns each <value> to each @variable if it does not exist. This function is useful for initializing global variables and optional function arguments.', \"defaults(@name, 'Smith')\\ndefaults(@first, 'a', @last, 'z')\", 'coalesce');\n"
	"describe('#utils', 'delete',\t\"?deleted = delete(@variable)\",\t\t\t\t\t\t\"Deletes the variable referenced to by @variable and returns true if the variable existed and was successfully deleted.\\n\\nNotice that this function can only delete a single variable at a time. This means only a single \\\"element\\\" in a \\\"container\\\" as well. Use prune() to delete an entire \\\"plain old data\\\" container and destruct() to delete an object.\", \"delete(@begone)\\ndelete(@hurray[1972])\", 'destruct, exists, prune');\n"
	"describe('#utils', 'evaluate',\t\"<result> = evaluate('code', [@frame])\",\t\t\t\"Evaluates 'code' and returns the result. You can decide which frame should execute the code by supplying a reference in @frame. Without @frame, code is executed in its own frame just as if you would call a function. Only the \\\"frame identifier\\\" of @frame is used.\\n\\nYou may for example pass @$ to execute in the current frame, or @^$ to execute in the caller's frame (the '$' is there to reference the correct frame even if it is a lambda expression), or @:: to execute in the global frame.\", \"evaluate('3 + 3') == 6\\nevaluate('x = random(1)', @x)\", 'bake, invoke, parse, run, sourceFor, toSource');\n"
	"describe('#utils', 'exists',\t\"?found = exists(@variable)\",\t\t\t\t\t\t\"Returns true if the variable referenced to by @variable is defined. Notice that this function does not \\\"fall back\\\" to the global frame if a local variable does not exist. Therefore you should always prefix the variable name with '::' if you want to check for the existance of a global variable or function.\", \"exists(@::aglobal)\\nexists(@users['magnus lidstrom'])\", 'coalesce, defaults, delete');\n"
	"describe('#utils', 'include',\t\"include('filePath')\",\t\t\t\t\t\t\t\t\"Runs a PikaScript source file as with run() but only if 'filePath' has not been \\\"included\\\" before. The function determines this by checking the existance of a global ::included[filePath]. It defines this global after completing execution the first time. (run() does not define or use this global.) Use include() with source files that defines library functions and constants, e.g. 'stdlib.pika'.\", \"include('stdlib.pika')\", 'run');\n"
	"describe('#utils', 'input',\t\t\"'answer' = input('question')\",\t\t\t\t\t\t\"Prints 'question' and returns a line read from the standard input stream (excluding any terminating line feed characters). May throw 'Unexpected end of input file' or 'Input file error'.\", \"name = input(\\\"What's your name? \\\")\", 'print');\n"
	"describe('#utils', 'invoke',\t\"<result> = invoke(['callee'], [>body], @args, [+offset = 0], [+count])\",\t\"Calls 'callee' (or >body) with the argument list @args. The difference between using 'callee' or >body is that the former should be a string with a function name, while the latter should be an actual function body. If both arguments are present, >body will be executed, but the called function's $callee variable will be set to 'callee'. For debugging purposes it is recommended that you use the 'callee' argument.\\n\\n+offset can be used to adjust the element index for the first argument. +count is the number of arguments. If it is omitted, [@args].n is used to determine the count.\\n\\nMay throw 'Too few array elements'.\", \"invoke('max', , @values)\\ninvoke('(callback)', $0, @$, 1, 4)\", \"evaluate, invokeMethod, run\");\n"
	"describe('#utils', 'load',\t\t\"'contents' = load('filePath')\",\t\t\t\t\t\"Loads a file from disk and returns it as a string. The standard implementation uses the file I/O of the standard C++ library, which takes care of line ending conversion etc. It can normally only handle ASCII text files. May throw 'Cannot open file for reading: {filePath}' or 'Error reading from file: {filePath}'.\", \"data = load('myfolder/myfile.txt')\", 'save');\n"
	"describe('#utils', 'max',\t\t'<m> = max(<x>, <y>, [<z>, ...])',\t\t\t\t\t'Returns the largest value of all the arguments.', \"max(5, 3, 7, 1, 4) == 7\\nmax('Sleepy', 'Grumpy', 'Happy', 'Bashful', 'Dopey', 'Sneezy', 'Doc') === 'Sneezy'\\nmax('Zero', '10', '5') === 'Zero'\", 'min');\n"
	"describe('#utils', 'min',\t\t'<m> = min(<x>, <y>, [<z>, ...])',\t\t\t\t\t'Returns the smallest value of all the arguments.', \"min(5, 3, 7, 1, 4) == 1\\nmin('Sleepy', 'Grumpy', 'Happy', 'Bashful', 'Dopey', 'Sneezy', 'Doc') === 'Bashful'\\nmin('Zero', '10', '5') === '5'\", 'max');\n"
	"describe('#utils', 'parse',\t\t\"+offset = parse('code', ?literal)\",\t\t\t\t\"Parses 'code' (without executing it) and returns the number of characters that was successfully parsed. 'code' is expected to start with a PikaScript expression or in case ?literal is true, a single literal (e.g. a number). If ?literal is false, the parsing ends when a semicolon or any unknown or unexpected character is encountered (including unbalanced parentheses etc). The resulting expression needs to be syntactically correct or an exception will be thrown.\\n\\nYou can use this function to extract PikaScript code (or constants) inlined in other text. You may then evaluate the result with evaluate() and continue processing after the extracted code. Pass true for ?literal in case you want to evaluate a single constant and prevent function calls, assignments etc to be performed. Valid literals are: 'void', 'false', 'true', numbers (incl. hex numbers and 'infinity' of any sign), escaped strings (with single or double quotes), natives (enclosed in '< >'), function or lambda definitions.\", \"parse('3+3', false) == 3\\nparse('3+3', true) == 1\\nparse('1 + 2 * 3 ; stop at semicolon', false) == 10\\nparse(' /* leading comment */ code_here /* skip */ /* trailing */ /* comments */ but stop here', false) == 74\\nparse('stop after space after stop', false) == 5\\nparse('x + x * 3 ) * 7 /* stop at unbalanced ) */', false) == 10\\nparse('+infinity', true) == 9\\nparse('\\\"stop after the \\\\\\\", but before\\\" this text', true) == 31\", 'escape, evaluate, run, tokenize');\n"
	"describe('#utils', 'print',\t\t\"print('textLine')\",\t\t\t\t\t\t\t\t\"Prints 'textLine' to the standard output, appending a newline character. (Sorry, but standard PikaScript provides no means for outputting text without the newline.)\", \"print('Hello world!')\", 'input');\n"
	"describe('#utils', 'run',\t\t\"<result> = run('filePath', [<args>, ...])\",\t\t\"Loads and executes a PikaScript source file. The code is executed in the \\\"closure\\\" of the global frame, which means that variable assignments etc work on globals. The code can still use temporary local variables with the $ prefix. The passed <args> are available in $1 and up. $0 will be 'filePath'. The returned value is the final result value of the executed code, just as it would be for a function call. The first line of the source code file may be a \\\"Unix shebang\\\" (in which case it is simply ignored). Use include() if you wish to avoid a file from running more than once.\", \"run('chess.pika')\\nhtmlCode = run('makeHTML.pika', 'Make This Text HTML')\", 'evaluate, include, invoke');\n"
	"describe('#utils', 'save',\t\t\"save('filePath', 'contents')\",\t\t\t\t\t\t\"Saves 'contents' to a file (replacing any existing file). The standard implementation uses the file I/O of the standard C++ library, which takes care of line ending conversion etc. It can normally only handle ASCII text files. May throw 'Cannot open file for writing: {filePath}' or 'Error writing to file: {filePath}'.\", \"save('myfolder/myfile.txt', 'No, sir, away! A papaya war is on!')\", 'load');\n"
	"describe('#utils', 'sourceFor', \"'code' = sourceFor(@variable|@container, ['prefix' = ''])\",\t\t\"*** WARNING, THIS FUNCTION IS SLIGHTLY BROKEN AND MAY BE REMOVED. USE AT OWN RISK! ***\\n\\nCreates source code for recalling the definition of @variable or @container (with all its sub-elements). The created code can be used with evaluate() to recall the definition in any given frame. For example: evaluate('code', @$) would recreate variables in the current local frame and evaluate('code', @::) would create them in the global frame.\\n\\nEach output line will be prefixed with 'prefix'.\", \"save('AllGlobals.pika', sourceFor(@::))\\nevaluate(sourceFor(@wildmatch)) == wildmatch\\nevaluate(sourceFor(@::globalToLocalPlease), @$)\\nprint(sourceFor(@myFunction, \\\"\\\\t\\\\t\\\"))\", \"dump, evaluate, toSource\");\n"
	"describe('#utils', 'system',\t\"+exitCode = system('command')\",\t\t\t\t\t\"Tries to execute 'command' through the operating system's command interpreter. +exitCode is the return value from the command interpreter (a value of 0 usually means no error). May throw 'Error executing system command: {command}'.\");\n"
	"describe('#utils', 'swap',\t\t\"swap(@a, @b)\",\t\t\t\t\t\t\t\t\t\t\"Swaps the contents of the variables referenced to by @a and @b. This function is useful in sorting algorithms.\", \"swap(@master, @slave)\", 'compare, qsort');\n"
	"describe('#utils', 'time',\t\t\"+secs = time()\",\t\t\t\t\t\t\t\t\t\"Returns the system clock as \\\"epoch time\\\", i.e. the number of seconds that has passed since a specific reference date (usually January 1 1970).\", \"elapsed = time() - lastcheck\");\n"
	"describe('#utils', 'throw',\t\t\"throw('error')\",\t\t\t\t\t\t\t\t\t\"Throws an exception. 'error' should describe the error in human readable form. Use try() to catch errors. (PikaScript exceptions are standard C++ exceptions. It is up to the host application how uncaught exceptions are handled.)\", \"throw('Does not compute')\", 'try');\n"
	"describe('#utils', 'toSource',\t\"'literal' = toSource(<value>)\",\t\t\t\t\t\"*** WARNING, THIS FUNCTION IS SLIGHTLY BROKEN AND MAY BE REMOVED. USE AT OWN RISK! ***\\n\\nReturns the source code literal for <value>. I.e. 'literal' is formatted so that calling evaluate('literal') would bring back the original <value>.\", \"toSource('string theory') === \\\"'string theory'\\\"\\ntoSource('') === 'void'\\ntoSource('{ /* get funcy */ }') === 'function { /* get funcy */ }'\\nevaluate(toSource(@reffy)) == @reffy\\nevaluate(toSource(>lambchop), @$) == (>lambchop)\", \"dump, evaluate, sourceFor\");\n"
	"describe('#utils', 'try',\t\t\"'exception' = try(>doThis)\",\t\t\t\t\t\t\"Executes >doThis, catching any thrown exceptions and returning the error string of the exception. If no exception was caught, void is returned. The returned value of >doThis is discarded. (Although PikaScript exceptions are standard C++ exceptions you can only catch PikaScript errors with this function.)\", \"error = try(>data = load(file))\\ntry(>1+1) === void\\ntry(>1+'a') === \\\"Invalid number: 'a'\\\"\\ntry(>throw('catchme')) === 'catchme'\", 'throw');\n"
	"describe('#utils', 'vargs',\t\t'vargs([@arguments, ...], , [@optionals, ...])',\t'As args() but you may define arguments that are optional. The caller of your function must pass all required arguments or an exception will be thrown. An exception will also be thrown if the caller passes more optional arguments than present in vargs(). An easy way to assign default values for optional arguments is with the default() function. Alternatively you may want to use coalesce().', 'vargs(@required1, @required2, , @optional1, @optional2)', 'args, coalesce, defaults');\n"
	"describe('#utils', 'VERSION',\t\"'v' = VERSION\",\t\t\t\t\t\t\t\t\t'VERSION is a global variable containing the PikaScript engine version as a human readable text string.');\n";

const char* BUILT_IN_INTERACTIVE =
	"#! /usr/local/bin/PikaCmd\n"
	"\n"
	"/*\n"
	"\tinteractive.pika v0.97\n"
	"\n"
	"\tPikaScript is released under the \"New Simplified BSD License\". http://www.opensource.org/licenses/bsd-license.php\n"
	"\t\n"
	"\tCopyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem\n"
	"\tAll rights reserved.\n"
	"*/\n"
	"\n"
	"interact = function {\n"
	"\tHELP = '\n"
	"Enter expressions to evaluate them interactively line by line. E.g.:\n"
	"\n"
	"    3+3\n"
	"    print(''hello world'')\n"
	"    run(''chess.pika'')\n"
	"    \n"
	"You can evaluate a multi-line expression by ending the first line with an opening curly bracket (''{''). Finish the expression with a single closing curly bracket (''}''). E.g.:\n"
	"\n"
	"    f = function {\n"
	"        print(''hello world'');\n"
	"    }\n"
	"    \n"
	"The special global variable ''_'' holds the result of the last evaluated expression. E.g.:\n"
	"\n"
	"    25 * 25\n"
	"    sqrt(_)\n"
	"    _ == 25\n"
	"\t\n"
	"Special commands are:\n"
	"\n"
	"    ?                        this help\n"
	"    <page>?                  shows a page from the standard library help system (type ''help?'' for more info)\n"
	"    =                        displays the full definition of the last evaluated expression\n"
	"    <variable>=              displays the full definition of a variable / function / container \n"
	"    %                        re-run last executed PikaScript source file\n"
	"    %['']<path>[''] [args...]  runs a PikaScript source file (optionally with arguments)\n"
	"    !<command>               executes a command with the operating system''s command interpreter\n"
	"    exit                     exits\n"
	"';\n"
	"\t\n"
	"\targs(@prompt, @where);\n"
	"\tprintReturn = printError = print;\n"
	"\tRETURN_COLOR = ERROR_COLOR = '';\n"
	"\tif (exists(@::PLATFORM) && ::PLATFORM === 'UNIX') {\n"
	"\t\tRETURN_COLOR = \"\\x1B[0;2m\";\n"
	"\t\tERROR_COLOR = \"\\x1B[1;31m\";\n"
	"\t\toldPrint = print;\n"
	"\t\toldInput = input;\n"
	"\t\t::print = function { <print>((if ($0{0} != \"\\x1B\") \"\\x1B[0;32m\") # $0) };\n"
	"\t\t::input = function { <input>(if ($0{0} != \"\\x1B\") \"\\x1B[0;33m\" # $0 # \"\\x1B[m\" else $0) };\n"
	"\t};\n"
	"\t::_ = void;\n"
	"\toneLine = function { limitLength(singleLine($0), CONSOLE_COLS - 7) };\n"
	"\tshow = function {\n"
	"\t\targs(@var);\n"
	"\t\tmore(var # if (classify(var) === 'reference' && (s = sourceFor(var, ' ')) !== '')\n"
	"\t\t\t\t(LF # repeat('=', 8) # LF # s # repeat('=', 8)));\n"
	"\t\t( void )\n"
	"\t};\n"
	"\tfor (; {\n"
	"\t\tif ((s = input(prompt)) === 'exit') ( false )\n"
	"\t\telse {\n"
	"\t\t\t::__ = void;\n"
	"\t\t\ttryThis => (::__ = evaluate(s, where));\n"
	"\t\t\tprintThis = RETURN_COLOR # '----- ( {oneLine(toSource(::__))} )';\n"
	"\t\t\t\n"
	"\t\t\tif (s{0} === '!') tryThis => (::__ = system(s{1:}))\n"
	"\t\t\telse if (s{0} === '%') {\n"
	"\t\t\t\tif (s === '%' && !exists(@arglist.n)) {\n"
	"\t\t\t\t\ttryThis = void;\n"
	"\t\t\t\t\tprintThis = \"No PikaScript source executed yet.\"\n"
	"\t\t\t\t} else {\n"
	"\t\t\t\t\tif (s !== '%') {\n"
	"\t\t\t\t\t\tfor (prune(@arglist); s !== void; s = s{i:}) {\n"
	"\t\t\t\t\t\t\ts = s{1 + span(s{1:}, ' '):};\n"
	"\t\t\t\t\t\t\tif (s !== void) {\n"
	"\t\t\t\t\t\t\t\tappend(@arglist, if (s{0} === \"'\" || s{0} === '\"') s{1:(i = 1 + find(s{1:}, s{0})) - 1}\n"
	"\t\t\t\t\t\t\t\t\t\telse s{:i = find(s, ' ')});\n"
	"\t\t\t\t\t\t\t}\n"
	"\t\t\t\t\t\t};\n"
	"\t\t\t\t\t\tif (!wildmatch(arglist[0], '*.[^/:\\]*')) arglist[0] #= '.pika';\n"
	"\t\t\t\t\t};\n"
	"\t\t\t\t\tl = '';\n"
	"\t\t\t\t\titerate(@arglist, >l #= ' ' # escape($2));\n"
	"\t\t\t\t\tprint('----- Running' # l);\n"
	"\t\t\t\t\ttryThis => (::__ = invoke('run', , @arglist));\n"
	"\t\t\t\t}\n"
	"\t\t\t}\n"
	"\t\t\telse if (s === '=') { tryThis => show(::_); printThis = void }\n"
	"\t\t\telse if (s === '?') { tryThis => print(HELP); printThis = void }\n"
	"\t\t\telse if (right(s, 1) === '?') { tryThis => help(chop(s, 1)); printThis = void }\n"
	"\t\t\telse if (right(s, 1) === '=') { tryThis => show(@[where][chop(s, 1)]); printThis = void }\n"
	"\t\t\telse if (right(s, 1) === '{') {\n"
	"\t\t\t\tprint('(Multi-line input. Finish with a single ''}''.)');\n"
	"\t\t\t\tfor (; { s #= LF # (l = input('')); l !== '}' }; );\n"
	"\t\t\t};\n"
	"\n"
	"\t\t\tif (tryThis !== void && (x = try(tryThis)) !== void) printThis = ERROR_COLOR # '!!!!! {x}';\n"
	"\t\t\tif (printThis !== void) print(bake(printThis));\n"
	"\t\t\t::_ = ::__;\n"
	"\t\t\t( true )\n"
	"\t\t}\n"
	"\t}; );\n"
	"\n"
	"\tif (exists(@oldPrint)) {\n"
	"\t\t::print = oldPrint;\n"
	"\t\t::input = oldInput;\n"
	"\t\tprint(\"\\x1B[m\");\n"
	"\t}\n"
	"};\n"
	"\n"
	"if (exists(@$1) && $1 == 'go') {\n"
	"\tinclude('stdlib.pika');\n"
	"\tinclude('debug.pika');\n"
	"\tprint(\"Type '?' for help.\");\n"
	"\tinteract('Pika> ', @::);\n"
	"};\n";

const char* BUILT_IN_STDLIB =
	"/*\n"
	"\tstdlib.pika v0.97\n"
	"\n"
	"\tPikaScript is released under the \"New Simplified BSD License\". http://www.opensource.org/licenses/bsd-license.php\n"
	"\t\n"
	"\tCopyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem\n"
	"\tAll rights reserved.\n"
	"*/\n"
	"\n"
	"// --- Utils ---\n"
	"\n"
	"args = function {\n"
	"\tif (^$n != $n) throw(if (^$n > $n) 'Too many arguments' else 'Too few arguments');\n"
	"\tfor (i = 0; i < $n; ++i) [$[i]] = ^$[i]\n"
	"};\n"
	"assert = function { };\n"
	"classifiers[''] = function { 'void' };\n"
	"classifiers['f'] = function { if ($0 === 'false') 'boolean' else 'string' };\n"
	"classifiers['t'] = function { if ($0 === 'true') 'boolean' else 'string' };\n"
	"classifiers['{'] = function { if ($0{length($0) - 1} === '}') 'function' else 'string' };\n"
	"classifiers['>'] = function { if ($0{1} === ':' && span(':', $0{2:}) == 1) 'function' else 'string' };\n"
	"classifiers[':'] = function { if (span(':', $0{1:}) == 1) 'reference' else 'string' };\n"
	"classifiers['<'] = function { if ($0{length($0) - 1} === '>') 'native' else 'string' };\n"
	"for ($i = 0; $i < 12; ++$i)\n"
	"\tclassifiers['0123456789+-'{$i}] = function { if (parse($0, true) == length($0)) 'number' else 'string' };\n"
	"classify = function { if (exists(p = @::classifiers[$0{0}])) [p]($0) else 'string' };\n"
	"coalesce = function {\n"
	"\tr = void;\n"
	"\tfor (i = 0; i < $n && ((r = $[i]) === void || classify(r) === 'reference' && if (!exists(r)) { r = void; true }\n"
	"\t\t\telse { r = [r]; false }); ++i);\n"
	"\t( r )\n"
	"};\n"
	"compare = function { if ($0 < $1) -1 else if ($0 > $1) 1 else 0 };\n"
	"defaults = function { for (i = 0; i + 2 <= $n; i += 2) if (!exists($[i])) [$[i]] = $[i + 1] };\n"
	"max = function { for (i = 1; i < $n; ++i) if ($[i] > $0) $0 = $[i]; ( $0 ) };\n"
	"min = function { for (i = 1; i < $n; ++i) if ($[i] < $0) $0 = $[i]; ( $0 ) };\n"
	"\n"
	"// FIX : sourceFor is outdated, we have new rules for dots etc in subscription\n"
	"sourceFor = function {\n"
	"\tvargs(@var, , @prefix);\n"
	"\tdefaults(@prefix, '');\n"
	"\ts0 = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$';\n"
	"\tsX = s0 # '0123456789';\n"
	"\ttoSymbol => {\n"
	"\t\t// FIX : doesn't work with arrays :(, becomes a.3 instead of a[3] (I need to look at every first char after .)\n"
	"\t\tif (find(($0 = $0{1 + find($0{1:}, ':') + 1:}){0}, s0) == 0 && span($0{1:}, sX) >= length($0) - 1\n"
	"\t\t\t\t&& search($0, '..') >= length($0)) ( $0 )\n"
	"\t\telse ( '[' # escape($0) # ']' )\n"
	"\t};\n"
	"\tsrc = '';\n"
	"\t// FIX : can't really use toSource because of the ambiguity with function and strings like '{ alfons aaberg }' ...\n"
	"\tif (exists(var)) src #= prefix # toSymbol(var) # ' = ' # toSource([var]) # ';' # LF;\n"
	"\tbuffer = '';\n"
	"\tforeach(var, >{\n"
	"\t\tbuffer #= prefix # toSymbol($0) # ' = ' # toSource($2) # ';' # LF;\n"
	"\t\tif (length(buffer) >= 2048) { src #= buffer; buffer = '' }\n"
	"\t});\n"
	"\t( src # buffer )\n"
	"};\n"
	"swap = function { t = [$0]; [$0] = [$1]; [$1] = t };\n"
	"\n"
	"// FIX : This function will describe any string looking like a function, e.g. '{ wrong!! }' as a function, even if it isn't.\n"
	"toSource = function {\n"
	"\tif ((c = classify($0)) === 'void') ( c )\n"
	"\telse if (c === 'string') ( escape($0) )\n"
	"\telse if (c === 'function') if ($0{0} !== '>') ( 'function ' # $0 ) else ( escape($0) )\n"
	"\telse if (c === 'reference') if ($0{0:2} === '::') ( '@' # $0 ) else ( escape($0) )\n"
	"\telse ( $0 )\n"
	"};\n"
	"vargs = function {\n"
	"\tfor (i = 0; i < $n && exists(@$[i]); ++i) {\n"
	"\t\tif (i >= ^$n) throw('Too few arguments');\n"
	"\t\tif (exists(@^$[i])) [$[i]] = ^$[i]\n"
	"\t};\n"
	"\tfor (++i; i < $n; ++i) if (i <= ^$n && exists(@^$[i - 1])) [$[i]] = ^$[i - 1] else delete($[i]);\n"
	"\tif (i <= ^$n) throw('Too many arguments');\n"
	"\t( i - 1 )\n"
	"};\n"
	"\n"
	"// --- Strings ---\n"
	"\n"
	"CR = \"\\r\";\n"
	"LF = \"\\n\";\n"
	"TAB = \"\\t\";\n"
	"WS = \" \\t\\r\\n\";\n"
	"bake = function {\n"
	"\topenLength = length(open = coalesce(@$1, '{'));\n"
	"\tcloseLength = length(close = coalesce(@$2, '}'));\n"
	"\tfor ({ out = ''; in = $0 }; in !== ''; ) {\n"
	"\t\tout #= in{:(i = search(in, open))};\n"
	"\t\tif ((in = in{i:}) !== '') {\n"
	"\t\t\ti = parse(in = in{openLength:}, false);\n"
	"\t\t\tout #= evaluate(in{:i}, @^$);\n"
	"\t\t\tif (in{i:closeLength} !== close) throw('Missing ' # escape(close));\n"
	"\t\t\tin = in{i + closeLength:};\n"
	"\t\t}\n"
	"\t};\n"
	"\t( out )\n"
	"};\n"
	"chop = function { $0{:length($0) - $1} };\n"
	"repeat = function { $1 *= length($0); for (s = $0; length(s) < $1;) s #= s; ( s{:$1} ) };\n"
	"replace = function {\n"
	"\tvargs(@src, @what, @with, , @findFunction, @dropCount, @replaceFunction);\n"
	"\tdefaults(@findFunction, search, @dropCount, length(what), @replaceFunction, >$1);\n"
	"\tif (dropCount <= 0) throw('Invalid drop count argument for replace()');\n"
	"\tfor (d = ''; { d #= src{:i = findFunction(src, what)}; i < length(src) }; src = src{i + dropCount:})\n"
	"\t\td #= replaceFunction(src{i:dropCount}, with);\n"
	"\t( d )\n"
	"};\n"
	"rfind = function { length($0) - 1 - find(reverse($0), $1) };\n"
	"right = function { $0{length($0) - $1:} };\n"
	"rspan = function { length($0) - 1 - span(reverse($0), $1) };\n"
	"rsearch = function { length($0) - length($1) - search(reverse($0), reverse($1)) };\n"
	"tokenize = function {\n"
	"\tvargs(@src, @func, , @delim);\n"
	"\tdefaults(@delim, LF);\n"
	"\tfor (; src !== ''; ) {\n"
	"\t\tfunc(src{:(i = find(src, delim))});\n"
	"\t\tsrc = src{i + 1:};\n"
	"\t};\n"
	"\t( void )\n"
	"};\n"
	"trim = function {\n"
	"\tvargs(@src, , @leading, @trailing);\n"
	"\tb = span(src, coalesce(@leading, WS));\n"
	"\te = rspan(src, coalesce(@trailing, WS));\n"
	"\t( src{b:e + 1 - b} )\n"
	"};\n"
	"unescape = function { if (span($0{0}, \"\\\"'\") != 1) throw('Invalid string literal'); ( evaluate($0) ) };\n"
	"\n"
	"// FIX : move to parsing.pika\n"
	"wildfind = function /* $0=string, $1=pattern, $2=from, $3=to, $4=captureQueue */ {\n"
	"\tif ((brk = find($1, '*?~[{}')) != 0) {\n"
	"\t\tfor ({ i = $2; $3 = min($3, length($0) - (l = length($1{:brk}))) }\n"
	"\t\t\t\t; (i += search($0{i:$3 + l - i}, $1{:brk})) <= $3 && wildfind($0, $1{brk:}, i + l, i + l, $4) === void\n"
	"\t\t\t\t; ++i)\n"
	"\t} else if ($1 === '') i = length($0)\n"
	"\telse if ((c = $1{0}) === '{' || c === '}') {\n"
	"\t\tif ((i = wildfind($0, $1{1:}, $2, $3, $4)) !== void)\n"
	"\t\t\tif (c === '{') { pushFront($4, popBack($4) - i); pushFront($4, i) } else pushBack($4, i)\n"
	"\t} else {\n"
	"\t\tset = '';\n"
	"\t\tif (c === '[') {\n"
	"\t\t\tif ($1{:2} === '[^' && $1{:3} !== '[^]' || $1{:4} === '[^]]') { findf = span; spanf = find; $1 = $1{2:} }\n"
	"\t\t\telse { findf = find; spanf = span; $1 = $1{1:} };\n"
	"\t\t\tfor (; $1 !== '' && $1{0} !== ']' || $1{:2} === ']]' || $1{:3} === ']^]'; $1 = $1{i:}) {\n"
	"\t\t\t\tset #= $1{:i = 1 + find($1{1:}, '-]')};\n"
	"\t\t\t\tif ($1{i} === '-' && $1{i + 1} !== ']') {\n"
	"\t\t\t\t\tfor ({ f = ordinal($1{i - 1}) + 1; t = ordinal($1{i + 1}) }; f <= t; ++f) set #= char(f);\n"
	"\t\t\t\t\ti += 2;\n"
	"\t\t\t\t}\n"
	"\t\t\t};\n"
	"\t\t\t$1 = $1{1:}\n"
	"\t\t};\n"
	"\t\tif (set === '') { findf => 0; spanf = length };\n"
	"\t\tmini = span($1, '?');\n"
	"\t\t$1 = $1{(maxi = mini + span($1{mini:}, '~')):};\n"
	"\t\tif (maxi == mini && $1{0} === '*') { maxi = 0x7FFFFFFF; $1 = $1{1:} }\n"
	"\t\telse if (mini == 0 && maxi == 0) mini = maxi = 1;\n"
	"\t\tif (mini == 0) findf => 0;\n"
	"\t\tfor (i = $2; (i += findf($0{i:$3 + 1 - i}, set)) <= $3 && ((n = spanf($0{i:maxi}, set)) < mini\n"
	"\t\t\t\t|| wildfind($0, $1, i + mini, i + n, $4) === void); i += max(n - mini, 0) + 1)\n"
	"\t};\n"
	"\t( if (i <= $3) i else void )\n"
	"};\n"
	"wildmatch = function {\n"
	"\tresetQueue(@cq);\n"
	"\tif (found = (wildfind($0, $1, 0, 0, @cq) == 0)) {\n"
	"\t\tfor (i = 2; i < $n; ++i) { offset = popFront(@cq); length = popFront(@cq); [$[i]] = $0{offset:length} }\n"
	"\t};\n"
	"\t( found )\n"
	"};\n"
	"\n"
	"// --- Math ---\n"
	"\n"
	"E = 2.71828182845904523536;\n"
	"PI = 3.14159265358979323846;\n"
	"cbrt = function { pow(abs($0), 1 / 3) * sign($0) };\n"
	"cube = function { $0 * $0 * $0 };\n"
	"factorial = function { if (~~$0 > 170) ( +infinity ) else { v = 1; for (i = 2; i <= $0; ++i) v *= i } };\n"
	"log2 = function { log($0) * 1.44269504088896340736 };\n"
	"logb = function { log($1) / log($0) };\n"
	"nroot = function { pow($1, 1 / $0) };\n"
	"round = function { floor($0 + 0.5) };\n"
	"sign = function { if ($0 < 0) -1 else if ($0 > 0) 1 else 0 };\n"
	"sqr = function { $0 * $0 };\n"
	"trunc = function {\n"
	"\tm = (if (exists(@$1)) pow(10, min(max(~~$1, 0), 23)) else 1);\n"
	"\t( (if ($0 < 0) ceil($0 * m) else floor($0 * m)) / m )\n"
	"};\n"
	"\n"
	"// --- Containers ---\n"
	"\n"
	"ascend = function { $0{:rfind($0, '.')} };\n"
	"clone = function { t = $1; foreach($0, >[t][$1] = $2); if (exists($0)) [t] = [$0]; ( $1 ) };\n"
	"map = function { for (i = 1; i + 2 <= $n; i += 2) [$0][$[i]] = $[i + 1]; ( $0 ) };\n"
	"prune = function { n = 0; foreach($0, >if (delete($0)) ++n); if (delete($0)) ++n; ( n ) };\n"
	"redotify = function { replace($0, '%', void, find, 3, >char(evaluate('0x' # $0{1:}))) };\n"
	"set = function { for (i = 1; i < $n; ++i) [$0][$[i]] = true; ( $0 ) };\n"
	"undotify = function { replace($0, '.%', void, find, 1, >'%' # radix(ordinal($0), 16, 2)) };\n"
	"\n"
	"// --- Arrays ---\n"
	"\n"
	"append = function { defaults(@[$0].n, 0); for (i = 1; i < $n; ++i) [$0][[$0].n++] = $[i]; ( $0 ) };\n"
	"compose = function { for (i = 1; i < $n; ++i) if (exists(@$[i])) [$0][i - 1] = $[i]; [$0].n = $n - 1; ( $0 ) };\n"
	"copy = function {\n"
	"\tif ($1 >= $4) for (i = 0; i < $2; ++i) [$3][$4 + i] = [$0][$1 + i]\n"
	"\telse for (i = $2 - 1; i >= 0; --i) [$3][$4 + i] = [$0][$1 + i];\n"
	"\t[$3].n = max(coalesce(@[$3].n, 0), $4 + $2);\n"
	"\t( $3 )\n"
	"};\n"
	"decompose = function { for (i = 1; i < $n; ++i) if (exists(@$[i])) [$[i]] = [$0][i - 1]; ( void ) };\n"
	"equal = function { ( [$0].n == [$1].n && { for (i = 0; i < [$0].n && [$1][i] == [$0][i];) ++i; } == [$0].n ) };\n"
	"fill = function { for (i = $1 + $2; --i >= $1;) [$0][i] = $3; [$0].n = max(coalesce(@[$0].n, 0), $1 + $2); ( $0 ) };\n"
	"inject = function {\n"
	"\tif ($4 < 0 || $4 > [$3].n) throw('Insertion index out of bounds');\n"
	"\t[$3].n += $2;\n"
	"\tfor (i = [$3].n - 1; i >= $4 + $2; --i) [$3][i] = [$3][i - $2];\n"
	"\tfor (; i >= $4; --i) [$3][i] = [$0][i - $4 + $1];\n"
	"\t( $3 )\n"
	"};\n"
	"insert = function { inject(@$, 2, $n - 2, $0, $1) };\n"
	"iterate = function {\n"
	"\tfor ({ i = coalesce(@[$0].m, 0); n = [$0].n }; i < n; ++i) if (exists(p = @[$0][i])) $1(p, i, [p]) else $1(p, i);\n"
	"\t( i )\n"
	"};\n"
	"qsort = function {\n"
	"\tfor (--$1; $0 + 1 < $1; $0 = $l) {\n"
	"\t\tfor ({ $l = $0; $h = $1; $m = ($l + $h) \\ 2 }; $l < $h; ) {\n"
	"\t\t\tfor (; $l <= $h && $2($l, $m) <= 0 && $2($h, $m) >= 0; { ++$l; --$h });\n"
	"\t\t\tfor (; $l <= $h && $2($h, $m) > 0; --$h);\n"
	"\t\t\tfor (; $l <= $h && $2($l, $m) < 0; ++$l);\n"
	"\t\t\tif ($m == $l || $m == $h) $m ^= $h ^ $l;\n"
	"\t\t\tif ($l < $h) $3($l, $h);\n"
	"\t\t};\n"
	"\t\tqsort($0, $l, $2, $3)\n"
	"\t};\n"
	"\tif ($0 < $1 && $2($0, $1) > 0) $3($0, $1)\n"
	"};\n"
	"remove = function {\n"
	"\tn = [$0].n;\n"
	"\t$2 = (if (exists(@$2)) max($2, 0) else 1) + min($1, 0);\n"
	"\t$1 = max($1, 0);\n"
	"\t[$0].n = $1 + max(n - $1 - $2, 0);\n"
	"\tfor (i = $1; i < [$0].n; ++i) [$0][i] = [$0][i + $2];\n"
	"\tfor (; i < n; ++i) delete(@[$0][i]);\n"
	"\t( $0 )\n"
	"};\n"
	"rsort = function { args(@a); qsort(0, [a].n, >-compare([a][$0], [a][$1]), >swap(@[a][$0], @[a][$1])); ( a ) };\n"
	"sort = function { args(@a); qsort(0, [a].n, >compare([a][$0], [a][$1]), >swap(@[a][$0], @[a][$1])); ( a ) };\n"
	"\n"
	"// --- Queues ---\n"
	"\n"
	"resetQueue = function { [$0].m = [$0].n = 0 };\n"
	"queueSize = function { [$0].n - [$0].m };\n"
	"popBack = function { if ([$0].n - [$0].m <= 0) throw('Queue underrun'); v = [r = @[$0][--[$0].n]]; delete(r); ( v ) };\n"
	"pushBack = function { [$0][[$0].n++] = $1; ( $0 ) };\n"
	"popFront = function { if ([$0].n - [$0].m <= 0) throw('Queue underrun'); v = [r = @[$0][[$0].m++]]; delete(r); ( v ) };\n"
	"pushFront = function { [$0][--[$0].m] = $1; ( $0 ) };\n"
	"\n"
	"// --- Objects ---\n"
	"\n"
	"_finddot = function { if ((i = rfind($0, '.')) < 0) throw('Non-method call'); ( i ) };\n"
	"construct = function { destruct($0); invoke($0 # '.construct', $1, @$, 2); ( $0 ) };\n"
	"destruct = function { if (exists(@[$0].destruct)) [$0].destruct(); ( prune($0) ) };\n"
	"gc = function {\n"
	"\tmark => {\n"
	"\t\tinclude = if ($1) >true else >{ (($t = $0{span($0, '0123456789')}) !== void && $t !== '.') };\n"
	"\t\tcheck => if ((include($1) && $2{0} === ':')\n"
	"\t\t\t\t&& (($c = span($2{($i = find($2{1:}, ':') + 2):}, '0123456789')) >= 1)\n"
	"\t\t\t\t&& (($t = $2{$i += $c}) === void || $t === '.')\n"
	"\t\t\t\t&& (!exists(@[($r = $2{:$i})].$_gc_marked))) { [$r].$_gc_marked = true; mark($r, true) };\n"
	"\t\tif (exists($0)) check($0, void, [$0]);\n"
	"\t\tforeach($0, check);\n"
	"\t};\n"
	"\tfor (frame = @^; { mark(frame, false); frame !== '::'; }; frame = @[frame # '^']);\n"
	"\tcount = 0;\n"
	"\tfor (frame = @^; {\n"
	"\t\t\tlast = void;\n"
	"\t\t\tforeach(frame, >if (last !== ($r = @[frame][$1{:($i = span($1, '0123456789'))}])\n"
	"\t\t\t\t\t\t&& (($t = $1{$i}) === void || $t === '.')) {\n"
	"\t\t\t\t\tif (!exists($m = @[(last = $r)].$_gc_marked)) { destruct($r); ++count };\n"
	"\t\t\t\t\tdelete($m)\n"
	"\t\t\t\t});\n"
	"\t\t\tframe !== '::'\n"
	"\t\t}; frame = @[frame # '^']);\n"
	"\t( count )\n"
	"};\n"
	"invokeMethod = function { defaults(@$3, 0, @$4, [$2].n - $3); invoke($0 # '.' # $1, [$0][$1], $2, $3, $4) };\n"
	"method = function { ^$callee{_finddot(^$callee) + 1:} };\n"
	"new = function { defaults(@::_alloc, 0); invoke(@::[x = ::_alloc++] # '.construct', $0, @$, 1); ( @::[x] ) };\n"
	"newLocal = function { defaults(@^_alloc, 0); invoke(@^[x = ^_alloc++] # '.construct', $0, @$, 1); ( @^[x] ) };\n"
	"this = function { if ((t = ^$callee{:_finddot(^$callee)}){0} === ':') ( t ) else ( @^^[t] ) };\n"
	"\n"
	"( void )\n";
/**
	\file PikaCmd.cpp

	PikaCmd is a simple command-line tool for executing a PikaScript source code file.
	
	Use PikaCmd -h for more info.

	\version

	Version 0.97
	
	\page Copyright

	PikaScript is released under the "New Simplified BSD License". http://www.opensource.org/licenses/bsd-license.php
	
	Copyright (c) 2008-2025, NuEdge Development / Magnus Lidstroem
	All rights reserved.

	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
	following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following
	disclaimer. 
	
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
	disclaimer in the documentation and/or other materials provided with the distribution. 
	
	Neither the name of the NuEdge Development nor the names of its contributors may be used to endorse or promote
	products derived from this software without specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define PIKA_UNICODE 0
#define QUICKER_SCRIPT 1

#if (!defined(PLATFORM_STRING))
	#error Must define PLATFORM_STRING
#endif

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cstdint>
#if !defined(PikaScript_h)
#include "../../src/PikaScript.h"
#endif

#if (QUICKER_SCRIPT)
	#if !defined(QStrings_h)
	#include "../../src/QStrings.h"
	#endif
	#if !defined(PikaScriptImpl_h)
	#include "../../src/PikaScriptImpl.h"
	#endif
	#if !defined(QuickVars_h)
	#include "../../src/QuickVars.h"
	#endif

	struct QuickerScriptConfig;
	typedef Pika::Script<QuickerScriptConfig> Script;

	struct QuickerScriptConfig {
		typedef Pika::STLValue< QStrings::QString<char> > Value;
		typedef Pika::QuickVars<Script::STLVariables> Locals;
		typedef Locals Globals;
	};
#else
	typedef Pika::StdScript Script;
#endif

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

extern const char* BUILT_IN_DEBUG;
extern const char* BUILT_IN_HELP;
extern const char* BUILT_IN_INTERACTIVE;
extern const char* BUILT_IN_STDLIB;

const char* BUILT_IN_USAGE =
		"print('"
		"\n"
		"PikaCmd [ -h | <filename> [<arguments> ...] | ''{'' <code> ''}'' ]\n"
		"\n"
		"Command-line arguments are available in the global scope variables $1, $2 etc. ($0 is the script filename.) "
		"The process exit code will be that of the global variable ''exitCode'' (default is 0), or 255 if an exception "
		"occurs. ''PLATFORM'' will contain an operating system identifier (e.g. ''WINDOWS''). ''s'' = "
		"getenv(''var'') can be used to retrieve environment variables.\n"
		"\n"
		"Notice that you may need to enclose <code> in double quotes (\") to prevent the special interpretation of "
		"some characters (e.g. < and >). Double quotes inside <code> may need to be escaped, for example: \\\".\n"
		"\n"
		"The standard library ''load'' function is overloaded to look for files in the directory of PikaCmd when "
		"files cannot be found in the current directory. There are also \"built-in\" versions of some standard "
		".pika files that will be used in case external files do not exist. The built-in files are: ''stdlib.pika''"
		", ''debug.pika'', ''help.pika'', ''interactive.pika'' and ''default.pika''.\n"
		"\n"
		"If you run PikaCmd without arguments it will execute ''default.pika''. The built-in ''default.pika'' runs "
		"''interactive.pika''.\n"
		"');";
		
const char* BUILT_IN_DEFAULT = "run('interactive.pika', 'go')";
			
const char* BUILT_IN_DIRECT =
		"{ include('stdlib.pika'); s = ''; for (i = 0; i < $n; ++i) s #= ' ' # $[i]; "
		"print('---- (' # evaluate(s, @::) # ')') }";

std::pair<Script::String, Script::String> BUILT_IN_FILES[] = {
	std::pair<Script::String, Script::String>("debug.pika", Script::String(BUILT_IN_DEBUG))
	, std::pair<Script::String, Script::String>("default.pika", Script::String(BUILT_IN_DEFAULT))
	, std::pair<Script::String, Script::String>("help.pika", Script::String(BUILT_IN_HELP))
	, std::pair<Script::String, Script::String>("interactive.pika", Script::String(BUILT_IN_INTERACTIVE))
	, std::pair<Script::String, Script::String>("stdlib.pika", Script::String(BUILT_IN_STDLIB))
	, std::pair<Script::String, Script::String>("-h", Script::String(BUILT_IN_USAGE))
};

static Script::String loadFile(std::basic_ifstream<Script::Char>& instream, const std::string& filename) {
	Script::String chars;
	Script::Char* buffer = new Script::Char[1024 * 1024];
	try {
		while (!instream.eof()) {
			if (instream.bad())
				throw Script::Xception(Script::String("Error reading from file: ") += Pika::escape(filename));
			instream.read(buffer, 1024 * 1024);
			chars += Script::String(buffer, static_cast<Script::String::size_type>(instream.gcount()));
		}
		instream.close();
	}
	catch (...) {
		delete [] buffer;
		throw;
	}
	delete [] buffer;
	return chars;
}

std::string pikaCmdDir;

Script::String overloadedLoad(const Script::String& filename) {
	std::string name(Pika::toStdString(filename));	// Sorry, can't pass a wchar_t filename. MSVC supports it, but it is non-standard. So we convert to a std::string to be on the safe side.
	{
		std::basic_ifstream<Script::Char> instream(name.c_str());
		if (instream.good()) return loadFile(instream, filename);
	}
	if (!pikaCmdDir.empty()) {
		std::basic_ifstream<Script::Char> instream((pikaCmdDir + name).c_str());
		if (instream.good()) return loadFile(instream, filename);
	}
	{
		for (int i = 0; i < sizeof (BUILT_IN_FILES) / sizeof (*BUILT_IN_FILES); ++i)
			if (filename == BUILT_IN_FILES[i].first) return BUILT_IN_FILES[i].second;
	}
	throw Script::Xception(Script::String("Cannot open file for reading: ") += Pika::escape(filename));
}

Script::String getEnvironmentVar(const Script::String& var) {
	const char* s = getenv(var.c_str());
	return (s == 0 ? Script::Value() : Script::String(s));
}

#ifdef LIBFUZZ

struct CallDepthException { };
struct TimeOutException { };

class LibFuzzRoot : public Script::FullRoot {
	typedef Script::FullRoot Super;
	public:		LibFuzzRoot() : userLevel(Pika::NO_TRACE), counter(0), deadline(0), callDepth(0) {
					deadline = time(0) + 5;
					updateTracer();
				}
	public:		virtual void trace(Frame& frame, const Script::String& source, Script::SizeType offset, bool lvalue
						, const Script::Value& value, Pika::Precedence level, bool exit) {
					if (level <= userLevel) Super::trace(frame, source, offset, lvalue, value, level, exit);
					if (level <= Pika::TRACE_LOOP && ++counter >= 100) {
						counter = 0;
						int timeNow = time(0);
						if (deadline - timeNow < 0) {
							throw TimeOutException();
						}
					}
					if (level == Pika::TRACE_CALL) {
						if (!exit && callDepth >= 20) {
							throw CallDepthException();
						}
						callDepth += (exit ? -1 : 1);
						assert(callDepth >= 0);
					}
				}
	public:		virtual void setTracer(Pika::Precedence traceLevel, const Script::Value& tracerFunction) throw() {
					userLevel = traceLevel;
					userTracer = tracerFunction;
					updateTracer();
				}
	protected:	void updateTracer() {
					Super::setTracer(static_cast<Pika::Precedence>(std::max(static_cast<int>(userLevel)
							, static_cast<int>(Pika::TRACE_LOOP))), userTracer);
				}
	protected:	Pika::Precedence userLevel;
	protected:	Script::Value userTracer;
	protected:	int counter;
	protected:	int deadline;
	protected:	int callDepth;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    try {
		LibFuzzRoot root;
		root.registerNative("load", overloadedLoad);
		root.registerNative("getenv", getEnvironmentVar);
		root.assign("exitCode", Script::Value(0));
		root.assign("PLATFORM", Script::String(TO_STRING(PLATFORM_STRING)));
		root.assign("save", Script::String("{}"));
		root.assign("input", Script::String("{void}"));
		root.erase("system");
		root.evaluate(Script::String(reinterpret_cast<const char*>(Data), reinterpret_cast<const char*>(Data) + Size));
	}
	catch (const Script::Xception& x) {
	}
	catch (const TimeOutException&) {
		std::cerr << "timed out" << std::endl;
	}
	catch (const CallDepthException&) {
		std::cerr << "call depth limit exceeded" << std::endl;
	}
  	return 0;  // Non-zero return values are reserved for future use.
}

#endif

#ifndef LIBFUZZ
int main(int argc, const char* argv[]) {
	int exitCode = 255;
	std::srand(static_cast<unsigned int>(std::time(0)) ^ static_cast<unsigned int>(std::clock()));
	rand();
	if (argc < 2)
		std::cout << "PikaCmd version " << PIKA_SCRIPT_VERSION << ". (C) 2008-2025 NuEdge Development. "
				"All rights reserved." << std::endl << "Run PikaCmd -h for command-line argument syntax."
				<< std::endl << std::endl;
	try {
		pikaCmdDir = argv[0];
		size_t pos = pikaCmdDir.find_last_of("/\\:");
		if (pos == std::string::npos) pikaCmdDir.clear();
		else pikaCmdDir = pikaCmdDir.substr(0, pos + 1);
		Script::FullRoot root;
		root.registerNative("load", overloadedLoad);
		root.registerNative("getenv", getEnvironmentVar);
		root.assign("exitCode", Script::Value(0));
		root.assign("PLATFORM", Script::String(TO_STRING(PLATFORM_STRING)));
		const Script::String fn(argc < 2 ? "default.pika" : argv[1]);
		root.assign(Script::String("$0"), fn);	// Also assign args to global scope so we can access them anywhere.
		std::vector<Script::Value> args(1, fn);
		for (int i = 2; i < argc; ++i) {
			args.push_back(Script::String(argv[i]));
			root.assign(Script::String("$") += Pika::intToString<Script::String>(i - 1), argv[i]);	// Also assign args to global scope so we can access them anywhere.
		}
		root.call("run", (fn[0] == '{' ? Script::String(BUILT_IN_DIRECT) : Script::Value()), args.size(), &args[0]);
		exitCode = static_cast<int>(root.getOptional("exitCode"));
	} catch (const Script::Xception& x) {
		std::cerr << "!!!! " << x.what() << std::endl;
		exitCode = 255;
	}
#if (QUICKER_SCRIPT)
	QStrings::QString<char>::deinit();
#endif
	return exitCode;
}
#endif

