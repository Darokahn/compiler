all: template view test

template: template.s
	gcc -r template.s -g -O0 -o asmTemplate

test: template test.s
	gcc template.s test.s -g -o a.out


view: template
	objdump -d asmTemplate
