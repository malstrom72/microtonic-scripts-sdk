// ESLint flat config for Sonic Charge script packages (ES5 "script" syntax).
//
// This file is shared between the Microtonic and Synplant script SDKs. Only the
// product configuration block below differs between the two copies; keep the
// rest byte-identical.

// ---- Product configuration (Microtonic) -------------------------------------
const PRODUCT_NAME = "Microtonic";
const RESOURCES_DIR = "Microtonic Resources";

// Host API that only this product provides (docs/Microtonic JS Reference.md and
// Microtonic Resources/main.js).
const PRODUCT_GLOBALS = {
  CHANNEL_COUNT: "readonly",
  DRUM_PATCH_PARAMS: "readonly",
  FREQ_VALUE_C4: "readonly",
  GLOBAL_PARAM_COUNT: "readonly",
  OCTAVE_STEP: "readonly",
  PATTERN_COUNT: "readonly",
  PATTERN_STEP_COUNT: "readonly",
  cbrt: "readonly",
  converge: "readonly",
  devLayout: "writable",
  parseArray: "readonly",
  parseStruct: "readonly",
  triggerChannel: "readonly",
  // Set by JSConsole.js since 2023; undocumented. Synplant sets it with
  // setCushyVariable instead. Unverified which form Microtonic reads.
  verboseErrors: "writable"
};

// Bundled examples and the JS Console keep persistent state in package globals
// that span several files; external scripts can add their own
// /* global name:writable */ comments instead.
const EXAMPLE_GLOBALS = {
  _: "writable",
  euclideanBeat: "writable",
  fmTool: "writable",
  jsConsole: "writable",
  macroTweak: "writable",
  mixConsole: "writable",
  polyChain: "writable"
};
// ---- End of product configuration --------------------------------------------

// Engine API and host helpers that both products provide.
const SHARED_GLOBALS = {
  // Standard ES5 addition that NuXJS scripts may use.
  JSON: "readonly",

  // Engine API.
  PARAMS: "readonly",
  PROGRAM_COUNT: "readonly",
  ask: "readonly",
  browse: "readonly",
  composeNumbstrict: "readonly",
  createElement: "readonly",
  dir: "readonly",
  display: "readonly",
  editParam: "readonly",
  fullPath: "readonly",
  getCushyVariable: "readonly",
  getElement: "readonly",
  getElementId: "readonly",
  getParam: "readonly",
  isMarshaledFormat: "readonly",
  load: "readonly",
  marshal: "readonly",
  paramText: "readonly",
  paramValue: "readonly",
  parseNumbstrict: "readonly",
  performCushyAction: "readonly",
  print: "writable",
  readClipboard: "readonly",
  run: "readonly",
  save: "readonly",
  saveUndo: "readonly",
  setCushyVariable: "readonly",
  setElement: "readonly",
  setParam: "readonly",
  translate: "readonly",
  unmarshal: "readonly",
  writeClipboard: "readonly",

  // Optional hook a script may install.
  handleCushyTrace: "writable",

  // Host helper layer defined by the product's main script.
  BUILD: "readonly",
  DIRS: "readonly",
  PLATFORM: "readonly",
  StringBuilder: "readonly",
  assert: "readonly",
  bounce: "readonly",
  clamp: "readonly",
  closeCushy: "readonly",
  createClass: "readonly",
  cube: "readonly",
  displayCushy: "readonly",
  fract: "readonly",
  isRepeating: "readonly",
  lerp: "readonly",
  random: "readonly",
  scale: "readonly",
  select: "readonly",
  selected: "readonly",
  square: "readonly",
  toggleCushy: "readonly",
  unescape: "readonly"
};

export default [
  {
    files: ["**/*.js"],
    ignores: [
      "IVG/**",
      RESOURCES_DIR + "/**",
      "CushyLint/**",
      "tmLanguages/**",
      "tools/IVG2PNG/**",
      "tools/jsconsole-bridge-mcp/**"
    ],
    languageOptions: {
      ecmaVersion: 5,
      sourceType: "script",
      globals: Object.assign({}, SHARED_GLOBALS, PRODUCT_GLOBALS, EXAMPLE_GLOBALS)
    },
    rules: {
      "no-undef": "error",
      "no-restricted-syntax": [
        "error",
        {
          selector: "Property[kind='get'], Property[kind='set']",
          message: "Getter/setter object literal syntax is not supported by " + PRODUCT_NAME + " scripts."
        }
      ],
      "no-with": "error"
    }
  }
];
