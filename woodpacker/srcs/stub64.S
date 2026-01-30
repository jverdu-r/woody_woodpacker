.text
.global woody_stub64
.global woody_stub64_end
.global woody_stub64_meta_orig_entry
.global woody_stub64_meta_text_vaddr
.global woody_stub64_meta_text_size
.global woody_stub64_meta_key
.global woody_stub64_msg

.equ WOODY_KEY_SIZE, 16

woody_stub64:
	push %rdi
	push %rsi
	push %rdx
	push %rbx
	push %rcx
	push %rax
	push %rbp

	call 1f
1:
	pop %rbp

	mov $1, %rax
	mov $1, %rdi
	lea woody_stub64_msg-1b(%rbp), %rsi
	mov $woody_stub64_msg_len, %edx
	syscall

	mov woody_stub64_meta_text_vaddr-1b(%rbp), %rbx
	mov %rbx, %rcx
	and $-4096, %rbx
	sub %rbx, %rcx
	mov woody_stub64_meta_text_size-1b(%rbp), %rdx
	add %rcx, %rdx
	add $4095, %rdx
	and $-4096, %rdx
	mov %rbx, %rdi
	mov %rdx, %rsi
	mov $7, %edx
	mov $10, %eax
	syscall

	xor %eax, %eax
	lea woody_stub64_meta_key-1b(%rbp), %rbx
	mov $WOODY_KEY_SIZE, %ecx
.Lseed64:
	movzbl (%rbx), %edx
	lea (%eax,%eax,4), %eax
	add %edx, %eax
	add $1, %eax
	inc %rbx
	loop .Lseed64
	lea woody_stub64_meta_key-1b(%rbp), %rbx

	mov woody_stub64_meta_text_vaddr-1b(%rbp), %rdi
	mov woody_stub64_meta_text_size-1b(%rbp), %rsi
	xor %rcx, %rcx
.Ldec64:
	cmp %rsi, %rcx
	jae .Ldec64_done
	mov %eax, %edx
	shl $13, %edx
	xor %edx, %eax
	mov %eax, %edx
	shr $17, %edx
	xor %edx, %eax
	mov %eax, %edx
	shl $5, %edx
	xor %edx, %eax
	mov %rcx, %r8
	and $(WOODY_KEY_SIZE - 1), %r8
	movzbl (%rbx,%r8,1), %edx
	xor %al, %dl
	xor %dl, (%rdi,%rcx,1)
	inc %rcx
	jmp .Ldec64
.Ldec64_done:
	mov woody_stub64_meta_text_vaddr-1b(%rbp), %rbx
	mov %rbx, %rcx
	and $-4096, %rbx
	sub %rbx, %rcx
	mov woody_stub64_meta_text_size-1b(%rbp), %rdx
	add %rcx, %rdx
	add $4095, %rdx
	and $-4096, %rdx
	mov %rbx, %rdi
	mov %rdx, %rsi
	mov $5, %edx
	mov $10, %eax
	syscall

	mov woody_stub64_meta_orig_entry-1b(%rbp), %r11
	pop %rbp
	pop %rax
	pop %rcx
	pop %rbx
	pop %rdx
	pop %rsi
	pop %rdi
	jmp *%r11

woody_stub64_msg:
	.ascii "....WOODY....\n"
woody_stub64_msg_len = . - woody_stub64_msg

	.balign 8
woody_stub64_meta_orig_entry:
	.quad 0
woody_stub64_meta_text_vaddr:
	.quad 0
woody_stub64_meta_text_size:
	.quad 0
woody_stub64_meta_key:
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

woody_stub64_end:
