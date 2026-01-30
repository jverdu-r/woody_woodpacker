NAME = woody_woodpacker

CC = cc
CFLAGS = -Wall -Wextra -Werror

ROOT_DIR = woodpacker
SRC_DIR = $(ROOT_DIR)/srcs
INC_DIR = $(ROOT_DIR)/includdes
LIBFT_DIR = $(INC_DIR)/libft
LIBFT = $(LIBFT_DIR)/libft.a

SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/parse.c \
	$(SRC_DIR)/pack.c

ASMS = \
	$(SRC_DIR)/stub64.S \
	$(SRC_DIR)/stub32.S

OBJS = $(SRCS:.c=.o) $(ASMS:.S=.o)

CPPFLAGS = -I$(INC_DIR) -I$(LIBFT_DIR)

all: $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(LIBFT) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBFT) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

bonus: CPPFLAGS += -DBONUS
bonus: fclean $(LIBFT) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBFT) -o $(NAME)

clean:
	$(MAKE) -C $(LIBFT_DIR) clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C $(LIBFT_DIR) fclean
	rm -f $(NAME)

re: fclean all

.PHONY: all bonus clean fclean re
