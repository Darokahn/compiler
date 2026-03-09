
all: main.c table/table.c dfa_gen/cGenerator.py dfa_gen/flowGenerator.py
	python3 dfa_gen/cGenerator.py dfa_gen/stateMachine > stateMachineDefs.h
	python3 dfa_gen/flowGenerator.py dfa_gen/stateMachine
	gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -fsanitize=address

c: main.c table/table.c
	gcc -rdynamic main.c table/table.c evilpatch.c tests.c -g -fsanitize=address -Wall -Wextra -Wno-unused-function
