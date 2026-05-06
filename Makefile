
TARGETS = main.c table/table.c evilpatch.c tests.c bytecode/asmTemplate

all: c

defs:
	python3 dfa_gen/cGenerator.py dfa_gen/stateMachine > stateMachineDefs.h

flow:
	python3 dfa_gen/flowGenerator.py dfa_gen/stateMachine

cWarn: main.c
	gcc -rdynamic $(TARGETS) -g -fsanitize=address -Wall -Wextra -Wno-unused-function -pedantic -std=c11

c: defs main.c
	gcc -rdynamic $(TARGETS) -g -fsanitize=address

unsan: defs main.c
	gcc -rdynamic $(TARGETS) -g -O0

fix:
	-gcc -rdynamic $(TARGETS) -g -fsanitize=address 2> compilationErrors.txt 
	nvim -q compilationErrors.txt -c copen

