
all: main.c table/table.c dfa_gen/cGenerator.py dfa_gen/flowGenerator.py
	python3 dfa_gen/cGenerator.py dfa_gen/stateMachine > stateMachineDefs.h
	python3 dfa_gen/flowGenerator.py dfa_gen/stateMachine
	gcc main.c table/table.c -g

c: main.c table/table.c
	gcc main.c table/table.c -g
