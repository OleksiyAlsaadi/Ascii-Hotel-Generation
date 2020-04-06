OBJS = new.c

CC = g++

COMPILER_FLAGS = -w

LINKER_FLAGS = -lncursesw

all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) 
