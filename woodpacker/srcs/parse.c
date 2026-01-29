#include "../includdes/woodpacker.h"

static int	is_elf_magic(const unsigned char *e_ident)
{
	static const unsigned char	magic[EI_NIDENT] = {
		0x7f, 'E', 'L', 'F', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	return (ft_memcmp(e_ident, magic, EI_MAG3 + 1) == 0);
}

void	cleanup_elf_ctx(t_elf_ctx *ctx)
{
	if (!ctx)
		return ;
	if (ctx->base && ctx->file_size > 0)
		munmap(ctx->base, ctx->file_size);
	ft_bzero(ctx, sizeof(*ctx));
}

void	cleanup_elf32_ctx(t_elf32_ctx *ctx)
{
	if (!ctx)
		return ;
	if (ctx->base && ctx->file_size > 0)
		munmap(ctx->base, ctx->file_size);
	ft_bzero(ctx, sizeof(*ctx));
}

int	parse_elf64(int fd, t_elf_ctx *ctx)
{
	struct stat	st;
	size_t		phdr_table_size;
	int			idx;

	if (!ctx)
		return (-1);
	ft_bzero(ctx, sizeof(*ctx));
	if (fstat(fd, &st) < 0)
	{
		perror("fstat");
		return (-1);
	}
	if (st.st_size < (off_t)sizeof(Elf64_Ehdr))
		return (-1);
	ctx->file_size = (size_t)st.st_size;
	ctx->base = mmap(NULL, ctx->file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->base == MAP_FAILED)
	{
		perror("mmap");
		ctx->base = NULL;
		return (-1);
	}
	ctx->ehdr = (Elf64_Ehdr *)ctx->base;
	if (!is_elf_magic(ctx->ehdr->e_ident))
		return (-1);
	if (ctx->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
		return (-1);
	if (ctx->ehdr->e_machine != EM_X86_64)
		return (-1);
	if (ctx->ehdr->e_phoff == 0 || ctx->ehdr->e_phentsize != sizeof(Elf64_Phdr))
		return (-1);
	phdr_table_size = (size_t)ctx->ehdr->e_phnum * sizeof(Elf64_Phdr);
	if (ctx->ehdr->e_phoff + phdr_table_size > ctx->file_size)
		return (-1);
	ctx->phdrs = (Elf64_Phdr *)((char *)ctx->base + ctx->ehdr->e_phoff);
	idx = 0;
	while (idx < ctx->ehdr->e_phnum)
	{
		if (ctx->phdrs[idx].p_type == PT_LOAD
			&& (ctx->phdrs[idx].p_flags & PF_X))
			ctx->text_phdr = &ctx->phdrs[idx];
		idx++;
	}
	if (!ctx->text_phdr)
		return (-1);
	return (0);
}

int	select_injection_site64(t_elf_ctx *ctx, size_t stub_size)
{
	size_t		file_off;
	Elf64_Addr	vaddr;
	size_t		next_off;
	int			idx;

	if (!ctx || !ctx->text_phdr)
		return (-1);
	if (stub_size == 0)
		return (-1);
	file_off = (size_t)ctx->text_phdr->p_offset
		+ (size_t)ctx->text_phdr->p_filesz;
	if (file_off > ctx->file_size)
		return (-1);
	next_off = (size_t)-1;
	idx = 0;
	while (idx < ctx->ehdr->e_phnum)
	{
		size_t	seg_off;

		seg_off = (size_t)ctx->phdrs[idx].p_offset;
		if (seg_off > file_off && seg_off < next_off)
			next_off = seg_off;
		idx++;
	}
	if (next_off != (size_t)-1 && file_off + stub_size > next_off)
		return (-1);
	vaddr = (Elf64_Addr)ctx->text_phdr->p_vaddr
		+ (Elf64_Addr)ctx->text_phdr->p_memsz;
	ctx->inject_off = file_off;
	ctx->inject_vaddr = vaddr;
	ctx->inject_size = stub_size;
	return (0);
}

int	parse_elf32(int fd, t_elf32_ctx *ctx)
{
	struct stat	st;
	size_t		phdr_table_size;
	int			idx;

	if (!ctx)
		return (-1);
	ft_bzero(ctx, sizeof(*ctx));
	if (fstat(fd, &st) < 0)
	{
		perror("fstat");
		return (-1);
	}
	if (st.st_size < (off_t)sizeof(Elf32_Ehdr))
		return (-1);
	ctx->file_size = (size_t)st.st_size;
	ctx->base = mmap(NULL, ctx->file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->base == MAP_FAILED)
	{
		perror("mmap");
		ctx->base = NULL;
		return (-1);
	}
	ctx->ehdr = (Elf32_Ehdr *)ctx->base;
	if (!is_elf_magic(ctx->ehdr->e_ident))
		return (-1);
	if (ctx->ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		return (-1);
	if (ctx->ehdr->e_machine != EM_386)
		return (-1);
	if (ctx->ehdr->e_phoff == 0 || ctx->ehdr->e_phentsize != sizeof(Elf32_Phdr))
		return (-1);
	phdr_table_size = (size_t)ctx->ehdr->e_phnum * sizeof(Elf32_Phdr);
	if (ctx->ehdr->e_phoff + phdr_table_size > ctx->file_size)
		return (-1);
	ctx->phdrs = (Elf32_Phdr *)((char *)ctx->base + ctx->ehdr->e_phoff);
	idx = 0;
	while (idx < ctx->ehdr->e_phnum)
	{
		if (ctx->phdrs[idx].p_type == PT_LOAD
			&& (ctx->phdrs[idx].p_flags & PF_X))
			ctx->text_phdr = &ctx->phdrs[idx];
		idx++;
	}
	if (!ctx->text_phdr)
		return (-1);
	return (0);
}

#ifdef BONUS
int	select_injection_site32(t_elf32_ctx *ctx, size_t stub_size)
{
	size_t		file_off;
	Elf32_Addr	vaddr;
	size_t		next_off;
	int			idx;

	if (!ctx || !ctx->text_phdr)
		return (-1);
	if (stub_size == 0)
		return (-1);
	file_off = (size_t)ctx->text_phdr->p_offset
		+ (size_t)ctx->text_phdr->p_filesz;
	if (file_off > ctx->file_size)
		return (-1);
	next_off = (size_t)-1;
	idx = 0;
	while (idx < ctx->ehdr->e_phnum)
	{
		size_t	seg_off;

		seg_off = (size_t)ctx->phdrs[idx].p_offset;
		if (seg_off > file_off && seg_off < next_off)
			next_off = seg_off;
		idx++;
	}
	if (next_off != (size_t)-1 && file_off + stub_size > next_off)
		return (-1);
	vaddr = (Elf32_Addr)ctx->text_phdr->p_vaddr
		+ (Elf32_Addr)ctx->text_phdr->p_memsz;
	ctx->inject_off = file_off;
	ctx->inject_vaddr = vaddr;
	ctx->inject_size = stub_size;
	return (0);
}
#endif