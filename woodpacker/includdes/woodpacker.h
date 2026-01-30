#ifndef WOODPACKER_H
#define WOODPACKER_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../includdes/libft/libft.h"

#define WOODY_STUB_SIZE 512
#define WOODY_KEY_SIZE 16

typedef struct s_elf_ctx
{
	void		*base;
	size_t		file_size;
	Elf64_Ehdr	*ehdr;
	Elf64_Phdr	*phdrs;
	Elf64_Phdr	*text_phdr;
	size_t		text_off;
	size_t		text_size;
	Elf64_Addr	text_vaddr;
	int			entry_in_text;
	size_t		inject_off;
	Elf64_Addr	inject_vaddr;
	size_t		inject_size;
}	t_elf_ctx;

typedef struct s_elf32_ctx
{
	void		*base;
	size_t		file_size;
	Elf32_Ehdr	*ehdr;
	Elf32_Phdr	*phdrs;
	Elf32_Phdr	*text_phdr;
	size_t		text_off;
	size_t		text_size;
	Elf32_Addr	text_vaddr;
	int			entry_in_text;
	size_t		inject_off;
	Elf32_Addr	inject_vaddr;
	size_t		inject_size;
}	t_elf32_ctx;

int	parse_elf64(int fd, t_elf_ctx *ctx);
int	parse_elf32(int fd, t_elf32_ctx *ctx);
void	cleanup_elf_ctx(t_elf_ctx *ctx);
void	cleanup_elf32_ctx(t_elf32_ctx *ctx);
int	select_injection_site64(t_elf_ctx *ctx, size_t stub_size);
#ifdef BONUS
int	select_injection_site32(t_elf32_ctx *ctx, size_t stub_size);
#endif
int	pack_elf64(int fd, t_elf_ctx *ctx);
#ifdef BONUS
int	pack_elf32(int fd, t_elf32_ctx *ctx);
#endif

const char	*get_pack_error(void);
const char	*get_parse_error(void);

#endif