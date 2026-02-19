from matplotlib.colors import to_rgba

def to_rgb(string):
    return tuple(map(lambda x: x * 255, to_rgba(string)[0:-1]))


colors = {
    # Punctuation & Operators (Light Gray: #D4D4D4)
    "comma": "#D4D4D4",
    "xoreq": "#D4D4D4",
    "arrow": "#D4D4D4",
    "assignment": "#D4D4D4",
    "modeq": "#D4D4D4",
    "dot": "#D4D4D4",
    "amp": "#D4D4D4",
    "shiftright": "#D4D4D4",
    "tilde": "#D4D4D4",
    "shiftleft": "#D4D4D4",
    "noteq": "#D4D4D4",
    "timeseq": "#D4D4D4",
    "bandeq": "#D4D4D4",
    "semicolon": "#D4D4D4",
    "langle": "#D4D4D4",
    "star": "#D4D4D4",
    "colon": "#D4D4D4",
    "ge": "#D4D4D4",
    "le": "#D4D4D4",
    "and": "#D4D4D4",
    "pluseq": "#D4D4D4",
    "mod": "#D4D4D4",
    "div": "#D4D4D4",
    "minus": "#D4D4D4",
    "plus": "#D4D4D4",
    "minuseq": "#D4D4D4",
    "caret": "#D4D4D4",
    "boreq": "#D4D4D4",
    "pipe": "#D4D4D4",
    "bnoteq": "#D4D4D4",
    "rangle": "#D4D4D4",
    "exc": "#D4D4D4",
    "eq": "#D4D4D4",
    "lor": "#D4D4D4",
    "question": "#D4D4D4",
    "whitespace": "#D4D4D4",
    "newline": "#D4D4D4",

    # Brackets & Delimiters (Gold: #FFD700)
    "lparen": "#FFD700",
    "rparen": "#FFD700",
    "lbracket": "#FFD700",
    "rbracket": "#FFD700",
    "lcurly": "#FFD700",
    "rcurly": "#FFD700",

    # Numbers & Literals (Light Green: #B5CEA8)
    "decimal": "#B5CEA8",
    "decimal_l": "#B5CEA8",
    "decimal_lu": "#B5CEA8",
    "decimal_ll": "#B5CEA8",
    "decimal_u": "#B5CEA8",
    "decimal_ull": "#B5CEA8",
    "decimal_uwb": "#B5CEA8",
    "decimal_wb": "#B5CEA8",
    "hexadecimal": "#B5CEA8",
    "hex_l": "#B5CEA8",
    "hex_lu": "#B5CEA8",
    "hex_ll": "#B5CEA8",
    "hex_u": "#B5CEA8",
    "hex_ull": "#B5CEA8",
    "hex_uwb": "#B5CEA8",
    "hex_wb": "#B5CEA8",
    "octal": "#B5CEA8",
    "oct_l": "#B5CEA8",
    "oct_lu": "#B5CEA8",
    "oct_ll": "#B5CEA8",
    "oct_u": "#B5CEA8",
    "oct_ull": "#B5CEA8",
    "oct_uwb": "#B5CEA8",
    "oct_wb": "#B5CEA8",
    "fraction": "#B5CEA8",
    "decimalfraction": "#B5CEA8",
    "hexfraction": "#B5CEA8",

    # Strings & Comments (Green: #6A9955)
    "string": "#6A9955",
    "charliteral": "#6A9955",
    "linecomment": "#6A9955",
    "blockcomment": "#6A9955",
    "preproc": "#6A9955",

    # Identifiers (Light Blue: #9CDCFE)
    "identifier": "#9CDCFE",

    # Errors & Special (Red: #F44747 / Gray: #808080)
    "bad": "#F44747",
    "cantmove": "#F44747",
    "eof": "#808080"
}

