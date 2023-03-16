	.file	"boot.c"
	.text
	.type	inb, @function
inb:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, %edx
#APP
# 8 "boot.h" 1
	inb %dx, %al
# 0 "" 2
#NO_APP
	movb	%al, -1(%rbp)
	movzbl	-1(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	inb, .-inb
	.type	inl, @function
inl:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movl	-20(%rbp), %eax
	movl	%eax, %edx
#APP
# 14 "boot.h" 1
	inl %dx, %eax
# 0 "" 2
#NO_APP
	movl	%eax, -4(%rbp)
	movl	-4(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	inl, .-inl
	.type	outb, @function
outb:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -4(%rbp)
	movl	%esi, %eax
	movb	%al, -8(%rbp)
	movl	-4(%rbp), %eax
	movl	%eax, %edx
	movzbl	-8(%rbp), %eax
#APP
# 19 "boot.h" 1
	outb %al, %dx
# 0 "" 2
#NO_APP
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	outb, .-outb
	.type	wait_disk, @function
wait_disk:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	nop
.L7:
	movl	$503, %edi
	call	inb
	movzbl	%al, %eax
	andl	$192, %eax
	cmpl	$64, %eax
	jne	.L7
	nop
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	wait_disk, .-wait_disk
	.type	read_disk, @function
read_disk:
.LFB4:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%rbx
	subq	$32, %rsp
	.cfi_offset 3, -24
	movq	%rdi, -32(%rbp)
	movl	%esi, -36(%rbp)
	movl	$0, %eax
	call	wait_disk
	movl	$1, %esi
	movl	$498, %edi
	call	outb
	movl	-36(%rbp), %eax
	movzbl	%al, %eax
	movl	%eax, %esi
	movl	$499, %edi
	call	outb
	movl	-36(%rbp), %eax
	sarl	$8, %eax
	movzbl	%al, %eax
	movl	%eax, %esi
	movl	$500, %edi
	call	outb
	movl	-36(%rbp), %eax
	sarl	$16, %eax
	movzbl	%al, %eax
	movl	%eax, %esi
	movl	$501, %edi
	call	outb
	movl	-36(%rbp), %eax
	sarl	$24, %eax
	orl	$-32, %eax
	movzbl	%al, %eax
	movl	%eax, %esi
	movl	$502, %edi
	call	outb
	movl	$32, %esi
	movl	$503, %edi
	call	outb
	movl	$0, %eax
	call	wait_disk
	movl	$0, -12(%rbp)
	jmp	.L9
.L10:
	movl	-12(%rbp), %eax
	cltq
	leaq	0(,%rax,4), %rdx
	movq	-32(%rbp), %rax
	leaq	(%rdx,%rax), %rbx
	movl	$496, %edi
	call	inl
	movl	%eax, (%rbx)
	addl	$1, -12(%rbp)
.L9:
	cmpl	$127, -12(%rbp)
	jle	.L10
	nop
	nop
	addq	$32, %rsp
	popq	%rbx
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE4:
	.size	read_disk, .-read_disk
	.type	copy_from_disk, @function
copy_from_disk:
.LFB5:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movq	-24(%rbp), %rax
	movl	%eax, -12(%rbp)
	movq	-24(%rbp), %rax
	movl	%eax, %edx
	movl	-28(%rbp), %eax
	addl	%edx, %eax
	movl	%eax, -4(%rbp)
	movl	-32(%rbp), %eax
	leal	511(%rax), %edx
	testl	%eax, %eax
	cmovs	%edx, %eax
	sarl	$9, %eax
	movl	%eax, -8(%rbp)
	jmp	.L12
.L13:
	movl	-8(%rbp), %eax
	movl	-12(%rbp), %edx
	movl	%eax, %esi
	movq	%rdx, %rdi
	call	read_disk
	addl	$512, -12(%rbp)
	addl	$1, -8(%rbp)
.L12:
	movl	-12(%rbp), %eax
	cmpl	-4(%rbp), %eax
	jb	.L13
	nop
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE5:
	.size	copy_from_disk, .-copy_from_disk
	.type	memcpy, @function
memcpy:
.LFB9:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -40(%rbp)
	movq	%rsi, -48(%rbp)
	movl	%edx, -52(%rbp)
	movq	-40(%rbp), %rax
	movq	%rax, -24(%rbp)
	movl	-52(%rbp), %edx
	movq	-24(%rbp), %rax
	addq	%rdx, %rax
	movq	%rax, -8(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -16(%rbp)
	jmp	.L15
.L16:
	movq	-16(%rbp), %rdx
	leaq	1(%rdx), %rax
	movq	%rax, -16(%rbp)
	movq	-24(%rbp), %rax
	leaq	1(%rax), %rcx
	movq	%rcx, -24(%rbp)
	movzbl	(%rdx), %edx
	movb	%dl, (%rax)
.L15:
	movq	-24(%rbp), %rax
	cmpq	-8(%rbp), %rax
	jb	.L16
	nop
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE9:
	.size	memcpy, .-memcpy
	.type	memset, @function
memset:
.LFB10:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -24(%rbp)
	movl	%esi, -28(%rbp)
	movl	%edx, -32(%rbp)
	movq	-24(%rbp), %rax
	movq	%rax, -16(%rbp)
	movl	-32(%rbp), %edx
	movq	-16(%rbp), %rax
	addq	%rdx, %rax
	movq	%rax, -8(%rbp)
	jmp	.L18
.L19:
	movq	-16(%rbp), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, -16(%rbp)
	movl	-28(%rbp), %edx
	movb	%dl, (%rax)
.L18:
	movq	-16(%rbp), %rax
	cmpq	-8(%rbp), %rax
	jb	.L19
	nop
	nop
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	memset, .-memset
	.comm	happy,4,4
	.globl	load_kernel
	.type	load_kernel, @function
load_kernel:
.LFB11:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movq	$32768, -16(%rbp)
	movq	-16(%rbp), %rax
	movl	$512, %edx
	movl	$130560, %esi
	movq	%rax, %rdi
	call	copy_from_disk
	movq	-16(%rbp), %rax
	movl	%eax, %edx
	movq	-16(%rbp), %rax
	movl	28(%rax), %eax
	addl	%edx, %eax
	movl	%eax, %eax
	movq	%rax, -24(%rbp)
	movq	-16(%rbp), %rax
	movzwl	44(%rax), %eax
	movzwl	%ax, %eax
	salq	$5, %rax
	movq	%rax, %rdx
	movq	-24(%rbp), %rax
	addq	%rdx, %rax
	movq	%rax, -8(%rbp)
	jmp	.L21
.L23:
	movq	-24(%rbp), %rax
	movl	(%rax), %eax
	cmpl	$1, %eax
	jne	.L22
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	movq	-16(%rbp), %rdx
	movl	%edx, %ecx
	movq	-24(%rbp), %rdx
	movl	4(%rdx), %edx
	addl	%ecx, %edx
	movl	%edx, %edx
	movq	%rdx, %rsi
	movq	-24(%rbp), %rdx
	movl	8(%rdx), %edx
	movl	%edx, %edx
	movq	%rdx, %rcx
	movl	%eax, %edx
	movq	%rcx, %rdi
	call	memcpy
	movq	-24(%rbp), %rax
	movl	20(%rax), %edx
	movq	-24(%rbp), %rax
	movl	16(%rax), %eax
	subl	%eax, %edx
	movl	%edx, %eax
	movq	-24(%rbp), %rdx
	movl	8(%rdx), %ecx
	movq	-24(%rbp), %rdx
	movl	16(%rdx), %edx
	addl	%ecx, %edx
	movl	%edx, %edx
	movq	%rdx, %rcx
	movl	%eax, %edx
	movl	$0, %esi
	movq	%rcx, %rdi
	call	memset
.L22:
	addq	$32, -24(%rbp)
.L21:
	movq	-24(%rbp), %rax
	cmpq	-8(%rbp), %rax
	jb	.L23
	movq	-16(%rbp), %rax
	movl	24(%rax), %eax
	movl	%eax, -28(%rbp)
	movl	-28(%rbp), %eax
	movq	%rax, %rdx
	movl	$0, %eax
	call	*%rdx
	movl	$1155, happy(%rip)
	nop
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE11:
	.size	load_kernel, .-load_kernel
	.ident	"GCC: (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	 1f - 0f
	.long	 4f - 1f
	.long	 5
0:
	.string	 "GNU"
1:
	.align 8
	.long	 0xc0000002
	.long	 3f - 2f
2:
	.long	 0x3
3:
	.align 8
4:
