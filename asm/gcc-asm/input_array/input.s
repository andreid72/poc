
.data

/* define variables */
input_size:
    .string "sizeof(array) = "
input_elem:
    .string "array[%d] = "
int_fmt:
    .string "%d"


.text
	.globl	main
	.type	main, @function

input_array:
    /* epilog */
    pushq %rbp
    movq %rsp, %rbp
    /* allocate space for the local variables:
        char *arr:      -8(%rbp)
        int *arr_size:  -16(%rbp)
        int arr_size:   -20(%rbp)
        int i:          -24(%rbp)
    */
    subq $32, %rsp   

    /* save input arguments */
    movq %rdi, -16(%rbp)

    /* output array size invocation */
    movl $input_size, %edi
    call puts

    /* input the size of array */
    movq -16(%rbp), %rsi
    movl $int_fmt, %edi
    call scanf

    /* save the array length to the local var & ebx*/
    movq -16(%rbp), %rax
    movl (%rax), %ebx
    movl %ebx, -20(%rbp)

    /* allocate the array */
    movl %ebx, %edi
    call malloc
    movq %rax, -8(%rbp)
    
    cmpq $0, %rax
    je input_array_exit

    movl $0, -24(%rbp)
input_array_loop1:
    /* check the exit condition */
    xorq %rax, %rax
    movl -24(%rbp), %eax
    cmpl -20(%rbp), %eax
    jnl input_array_exit    

    /* output invocation */
    movl $input_elem, %edi
    movl %eax, %esi
    call printf

    /* prepare the address of the new element */
    movq -8(%rbp), %rbx
    xorq %rax, %rax
    movl -24(%rbp), %eax
    leaq (%rbx, %rax, 4), %rsi
    movl $int_fmt, %edi
    call scanf

    /* increment the counter */    
    xorq %rax, %rax
    movl -24(%rbp), %eax
    addl $1, %eax
    movl %eax, -24(%rbp)

    jmp input_array_loop1

input_array_exit:
    movq -8(%rbp), %rax
    addq $32, %rsp   
    popq %rbp
    ret

main:
    /* epilog */
    pushq %rbp
    movq %rsp, %rbp
    /* allocate space for the local variables:
        char *arr:      -8($rbp)
        int arr_size:   -12($rbp)
    */
    subq $16, %rsp

    /* load the address of arr to the first param */
    leaq -12(%rbp), %rdi
    call input_array
    movq %rax, -8(%rbp)

    pop %rbp
    movl $0, %eax
    ret

    
    

