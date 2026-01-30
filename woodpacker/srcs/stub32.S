.code32
.text
.global woody_stub32
.global woody_stub32_end
.global woody_stub32_meta_orig_entry
.global woody_stub32_meta_text_vaddr
.global woody_stub32_meta_text_size
.global woody_stub32_meta_key
.global woody_stub32_msg

.equ WOODY_KEY_SIZE, 16

woody_stub32:
	push %eax
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp

	call 1f
1:
	pop %ebp

	mov $4, %eax
	mov $1, %ebx
	lea woody_stub32_msg-1b(%ebp), %ecx
	mov $woody_stub32_msg_len, %edx
	int $0x80

	mov woody_stub32_meta_text_vaddr-1b(%ebp), %ebx
	mov %ebx, %ecx
	and $-4096, %ebx
	sub %ebx, %ecx
	mov woody_stub32_meta_text_size-1b(%ebp), %esi
	add %ecx, %esi
	add $4095, %esi
	and $-4096, %esi
	mov $125, %eax
	mov %ebx, %ebx
	mov %esi, %ecx
	mov $7, %edx
	int $0x80

	xor %eax, %eax
	lea woody_stub32_meta_key-1b(%ebp), %edi
	mov $WOODY_KEY_SIZE, %ecx
.Lseed32:
	movzbl (%edi), %edx
	lea (%eax,%eax,4), %eax
	add %edx, %eax
	add $1, %eax
	inc %edi
	loop .Lseed32

	mov woody_stub32_meta_text_vaddr-1b(%ebp), %edi
	mov woody_stub32_meta_text_size-1b(%ebp), %esi
	xor %ecx, %ecx
.Ldec32:
	cmp %esi, %ecx
	jae .Ldec32_done
	mov %eax, %edx
	shl $13, %edx
	xor %edx, %eax
	mov %eax, %edx
	shr $17, %edx
	xor %edx, %eax
	mov %eax, %edx
	shl $5, %edx
	xor %edx, %eax
	mov %ecx, %ebx
	and $(WOODY_KEY_SIZE - 1), %ebx
	movzbl woody_stub32_meta_key-1b(%ebp,%ebx,1), %edx
	xor %al, %dl
	xor %dl, (%edi,%ecx,1)
	inc %ecx
	jmp .Ldec32
.Ldec32_done:
	mov woody_stub32_meta_text_vaddr-1b(%ebp), %ebx
	mov %ebx, %ecx
	and $-4096, %ebx
	sub %ebx, %ecx
	mov woody_stub32_meta_text_size-1b(%ebp), %esi
	add %ecx, %esi
	add $4095, %esi
	and $-4096, %esi
	mov $125, %eax
	mov %ebx, %ebx
	mov %esi, %ecx
	mov $5, %edx
	int $0x80

	mov woody_stub32_meta_orig_entry-1b(%ebp), %eax
	pop %ebp
	pop %edi
	pop %esi
	pop %edx
	pop %ecx
	pop %ebx
	add $4, %esp
	jmp *%eax

woody_stub32_msg:
	.ascii "....WOODY....\n"
woody_stub32_msg_len = . - woody_stub32_msg

	.balign 4
woody_stub32_meta_orig_entry:
	.long 0
woody_stub32_meta_text_vaddr:
	.long 0
woody_stub32_meta_text_size:
	.long 0
woody_stub32_meta_key:
	.byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

woody_stub32_end:
