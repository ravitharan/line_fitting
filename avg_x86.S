
	.globl	average_asm
average_asm:
	pushq	%rbp
	movq	%rsp, %rbp

  movq  $0, %rax
  mov   %esi, %edx
loop:
  cmpl  $0, %edx
  jz    exit
  add   (%rdi), %eax
  add   $4, %rdi
  dec   %edx
  jmp   loop

exit:
  div   %esi
	popq	%rbp
	ret
