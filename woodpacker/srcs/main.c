#include "../includdes/woodpacker.h"



static int	open_input_file(const char *path)
{
	int	fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		perror("open");
		return (-1);
	}
	return (fd);
}

static int	read_ident(int fd, unsigned char *ident)
{
	ssize_t	bytes;

	bytes = read(fd, ident, EI_NIDENT);
	if (bytes != EI_NIDENT)
		return (-1);
	if (lseek(fd, 0, SEEK_SET) < 0)
		return (-1);
	return (0);
}

static int	dispatch_parser(int fd, const unsigned char *key_override)
{
	unsigned char	ident[EI_NIDENT];
	t_elf_ctx		ctx64;
#ifdef BONUS
	t_elf32_ctx		ctx32;
#endif
	const size_t	stub_size = WOODY_STUB_SIZE;

	if (read_ident(fd, ident) < 0)
		return (-1);
	if (ident[EI_MAG0] != ELFMAG0 || ident[EI_MAG1] != ELFMAG1
		|| ident[EI_MAG2] != ELFMAG2 || ident[EI_MAG3] != ELFMAG3)
		return (-1);
	if (ident[EI_CLASS] == ELFCLASS64)
	{
		if (parse_elf64(fd, &ctx64) < 0)
			return (-1);
		if (select_injection_site64(&ctx64, stub_size) < 0)
		{
			cleanup_elf_ctx(&ctx64);
			return (-1);
		}
		if (pack_elf64(fd, &ctx64, key_override) < 0)
		{
			cleanup_elf_ctx(&ctx64);
			return (-3);
		}
		cleanup_elf_ctx(&ctx64);
		return (0);
	}
#ifdef BONUS
	if (ident[EI_CLASS] == ELFCLASS32)
	{
		if (parse_elf32(fd, &ctx32) < 0)
			return (-1);
		if (select_injection_site32(&ctx32, stub_size) < 0)
		{
			cleanup_elf32_ctx(&ctx32);
			return (-1);
		}
		if (pack_elf32(fd, &ctx32, key_override) < 0)
		{
			cleanup_elf32_ctx(&ctx32);
			return (-3);
		}
		cleanup_elf32_ctx(&ctx32);
		return (0);
	}
#endif
	return (-2);
}

int	main(int argc, char **argv)
{
	int	fd;
	unsigned char	key[WOODY_KEY_SIZE];
	const unsigned char	*key_override;
	const char	*input_path;

	key_override = NULL;
	input_path = NULL;
	if (argc == 2)
		input_path = argv[1];
	else if (argc == 4 && ft_strcmp(argv[1], "-k") == 0)
	{
		if (parse_hex_key(argv[2], key) < 0)
		{
			printf("Invalid key format. Expected 32 hex chars.\n");
			return (1);
		}
		key_override = key;
		input_path = argv[3];
	}
	else
	{
		printf("Usage: %s <elf_binary>\n", argv[0]);
		printf("       %s -k <hexkey> <elf_binary>\n", argv[0]);
		return (1);
	}
	fd = open_input_file(input_path);
	if (fd < 0)
		return (1);
	{
		int	res;

		res = dispatch_parser(fd, key_override);
		if (res < 0)
		{
			if (res == -2)
				printf("File architecture not suported. x86_64 only\n");
			else if (res == -3)
				printf("Packing failed: %s\n", get_pack_error());
			else
				printf("Packing failed: %s\n", get_parse_error());
			close(fd);
			return (1);
		}
	}
	if (close(fd) < 0)
	{
		perror("close");
		return (1);
	}
	return (0);
}