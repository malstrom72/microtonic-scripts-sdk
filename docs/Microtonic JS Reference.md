# Microtonic JavaScript API Reference v1.0

## Table of Contents

-   [Engine](#engine)
-   [Constants](#constants)
    -   [Build Constants](#build-constants)
    -   [Directory Constants](#directory-constants)
    -   [Execution Constants](#execution-constants)
    -   [Microtonic Constants](#microtonic-constants)
    -   [Parameter Constants](#parameter-constants)
-   [Functions](#functions)
    -   [ask](#ask)
    -   [browse](#browse)
    -   [composeNumbstrict](#composenumbstrict)
    -   [createElement](#createelement)
    -   [dir](#dir)
    -   [display](#display)
    -   [editParam](#editparam)
    -   [fullPath](#fullpath)
    -   [getCushyVariable](#getcushyvariable)
    -   [getElement](#getelement)
    -   [getElementId](#getelementid)
    -   [getParam](#getparam)
    -   [isMarshaledFormat](#ismarshaledformat)
    -   [load](#load)
    -   [marshal](#marshal)
    -   [paramText](#paramtext)
    -   [paramValue](#paramvalue)
    -   [parseArray](#parsearray)
    -   [parseNumbstrict](#parsenumbstrict)
    -   [parseStruct](#parsestruct)
    -   [performCushyAction](#performcushyaction)
    -   [print](#print)
    -   [readClipboard](#readclipboard)
    -   [run](#run)
    -   [save](#save)
    -   [saveUndo](#saveundo)
    -   [select](#select)
    -   [selected](#selected)
    -   [setCushyVariable](#setcushyvariable)
    -   [setElement](#setelement)
    -   [setParam](#setparam)
    -   [translate](#translate)
    -   [triggerChannel](#triggerchannel)
    -   [unmarshal](#unmarshal)
    -   [writeClipboard](#writeclipboard)
-   [Objects](#objects)
    -   [drumPatch object](#drumpatch-object)
    -   [file object](#file-object)
    -   [midiConfig object](#midiconfig-object)
    -   [pattern object](#pattern-object)
    -   [preset object](#preset-object)
    -   [visuals object](#visuals-object)
-   [Utilities](#utilities)
    -   [cbrt](#cbrt)
    -   [clamp](#clamp)
    -   [closeCushy](#closecushy)
    -   [createClass](#createclass)
    -   [cube](#cube)
    -   [displayCushy](#displaycushy)
    -   [fract](#fract)
    -   [lerp](#lerp)
    -   [scale](#scale)
    -   [square](#square)
    -   [unescape](#unescape)
    -   [random](#random)
    -   [StringBuilder](#stringbuilder)
    -   [toggleCushy](#togglecushy)
-   [Cushy Interface](#cushy-interface)
    -   [Script Types](#script-types)
    -   [GUI Script Startup](#gui-script-startup)
    -   [GUI Variables](#gui-variables)
    -   [GUI Actions](#gui-actions)

## Engine

The JavaScript engine in Microtonic is a proprietary JavaScript engine written by Magnus Lidström. It was designed to be
small, fast, and easy to integrate into existing products. It is fully EcmaScript 3 compliant with partial EcmaScript 5
support.

Ecmascript 3 was chosen because it was the first widely adopted standardized version of JavaScript. It is a version many
programmers are familiar with, and it has everything you need from a scripting language. Naturally, more recent
versions of Ecmascript offer improvements but at the cost of larger standard libraries and more complex
compilers/interpreters.

With that said, a few features from Ecmascript 5 were “retrofitted” into this engine. Most notably: using `[ ]` for
reading individual characters from String objects and JSON support. Microtonic also sets up `Object.assign` and
`Date.now` from ES6 using "polyfills".

Every instance of Microtonic runs in its own JavaScript environment, but different scripts share the same environment.
The engine only works when the GUI window is open. The entire environment is destroyed and reset when the window is
closed (all global variables are lost).

While script code is running, the user interface will freeze. If a script runs for more than 20 seconds, it will be
suspended, and the user gets the option of aborting it or continuing. There is a limit of around 64MB memory per
Microtonic instance (after garbage collection). If a script uses more memory than this, it is terminated.

A console (`JSConsole`) is available for developing scripts. It runs inside Microtonic and allows you to enter
JavaScript code interactively, see traces, reload all resources and see script performance (as frames per second).

## Constants

### Build Constants

    PLATFORM = {
        CPU:        string          // 'arm64' | 'x86-64'
        OS:         string          // 'windows' | 'mac'
    }

    BUILD = {
        COMPILER:   string
        COPYRIGHT:  string          // e.g. "2003-2022"
        DATE:       string          // e.g. "Nov 25 2022"
        LIBS:       string
        NUMBER:     number          // build number, e.g. 1048
        TARGET:     string          // 'Debug' | 'Beta' | 'Release'
        TIME:       string          // e.g. "21:33:48"
        VERSION:    string          // e.g. "3.3.4"
    }

### Directory Constants

    DIRS = {
        DOCUMENTATION:  string
        DRUM_PATCHES:   string
        PRESETS:        string
        SCRIPTS:        string
        SKINS:          string
    }

Paths to various installation directories. These are always full absolute paths ending with a directory slash (`'/'` or
`'\'`). `DIRS.SCRIPTS` is the default directory used when loading and saving files without specifying an absolute path.

### Execution Constants

    isRepeating: boolean

If the global `isRepeating` is `true` the user is running the script by shift-clicking the script menu button. Usually
this means you should rerun the algorithms using the same parameters as last time without interrupting to ask the user
for input.

### Microtonic Constants

    CHANNEL_COUNT =         8
    PATTERN_COUNT =         12
    PROGRAM_COUNT =         16
    PATTERN_STEP_COUNT =    16
    GLOBAL_PARAM_COUNT =    15
    FREQ_VALUE_C4 =         0.4725600599127338      // The frequency value for middle C (523.25 Hz) on "OscFreq", "NFilFrq" and "EQFreq".
    OCTAVE_STEP =           0.1003433318879937      // The parameter value step for a full octave on "OscFreq", "NFilFrq" and "EQFreq".

### Parameter Constants

    PARAMS = [
        {
            NAME:   string              // e.g. 'NEnvAtk.8'
            STEPS:  number|null
        } * 215
    ]

    DRUM_PATCH_PARAMS = [
        {
            NAME:   string              // e.g. 'NEnvAtk'
            STEPS:  number|null
        } * 25
    ]

Drum patch and global parameters are usually accessed via their symbolic names (although some functions also take an
index). The `PARAMS` and `DRUM_PATCH_PARAMS` arrays can be used to look up symbolic names for specific parameter
indexes. Notice that the `PARAMS` array contains parameters for all eight drum patches as well as global parameters.

If `STEPS` of a parameter element is not `null`, the parameter is "stepped". Stepped parameters range from 0 to 1, just
like all other parameters. To normalize a stepped parameter from integer to floating point, use the following formula:

    paramValue = steppedInteger / (STEPS - 1)

To convert a normalized floating point value back to an unnormalized integer, use this formula:

    steppedInteger = Math.min(Math.floor(paramValue * STEPS), STEPS - 1)

## Functions

### ask

    function ask(question: string, [defaultAnswer: string = '']) : string|null

Asks a question and lets the user respond with a string through a simple text edit dialog. If the user clicks the cancel
button, `null` is returned.

See also: [display](#display)

### browse

    function browse([type: string], [fileTypes: string|array|null = null], [initialDir: string|null = null], [defaultFilename: string = '']) : string|null

Opens a file browser window and returns the path browsed to. `type` can be `'open'` (default) or `'save'` (opens a
"save as" file dialog). `fileTypes` can be a single string or an array of strings of file extensions that should be
selectable in the file browser (only supported by `'open'` browsers at the moment). If omitted (or `null`), the user
can select all file types. `initialDir` can be a path to the initial directory or `null` to use an appropriate default
directory as determined by `fileTypes` (in case it is an array, the first element in the array). `defaultFilename` is
only used for `'save'` browsers (you should not include the file extension). If the user presses cancel, `null` is
returned, otherwise a full path string is returned.

See also: [dir](#dir), [fullPath](#fullpath), [load](#load), [save](#save)

### composeNumbstrict

    function composeNumbstrict(data: object|array|boolean|number|string, [multiLine: boolean = false], [brackets: boolean = true]) : string

Converts a JavaScript object, array, boolean, number, or string into a "Numbstrict" string. If `data` is an object and
`multiLine` is `true`, the resulting string will have newlines and indentation for readability. If `data` is an object
and `brackets` is `true`, the resulting string will have curly braces { } surrounding it. Numbstrict is the modern data
format used in Sonic Charge products. It is similar to JSON but designed to be more user-friendly, with support for
comments, hex values, and more.

See also: [parseNumbstrict](#parsenumbstrict)

### createElement

    function createElement(type: string) : object

Creates a new initialized preset, drum-patch, pattern or midi-config structure. `type` should be `'preset'`,
`'drumPatch'`, `'pattern'` or `'midiConfig'`. See [Objects](#objects) for descriptions of the different element data.

See also: [getElement](#getelement), [setElement](#setelement), [Objects](#objects)

### dir

    function dir(path: string, [extensionFilter: string|array|null = null]): array

Returns a list of objects representing the files in the given directory. Each object has the following properties:
`name`, `size`, `created`, `modified`, `isDirectory` and `isReadOnly`. `extensionFilter` can be a single string or an
array of strings and if present, only files with the given extension(s) are returned. Forward slash (`'/'`) in paths
work on both Windows and OS X, backward slash (`'\'`) only works on Windows. If the path is not an "absolute full path"
it is considered relative to the script directory. 

See also: [File Object](#file-object), [browse](#browse), [fullPath](#fullpath), [load](#load), [save](#save)

### display

    function display(message: string, [type: string = 'plain'], [buttons: string = 'ok'], [defaultButton: string = 'ok']) : string

Displays a message box (a so called "alert") and returns the name of the button pressed. `type` can be any of
`'plain'`, `'question'`, `'warning'`, `'info'` and `'error'`. `buttons` can be `'ok'`, `'ok cancel'`, `'retry cancel'`,
`'yes no'`, `'yes no cancel'` or `'abort retry ignore'`. Naturally, `defaultButton` can be any of the buttons just
listed.

See also: [ask](#ask), [print](#print)

### editParam

    function editParam(param: string|number, beginEdit: boolean)

Begins or ends user manipulation of a parameter. `param` is a string or number (e.g. `'ModRate.1'` and 20 are
equivalent). Will notify the host that the user is controlling the parameter, e.g. to begin or end automation
recording.

See also: [setParam](#setparam), [getParam](#getparam)

### fullPath

    function fullPath(base: string|null, path: string) : string

Canonicalizes the relative `path` against the `base` path (i.e. returns a distinct full path specification). E.g.,
`fullPath('./', 'file')` returns the absolute path to `'file'` in the current working dir. `fullPath('file', '../')`
returns the absolute path to the parent directory for `'file'`. Pass `null` for `base` to make a path relative to the
script directory.

See also: [browse](#browse), [dir](#dir), [load](#load), [save](#save)

### getCushyVariable

    function getCushyVariable(identifier: string) : string

Retrieves the value of a "Cushy GUI variable". Cushy variables are string variables created and used by the GUI layer
in Microtonic. See [Cushy Interface](#cushy-interface) for more information.

See also: [Cushy Interface](#cushy-interface), [performCushyAction](#performcushyaction),
[setCushyVariable](#setcushyvariable)

### getElement

    function getElement(type: string) : object

Returns the current preset, drum-patch (of current channel), pattern (currently selected), midi-config or visuals.
`type` should be `'preset'`, `'drumPatch'`, `'pattern'`, `'midiConfig'` or `'visuals'`. Returns an object containing
the data for the requested element. See [Objects](#objects) for descriptions of the different element data.

See also: [createElement](#createelement), [setElement](#setelement), [getElementId](#getelementid), [Objects](#objects)

### getElementId

    function getElementId(type: string) : number

Returns the unique identifier integer of the preset, drum-patch (of current channel), pattern (currently selected) or
midi-config. `type` should be `'preset'`, `'drumPatch'`, `'pattern'` or `'midiConfig'`. The identifier of an element is
updated whenever any property of the element is changed. The id is always a unique number for this "session" (e.g. when
the host is restarted the id counter will reset.). No two different element can ever share the same id. Calling this
function is more efficient than calling `getElement` if you only need to read the `id` field.

See also: [createElement](#createelement), [getElement](#getelement), [setElement](#setelement), [Objects](#objects)

### getParam

    function getParam(param: string|number) : number

Retrieves the current setting of a single parameter. `param` is a string or number (e.g. `'ModRate.1'` and 20 are
equivalent). The returned value is a floating point number between 0 and 1.

See also: [getElement](#getelement), [paramText](#paramtext), [setParam](#setparam)

### isMarshaledFormat

    function isMarshaledFormat(type: string, string: string) : boolean

Returns `true` if `string` is valid Microtonic file / clipboard format. `type` should be `'preset'`, `'drumPatch'`,
`'pattern'` or `'midiConfig'`

See also: [marshal](#marshal), [unmarshal](#unmarshal)

### load

    function load(path: string) : string

Loads a text file (UTF-8 format). If `path` is not an "absolute full path" it is considered relative to the script
directory. Alternatively, if the file does not exist and a built-in resource exists with the name, the resource is
loaded instead. Forward slash (`'/'`) in paths work on both Windows and OS X, backward slash (`'\'`) only works on
Windows.

See also: [browse](#browse), [dir](#dir), [fullPath](#fullPath), [run](#run), [save](#save)

### marshal

    function marshal(type: string, source: object) : string

Converts an object (in `source`) to Microtonic file / clipboard format (`type` should be `'preset'`, `'drumPatch'`,
`'pattern'` or `'midiConfig'`).

See also: [createElement](#createelement), [getElement](#getelement), [isMarshaledFormat](#ismarshaledformat),
[unmarshal](#unmarshal)

### paramText

    function paramText(param: string|number, value: number, [highPrecision: boolean = false], [context: object = null]) : string

Converts a parameter value to a human readable string. `value` must be a floating point number between 0 and 1. If
`highPrecision` is `true`, the returned text will have enough precision to preserve all the bits of the original
floating-point value. `context` is optional and if present should reference a "drumPatch" object. Some parameter's
textual representation depend on other parameter settings (such as "ModRate" depending on "ModMode"), and `context` can
be used to declare these settings. If `context` is omitted (or `null`), the currently active drum patch is used.

See also: [getParam](#getparam), [paramValue](#paramvalue)

### paramValue

    function paramValue(param: string|number, text: string, [context: object = null]) : number

Converts a human readable string to a parameter value. `paramValue` will return a default standard value (for the
parameter) if it is not convertable. `context` is optional and if present should reference a "drumPatch" object. Some
parameter's textual representation depend on other parameter settings (such as "ModRate" depending on "ModMode"), and
`context` can be used to declare these settings. If `context` is omitted (or `null`), the currently active drum patch
is used.

See also: [paramText](#paramtext), [setParam](#setparam)

### parseArray

    function parseArray(string: string) : array

Parses `string` that represents an array in "NumbStruck" format (the legacy data format used in Sonic Charge products).
Returns the result as an array. Notice that "NumbStruck" parsing is very forgiving. Any string is considered valid
"NumbStruck" format, although only correctly formatted strings will actually generate any data.

See also: [parseStruct](#parsestruct), [parseNumbstrict](#parsenumbstrict)

### parseNumbstrict

    function parseNumbstrict(numbstrict: string) : object|array|boolean|number|string

Parses a "Numbstrict" string and returns the corresponding JavaScript object, array, boolean, number, or string.
Numbstrict is the modern data format used in Sonic Charge products. It is similar to JSON but designed to be more user
friendly with support for comments, hex values and more.

See also: [composeNumbstrict](#composenumbstrict)

### parseStruct

    function parseStruct(data: string) : object

Parses `data` that represents a struct / associative array in "NumbStruck" format (the legacy data format used in Sonic
Charge products). Returns the result as an object. Notice that "NumbStruck" parsing is very forgiving. Any string is
considered valid "NumbStruck" format, although only correctly formatted strings will actually generate any data.

See also: [parseArray](#parsearray)

### performCushyAction

    function performCushyAction(action: string, [params: string = ''])

Performs a "Cushy GUI action" with optional parameter string. "Cushy GUI actions" are part of the GUI layer in
Microtonic. See [Cushy Interface](#cushy-interface) for more information.

See also: [Cushy Interface](#cushy-interface), [getCushyVariable](#getcushyvariable), [setCushyVariable](#setcushyvariable)

### print

    function print(string: string)

Prints to standard output (on Mac) or DebugView (on Windows). If you install `JSConsole.mtscript` it will replace this
function to print to a built-in console inside Microtonic.

See also: [display](#display)

### readClipboard

    function readClipboard() : string

Reads a string from the clipboard. On Windows, CR LF (`'\r\n'`) are automatically translated to LF (`'\n'`).

See also: [writeClipboard](#writeClipboard)

### run

    function run(path: string)

Loads a JavaScript source file and executes it. If `path` is not an "absolute full path" it is considered relative to
the script directory. Alternatively, if the file does not exist and a built-in resource exists with the name, the
resource is loaded instead. Forward slash (`'/'`) in paths work on both Windows and OS X, backward slash (`'\'`) only
works on Windows.

See also: [fullPath](#fullpath), [load](#load)

### save

    function save(path: string, contents: string)

Saves a file. If `path` is not an "absolute full path" it is considered relative to the script directory. Forward slash
(`'/'`) in paths work on both Windows and OS X, backward slash (`'\'`) only works on Windows.

See also: [browse](#browse), [dir](#dir), [fullPath](#fullpath), [load](#load)

### saveUndo

    function saveUndo(label: string, [collapse: boolean = false])

Stores a snapshot of the entire Microtonic state in the undo history. `label` should be a user-readable string
(e.g. "Undo Mute Channel 1"). (A good idea is to call `translate` on the static part of the label text to support
localization in the future.). If `collapse` is `true` and `label` is identical to the last element in the undo history,
Microtonic will not create a new snapshot (use for repeated actions to avoid "spamming" the undo history). Notice that
calling this function is rarely necessary for GUI-less scripts since they automatically save an undo snapshot when they
detect a change of the Microtonic state. 

See also: [Cushy Interface](#cushy-interface), [translate](#translate)

### select

    function select(type: string, index: number)

Selects the current program, drum channel or pattern. `type` should be `'program'`, `'channel'` or `'pattern'`. The
range for programs are 0 to 15 (defined by `PROGRAM_COUNT`). The range for channels are 0 to 7 (defined by
`CHANNEL_COUNT`). The range for patterns are 0 to 11 (defined by `PATTERN_COUNT`).

See also: [selected](#selected)

### selected

    function selected(type: string) : number

Returns the currently selected program, drum channel or pattern. `type` should be `'program'`, `'channel'` or
`'pattern'`. The range for programs are 0 to 15 (defined by `PROGRAM_COUNT`). The range for channels are 0 to 7
(defined by `CHANNEL_COUNT`). The range for patterns are 0 to 11 (defined by `PATTERN_COUNT`).

See also: [select](#select)

### setCushyVariable

    function setCushyVariable(identifier: string, value: string)

Updates or creates a new "Cushy GUI variable". Cushy variables are string variables created and used by the GUI layer
in Microtonic. See [Cushy Interface](#cushy-interface) for more information.

See also: [Cushy Interface](#cushy-interface), [getCushyVariable](#getcushyvariable),
[performCushyAction](#performcushyaction)

### setElement

    function setElement(type: string, source: object)

Updates current preset, drum-patch (for current channel), pattern (currently selected) or midi-config from `source`.
`type` should be `'preset'`, `'drumPatch'`, `'pattern'` or `'midiConfig'`. See [Objects](#objects) for descriptions of
the different element data.

See also: [createElement](#createelement), [getElement](#getelement), [getElementId](#getelementid), [Objects](#objects)

### setParam

    function setParam(param: string|number, value: number)

Updates a single parameter immediately. `param` is a string or number (e.g. `'ModRate.1'` and 20 is equivalent).
`value` must be a floating point number between 0 and 1.

See also: [setElement](#setelement), [editParam](#editparam), [getParam](#getparam), [paramValue](#paramvalue)

### translate

    function translate(text: string) : string

Translates a string through the currently active translation table. Used mainly for GUI localization. See
[Cushy Interface](#cushy-interface) for more information.

See also: [Cushy Interface](#cushy-interface)

### triggerChannel

    function triggerChannel(channel: number, velocity: number)

Triggers a Microtonic drum channel (0 to 7). `velocity` should be an integer between 0 and 127. 127 is equivalent to
accented steps in the Microtonic sequencer, and 64 is equivalent to unaccented steps. Calling this function will never
change the selected channel, regardless if "Select channel with MIDI" is active.

### unmarshal

    function unmarshal(type: string, string: string) : object

Converts Microtonic file / clipboard format to an object (type should be `'preset'`, `'drumPatch'`, `'pattern'` or
`'midiConfig'`). Returns the resulting object.

See also: [isMarshaledFormat](#ismarshaledformat), [marshal](#marshal), [setElement](#setelement)

### writeClipboard

    function writeClipboard(string: string)

Puts a string to the clipboard. On Windows, LF (`'\n'`) are automatically translated to CR LF (`'\r\n'`).

See also: [readClipboard](#readclipboard)

## Objects

### drumPatch object

    {
        id:         number                              // Unique identifier integer (incremented automatically on write).
        modified:   boolean
        name:       string
        path:       string|null                         // Notice that this path may be relative to factory drum patch directory (in which case on Windows, forward slashes `/` are used instead of backward slashes `\`).
        Choke:      number                              // 0 | 1
        DistAmt:    number                              // 0 .. 1
        EQFreq:     number                              // 0 .. 1
        EQGain:     number                              // 0 .. 1
        Level:      number                              // 0 .. 1
        Mix:        number                              // 0 .. 1
        ModAmt:     number                              // 0 .. 1
        ModMode:    number                              // 0 | 0.5 | 1: decay | sine | noise
        ModRate:    number                              // 0 .. 1
        ModVel:     number                              // 0 .. 1
        NEnvAtk:    number                              // 0 .. 1
        NEnvDcy:    number                              // 0 .. 1
        NEnvMod:    number                              // 0 | 0.5 | 1: exponential | linear | modulated
        NFilFrq:    number                              // 0 .. 1
        NFilMod:    number                              // 0 | 0.5 | 1: lp | bp | hp
        NFilQ:      number                              // 0 .. 1
        NStereo:    number                              // 0 | 1
        NVel:       number                              // 0 .. 1
        OscAtk:     number                              // 0 .. 1
        OscDcy:     number                              // 0 .. 1
        OscFreq:    number                              // 0 .. 1
        OscVel:     number                              // 0 .. 1
        OscWave:    number                              // 0 | 0.5 | 1: sine | triangle | sawtooth
        Output:     number                              // 0 | 1
        Pan:        number                              // 0 .. 1
    }

### file object

    {
        name:           strings                         // including any extension
        size:           number                          // in bytes
        created:        Date
        modified:       Date
        isDirectory:    boolean
        isReadOnly:     boolean
    }

### midiConfig object

    {
        id:                         number              // Unique identifier integer (incremented automatically on write).
        improvedCCScaling:          boolean
        midiCCAutomates:            boolean
        midiCCOnSelectedChannel:    boolean
        midiNotesSelectChannel:     boolean
        midiReceiveChannel:         number              // null | 1 .. 16
        midiTransmitChannel:        number              // null | 1 .. 16
        patternNoteMode:            string              // 'SwitchNext' | 'SwitchNow' | 'Retrigger' | 'RetriggerGated'
        patternsSendMidi:           boolean
        pitchedMode:                boolean
        pitchWheelRange:            number              // 1 | 2 | 3 | 4 | 7 | 12 | 19 | 24
        sendMidiCC:                 boolean
        supportMidiNoteOff:         boolean
        supportMidiProgramChange:   boolean
        globalCC: {
            FillRate:               number              // null | 0 .. 127
            MastVol:                number              // null | 0 .. 127
            Morph:                  number              // null | 0 .. 127
            Pattern:                number              // null | 0 .. 127
            PlayStop:               number              // null | 0 .. 127
            StepRate:               number              // null | 0 .. 127
            Swing:                  number              // null | 0 .. 127
        },
        channelCC: [
            {
                Choke:              number              // null | 0 .. 127
                DistAmt:            number              // null | 0 .. 127
                EQFreq:             number              // null | 0 .. 127
                EQGain:             number              // null | 0 .. 127
                Level:              number              // null | 0 .. 127
                Mix:                number              // null | 0 .. 127
                ModAmt:             number              // null | 0 .. 127
                ModMode:            number              // null | 0 .. 127
                ModRate:            number              // null | 0 .. 127
                ModVel:             number              // null | 0 .. 127
                NEnvAtk:            number              // null | 0 .. 127
                NEnvDcy:            number              // null | 0 .. 127
                NEnvMod:            number              // null | 0 .. 127
                NFilFrq:            number              // null | 0 .. 127
                NFilMod:            number              // null | 0 .. 127
                NFilQ:              number              // null | 0 .. 127
                NStereo:            number              // null | 0 .. 127
                NVel:               number              // null | 0 .. 127
                OscAtk:             number              // null | 0 .. 127
                OscDcy:             number              // null | 0 .. 127
                OscFreq:            number              // null | 0 .. 127
                OscVel:             number              // null | 0 .. 127
                OscWave:            number              // null | 0 .. 127
                Output:             number              // null | 0 .. 127
                Pan:                number              // null | 0 .. 127
            } * 8
        ],
        muteCC: array,                                  // [ (null | 0 .. 127) * 8 ]
        patternCC: [
            {
                triggers:           number              // null | 0 .. 127,
                accents:            number              // null | 0 .. 127,
                fills:              number              // null | 0 .. 127
            } * 8
        ],
        notes: {
            start:                  number              // null | 0 .. 127,
            stop:                   number              // null | 0 .. 127,
            mutes:                  array               // [ (null | 0 .. 127) * 8 ]
            patterns:               array               // [ (null | 0 .. 127) * 12 ]
            programs:               array               // [ (null | 0 .. 127) * 16 ]
            triggers:               array               // [ (null | 0 .. 127) * 8 ]
        }
    }

### pattern object

    {
        id:                 number                      // Unique identifier integer (incremented automatically on write).
        steps:              number                      // 1 .. 16
        channels: [
            {
                triggers:   array                       // [ boolean * 16 ]
                accents:    array                       // [ boolean * 16 ]
                fills:      array                       // [ boolean * 16 ]
            } * 8
        ]
    }

### preset object

    {
        id:             number                          // Unique identifier integer (incremented automatically on write).
        modified:       boolean
        name:           string
        path:           string|null                     // Notice that this path will always be a full path with native directory slashes (`\` on Windows, `/` on Mac).
        tempo:          number|null                     // 1 .. 999
        Pattern:        number                          // 0 .. 1 in 1 / 12 steps
        PlayStop:       number                          // 0 | 1/3 | 2/3 | 1: forced stop | stop | play | forced play
        StepRate:       number                          // 0 | 0.25 | 0.5 | 0.75 | 1: 1/8 | 1/8T | 1/16 | 1/16T | 1/32
        Swing:          number                          // 0 .. 1
        FillRate:       number                          // 0 .. 1
        MastVol:        number                          // 0 .. 1
        mutes:          array                           // [ (0 | 1) * 8 ]. Normally only 0 and 1 but host may set them to other values.
        drumPatches:    array                           // [ drumPatch * 8 ]
        patterns:       array                           // [ pattern * 12 ]
    }

### visuals object

    {
        isPlaying:         boolean
        currentPattern:    number                       // 0 .. 11
        currentStep:       number                       // 0 .. 15
        lastTriggers:      parseArray                   // [ null|number * 8 ]. Timestamps when channel was last triggered (epoch milliseconds, as with `Date.now()`). Will initially be null.
    }

## Utilities

### cbrt

    function cbrt(x: number) : number

Returns the cube root of `x` (i.e. `x ^ 1/3`). Negative root is returned for negative `x`.

### clamp

    function clamp(x: number, min: number, max: number) : number

Returns the value `x`, clamped between `min` and `max` (i.e. `Math.min(Math.max(x, min), max)`).

### closeCushy

    function closeCushy()

Close the currently open "Cushy window". See [Cushy Interface](#cushy-interface)

### createClass

    function createClass(definition: object, [inherit: object])

Utility for class creation with inheritance. Use like this:

    MyClass = createClass({
        constructor: function(x) { this.field = x },
        getField: function() { return this.field }
    })

`super` can be used to access inherited prototype:

    MySubClass = createClass({
        // no constructor defined = will use MyClass constructor
        getField: function() { return this.super.getField.call(this) * 100 }
    }, MyClass)

### cube

    function cube(x: number) : number

Returns `x` * `x` * `x`.

### displayCushy

    function displayCushy(name: string)

Opens a "Cushy window". See [Cushy Interface](#cushy-interface)

### fract

    function fract(x: number) : number

Returns the fractional part of `x` (i.e. `x - Math.floor(x)`).

### lerp

    function lerp(y0: number, y1: number, x: number) : number

Linearly interpolates `x` between `y0` (for `x == 0`) and `y1` (for `x == 1`).

### scale

    function scale(x: number, x0: number, x1: number, y0: number, y1: number) : number

Linearly interpolates `x` from `x0` to `x1` into `y0` (for `x == x0`) to `y1` (for `x == x1`).

### square

    function square(x: number) : number

Returns `x` * `x`.

### unescape

    function unescape(s: string) : string

If `s` is enclosed in single-quotes (`'`) or double-quotes (`"`) it will be parsed as a Numbstrict-style string
constant (virtually the same as a C string constant). If `s` is not enclosed in quotes it will be returned as is.

### random

    random = {
        uniform: function
        integer: function
        normal: function
    }

    function random.uniform() : number

Same as `Math.random`.

    function random.integer([ceiling: number]) : number

Returns a random integer between 0 and `ceiling - 1`. If `ceiling` is omitted, a random integer in the full 32-bit range
is returned.

    function random.normal([mu: number = 0], [sigma: number = 1]) : number

Returns a random number with normal distribution around mean `mu` with standard deviation `sigma`.

### StringBuilder

    constructor StringBuilder(init: string): object

`StringBuilder` is a class that can be used to improve performance when building very long strings. Use like this:

    var builder = new StringBuilder("init string");
    while (/* loop making big string */) {
        builder.append("lots of string data here");
    }
    var finalString = builder.build();

### toggleCushy

    function toggleCushy(cushyName: string)

If the "Cushy window" with `cushyName` is already open, close it. Otherwise, open it. See
[Cushy Interface](#cushy-interface)

## Cushy Interface

### Script Types

There are two types of Microtonic scripts, classic GUI-less scripts and modern scripts with GUIs. A GUI-less script is a
single JavaScript (or PikaScript) file in the `Microtonic Scripts` directory. It provides no way to interact with the
user except for displaying alerts and getting user input through a simple text input dialog. To provide a more
sophisticated user interface, you need to create a GUI script with JavaScript and "Cushy."

Cushy is the GUI / layout engine used in all Sonic Charge products. A full description of the Cushy system is beyond the
scope of this document, but `.cushy` files define the layout of views and configure how the user can interact with the
plug-in through "GUI variables" and "GUI actions."

Note: Microtonic 3.x is still not fully ported to Cushy. Most of the standard knobs and buttons are legacy "hard-wired"
GUI elements, but on top of this, there is a layer of Cushy.

### GUI Script Startup

A GUI script is installed as a sub-directory with the extension `.mtscript` under the `Microtonic Scripts` directory.
Inside this directory, you need one `.js` file with the same filename as the directory and script. This `.js` is used
to start up the script and open a "Cushy window" for the script. It could look something like this:

    if (BUILD.NUMBER < 1048) {
        display(translate("This script requires") + " Microtonic v3.3.4", "error");
    } else {
        toggleCushy('MyScript.mtscript/MyScript_main');
    }

This script assumes a `MyScript_main.cushy` file is located under `MyScript.mtscript/`. It should contain the layout
definition of the user interface. Most often, this `.cushy` will include common macros from `scriptSupport.makaron`,
use the `@window` macro and define the window contents. Notice that you must include the `MyScript.mtscript/` path in
the call to `toggleCushy`. It is necessary because the root directory for all scripts is the common `Microtonic
Scripts` folder.

This is how the folder structure might look:

    Microtonic Scripts/
    ├─ MyScript.mtscript/
    │  ├─ MyScript.js
    │  ├─ MyScript_main.cushy
    │  ├─ MyScript_main.js

When the GUI engine opens a Cushy file, it will first look for a `.js` file with the same name as the `.cushy` file and
run it if it exists. In the above example, this would be `MyScript.mtscript/MyScript_main.js`. This JavaScript file
sets up all the functions necessary to run the script. Notice that Cushy runs this Javascript every time the user opens
the script and when a "rebuild" of the GUI is performed, e.g., when choosing a different zoom scale. You should
therefore avoid resetting global variables if they already exist.

Since all scripts share the same JavaScript global variable space, putting functions and variables under a single
JavaScript object with the same name as the script is good practice. Doing so prevents littering the variable space and
you minimize the risk of colliding with other scripts. If you run your script with the `JSConsole` open, it will alert
every time a global variable is created. This helps you catch mistakes such as missing the `var` keyword when declaring
a variable.

### GUI Variables

The GUI views in Cushy will interface with your script (and the Microtonic backend) through "GUI variables" and
"GUI actions." You might, for example, tie a knob view to a variable called `MyScript.myKnob`. The view will then
attempt to synchronize its state with the scripting engine in both directions. It will update `MyScript.myKnob` when
the user turns the knob, but it will also continuously read `MyScript.myKnob` to update its graphical knob position.

All GUI variable values are of string type, but they may contain numbers in decimal text format (or, in rare cases, even
arrays, and structures in Numbstrict text format). There are four different ways that Cushy will get and set GUI
variables:

- If the GUI variable is an existing JS string variable, it will get and set it directly as a string.
- If the GUI variable is an existing JS function, it will be called continuously to get the current value (in this case,
  Cushy can never update the GUI variable).
- If the GUI variable is an existing JS object, it can contain a "setter" and a "getter" function aptly named `set` and
  `get`. The `set` function will receive a single string argument. The `get` function is expected to return a string
  value.
- Finally, if the variable name is not referring to an existing JS variable, Cushy will create a Cushy-only variable
  (not visible from Javascript). Never do this in a script, please!

Here is an example of using a "setter" and "getter" function for a GUI variable that performs a transformation of the
knob value to logarithmic scale:

    //
    // First time run, myScript doesn't exist, set up initial state.
    //
    if (!myScript) {
        myScript = {
            myValue: 0.5
        }
    }
    //
    // By assigning this every time the JS is run we can easily update the code by clicking "reload" in JSConsole.
    //
    myScript.myKnob = {
        get: function() { return '' + ((Math.exp(myScript.myValue) - 1) / 10); },
        set: function(s) { myScript.myValue = Math.log(1 + 10 * +s); }
    }

Besides `get` and `set`, there is a third function you may declare with the name `touch`. It will be called when the
user begins and ends editing the GUI variable (e.g. when the knob is clicked and when the mouse button is released). A
single boolean parameter is passed to the function: `true` when the user begins editing and `false` when the user
ends.

There are a few built-in Cushy-only variables in Microtonic. You should avoid using these names. This is the list:
`'programsLocked'`, `'Morph'`, `'Morph.human'`, `'undo.description'`, `'redo.description'`, `'shortModifierKey'`,
`'po32Mode'`, `'isRegistered'`, `'uiScale'`, and `'skinResourcesPath'`.

### GUI Actions

When the user clicks a button or selects an item from a menu, a "GUI action" is performed (as configured by the `.cushy`
file). The action can be a built-in action or one your script provides as a Javascript function. Cushy calls the
actions with a single string parameter, but often these contain Numbstrict structures with key/value pairs. The
parameter is passed to Javascript in its raw unparsed format. E.g., a string will contain its enclosing quotes (you can
use [unescape](#unescape) to remove them).

When you implement an action with a JS function, you have two options:

- Use a function with the same name as the "action." It will be called with the single string parameter as input.
- Use an object to define the action. It should contain an `execute` function and optionally an `enabled` and a
  `checked` function (both receiving the same parameter as input, and they are expected to return a boolean).

In the latter case, you can use the `enabled` function to tell the GUI engine to indicate that an action is unavailable
visually. E.g., if the action is tied to a menu item, it will be grayed out if `enabled` returns `false`. The `checked`
function is similarly used to tell the GUI engine to visually indicate that the action is active and "in effect," e.g.,
by inserting a checkmark before a menu item.

Here is an example of an action:

    myScript.resetMyValue = {
        execute: function(p) { myScript.myValue = +unescape(p); }
        checked: function(p) { return myScript.myValue == +unescape(p); }
        enabled: function(p) { return myScript.canReset; }
    }

Besides getting called from user actions, a `.cushy` file can configure actions to be called by the following events:

- When the Cushy script is opened (right before setting up the views).
- When the Cushy script is closed (right before the views are removed).
- Regularly with a specific time interval.
- Whenever the value of a "GUI Variable" change.

Cushy has a number of built-in actions. They are documented elsewhere, but this is the list: `'nop'`, `'set'`,
`'switch'`, `'batch'`, `'execute'`, `'alert'`, `'hint'`, `'popup'`, `'edit'`, `'launch'`, `'reload'`, `'assert'`,
and `'assign'`

Microtonic also defines these actions for internal use: `'scriptPopup'`, `'programPopup'`, `'morphContextMenu'`,
`'toggleAbout'`, `'undo'`, `'redo'`, `'register'`, and `'focusToMain'`. Avoid using these names.
