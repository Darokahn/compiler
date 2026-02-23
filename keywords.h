#pragma once

enum keywords {
    AUTO,
    BREAK,
    CASE,
    CONST,
    CONTINUE,
    DEFAULT,
    DO,
    DOUBLE,
    ELSE,
    ENUM,
    FOR,
    GOTO,
    IF,
    RETURN,
    SIZEOF,
    STATIC,
    STRUCT,
    SWITCH,
    TYPEDEF,
    UNION,
    WHILE,
    LASTKEYWORD
};

static const char* keywordStrings[] = {
    "auto",
    "break",
    "case",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "for",
    "goto",
    "if",
    "return",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "while"
};

enum builtinTypes {
    CHAR,
    VOID,
    SHORT,
    SIGNED,
    UNSIGNED,
    VOLATILE,
    REGISTER,
    LONG,
    INT,
    FLOAT,
    EXTERN,
    LASTTYPE,
};

static const char* builtinTypeStrings[] = {
    "char",
    "void",
    "short",
    "signed",
    "unsigned",
    "volatile",
    "register",
    "long",
    "int",
    "float",
    "extern",
};
