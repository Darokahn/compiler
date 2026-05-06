.globl op_plus;       op_plus:       pop %rax; pop %rdi; add %rdi, %rax;                                                                          push %rax;     nop;
.globl op_minus;      op_minus:      pop %rax; pop %rdi; sub %rax, %rdi;                                                                          push %rdi;     nop;
.globl op_times;      op_times:      pop %rax; pop %rdi; imul %rdi, %rax;                                                                         push %rax;     nop;
.globl op_divide;     op_divide:     pop %rdi; pop %rax; cqo; idiv %rdi;                                                                          push %rdi;     nop;
.globl op_mod;        op_mod:        pop %rdi; pop %rax; cqo; idiv %rdi;                                                                          push %rdx;     nop;
.globl op_and;        op_and:        pop %rax; pop %rdi; test %rax, %rax; setnz %al; test %rdi, %rdi; setnz %dil; and %dil, %al; movzx %al, %rax; push %rax;     nop;
.globl op_or;         op_or:         pop %rax; pop %rdi; test %rax, %rax; setnz %al; test %rdi, %rdi; setnz %dil; or %dil, %al;  movzx %al, %rax; push %rax;     nop;
.globl op_bitwiseAnd; op_bitwiseAnd: pop %rax; pop %rdi; and %rdi, %rax;                                                                          push %rax;     nop;
.globl op_bitwiseOr;  op_bitwiseOr:  pop %rax; pop %rdi; or %rdi, %rax;                                                                           push %rax;     nop;
.globl op_xor;        op_xor:        pop %rax; pop %rdi; xor %rdi, %rax;                                                                          push %rax;     nop;
.globl op_shl;        op_shl:        pop %rcx; pop %rax; sal %cl, %rax;                                                                           push %rax;     nop;
.globl op_shr;        op_shr:        pop %rcx; pop %rax; sar %cl, %rax;                                                                           push %rax;     nop;
.globl op_eq;         op_eq:         pop %rax; pop %rdi; cmp %rax, %rdi; sete %al;  movzx %al, %rax;                                             push %rax;      nop;
.globl op_neq;        op_neq:        pop %rax; pop %rdi; cmp %rax, %rdi; setne %al; movzx %al, %rax;                                             push %rax;      nop;
.globl op_ge;         op_ge:         pop %rax; pop %rdi; cmp %rax, %rdi; setge %al; movzx %al, %rax;                                             push %rax;      nop
.globl op_gt;         op_gt:         pop %rax; pop %rdi; cmp %rax, %rdi; setg %al;  movzx %al, %rax;                                             push %rax;      nop;
.globl op_le;         op_le:         pop %rax; pop %rdi; cmp %rax, %rdi; setle %al; movzx %al, %rax;                                             push %rax;      nop;
.globl op_lt;         op_lt:         pop %rax; pop %rdi; cmp %rax, %rdi; setl %al;  movzx %al, %rax;                                             push %rax;      nop;

.globl op_prologue;
op_prologue:
    push %rbp
    mov %rsp, %rbp
    sub $256, %rsp
    nop

.globl op_epilogue
op_epilogue:
    mov %rbp, %rsp
    pop %rbp
    ret
    nop

.globl op_printInt
op_printInt:
    pop %rax
    mov %rsp, %r10
    movb $32, (%r10)
    cmp $0, %rax
    jne 1f
    sub $1, %r10
    movb $48, (%r10)
    jmp 2f
1:
    cmp $0, %rax
    je 2f
    cqo
    mov $10, %rcx
    idiv %rcx
    add $48, %rdx
    sub $1, %r10
    mov %dl, (%r10)
    jmp 1b
2:
    mov %rsp, %rdx
    sub %r10, %rdx
    add $1, %rdx
    mov $1, %rax
    mov $1, %rdi
    mov %r10, %rsi
    syscall

    nop
    mov $60, %rax
    syscall
