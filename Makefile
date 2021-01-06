OBJS	= main.o structs.o
SOURCE	= main.c worker.c structs.c
HEADER	= structs.h
OUT	= main
CC	 = gcc
FLAGS	 = -g -c # -Wall
LFLAGS	 = -lpthread

all: $(OBJS) worker
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) main.c 

structs.o: structs.c
	$(CC) $(FLAGS) structs.c

worker.o: worker.c
	$(CC) $(FLAGS) worker.c

worker: worker.o
	$(CC) -g worker.o structs.o -o worker $(LFLAGS)

clean:
	rm -f $(OBJS) $(OUT) worker

run: $(OUT)
	./$(OUT) -q 1000 -max 10000

valgrind: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all ./$(OUT) -q 1000 -max 10000
	