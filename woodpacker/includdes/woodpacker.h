#ifndef WOODPACKER_H
#define WOODPACKER_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../includdes/libft/libft.h"

typedef struct s_elf_ctx
{
	void		*base;
	size_t		file_size;
	Elf64_Ehdr	*ehdr;
	Elf64_Phdr	*phdrs;
	Elf64_Phdr	*text_phdr;
}	t_elf_ctx;

typedef struct s_elf32_ctx
{
	void		*base;
	size_t		file_size;
	Elf32_Ehdr	*ehdr;
	Elf32_Phdr	*phdrs;
	Elf32_Phdr	*text_phdr;
}	t_elf32_ctx;

int	parse_elf64(int fd, t_elf_ctx *ctx);
int	parse_elf32(int fd, t_elf32_ctx *ctx);
void	cleanup_elf_ctx(t_elf_ctx *ctx);
void	cleanup_elf32_ctx(t_elf32_ctx *ctx);

#endif