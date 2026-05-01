
all: c

defs:
	python3 dfa_gen/cGenerator.py dfa_gen/stateMachine > stateMachineDefs.h

flow:
	python3 dfa_gen/flowGenerator.py dfa_gen/stateMachine

cWarn: main.c
	gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -fsanitize=address -Wall -Wextra -Wno-unused-function -pedantic -std=c11

c: defs main.c
	gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -fsanitize=address
	#gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -O0

fix:
	-gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -fsanitize=address 2> compilationErrors.txt 
	nvim -q compilationErrors.txt -c copen

