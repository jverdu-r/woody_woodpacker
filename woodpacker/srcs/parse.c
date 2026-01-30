#include "../includdes/woodpacker.h"

static const char	*g_parse_error = "unknown";

const char	*get_parse_error(void)
{
	return (g_parse_error);
}

#define RETURN_ERR(msg) do { g_parse_error = (msg); return (-1); } while (0)

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
	size_t		shdr_table_size;
	Elf64_Shdr	*shdrs;
	Elf64_Shdr	*shstr;
	const char	*shstrtab;
	int			idx;

	if (!ctx)
		RETURN_ERR("null context");
	ft_bzero(ctx, sizeof(*ctx));
	if (fstat(fd, &st) < 0)
	{
		perror("fstat");
		RETURN_ERR("fstat failed");
	}
	if (st.st_size < (off_t)sizeof(Elf64_Ehdr))
		RETURN_ERR("file too small");
	ctx->file_size = (size_t)st.st_size;
	ctx->base = mmap(NULL, ctx->file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->base == MAP_FAILED)
	{
		perror("mmap");
		ctx->base = NULL;
		RETURN_ERR("mmap failed");
	}
	ctx->ehdr = (Elf64_Ehdr *)ctx->base;
	if (!is_elf_magic(ctx->ehdr->e_ident))
		RETURN_ERR("invalid ELF magic");
	if (ctx->ehdr->e_ident[EI_CLASS] != ELFCLASS64)
		RETURN_ERR("not ELF64");
	if (ctx->ehdr->e_machine != EM_X86_64)
		RETURN_ERR("not x86_64");
	if (ctx->ehdr->e_phoff == 0 || ctx->ehdr->e_phentsize != sizeof(Elf64_Phdr))
		RETURN_ERR("invalid program header table");
	phdr_table_size = (size_t)ctx->ehdr->e_phnum * sizeof(Elf64_Phdr);
	if (ctx->ehdr->e_phoff + phdr_table_size > ctx->file_size)
		RETURN_ERR("program headers out of bounds");
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
		RETURN_ERR("no executable PT_LOAD");
	if (ctx->ehdr->e_shoff == 0
		|| ctx->ehdr->e_shentsize != sizeof(Elf64_Shdr)
		|| ctx->ehdr->e_shnum == 0
		|| ctx->ehdr->e_shstrndx == SHN_UNDEF)
		RETURN_ERR("invalid section headers");
	shdr_table_size = (size_t)ctx->ehdr->e_shnum * sizeof(Elf64_Shdr);
	if (ctx->ehdr->e_shoff + shdr_table_size > ctx->file_size)
		RETURN_ERR("section headers out of bounds");
	shdrs = (Elf64_Shdr *)((char *)ctx->base + ctx->ehdr->e_shoff);
	if (ctx->ehdr->e_shstrndx >= ctx->ehdr->e_shnum)
		RETURN_ERR("invalid shstrndx");
	shstr = &shdrs[ctx->ehdr->e_shstrndx];
	if (shstr->sh_offset + shstr->sh_size > ctx->file_size)
		RETURN_ERR("shstrtab out of bounds");
	shstrtab = (const char *)ctx->base + shstr->sh_offset;
	idx = 0;
	while (idx < ctx->ehdr->e_shnum)
	{
		const char	*name;

		name = shstrtab + shdrs[idx].sh_name;
		if (ft_strncmp(name, ".text", 6) == 0)
		{
			ctx->text_off = (size_t)shdrs[idx].sh_offset;
			ctx->text_size = (size_t)shdrs[idx].sh_size;
			ctx->text_vaddr = (Elf64_Addr)shdrs[idx].sh_addr;
			break ;
		}
		idx++;
	}
	if (ctx->text_size == 0)
		RETURN_ERR(".text not found");
	ctx->entry_in_text = (ctx->ehdr->e_entry >= ctx->text_vaddr
		&& ctx->ehdr->e_entry < ctx->text_vaddr + ctx->text_size);
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
		+ (Elf64_Addr)ctx->text_phdr->p_filesz;
	ctx->inject_off = file_off;
	ctx->inject_vaddr = vaddr;
	ctx->inject_size = stub_size;
	return (0);
}

int	parse_elf32(int fd, t_elf32_ctx *ctx)
{
	struct stat	st;
	size_t		phdr_table_size;
	size_t		shdr_table_size;
	Elf32_Shdr	*shdrs;
	Elf32_Shdr	*shstr;
	const char	*shstrtab;
	int			idx;

	if (!ctx)
		RETURN_ERR("null context");
	ft_bzero(ctx, sizeof(*ctx));
	if (fstat(fd, &st) < 0)
	{
		perror("fstat");
		RETURN_ERR("fstat failed");
	}
	if (st.st_size < (off_t)sizeof(Elf32_Ehdr))
		RETURN_ERR("file too small");
	ctx->file_size = (size_t)st.st_size;
	ctx->base = mmap(NULL, ctx->file_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->base == MAP_FAILED)
	{
		perror("mmap");
		ctx->base = NULL;
		RETURN_ERR("mmap failed");
	}
	ctx->ehdr = (Elf32_Ehdr *)ctx->base;
	if (!is_elf_magic(ctx->ehdr->e_ident))
		RETURN_ERR("invalid ELF magic");
	if (ctx->ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		RETURN_ERR("not ELF32");
	if (ctx->ehdr->e_machine != EM_386)
		RETURN_ERR("not i386");
	if (ctx->ehdr->e_phoff == 0 || ctx->ehdr->e_phentsize != sizeof(Elf32_Phdr))
		RETURN_ERR("invalid program header table");
	phdr_table_size = (size_t)ctx->ehdr->e_phnum * sizeof(Elf32_Phdr);
	if (ctx->ehdr->e_phoff + phdr_table_size > ctx->file_size)
		RETURN_ERR("program headers out of bounds");
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
		RETURN_ERR("no executable PT_LOAD");
	if (ctx->ehdr->e_shoff == 0
		|| ctx->ehdr->e_shentsize != sizeof(Elf32_Shdr)
		|| ctx->ehdr->e_shnum == 0
		|| ctx->ehdr->e_shstrndx == SHN_UNDEF)
		RETURN_ERR("invalid section headers");
	shdr_table_size = (size_t)ctx->ehdr->e_shnum * sizeof(Elf32_Shdr);
	if (ctx->ehdr->e_shoff + shdr_table_size > ctx->file_size)
		RETURN_ERR("section headers out of bounds");
	shdrs = (Elf32_Shdr *)((char *)ctx->base + ctx->ehdr->e_shoff);
	if (ctx->ehdr->e_shstrndx >= ctx->ehdr->e_shnum)
		RETURN_ERR("invalid shstrndx");
	shstr = &shdrs[ctx->ehdr->e_shstrndx];
	if (shstr->sh_offset + shstr->sh_size > ctx->file_size)
		RETURN_ERR("shstrtab out of bounds");
	shstrtab = (const char *)ctx->base + shstr->sh_offset;
	idx = 0;
	while (idx < ctx->ehdr->e_shnum)
	{
		const char	*name;

		name = shstrtab + shdrs[idx].sh_name;
		if (ft_strncmp(name, ".text", 6) == 0)
		{
			ctx->text_off = (size_t)shdrs[idx].sh_offset;
			ctx->text_size = (size_t)shdrs[idx].sh_size;
			ctx->text_vaddr = (Elf32_Addr)shdrs[idx].sh_addr;
			break ;
		}
		idx++;
	}
	if (ctx->text_size == 0)
		RETURN_ERR(".text not found");
	ctx->entry_in_text = (ctx->ehdr->e_entry >= ctx->text_vaddr
		&& ctx->ehdr->e_entry < ctx->text_vaddr + ctx->text_size);
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
		+ (Elf32_Addr)ctx->text_phdr->p_filesz;
	ctx->inject_off = file_off;
	ctx->inject_vaddr = vaddr;
	ctx->inject_size = stub_size;
	return (0);
}
#endif