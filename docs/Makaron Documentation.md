Makaron
=======

_Makaron_ is a macro expansion language designed to simplify and streamline your code. Makaron allows you to define and use macros for text substitution in your code, making it more readable and maintainable. The language includes features for defining string macros with [`@define`](#define), parametric macros with [`@begin`](#begin) and `@end`, and conditional output macros with [`@if`](#if). These macros can be [invoked](#invocation) by prefixing the macro name with "@" in the text.

Table of Contents
-----------------

- [@define](#define)
- [@redefine](#redefine)
- [@begin](#begin)
- [@if](#if)
- [@include](#include)
- [invocation](#invocation)
- [indirect invocation](#indirect-invocation)
- [literal @](#literal)
- [raw value](#raw-value)

@define
-------

Define a macro that expands to a string.

Syntax:

	@define <macro> = <value>

- `<macro>`: name of the macro to define
- `<value>`: the macro will expand to this string

Example:

	@define color = red

Any instance of `@color` will thereafter expand to `red`.

Macro names can begin with `a` to `z`, `$` or `_,` but the rest may also contain `0` to `9`. You should not start the name with `@`. It is an error to define a macro with the same name twice. Use [`@redefine`](#redefine) if you want to reassign the value of a macro.

The `<value>` definition will include everything until the end of the line unless you use a [_raw value_](#raw-value) (`@<` `@>`). This can lead to unexpected results if you have a comment at the end of the line. For example, suppose we define `viewWidth` like this: 

	@define viewWidth = 5 // end of line comment

If we later use `viewWidth`, for example in the expression `@viewWidth+100` it will expand to:

	5 // end of line comment+100

Obviously, this is different from what you want. To avoid this situation, you can define the macro with a [_raw value_](#raw-value) value like this:

	@define viewWidth = @<5@> // end of line comment

@redefine
---------

Redefine a macro that you have already defined.

Syntax:

	@redefine <macro> = <value>

- `<macro>`: name of the macro to define
- `<value>`: the macro will expand to this string

Example:

	@redefine color = green

You cannot redefine a macro that has not already been defined. Furthermore, it is only possible to redefine _string macros_ defined with [`@define`](#define). It is not possible to redefine a _parametric macro_ that you have defined with [`@begin`](#begin).

@begin
------

Defines a _parametric macro_.

Syntax:

	@begin <macro>[ '(' <param>[,<param>,...] ')' ]
		<body>
	@end

- `<macro>`: name of the macro to define
- `<param>`: names of the parameters (optional)
- `<body>`: the macro definition

Example:

	@begin captionView(bounds, text)
		{
			type: "caption"
			bounds: @bounds
			text: @text
		}
	@end

Macro names can begin with a to z, $ or _, but the rest may also contain 0 to 9. You should not start the name with `@`. It is an error to define a macro with the same name twice. Unlike _string macros_ created with [`@define`](#define) you are *not* allowed to redefine _parametric macros_ with [`@redefine`](#redefine).

You are allowed to declare a _parametric macro_ without parameters, effectively creating a _string macro_. It is useful for creating macros that span several lines. The alternative is to use [`@define`](#define) with a [_raw value_](#raw-value). E.g. the following two examples are equivalent:

	@begin macro1
	expand
	to
	many
	lines
	@end

	@define macro2=@<expand
	to
	many
	lines
	@>

It is also possible to declare a macro with `@begin` and `@end` on a single line like this:

	@begin myMacro() contents of the macro here @end

Notice that `()` is optional when you declare a macro without parameters.

See [invocation](#invocation) for information on how to invoke _parametric macros_.

@if
---

Conditional output.

Syntax:

	@if '(' <value> (==/!=) <value> ')'
		<main-body>
	[@elif '(' <value> (==/!=) <value> ')'
		<elif-body>
	]
	[@elif '(' <value> (==/!=) <value> ')'
		<elif-body>
	]
	...
	[@else
		<else-body>
	]
	@endif

- `<value>`: the two values to compare
- `==/!=`: condition to test, _equal_ `==` or _not equal_ `!=`
- `<main-body>`: the body to include if the condition is true
- `<elif-body>`: output for additional tests, if the main condition is false
- `<else-body>`: output to include if all conditions are false

Example:

	@if (@index != 0)
		@if (@color == red)
			pen: #FF2244
		@elif (@color == green)
			pen: #22FF44
		@else
			pen: #888888
		@endif
	@endif

Comparison always compares strings, character by character. E.g., `5.0` and `5` are _not_ considered equal.

Don't forget to prefix _Makaron variables_ and _parameters_ with `@` inside the condition. Also, remember that literal strings in _Makaron_ are not enclosed in quotes, but you can use [_raw value_](#raw-value) syntax (`@<` `@>`).

@include
--------

Include and process another file.

Syntax:

	@include <name>

- `<name>`: the name of the file to include

Example:

	@include anotherFile.makaron

The file can be an external file or an "asset" provided by the hosting application. Notice that you specify `<name>` using a regular _Makaron value_. This means you do not enclose it in quotes, but you are allowed to use [_raw value_](#raw-value) syntax (`@<` `@>`).

invocation
----------

Call on a macro and include its output.

Syntax:

	@<macro>[ '(' <argument>[,<argument>,...] ')' ]

- `<argument>`: list of arguments provided for _parametric macros_

Example:

	@captionView({ 40,20,100,50 }, "The color is @color")

In the above example, `@captionView` is a _parametric macro_ and `@color` is a _string macro_.

An argument can be empty. If you want to pass a single empty argument, you use this syntax:

	@macroExpectingOneParam()

To invoke a macro that expects zero parameters, you need to exclude `()`, e.g.:

	@macroExpectingZeroParams

_Makaron_ expects certain characters to be properly "balanced" when used in arguments. The characters are:

- `(` and `)` 
- `[` and `]`
- `{` and `}`
- `"` and `"`
- `'` and `'`

E.g., the following line is invalid because it misses a terminating `}` character in the first argument:

	@captionView({ 40,20,100,50, "The color is @color")

If you need to include unbalanced characters, use a [_raw value_](#raw-value) string like this:

	@captionView({ 40,20,100,50 }, @<[@>)

indirect invocation
-------------------

Interpret a _Makaron value_ and use the output to call on a macro.

Syntax:

	@ '(' <value> ')'

- `<value>`: the value to interpret as the name of a macro

Example:

	@define rgb_red=#FF0000
	@define rgb_green=#00FF00
	@define rgb_blue=#0000FF
	@define color=red
	pen: @(rgb_@color)

`rgb_@color` will evaluate to `rgb_red` which in turn will be invoked to output `#FF0000`.

literal @
---------

Output one _at character_ (`@`).

Syntax:

	@@

Example:

	E-mail me at johndoe@@acme.com

Because `@` is used for executing _Makaron_ statements, you need to use `@@` for any `@` that you want to preserve in the output.

raw value
---------

Use an exact (possibly multi-line) string as a _Makaron value_.

Syntax:

	@< ... @>

Example:

	@define multiLineMacro=@<
		this
		macro
		expands
		to
		many
		lines
	@>

 Notice that macro expansion is still performed inside _raw values_. To include _at characters_, you need to use `@@` just like everywhere else.
