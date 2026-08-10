```json
{
    "comments": {
        "lineComment": {
            "comment" : "#",
            "noIndent" : false
        },
        "blockComment": ["#*", "*#"]
    },
    "brackets": [
        ["{", "}"],
        ["(", ")"],
        ["[", "]"]
    ],
    "autoClosingPairs": [
        { "open": "{", "close": "}" },
        { "open": "(", "close": ")" },
        { "open": "[", "close": "]" },
        { "open": "\"", "close": "\"" },
        { "open": "'", "close": "'" },
        { "open": "f\"", "close": "\"" },
        { "open": "f'", "close": "'" },
        { "open": "#*", "close": "*#" },
    ]
}
```

## Guide: Setting Up Native VHG Language Support in VS Code

This setup allows VS Code to recognize `.vhg` files as a formal language, enabling **perfect "Ctrl + /" comment toggling**, auto-closing brackets, and custom syntax highlighting without any buggy snippets.

## Table of Contents

- [Step 1: Create the Extension Folder](#step-1-create-the-extension-folder)
- [Step 2: Create the Configuration Files](#step-2-create-the-configuration-files)
  - [1. `package.json`](#1-packagejson)
  - [2. `language-configuration.json`](#2-language-configurationjson)
  - [3. `syntaxes/vhg.tmLangguage.json`](#3-syntaxesvhgtmlangguagejson)
  - [4. `snippets/vhg.json`](#4-snippetsvhgjson)
- [Step 3: Update VS Code Global Settings](#step-3-update-vs-code-global-settings)
- [Step 4: Custom Syntax Highlighting (Colors)](#step-4-custom-syntax-highlighting-colors)
- [Step 5: Finalize](#step-5-finalize)
- [Key Features of this Setup:](#key-features-of-this-setup)

### Step 1: Create the Extension Folder

1. Open your File Explorer.
2. Navigate to the VS Code extensions folder. You can do this by pasting this path into your address bar:
   `%USERPROFILE%\.vscode\extensions` (on Windows).
   `~/.vscode/extensions` (on Linux).
   `~/.vscode/extensions` (on macOS).
3. Create a new folder named `vhg-lang`.

* Or you can just simply copy and paste this folder into `EXTENSION_PATH` (based on OS you're using). Rename this folder to:
  `vahagn-mardyan.vhg-language-0.0.1`

---

### Step 2: Create the Configuration Files

Inside the `vhg-lang` folder, create the following two files:

#### 1. `package.json`

This file registers the language ID and connects the `.vhg` extension.

```json
{
    "name": "vhg-language",
    "displayName": "VHG Language Support",
    "version": "0.0.1",
    "publisher": "vahagn-mardyan",
    "engines": {
        "vscode": "^1.80.0"
    },
    "categories": ["Programming Languages"],
    "contributes": {
        "languages": [{
            "id": "vhg",
            "extensions": [".vhg"],
            "configuration": "./language-configuration.json"
        }],
        "grammars": [{
            "language": "vhg",
            "scopeName": "source.vhg",
            "path": "./syntaxes/vhg.tmLanguage.json"
        }],
        "snippets": [{
            "language": "vhg",
            "path": "./snippets/vhg.json"
        }]
    }
}
```

#### 2. `language-configuration.json`

This file defines the behavior of the editor (comments and brackets).

```json
{
    "comments": {
        "lineComment": {
            "comment" : "#",
            "noIndent" : false
        },
        "blockComment": ["#*", "*#"]
    },
    "brackets": [
        ["{", "}"],
        ["(", ")"],
        ["[", "]"]
    ],
    "autoClosingPairs": [
        { "open": "{", "close": "}" },
        { "open": "(", "close": ")" },
        { "open": "[", "close": "]" },
        { "open": "\"", "close": "\"" },
        { "open": "'", "close": "'" },
        { "open": "f\"", "close": "\"" },
        { "open": "f'", "close": "'" },
        { "open": "F\"", "close": "\"" },
        { "open": "F'", "close": "'" },
        { "open": "#*", "close": "*#" },
    ]
}
```

#### 3. `syntaxes/vhg.tmLangguage.json`

```json
{
    "name": "VHG",
    "scopeName": "source.vhg",
    "patterns": [
        { "include": "#comments" },
        { "include": "#fstrings" },
        { "include": "#strings" },
        { "include": "#keywords" },
        { "include": "#builtIn" },
        { "include": "#math_functions" },
        { "include": "#function_definition" },
        { "include": "#function_call" },
        { "include": "#boolean" },
        { "include": "#logical_operators" },
        { "include": "#numbers" },
        { "include": "#operators" },
        { "include": "#variables" }
    ],
    "repository": {
        "comments": {
            "patterns": [
                {
                    "name": "comment.block.vhg",
                    "begin": "#\\*",
                    "end": "\\*#"
                },
                {
                    "name": "comment.line.vhg",
                    "match": "#.*$"
                }
            ]
        },
        "strings": {
            "patterns": [
                {
                    "name": "string.quoted.double.vhg",
                    "begin": "\"",
                    "end": "\"",
                    "patterns": [{
                        "name": "constant.character.escape.vhg",
                        "match": "\\\\."
                    }]
                },
                {
                    "name": "string.quoted.single.vhg",
                    "begin": "'",
                    "end": "'",
                    "patterns": [{
                        "name": "constant.character.escape.vhg",
                        "match": "\\\\."
                    }]
                }
            ]
        },
        "fstrings": {
            "patterns": [
                {
                    "name": "string.interpolated.double.vhg",
                    "begin": "(?i)\\bf\"",
                    "beginCaptures": { "0": { "name": "punctuation.definition.string.begin.vhg" } },
                    "end": "\"",
                    "endCaptures": { "0": { "name": "punctuation.definition.string.end.vhg" } },
                    "patterns": [
                        { "name": "constant.character.escape.vhg", "match": "\\{\\{|\\}\\}" },
                        { "name": "constant.character.escape.vhg", "match": "\\\\." },
                        {
                            "name": "meta.embedded.line.vhg",
                            "begin": "\\{",
                            "beginCaptures": { "0": { "name": "punctuation.section.embedded.begin.vhg" } },
                            "end": "\\}",
                            "endCaptures": { "0": { "name": "punctuation.section.embedded.end.vhg" } },
                            "patterns": [
                                { "include": "#builtIn" },
                                { "include": "#math_functions" },
                                { "include": "#boolean" },
                                { "include": "#logical_operators" },
                                { "include": "#numbers" },
                                { "include": "#function_call" },
                                { "include": "#operators" },
                                { "include": "#variables" }
                            ]
                        }
                    ]
                },
                {
                    "name": "string.interpolated.single.vhg",
                    "begin": "(?i)\\bf'",
                    "beginCaptures": { "0": { "name": "punctuation.definition.string.begin.vhg" } },
                    "end": "'",
                    "endCaptures": { "0": { "name": "punctuation.definition.string.end.vhg" } },
                    "patterns": [
                        { "name": "constant.character.escape.vhg", "match": "\\{\\{|\\}\\}" },
                        { "name": "constant.character.escape.vhg", "match": "\\\\." },
                        {
                            "name": "meta.embedded.line.vhg",
                            "begin": "\\{",
                            "beginCaptures": { "0": { "name": "punctuation.section.embedded.begin.vhg" } },
                            "end": "\\}",
                            "endCaptures": { "0": { "name": "punctuation.section.embedded.end.vhg" } },
                            "patterns": [
                                { "include": "#builtIn" },
                                { "include": "#math_functions" },
                                { "include": "#boolean" },
                                { "include": "#logical_operators" },
                                { "include": "#numbers" },
                                { "include": "#function_call" },
                                { "include": "#operators" },
                                { "include": "#variables" }
                            ]
                        }
                    ]
                }
            ]
        },
        "keywords": {
            "match": "(?i)\\b(if|else|while|for|print|function|return|import|break|continue|switch|case|default)\\b",
            "name": "keyword.control.vhg"
        },
        "boolean": {
            "match": "(?i)\\b(true|false|none|var|variable)\\b",
            "name": "keyword.control_boolean.vhg"
        },
        "logical_operators": {
            "match": "(?i)\\b(and|or|not|local|global)\\b",
            "name": "keyword.operator.logical.vhg"
        },
        "numbers": {
            "match": "\\b\\d[\\d_]*(\\.\\d[\\d_]*)?(([eE][+-]?\\d+))?\\b",
            "name": "constant.numeric.vhg"
        },
        "builtIn": {
            "match": "(?i)\\b(input|length|void|m_e|m_pi|m_inf|m_max|ord|chr|bin|oct|dec|hex|type|array|array_push|array_pop|array_insert|array_remove|number|string)\\b",
            "name": "support.function.vhg"
        },
        "math_functions": {
            "match": "(?i)\\b(sin|cos|tan|asin|acos|atan|atan2|sqrt|cbrt|exp|log|log2|log10|log_ab|ceil|floor|round|abs|fmod|random)\\b",
            "name": "support.math.functions.vhg"
        },
        "function_definition": {
            "match": "(?i)\\bfunction\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
            "captures": {
                "1": { "name": "entity.name.function.vhg" }
            }
        },
        "function_call": {
            "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
            "captures": {
                "1": { "name": "support.function.user.vhg" }
            }
        },
        "operators": {
            "match": "(\\+|-|\\*|/|%|&|\\||\\^|<<|>>|\\+\\=|\\-\\=|\\*\\=|\\/\\=|\\%\\=|\\^\\=)?=",
            "name": "keyword.operator.assignment.vhg"
        },
        "variables": {
            "match": "\\b[a-zA-Z_][a-zA-Z0-9_]*\\b",
            "name": "variable.other.vhg"
        }
    }
}
```

#### 4. `snippets/vhg.json`

```json
{
    "sin function": {
        "prefix": "sin",
        "body": "sin(${1:angle})",
        "description": "sin(arg) -> number - Sine function"
    },
    "cos function": {
        "prefix": "cos",
        "body": "cos(${1:angle})",
        "description": "cos(arg) -> number - Cosine function"
    },
    "tan function": {
        "prefix": "tan",
        "body": "tan(${1:angle})",
        "description": "tan(arg) -> number - Tangent function"
    },
    "asin function": {
        "prefix": "asin",
        "body": "asin(${1:value})",
        "description": "asin(arg) -> number - Arc sine (inverse sine)"
    },
    "acos function": {
        "prefix": "acos",
        "body": "acos(${1:value})",
        "description": "acos(arg) -> number - Arc cosine"
    },
    "atan function": {
        "prefix": "atan",
        "body": "atan(${1:value})",
        "description": "atan(arg) -> number - Arc tangent"
    },
    "atan2 function": {
        "prefix": "atan2",
        "body": "atan2(${1:y}, ${2:x})",
        "description": "atan2(y, x) -> number - 2-argument arctangent"
    },
    "sqrt function": {
        "prefix": "sqrt",
        "body": "sqrt(${1:number})",
        "description": "sqrt(arg) -> number - Square root"
    },
    "cbrt function": {
        "prefix": "cbrt",
        "body": "cbrt(${1:number})",
        "description": "cbrt(arg) -> number - Cube root"
    },
    "pow function": {
        "prefix": "pow",
        "body": "pow(${1:base}, ${2:exponent})",
        "description": "pow(base, exponent) -> number - Power function (base^exponent)"
    },
    "exp function": {
        "prefix": "exp",
        "body": "exp(${1:number})",
        "description": "exp(arg) -> number - Exponential function (e^x)"
    },
    "log function": {
        "prefix": "log",
        "body": "log(${1:number})",
        "description": "log(arg) -> number - Natural logarithm (base e)"
    },
    "log2 function": {
        "prefix": "log2",
        "body": "log2(${1:number})",
        "description": "log2(arg) -> number - Base-2 logarithm"
    },
    "log10 function": {
        "prefix": "log10",
        "body": "log10(${1:number})",
        "description": "log10(arg) -> number - Base-10 logarithm"
    },
    "ln function": {
        "prefix": "ln",
        "body": "ln(${1:number})",
        "description": "ln(arg) -> number - Natural logarithm (same as log)"
    },
    "log_ab function": {
        "prefix": "log_ab",
        "body": "log_ab(${1:base}, ${2:exponent})",
        "description": "log_ab(a, b) -> number - Logarithm of b with base a"
    },
    "ceil function": {
        "prefix": "ceil",
        "body": "ceil(${1:number})",
        "description": "ceil(arg) -> number - Round up to nearest integer"
    },
    "floor function": {
        "prefix": "floor",
        "body": "floor(${1:number})",
        "description": "floor(arg) -> number - Round down to nearest integer"
    },
    "round function": {
        "prefix": "round",
        "body": "round(${1:number})",
        "description": "round(arg) -> number - Round to nearest integer"
    },
    "abs function": {
        "prefix": "abs",
        "body": "abs(${1:number})",
        "description": "abs(arg) -> number - Absolute value"
    },
    "fmod function": {
        "prefix": "fmod",
        "body": "fmod(${1:x}, ${2:y})",
        "description": "fmod(x, y) -> number - Floating-point remainder"
    },
    "length function": {
        "prefix": "length",
        "body": "length(${1:array_or_string})",
        "description": "length(arg) -> number - Returns the size of a string or array"
    },
    "type function": {
        "prefix": "type",
        "body": "type(${1:argument})",
        "description": "type(arg) -> string - Returns the type of argument (string, number, array or none)"
    },
    "array function": {
        "prefix": "array",
        "body": "array(${1:size})",
        "description": "array(n) -> array - With one argument: a new array of size n, each element initialized to none. With zero or two-or-more arguments, builds an array from those values instead: array() -> [], array(1, 2, 3) -> [1, 2, 3]."
    },
    "number constructor" : {
        "prefix" : "number",
        "body" : "number(${1:string})",
        "description" : "number(x) -> number - Converts x to a number; errors on an invalid string, none, or an array"
    },
    "string constructor" : {
        "prefix" : "string",
        "body" : "string(${1:value})",
        "description" : "string(x) -> string - Converts x to a string; always succeeds"
    },
    "array literal": {
        "prefix": "arrlit",
        "body": "[${1:1, 2, 3}]",
        "description": "[e1, e2, ...] - Array literal (elements may be any type, including nested arrays)"
    },
    "array_push function": {
        "prefix": "array_push",
        "body": "array_push(${1:array}, ${2:value});",
        "description": "array_push(arr, value) - Appends value to arr in place; returns the new length"
    },
    "array_pop function": {
        "prefix": "array_pop",
        "body": "array_pop(${1:array})",
        "description": "array_pop(arr) -> value - Removes and returns the last element of arr in place"
    },
    "array_insert function": {
        "prefix": "array_insert",
        "body": "array_insert(${1:array}, ${2:index}, ${3:value});",
        "description": "array_insert(arr, index, value) - Inserts value at index in arr in place; returns the inserted value"
    },
    "array_remove function": {
        "prefix": "array_remove",
        "body": "array_remove(${1:array}, ${2:index})",
        "description": "array_remove(arr, index) -> value - Removes and returns the element at index from arr in place"
    },
    "array iteration": {
        "prefix": "forarr",
        "body": [
            "var ${2:i} = 0;",
            "while (${2:i} < length(${1:array})) {",
            "    ${0}",
            "    ${2:i} += 1;",
            "}"
        ],
        "description": "Iterate over every element of an array by index"
    },
    "ord function": {
        "prefix": "ord",
        "body": "ord(${1:string})",
        "description": "ord(string) -> number Returns the Unicode code point for the first character of a string"
    },
    "chr function": {
        "prefix": "chr",
        "body": "chr(${1:number})",
        "description": "chr(number) -> string Returns a Unicode string of one character with code point i; 0 <= i <= 0x10FFFF"
    },
    "bin function": {
        "prefix": "bin",
        "body": "bin(${1:integer})",
        "description": "bin(arg) -> string - Binary representation of an integer"
    },
    "oct function": {
        "prefix": "oct",
        "body": "oct(${1:integer})",
        "description": "oct(arg) -> string - Octal representation of an integer"
    },
    "hex function": {
        "prefix": "hex",
        "body": "hex(${1:integer})",
        "description": "hex(arg) -> string - Hexadecimal representation of an integer"
    },
    "dec function": {
        "prefix": "dec",
        "body": "dec(${1:string})",
        "description": "dec(arg) -> number - Decimal representation of given argument"
    },
    "random function": {
        "prefix": "random",
        "body": "random(${1:min}, ${2:max})",
        "description": "random(min=0, max=1) -> number - Generates a random floating-point number in the range [min, max). Default range is [0, 1)."
    },
    "input function": {
        "prefix": "input",
        "body": "input(${1:prompt})",
        "description": "input(prompt) -> string/number - User input"
    },
    "m_pi constant": {
        "prefix": "pi|m_pi",
        "body": "m_pi",
        "description": "m_pi -> number - Mathematical constant π (3.14159...)"
    },
    "m_e constant": {
        "prefix": "m_e",
        "body": "m_e",
        "description": "m_e -> number - Mathematical constant e (2.71828...)"
    },
    "m_inf constant": {
        "prefix": "inf|m_inf",
        "body": "m_inf",
        "description": "m_inf -> number - Infinity"
    },
    "m_max constant": {
        "prefix": "max|m_max",
        "body": "m_max",
        "description": "The most possible maximum value."
    },
    "if statement": {
        "prefix": "if",
        "body": [
            "if (${1:condition}) {",
                "",
            "}"
        ],
        "description": "if statement - Conditional execution"
    },
    "if-else statement": {
        "prefix": "ife",
        "body": [
            "if (${1:condition}) {",
                "",
            "} else {",
                "",
            "}"
        ],
        "description": "if-else statement - Conditional execution with else branch"
    },
    "if-else if-else statement": {
        "prefix": "ifei",
        "body": [
            "if (${1:condition}) {",
                "",
            "} else if (${3:condition}) {",
                "",
            "} else {",
                "",
            "}"
        ],
        "description": "if-else if-else statement - Multiple conditions"
    },
    "else-if statement": {
        "prefix" : "elif",
        "body": [
            "else if(${1:condition}) {",
                "",
            "}"
        ],
        "description" : "if-else statement"
    },
    "while loop": {
        "prefix": "while",
        "body": [
            "while (${1:condition}) {",
                "",
            "}"
        ],
        "description": "while loop - Execute while condition is true"
    },
    "for loop": {
        "prefix": "for",
        "body": [
            "for (${1:variable i = 0}; ${2:i < count}; ${3:i = i + 1}) {",
                "",
            "}"
        ],
        "description": "for loop - Execute with initialization, condition, and update"
    },
    "function definition": {
        "prefix": "function",
        "body": [
            "function ${1:name}(${2:params}) {",
                "",
            "  return ${4:value};",
            "}"
        ],
        "description": "function - Define a function with return value"
    },
    "void function": {
        "prefix": "vfunction | void",
        "body": [
            "void function ${1:name}(${2:params}) {",
                "",
            "}"
        ],
        "description": "void function - Define a function without return value"
    },
    "function with default argument": {
        "prefix": "functiond",
        "body": [
            "function ${1:name}(${2:param}, ${3:optional} = ${4:default}) {",
                "",
            "  return ${0:value};",
            "}"
        ],
        "description": "function - Define a function with a default-valued parameter"
    },
    "function with variadic parameter": {
        "prefix": "functionv",
        "body": [
            "function ${1:name}(*${2:args}) {",
                "",
            "  return ${0:value};",
            "}"
        ],
        "description": "function - Define a function with a variadic (*args) parameter, collected as an array"
    },
    "print statement": {
        "prefix": "print",
        "body": "print(${1:expression});",
        "description": "print - Output to console"
    },
    "print with newline": {
        "prefix": "printn",
        "body": "print(\"${1:message}\\n\");",
        "description": "print with newline - Output with explicit newline"
    },
    "return statement": {
        "prefix": "return",
        "body": "return ;",
        "description": "return - Return value from function"
    },
    "local variable": {
        "prefix": "local",
        "body": "local",
        "description": "local - Declare a local variable"
    },
    "global variable": {
        "prefix": "global",
        "body": "global",
        "description": "global - Declare a global variable"
    },
    "variable declaration": {
        "prefix":"variable | var",
        "body": "variable",
        "description": "Declare a variable"
    },
    "or keyword": {
        "prefix": "or",
        "body": "or",
        "description": "or - Logical or operator"
    },
    "and keyword": {
        "prefix": "and",
        "body": "and",
        "description": "and - Logical and operator"
    },
    "not keyword": {
        "prefix": "not",
        "body": "not",
        "description": "not - Logical not operator"
    },
    "true keyword": {
        "prefix": "true",
        "body": "true",
        "description": "true - Boolean true value"
    },
    "false keyword": {
        "prefix": "false",
        "body": "false",
        "description": "false - Boolean false value"
    },
    "none keyword": {
        "prefix": "none",
        "body": "none",
        "description": "none - None/null type"
    },
    "import statement": {
        "prefix": "import",
        "body": "import \"${1:filename}\";",
        "description": "import - Import variables or functions from other files"
    },
    "break keyword": {
        "prefix": "break",
        "body": "break;",
        "description": "break - Exit loop or switch early"
    },
    "continue keyword": {
        "prefix": "continue",
        "body": "continue;",
        "description": "continue - Skip to next iteration"
    },
    "switch statement": {
        "prefix": "switch",
        "body": [
            "switch(${1:value}) {",
            "  case ${2:c1}, ${3:c2}:",
                "",
            "break;",
            "default:",
                "",
            "}"
        ],
        "description": "switch-case - Multi-way conditional execution"
    },
    "case": {
        "prefix":"case",
        "body": [
            "case ${1:c1}, ${2:c2}:",
                "",
            "break;"
        ]
    },
    "ternary operator": {
        "prefix": "tern",
        "body": "${1:condition} ? ${2:true_value} : ${3:false_value}",
        "description": "ternary - Conditional expression (condition ? true : false)"
    },
    "f-string": {
        "prefix": "fstring",
        "body": "f\"${1:text} {${2:expr}}\"",
        "description": "f\"...{expr}...\" - String with {expr} interpolated and converted to a string. Use {{ and }} for a literal brace. If your expr needs a string literal, open the f-string with the other quote character (f'...' vs f\"...\") to avoid a quote collision."
    },
    "comment block": {
        "prefix": "#*",
        "body": [
            "#*",
            " * ${1:comment}",
            "*#"
        ],
        "description": "comment - Multi-line comment block"
    },
    "entry-function": {
        "prefix":"main",
        "body" : [
            "void function main() {",
                "",
            "}"
        ]
    }
}
```

---

### Step 3: Update VS Code Global Settings

Now, tell VS Code to use this new "vhg" language for your files.

1. Open VS Code.
2. Press `Ctrl + Shift + P`, type **"Open User Settings (JSON)"**, and press Enter.
3. Find or add the `files.associations` section and set it like this:

```json
"files.associations": {
    "*.vhg": "vhg"
}
```

4. Ensure your `highlight.regexes` are still there to provide the colors (as we configured previously), but make sure they target the `.vhg` files.

---

### Step 4: Custom Syntax Highlighting (Colors)

To apply colors to your language, add the following block to your `settings.json` under the `highlight.regexes` section. This ensures keywords, numbers, and comments are colored specifically for the `vhg` language:

```json
"editor.tokenColorCustomizations": {
    "textMateRules": [
        { "scope": "keyword.control.vhg", "settings": { "foreground": "#C586C0" } },
        { "scope": "keyword.operator.logical.vhg", "settings": { "foreground": "#0b4da3" } },
        { "scope": "keyword.control_boolean.vhg", "settings": {"foreground": "#1e73ea"} },
        { "scope": "string.quoted.double.vhg", "settings": { "foreground": "#CE7744" } },
        { "scope": "string.quoted.single.vhg", "settings": { "foreground": "#CE7744" } },
        { "scope": "constant.numeric.vhg", "settings": { "foreground": "#B5CEA8" } },
        { "scope": "variable.other.vhg", "settings": { "foreground": "#9CDCFE" } },
        { "scope": "comment.line.vhg", "settings": { "foreground": "#6A9955", "fontStyle": "italic" } },
        { "scope": "comment.block.vhg", "settings": { "foreground": "#6A9955", "fontStyle": "italic" } },
        { "scope": "support.function.vhg", "settings": { "foreground": "#007bff" } },
        { "scope": "support.math.functions.vhg", "settings": { "foreground": "#DCCE80" } },
        { "scope": "entity.name.function.vhg", "settings": { "foreground": "#DCDCAA" } },
        { "scope": "support.function.user.vhg", "settings": { "foreground": "#DCDCAA" } },
        { "scope": "keyword.operator.assignment.vhg", "settings": { "foreground": "#C586C0" } }
    ]
},
```

### Step 5: Finalize

1. **Remove old shortcuts:** Delete any `ctrl + /` bindings in `keybindings.json`.
2. **Restart VS Code:** This is required for the new extension to load.

### Key Features of this Setup:

* Native Toggle: `Ctrl + /` now works instantly using the `#` symbol.
* Auto-Close: Typing `#*`, `(`, `[` or `{` will automatically generate the closing pair.
* Semantic Colors: Your code is now visually organized and easy to read.
