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

static int	dispatch_parser(int fd)
{
	unsigned char	ident[EI_NIDENT];
	t_elf_ctx		ctx64;
	t_elf32_ctx		ctx32;

	if (read_ident(fd, ident) < 0)
		return (-1);
	if (ident[EI_MAG0] != ELFMAG0 || ident[EI_MAG1] != ELFMAG1
		|| ident[EI_MAG2] != ELFMAG2 || ident[EI_MAG3] != ELFMAG3)
		return (-1);
	if (ident[EI_CLASS] == ELFCLASS64)
	{
		if (parse_elf64(fd, &ctx64) < 0)
			return (-1);
		cleanup_elf_ctx(&ctx64);
		return (0);
	}
	if (ident[EI_CLASS] == ELFCLASS32)
	{
		if (parse_elf32(fd, &ctx32) < 0)
			return (-1);
		cleanup_elf32_ctx(&ctx32);
		return (0);
	}
	return (-1);
}

int	main(int argc, char **argv)
{
	int	fd;

	if (argc != 2)
	{
		printf("Usage: %s <elf_binary>\n", argv[0]);
		return (1);
	}
	fd = open_input_file(argv[1]);
	if (fd < 0)
		return (1);
	if (dispatch_parser(fd) < 0)
	{
		printf("Unsupported or invalid ELF file.\n");
		close(fd);
		return (1);
	}
	if (close(fd) < 0)
	{
		perror("close");
		return (1);
	}
	return (0);
}