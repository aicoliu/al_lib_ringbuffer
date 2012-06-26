CC=gcc
FLAGS=-ggdb -g -w -Werror -Wextra -Wall -pedantic
STD=-std=c89
LIB=rbuffer.o
EXE=rbtest

all:
	$(CC) $(FLAGS) $(STD) -o $(EXE) src/ring_buffer.c test/*.c
