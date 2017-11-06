# make mext2
# by zhangtian
# @email: eleztian@gmail.com

DIR_INC = ./inc
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_BIN = ./bin

SRC = $(wildcard ${DIR_SRC}/*.c)
OBJ = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${SRC}))

TARGET = mext2

CC = gcc
CFLAGS = -g -Wall -I${DIR_INC}

${DIR_BIN}/${TARGET}:${OBJ}
	$(CC) $(OBJ) -o $@

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	find ${DIR_OBJ} -name *.o -exec rm -rf {} \;
.PHONY: run
run:
	./bin/${TARGET}
	

 
