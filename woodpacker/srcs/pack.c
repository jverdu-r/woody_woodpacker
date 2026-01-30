#include "../includdes/woodpacker.h"
#include <stdint.h>

#define STUB_CODE_SIZE 12

static const char	*g_pack_error = "unknown";

const char	*get_pack_error(void)
{
	return (g_pack_error);
}

extern unsigned char	woody_stub64[];
extern unsigned char	woody_stub64_end[];
extern unsigned char	woody_stub64_meta_orig_entry[];
extern unsigned char	woody_stub64_meta_text_vaddr[];
extern unsigned char	woody_stub64_meta_text_size[];
extern unsigned char	woody_stub64_meta_key[];

#ifdef BONUS
extern unsigned char	woody_stub32[];
extern unsigned char	woody_stub32_end[];
extern unsigned char	woody_stub32_meta_orig_entry[];
extern unsigned char	woody_stub32_meta_text_vaddr[];
extern unsigned char	woody_stub32_meta_text_size[];
extern unsigned char	woody_stub32_meta_key[];
#endif

typedef struct s_woody_meta64
{
	uint64_t	orig_entry;
	uint64_t	text_vaddr;
	uint64_t	text_size;
	unsigned char	key[WOODY_KEY_SIZE];
} 	t_woody_meta64;

typedef struct s_woody_meta32
{
	uint32_t	orig_entry;
	uint32_t	text_vaddr;
	uint32_t	text_size;
	unsigned char	key[WOODY_KEY_SIZE];
} 	t_woody_meta32;

static int	read_full(int fd, void *buf, size_t size)
{
	size_t	read_total;
	ssize_t	chunk;

	read_total = 0;
	while (read_total < size)
	{
		chunk = read(fd, (unsigned char *)buf + read_total, size - read_total);
		if (chunk <= 0)
			return (-1);
		read_total += (size_t)chunk;
	}
	return (0);
}

static int	write_full(int fd, const void *buf, size_t size)
{
	size_t	written;
	ssize_t	chunk;

	written = 0;
	while (written < size)
	{
		chunk = write(fd, (const unsigned char *)buf + written,
			size - written);
		if (chunk <= 0)
			return (-1);
		written += (size_t)chunk;
	}
	return (0);
}

static uint32_t	seed_from_key(const unsigned char *key)
{
	uint32_t	seed;
	int		idx;

	seed = 0;
	idx = 0;
	while (idx < WOODY_KEY_SIZE)
	{
		seed = (seed * 5u) + (uint32_t)key[idx] + 1u;
		idx++;
	}
	return (seed);
}

static void	encrypt_region(unsigned char *buf, size_t off, size_t len,
		const unsigned char *key)
{
	size_t	idx;
	uint32_t	state;
	uint8_t	k;

	state = seed_from_key(key);
	idx = 0;
	while (idx < len)
	{
		state ^= (state << 13);
		state ^= (state >> 17);
		state ^= (state << 5);
		k = (uint8_t)(key[idx % WOODY_KEY_SIZE] ^ (uint8_t)state);
		buf[off + idx] ^= k;
		idx++;
	}
}

static int	generate_key(unsigned char *key)
{
	int	fd;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return (-1);
	if (read_full(fd, key, WOODY_KEY_SIZE) < 0)
	{
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

static void	print_key(const unsigned char *key)
{
	int	idx;

	printf("key_value: ");
	idx = 0;
	while (idx < WOODY_KEY_SIZE)
	{
		printf("%02X", key[idx]);
		idx++;
	}
	printf("\n");
}

static int	apply_injection64(unsigned char **buf, size_t *size,
		const t_elf_ctx *ctx, const unsigned char *key)
{
	t_woody_meta64	meta;
	size_t		new_size;
	size_t		stub_len;
	unsigned char	*dst;

	if (!buf || !*buf || !size || !ctx || !ctx->ehdr || !ctx->text_phdr)
		return (-1);
	if (ctx->inject_size == 0)
		return (-1);
	new_size = *size;
	if (ctx->inject_off + ctx->inject_size > new_size)
		new_size = ctx->inject_off + ctx->inject_size;
	if (new_size != *size)
	{
		unsigned char	*new_buf;

		new_buf = (unsigned char *)malloc(new_size);
		if (!new_buf)
			return (-1);
		ft_bzero(new_buf, new_size);
		ft_memcpy(new_buf, *buf, *size);
		free(*buf);
		*buf = new_buf;
		*size = new_size;
	}
	stub_len = (size_t)(woody_stub64_end - woody_stub64);
	if (stub_len > ctx->inject_size)
		return (-1);
	dst = *buf + ctx->inject_off;
	ft_bzero(dst, ctx->inject_size);
	ft_memcpy(dst, woody_stub64, stub_len);
	ft_bzero(&meta, sizeof(meta));
	meta.orig_entry = ctx->ehdr->e_entry;
	meta.text_vaddr = ctx->entry_in_text ? ctx->text_vaddr : 0;
	meta.text_size = ctx->entry_in_text ? ctx->text_size : 0;
	ft_memcpy(meta.key, key, WOODY_KEY_SIZE);
	ft_memcpy(dst + (size_t)(woody_stub64_meta_orig_entry - woody_stub64),
		&meta.orig_entry, sizeof(meta.orig_entry));
	ft_memcpy(dst + (size_t)(woody_stub64_meta_text_vaddr - woody_stub64),
		&meta.text_vaddr, sizeof(meta.text_vaddr));
	ft_memcpy(dst + (size_t)(woody_stub64_meta_text_size - woody_stub64),
		&meta.text_size, sizeof(meta.text_size));
	ft_memcpy(dst + (size_t)(woody_stub64_meta_key - woody_stub64),
		meta.key, WOODY_KEY_SIZE);
	((Elf64_Ehdr *)(*buf))->e_entry = ctx->inject_vaddr;
	((Elf64_Phdr *)((unsigned char *)(*buf)
		+ ((Elf64_Ehdr *)(*buf))->e_phoff))
		[ctx->text_phdr - ctx->phdrs].p_filesz += ctx->inject_size;
	((Elf64_Phdr *)((unsigned char *)(*buf)
		+ ((Elf64_Ehdr *)(*buf))->e_phoff))
		[ctx->text_phdr - ctx->phdrs].p_memsz += ctx->inject_size;
	return (0);
}

#ifdef BONUS
static int	apply_injection32(unsigned char **buf, size_t *size,
		const t_elf32_ctx *ctx, const unsigned char *key)
{
	t_woody_meta32	meta;
	size_t		new_size;
	size_t		stub_len;
	unsigned char	*dst;

	if (!buf || !*buf || !size || !ctx || !ctx->ehdr || !ctx->text_phdr)
		return (-1);
	if (ctx->inject_size == 0)
		return (-1);
	new_size = *size;
	if (ctx->inject_off + ctx->inject_size > new_size)
		new_size = ctx->inject_off + ctx->inject_size;
	if (new_size != *size)
	{
		unsigned char	*new_buf;

		new_buf = (unsigned char *)malloc(new_size);
		if (!new_buf)
			return (-1);
		ft_bzero(new_buf, new_size);
		ft_memcpy(new_buf, *buf, *size);
		free(*buf);
		*buf = new_buf;
		*size = new_size;
	}
	stub_len = (size_t)(woody_stub32_end - woody_stub32);
	if (stub_len > ctx->inject_size)
		return (-1);
	dst = *buf + ctx->inject_off;
	ft_bzero(dst, ctx->inject_size);
	ft_memcpy(dst, woody_stub32, stub_len);
	ft_bzero(&meta, sizeof(meta));
	meta.orig_entry = ctx->ehdr->e_entry;
	meta.text_vaddr = ctx->entry_in_text ? ctx->text_vaddr : 0;
	meta.text_size = ctx->entry_in_text ? ctx->text_size : 0;
	ft_memcpy(meta.key, key, WOODY_KEY_SIZE);
	ft_memcpy(dst + (size_t)(woody_stub32_meta_orig_entry - woody_stub32),
		&meta.orig_entry, sizeof(meta.orig_entry));
	ft_memcpy(dst + (size_t)(woody_stub32_meta_text_vaddr - woody_stub32),
		&meta.text_vaddr, sizeof(meta.text_vaddr));
	ft_memcpy(dst + (size_t)(woody_stub32_meta_text_size - woody_stub32),
		&meta.text_size, sizeof(meta.text_size));
	ft_memcpy(dst + (size_t)(woody_stub32_meta_key - woody_stub32),
		meta.key, WOODY_KEY_SIZE);
	((Elf32_Ehdr *)(*buf))->e_entry = ctx->inject_vaddr;
	((Elf32_Phdr *)((unsigned char *)(*buf)
		+ ((Elf32_Ehdr *)(*buf))->e_phoff))
		[ctx->text_phdr - ctx->phdrs].p_filesz += ctx->inject_size;
	((Elf32_Phdr *)((unsigned char *)(*buf)
		+ ((Elf32_Ehdr *)(*buf))->e_phoff))
		[ctx->text_phdr - ctx->phdrs].p_memsz += ctx->inject_size;
	return (0);
}
#endif

int	pack_elf64(int fd, t_elf_ctx *ctx)
{
	unsigned char	*buf;
	unsigned char	key[WOODY_KEY_SIZE];
	size_t			off;
	size_t			len;
	size_t			file_size;
	int				outfd;

	if (!ctx || !ctx->text_phdr)
	{
		g_pack_error = "invalid context";
		return (-1);
	}
	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		g_pack_error = "lseek failed";
		return (-1);
	}
	file_size = ctx->file_size;
	buf = (unsigned char *)malloc(file_size);
	if (!buf)
	{
		g_pack_error = "malloc failed";
		return (-1);
	}
	if (read_full(fd, buf, file_size) < 0)
	{
		g_pack_error = "read failed";
		free(buf);
		return (-1);
	}
	off = ctx->text_off;
	len = ctx->text_size;
	(void)ctx;
	if (off + len > file_size)
	{
		g_pack_error = "text range out of bounds";
		free(buf);
		return (-1);
	}
	if (generate_key(key) < 0)
	{
		g_pack_error = "key generation failed";
		free(buf);
		return (-1);
	}
	print_key(key);
	if (!ctx->entry_in_text)
	{
		outfd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, 0755);
		if (outfd < 0)
		{
			g_pack_error = "open output failed";
			free(buf);
			return (-1);
		}
		if (write_full(outfd, buf, file_size) < 0)
		{
			g_pack_error = "write output failed";
			close(outfd);
			free(buf);
			return (-1);
		}
		close(outfd);
		free(buf);
		return (0);
	}
	encrypt_region(buf, off, len, key);
	if (apply_injection64(&buf, &file_size, ctx, key) < 0)
	{
		g_pack_error = "stub injection failed";
		free(buf);
		return (-1);
	}
	outfd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, 0755);
	if (outfd < 0)
	{
		g_pack_error = "open output failed";
		free(buf);
		return (-1);
	}
	if (write_full(outfd, buf, file_size) < 0)
	{
		g_pack_error = "write output failed";
		close(outfd);
		free(buf);
		return (-1);
	}
	close(outfd);
	free(buf);
	return (0);
}

#ifdef BONUS
int	pack_elf32(int fd, t_elf32_ctx *ctx)
{
	unsigned char	*buf;
	unsigned char	key[WOODY_KEY_SIZE];
	size_t			off;
	size_t			len;
	size_t			file_size;
	int				outfd;

	if (!ctx || !ctx->text_phdr)
	{
		g_pack_error = "invalid context";
		return (-1);
	}
	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		g_pack_error = "lseek failed";
		return (-1);
	}
	file_size = ctx->file_size;
	buf = (unsigned char *)malloc(file_size);
	if (!buf)
	{
		g_pack_error = "malloc failed";
		return (-1);
	}
	if (read_full(fd, buf, file_size) < 0)
	{
		g_pack_error = "read failed";
		free(buf);
		return (-1);
	}
	off = ctx->text_off;
	len = ctx->text_size;
	(void)ctx;
	if (off + len > file_size)
	{
		g_pack_error = "text range out of bounds";
		free(buf);
		return (-1);
	}
	if (generate_key(key) < 0)
	{
		g_pack_error = "key generation failed";
		free(buf);
		return (-1);
	}
	print_key(key);
	if (!ctx->entry_in_text)
	{
		outfd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, 0755);
		if (outfd < 0)
		{
			g_pack_error = "open output failed";
			free(buf);
			return (-1);
		}
		if (write_full(outfd, buf, file_size) < 0)
		{
			g_pack_error = "write output failed";
			close(outfd);
			free(buf);
			return (-1);
		}
		close(outfd);
		free(buf);
		return (0);
	}
	encrypt_region(buf, off, len, key);
	if (apply_injection32(&buf, &file_size, ctx, key) < 0)
	{
		g_pack_error = "stub injection failed";
		free(buf);
		return (-1);
	}
	outfd = open("woody", O_CREAT | O_TRUNC | O_WRONLY, 0755);
	if (outfd < 0)
	{
		g_pack_error = "open output failed";
		free(buf);
		return (-1);
	}
	if (write_full(outfd, buf, file_size) < 0)
	{
		g_pack_error = "write output failed";
		close(outfd);
		free(buf);
		return (-1);
	}
	close(outfd);
	free(buf);
	return (0);
}
#endif
