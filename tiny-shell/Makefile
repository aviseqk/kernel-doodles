CC := gcc
FLAGS := -Wall -Wextra -g
SRCS := main.c
OUT := tiny_shell_1
INC := job_manager.c

all: build

build:
	@echo "Compiling normal build, without log messages"
	$(CC) $(FLAGS) $(SRCS) $(INC) -o $(OUT)

debug:
	@echo "Compiling debug build, with log messages..."
	$(CC) $(FLAGS) $(SRCS) $(INC) -DLOG_LEVEL=3 -o $(OUT)

build_addr:
	@echo "Compiling build with AddressSanitizer, and with log messages..."
	$(CC) $(FLAGS) $(SRCS) $(INC) -DLOG_LEVEL=3 -fsanitize=address -o $(OUT)

clean:
	@echo "Cleaning up..."
	rm -rf $(OUT)
