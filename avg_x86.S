

/* count should be multiple of 8 */
/* maximum count without total overflow is 8 * ( 1 << 16) = 512 * 1024 */
	.globl	total_simd
total_simd:
	push 	%rbp
	mov 	%rsp, %rbp

  vpxor %ymm0, %ymm0, %ymm0
  mov   $0, %rax
  mov   %esi, %ecx
loop_simd:
  cmp   $0, %ecx
  jz    exit_simd
  vpmovzxwd (%rdi), %ymm1
  vpaddd    %ymm1, %ymm0, %ymm0
  add   $16, %rdi
  sub   $8,  %ecx
  jmp   loop_simd

exit_simd:
  vextracti128  $1,  %ymm0, %xmm1
  vpmovzxdq %xmm0, %ymm2
  vpmovzxdq %xmm1, %ymm3
  vpaddq    %ymm3, %ymm2, %ymm0
  vextracti128  $1,  %ymm0, %xmm1
  vpextrq   $0,    %xmm0, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm0, %rdx
  add       %rdx,  %rax
  vpextrq   $0,    %xmm1, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm1, %rdx
  add       %rdx,  %rax
	pop 	%rbp
	ret



/* count should be multiple of 8 */

#define MEAN_X    ymm0
#define MEAN_Y    ymm1
#define DATA_X    ymm2
#define DATA_Y    ymm3
#define VARIANCE  ymm4
#define COVARNCE  ymm5

	.globl	cal_variance_covariance_simd
cal_variance_covariance_simd:
	push 	%rbp
	mov 	%rsp, %rbp
  sub   $20,  %rsp

  mov   %edx, 0x10(%rsp)

  vpbroadcastw  0x10(%rsp), %xmm0
  vpmovzxwd     %xmm0,      %MEAN_X 
  vpbroadcastw  0x12(%rsp), %xmm1
  vpmovzxwd     %xmm1,      %MEAN_Y 

  vpxor %VARIANCE, %VARIANCE, %VARIANCE
  vpxor %COVARNCE, %COVARNCE, %COVARNCE

loop_cal:
  cmp   $0, %ecx
  jz    exit_cal

  vmovdqa     (%rdi),   %xmm2
  add         $16,      %rdi
  vpmovzxwd   %xmm2,    %DATA_X 
  vpsubd      %MEAN_X,  %DATA_X,    %DATA_X
  vpmulld     %DATA_X,  %DATA_X,    %ymm6

  vextracti128  $1,     %ymm6, %xmm7
  vpmovsxdq     %xmm6,  %ymm6
  vpmovsxdq     %xmm7,  %ymm7
  vpaddq        %ymm6,    %VARIANCE,  %VARIANCE
  vpaddq        %ymm7,    %VARIANCE,  %VARIANCE

  vmovdqa     (%rsi),   %xmm3
  add         $16,      %rsi
  vpmovzxwd   %xmm3,    %DATA_Y 
  vpsubd      %MEAN_Y,  %DATA_Y,    %DATA_Y
  vpmulld     %DATA_X,  %DATA_Y,    %ymm6

  vextracti128  $1,     %ymm6, %xmm7
  vpmovsxdq     %xmm6,  %ymm6
  vpmovsxdq     %xmm7,  %ymm7
  vpaddq        %ymm6,    %COVARNCE,  %COVARNCE
  vpaddq        %ymm7,    %COVARNCE,  %COVARNCE

  sub   $8,  %ecx
  jmp   loop_cal

exit_cal:

  mov   $0, %rax
  vextracti128  $1,  %ymm4, %xmm1 /*VARIANCE*/
  vpextrq   $0,    %xmm4, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm4, %rdx
  add       %rdx,  %rax
  vpextrq   $0,    %xmm1, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm1, %rdx
  add       %rdx,  %rax
  mov       %rax, (%r8)

  mov   $0, %rax
  vextracti128  $1,  %ymm5, %xmm1 /*COVARNCE*/
  vpextrq   $0,    %xmm5, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm5, %rdx
  add       %rdx,  %rax
  vpextrq   $0,    %xmm1, %rdx
  add       %rdx,  %rax
  vpextrq   $1,    %xmm1, %rdx
  add       %rdx,  %rax
  mov       %rax, (%r9)

	add 	$20, %rsp
	pop 	%rbp
	ret
