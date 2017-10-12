	.file	"sqrt.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"z = %f"
	.text
	.globl	test_sqrt
	.type	test_sqrt, @function
test_sqrt:
.LFB27:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movsd	.LC0(%rip), %xmm0
	movl	$.LC1, %esi
	movl	$1, %edi
	movl	$1, %eax
	call	__printf_chk
	movss	.LC2(%rip), %xmm0
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE27:
	.size	test_sqrt, .-test_sqrt
	.globl	main
	.type	main, @function
main:
.LFB28:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	xorpd	%xmm0, %xmm0
	movl	$.LC1, %esi
	movl	$1, %edi
	movl	$1, %eax
	call	__printf_chk
	movl	$0, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE28:
	.size	main, .-main
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC0:
	.long	1610612736
	.long	1073127582
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC2:
	.long	1068827891
	.ident	"GCC: (Ubuntu 4.8.2-19ubuntu1) 4.8.2"
	.section	.note.GNU-stack,"",@progbits
