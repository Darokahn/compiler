# Utah Tech University Course 4450 "C++" compiler

The readme is up-to-date for commit d953171fb217985ebcc2b963570f85875de40374

This project was started as an assignment for the 4550 course offered by Utah Tech University.

The project is a "C++" compiler; however, the coursework version is incredibly basic and makes some odd decisions.

It would be better off calling itself a C compiler.

A few decisions I have made differently are:
- I wrote the project in C; the book urges C++ and writes all example code with classes.
- I wrote the scanner implementation to memory map input files and build the program out of raw lexeme pointers into the source.
- The book makes some odd decisions as to what gets to be first-class. `main` is a distinct token type, as is `cout`. I have opted for a more generic approach.
- Generally, the implementation is very bare, and I have expanded on it in a lot of miscellaneous ways.

Places where I have gone further than the book:
- I wrote a DFA generator and used it to expand the scanner's ability to recognize tokens. It uses python to generate a header with the necessary information. ***Right now***, it can recognize a decent subset of C syntax.
- I had to implement a few things from scratch that would have been given in C++, including a hash table. Technically, the book doesn't actually use one, but it could easily have.
- I created a testing harness that loads specially named test symbols with `dlsym` and runs them with a logging interface.
- I created a generic string streaming interface in which a string-generating function may pass its output to a type-erased object via the provided function pointer. I created a few implementations of it. Several of my data structures use the streaming interface to print themselves to an arbitrary output source. My implementations are a statically allocated string, a dynamic heap string, and a logging wrapper for any other streaming interface. The interface copies the signature of the builtin FILE* type and fprintf, so it works with those two without modification.


I am still following along with the book, though; as of ***right now***, we have finished the Scanner, Symbol Table, and the first steps of the Parse Tree.

The remaining steps are the rest of the Parser, Interpreter, and Machine Language emission.

The course will not expand the implementation to accomodate function definitions beyond `main` (whose declaration is just a magic pattern), basic integer arithmetic, variable assignment, and a hardcoded emulation of `cout << integer` printing that integer.

As of ***right now***, the files in this directory:

## main.c
- testing harness that initializes a line printer stream, finds symbols named `test_0`, `test_1`, `test_...` and run them, passing the line printer, until it fails to find one. Formatted test feedback.

## token.h
- struct with data for a token. Includes the lexeme and type. An init function checks the lexeme to decide whether the token should have a special type beyond what the scanner gave it.
- plethora of functions which are options for printing the token.

## tests.c
- definitions for all of the tests main searches for.

## stateMachine.h
- functions for processing characters and using them as edges in a DFA to generate tokens.

## stateMachineDefs.h
- generated file that contains all of the data about transitions from one state to the next. Also defines all token types, an array of strings for printing them, and associated colors so they can be printed with syntax highlighting.

## scanner.h
- basic mmap-backed file reader with functions for consuming tokens from the file.

## table/table.c
- basic hash table implementation that maps string:integer.

## symbols.h
- symbol table that wraps a table backend and uses it to index a dense array. also has a plethora of debug functions.

## stringStreaming.h
- implementations of string streaming interface. static string, heap string, and line printer.

## keywords.h
- lists of keywords used for token.h to classify tokens with

## evilpatch.c
- hack that manually overwrites `realloc` with a function that always returns `NULL` for testing. Needed realloc to only work differently during one test, didn't want to figure out how to globally link it differently.

## dfa\_gen
- some python scripts and yaml files used to generate the DFA used by my program and a graph visualizing it.

## testfiles
- misc files for testing code on
