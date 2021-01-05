OBJS	= main.o structs.o
SOURCE	= main.c structs.c
HEADER	= structs.h
OUT	= main
CC	 = gcc
FLAGS	 = -g -c # -Wall
LFLAGS	 = -lpthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) main.c 

structs.o: structs.c
	$(CC) $(FLAGS) structs.c 

clean:
	rm -f $(OBJS) $(OUT)

run: $(OUT)
	./$(OUT)