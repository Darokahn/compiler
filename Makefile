
all: main.c table/table.c
	python3 flowgen/converter.py flowgen/doc.csv > stateMachine.h
	gcc main.c table/table.c -g
