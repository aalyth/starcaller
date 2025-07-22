
all: main.c
	clang -O3 -o star main.c -Iinclude/ ./src/*.c -lpthread -std=c23
