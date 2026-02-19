/* --- INTEGER LITERALS --- */
0               /* Simple zero (Dec or Octal, doesn't matter) */
077             /* Valid Octal (Value: 63) */
0xDeadBeef      /* Valid Hex */
0X1234          /* Uppercase prefix */
123456789u      /* Unsigned decimal */
0xABCDEFul      /* Hex Unsigned Long */
42ULL           /* Long Long */
0123l           /* Octal Long */
18446744073709551615ULL /* Max 64-bit unsigned (should lex as one token) */

/* --- CHARACTER LITERALS --- */
'A'             /* Single char */
'\t'            /* Tab escape */
'\177'          /* Max 3-digit octal escape */
'\x1'           /* Short hex escape */
'\xabcdef'      /* Long hex escape (Valid, though may overflow char) */
'word'          /* Multi-char constant (4 chars is usually the limit for int) */
'\\'            /* Escaped backslash */
'\''            /* Escaped single quote */

/* --- STRING LITERALS --- */
""              /* Empty string */
" "             /* String with space */
"Tab\tBackslash\\Quote\"" /* Multiple escapes */
"\1234"         /* Octal \123 followed by char '4' */
"\x1234wxyz"    /* Hex \1234 followed by string "wxyz" */
"Long string spanning no lines but containing many \"quotes\" and \122 \x44 escapes"

/* --- COMPLEX MUNCHING --- */
0x123abc        /* Should not stop at 'a' */
123L            /* Should not stop at '3' and think 'L' is an identifier */
"String"123     /* Should lex as String("String") and then Integer(123) */
'A''B'          /* Should lex as Char('A') then Char('B') */

/* --- SUFFIX SOUP --- */
42uLL           /* Unsigned Long Long - mixed case */
0x123UL         /* Hex Unsigned Long */
0777L           /* Octal Long */
1ULL            /* Minimum value for a double-L suffix */

/* --- TIGHT SQUEEZES (No Whitespace) --- */
'1''2''3'       /* Three distinct char tokens */
"str"0x123      /* String token followed immediately by Hex token */
123L"suffix"    /* Long token followed immediately by String token */
0xabcL          /* Is it Hex(0xabcL) or Hex(0xabc) + Identifier(L)? 
                   In C, 'L' is a suffix, so this is ONE token: Hex 0xABC, type Long. */

/* --- ESCAPE SEQUENCES IN ACTION --- */
"\0000"         /* Should be Octal(\000) then Digit('0') */
"\x0123G"       /* Should be Hex(\x0123) then Char('G') */
"??="           /* Trigraph for # (If your lexer supports them, otherwise just a string) */
"Newline coming...\n...done" 

/* --- OVERFLOW & SPECIAL CASES --- */
0xFFFFFFFFFFFFFFFFULL /* Max 64-bit Hex */
'\xff'          /* Max hex char escape */
'\377'          /* Max octal char escape (255) */

/* --- CHARACTER MULTI-MUNCH --- */
'abcd'          /* Multi-char constant */
'\1'            /* Single digit octal escape - valid */
'\12'           /* Two digit octal escape - valid */

/* --- ADJACENT LITERALS (The "No-Space" Stress Test) --- */
123UL"str"      /* Decimal 123 (unsigned long) immediately followed by string "str" */
"str"'c'0x1     /* String, then Char, then Hex all touching */

/* --- HEX & SUFFIX AMBIGUITY --- */
0xabcdefl       /* Is 'l' a hex digit or a suffix? (In C, it's a suffix) */
0xABCDEFuLL     /* Multiple suffix chars following hex digits */
0X0123456789ABCDEF /* Pure 64-bit hex */

/* --- ESCAPE SEQUENCE DEPTH --- */
"\x0123456789ABCDEF" /* Hex escapes are UNBOUNDED. This is ONE character in the string. */
"\1\12\123"          /* 1, 2, and 3-digit octal escapes in a row */
"Escaped\?Question"  /* The \? escape is valid but often forgotten */

/* --- MULTI-CHARACTER OVERLOAD --- */
'12345678'      /* Valid as a multi-char constant, even if it exceeds int size */
'\01'           /* Two-digit octal */
'\xaf'          /* Two-digit hex char */

