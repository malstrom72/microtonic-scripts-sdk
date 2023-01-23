# ImpD

## Table of Contents

-   [Syntax](#syntax)
    -   [Comments](#comments)
    -   [Variables](#variables)
    -   [Spaces](#spaces)
    -   [Line breaks](#line-breaks)
    -   [Expressions](#expressions)
    -   [Primitive Functions](#primitive-functions)
    -   [Primitive Instructions](#primitive-instructions)
        -   [call](#call)
        -   [debug](#debug)
        -   [for](#for)
        -   [format](#format)
        -   [if](#if)
        -   [include](#include)
        -   [local](#local)
        -   [meta](#meta)
        -   [repeat](#repeat)
        -   [return](#return)
        -   [stop](#stop)
        -   [trace](#trace)
    -   [Faux data structures](#faux-data-structures)

## Introduction

_ImpD_ is a text-based data format based on a simple imperative language designed to be reasonably compact yet easy to
read and write for humans and machines. The language includes variables, and basic control flow statements like `if`,
`while`, and function calls, which enable "intelligent" data that adapt to loading conditions and allow data to be
generated on the fly when a file is being loaded. _ImpD_ was created for the _IVG_ graphics format, but _ImpD_ is
generic and can be used for any file data.

## Syntax

An _ImpD_ file consists of a sequence of _statements_, usually one per line. Except for _variable assignments_ (which
have a different syntax), each statement begins with an _instruction_ followed by _arguments_ separated by space.
Arguments may be _ordinal_ or prefixed with a _label_ and a colon. Labeled arguments are often (but not always) optional
and may appear mixed with ordinal arguments in any order, but each unique label may occur only once. Instructions,
labels, and most argument values are case insensitive, but otherwise, _ImpD_ is generally strict about its syntax and
does not allow unrecognized instruction and superfluous arguments.

As an example, the first statement should be the `format` instruction which looks like this:

    format IVG-2 requires:ImpD-1

The loading program must recognize the format (`IVG-2` above), or the file will not load. The labeled `requires`
argument tells the parser that _ImpD_ version 1 is required to interpret the file (other requirements may follow
separated by comma).

### Comments

Comments are written in C++ style:

	// single line comment
	
	/*
	    multi
	    -line
	    comment
	*/

### Variables

Variable assignment looks like this:

	my_variable = some text here

Variable names must begin with lower- or uppercase `a` to `z` or `_` but may also contain `0` to `9`, `-` and `.`.
Except for leading and trailing spaces, all other spaces on the right-hand side of `=` are included.

`$` is used to expand variables:

	// print: some text here
	trace $my_variable

_ImpD_ also performs variable expansion in assignment statements, both on the left-hand and right-hand sides. For
example:

	// Equivalent to myVar = 1337:
	assignTo = myVar
	assignValue = 1337
	$assignTo = $assignValue

You can "delay" variable expansion of the right-hand side by enclosing it in brackets `[` and `]` or prevent expansion
entirely by enclosing it in double quotes `"`. You would typically use brackets to allow spaces in arguments or to
define the body for a function call, if-statement or loop.

For example, `TEXT` is part of the IVG format and takes one single argument. If we want to pass an arbitrary string as
its only argument, we need to enclose it in brackets like this:

	TEXT [$my_variable]

If we did not use brackets, `$my_variable` would expand to three different arguments before `TEXT` is invoked (`some`,
`text`, and `here`). By enclosing `$my_variable` in brackets, the full string will be passed on to `TEXT` as its first
argument, and the `TEXT` instruction will remove the brackets and perform the variable expansion during invocation.

Notice that the built-in `trace` primitive does not parse its arguments. You use it for debugging purposes, and the
entire trace line is output without removing any brackets or quotes.

### Spaces

Brackets are one of three different techniques that exist for including spaces in arguments:

	TEXT [
	    This argument can now include spaces, span several lines, and include delayed
	    $var_expansion . You should know that consecutive space characters, including
	    linefeeds, etc., are joined into one single space character. Also, /* comments
	    are removed */.
	]
	
	TEXT Backslash\ can\ escape\ spaces\
	and\ even\ linefeeds
	
	TEXT "Quotes will preserve spaces exactly
	    including tabs, linefeeds, etc., and no $var_expansion is performed."

Besides using backslash to escape space characters, all the conventional C-style escape codes, such as `\n`, `\r`, `\t`
etc. are available, as well as `\x` for two-digit hex values, `\u` for four-digit hex values, and `\U` for eight-digit
hex values. `\` followed by a decimal number also works.

When instructions expect lists, you can delimit the elements with commas or spaces. It is just a matter of taste:

	// Equivalent:
	RECT [10 10 100 100]
	RECT 10,10,100,100

### Line breaks

A single linefeed, where the following argument would otherwise appear, is enough to mark the end of a statement. If you
want to break up a single statement over several lines, start the following line with two periods. Like this:

	pen green
	    .. width:5
	    .. dash:10

Use semicolon if you wish to do the opposite and write more than one statement on a single line:

	scale 2.0; offset 5,5; rotate 7

Notice: semicolon terminates a variable assignment line too. E.g.:

	// This will trace B, then A,
	// $brokenLine will be `trace A` only:
	brokenLine=trace A; trace B
	$brokenLine
 	
 	// This on the other hand will trace A, then B,
 	// $wholeLine will be `[trace A; trace B]`:
 	wholeLine=[trace A; trace B]
 	$wholeLine

### Expressions

You use curly brackets `{` and `}` to evaluate C-style expressions and inject their results as strings. The following
operations are supported (in order of descending precedence):

| Example          | Result  | Description                                            |
| :--------------- | :------ | :----------------------------------------------------- |
| `{"a" "b"}`      | `ab`    | string concatenation                                   |
| `{$myVar}`       | `???`   | variable lookup                                        |
| `{$number$i}`    | `???`   | spliced variable name lookup (same as `{$(number$i)}`) |
| `{2 ** 3}`       | `8`     | exponentiation                                         |
| `{10%}`          | `0`.1   | percentage                                             |
| `{"b"{1}}`       | `b`     | indexing (first character is `{0}`)                    |
| `{"abcde"{1:2}}` | `bc`    | substring (`{offset:length}`)                          |
| `{!(5 < 3)}`     | `yes`   | logical not                                            |
| `{-(3 + 2)}`     | `-5`    | numerical negation                                     |
| `{+"056.0"}`     | `56`    | convert to number                                      |
| `{3 * 9}`        | `27`    | multiplication                                         |
| `{9 / 3}`        | `3`     | division (division by 0 is an error)                   |
| `{-11 % 3}`      | `-2`    | modulo (modulo with 0 is an error)                     |
| `{123 + 123}`    | `246`   | addition                                               |
| `{246 - 123}`    | `123`   | subtraction                                            |
| `{100 < 200}`    | `yes`   | less than (prefers numerical comparison)               |
| `{100 <= 100}`   | `yes`   | less than or equal (prefers numerical comparison)      |
| `{100 == 100.0}` | `yes`   | equals (prefers numerical comparison)                  |
| `{100 != 100.0}` | `no`    | not equals (prefers numerical comparison)              |
| `{b > a}`        | `yes`   | greater than (prefers numerical comparison)            |
| `{B >= c}`       | `no`    | greater than or equal (prefers numerical comparison)   |
| `{yes && no}`    | `no`    | logical and                                            |
| `{no || yes}`    | `yes`   | logical or                                             |
| `{yes ? 3 : 1}`  | `3`     | conditional (`boolean ? true : false`)                 |

Notice that you use the strings `yes` and `no` for boolean values (not `true` and `false` like in most C-style
languages).

Inside curly brackets, anything that is not a number, a known operator, a comment, etc., is taken as a literal string,
but you may also use double quotes. E.g.

	// These two will both concatenate to abc:
	trace {a b c}
	trace {"a" "b" "c"}

Parenthesis `(` and `)` are used to group sub-expressions as expected. Brackets `[` and `]` may be used to process a
string as it would be processed outside curly brackets. E.g.:

	// First two traces are identical:
	myVar = 99
	trace {[$myVar bottles of beer]}
	trace $myVar bottles of beer
	
	// But this one will concatenate all strings to 99bottlesofbeer:
	trace {$myVar bottles of beer}

### Primitive Functions

Inside curly brackets, the following functions are available:

| Function    | Description                                                      |
| :---------- | :--------------------------------------------------------------- |
| `abs(x)`    | returns the absolute value of a number                           |
| `acos(x)`   | returns the inverse cosine of a number in radians                |
| `asin(x)`   | returns the inverse sine of a number in radians                  |
| `atan(x)`   | returns the inverse tangent of a number in radians               |
| `ceil(x)`   | returns the smallest integer greater than or equal to a number   |
| `cos(x)`    | returns the cosine of a number in radians                        |
| `cosh(x)`   | returns the hyperbolic cosine of a number                        |
| `exp(x)`    | returns the value of `e` raised to a power                       |
| `floor(x)`  | returns the largest integer less than or equal to a number       |
| `log(x)`    | returns the natural logarithm of a number                        |
| `log10(x)`  | returns the base-10 logarithm of a number                        |
| `round(x)`  | returns the nearest integer to a number (.5 rounds upwards)      |
| `sin(x)`    | returns the sine of a number in radians                          |
| `sinh(x)`   | returns the hyperbolic sine of a number                          |
| `sqrt(x)`   | returns the square root of a number                              |
| `tan(x)`    | returns the tangent of a number in radians                       |
| `tanh(x)`   | returns the hyperbolic tangent of a number                       |
| `len(s)`    | returns the length of a string                                   |
| `def(var)`  | checks if a variable is defined (do not begin variable with `$`) |

There is also a single constant: `pi`.

### Primitive Instructions

#### call

The `call` instruction parses the unlabeled `args` into `$0` and up (labeled arguments go into `$<name>`) and then
executes the `body` code in a new _function frame_:

	call <body> [args ...]                                              

`body` is usually a variable that contains the function definition. For example:

	fibonacci = [
		local y = $1
		if {$y >= 2} [
			local x
			call $fibonacci x {$y - 1}
			call $fibonacci y {$y - 2}
			y = {$x + $y}
		]
		return $0 = $y
	]
 	
	local result
	call $fibonacci result 10
	trace the 10th fibonacci number is $result

Another choice is to put the `call` instruction in the function definition instead of function invocation. For example:

	sum = [ return $0 = {$1 + $2} ]
	call $sum x 123 456
 	
	// Can also be written like this:
 	
	sum = call [ return $0 = {$1 + $2} ]
	$sum x 123 456

Both solutions expand into the same `call [ return $0 = {$1 + $2} ] x 123 456` line for the interpreter. It is just a
matter of taste.

Notice that _ImpD_ uses *dynamic* scoping. All local variables of the caller are accessible to the callee. For example:

	// This will trace:
	//	valueB
	//
	funcA = [
		local aVariable = valueA
		call $funcB
		trace $aVariable
	]
	funcB = [
		aVariable = valueB
	]
	call $funcA

#### debug

The `_debug` instruction is for testing only:

	_debug [expand:(yes|no)=no] [args ...]                                

It outputs the specified `args` after optional "preprocessing" according to the `expand` parameter.

#### for

The `for` instruction iterates over a range of numbers or a list of elements and sets a `var` to each value, then
executes a code `body`:

	for [var] (from:<n> to:<n> | in:<list> [reverse:(yes|no)=no]) <body>

The `to:` parameter is inclusive. The `in:` parameter parses and iterates over a comma or space-separated list. You can
use the `reverse` parameter to iterate in reverse order.

Examples:

	// This example traces the numbers 1 to 5
	//
	for i from:1 to:5 [
		trace $i
	]
 	
	// This example traces:
	// 	orange
	// 	banana
	//	apple
	//
	for item in:[apple,banana,orange] reverse:yes [
	    trace $item
	]

#### format

The `format` instruction specifies the file format and its requirements:

	format <id> [uses:<id>[,<id>,...]] [requires:<id>[,<id>,...]]

The `id` is typically in the form of `<name>-<version>`. The `uses` parameter specifies meta tags `id`s. The `requires`
parameter specifies the baseline _ImpD_ (e.g. `ImpD-1`) and any custom extensions. If the parser does not recognize a
required `id`, it cannot load the file.

Including a `format` instruction in your _ImpD_ document is good practice, although not strictly necessary. It should be
the first instruction in your document, and no more than one `format` instruction should be present.

#### if

The `if` instruction executes a code `body` if a `condition` is `yes` (otherwise, it executes an optional `else` body):

	if [<condition>] [<body>] [else:<body>]

#### include

The `include` instruction is similar to [`call`](#call), but the `body` code gets loaded from a file or another type of
"asset" implemented by the host:

	include <file> [args ...]

#### local

The `local` instruction declares that a variable is local to the current call and optionally initializes it:

	local <var> [= ...]

If you assign using `<var> = ...` *without* `local`, the calling chain is searched upwards for the presence of `<var>`.
If it is found in any caller, its value is updated. If the variable is not found, a new global variable at the _root
frame_ is created. `local`, on the other hand, always creates variables in the current _variable frame_. You cannot
create a local variable with a name that already exist.

#### meta

The `meta` instruction is akin to a comment for the client software or a vendor-specific instruction:

	meta <id> ...

The `id` must be a unique identifier. The [`format`](#format) instruction should have listed it in the `uses` argument.
Unknown `id`s are ignored.

#### repeat

The `repeat` instruction repeats a code `body` either until the `condition` is not `yes` or the specified number of
`count` times:

	repeat <count> [while:<condition>] <body>

#### return

The `return` instruction assigns a string to a variable outside the current _variable frame_:

	return <var> [= ...]

As with ordinary assignments using `<var> = ...`, the calling chain is searched upwards for the presence of `<var>`, but
the search starts in the *caller's* _variable frame_. If it is found in any caller, its value is updated. If the
variable is not found, a new global variable at the _root frame_ is created.

Without the `[= ...]` argument, `return` will lookup `<var>` and use its existing value.

Notice that `return` does not actually stop executing the function body. It merely assigns values.

#### stop

The `stop` instruction terminates the parsing (early exit):

	stop

#### trace

The `trace` instruction outputs an arbitrary string without argument parsing:

	trace ...

### Faux data structures

_ImpD_ does not support _first class objects_ such as _arrays_ and _structs_, but you can "simulate" these data
structures using _spliced variable names_. For example, a random access array can be created by appending an index
suffix to a variable name. For example:

	// Create a sine table by creating 1000 variables with the name `sine.` and an index.
	for i from:0 to:999 [
		sine.$i = {sin(($i + 0.5) / 1000 * 2 * pi)}
	]
	// Lookup indexes from a list
	for e in:[0,614,287,511,999] [
		trace {$sine.$e} // same as {$(sine.$e)}
	]

Here is an example of using faux key/value structures:

	drawBall = call [
		fill {$($0).color}
		ellipse {$($0).x},{$($0).y},{$($0).size}
	]
 	
	ballA.color=yellow; ballA.size=5;  ballA.x=23;  ballA.y=50
	ballB.color=lime;   ballB.size=25; ballB.x=77;  ballB.y=33
	ballC.color=red;    ballC.size=13; ballC.x=133; ballC.y=20
 	
	$drawBall ballA
	$drawBall ballB
	$drawBall ballC

Notice that `.` before the key names is an arbitrary choice, you might as well use `_`, `$` or any other character that
is allowed in a variable name.
