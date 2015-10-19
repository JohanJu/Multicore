CC		= gcc

CFLAGS		= -O3 -maltivec -Wall -pedantic -std=c99 -m64

OBJS		= main.o list.o error.o random.o set.o dataflow.o

OUT		= live

LDFLAGS		= -pthread

S=100000
V=100000
U=4
A=1000
T=4
P=0

all: $(OBJS)	
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(OUT)
	# valgrind --tool=helgrind ./$(OUT) $(S) $(V) $(U) $(A) $(T) $(P) $(L)
	./$(OUT) $(S) $(V) $(U) $(A) $(T) $(P)
	# diff -q -s new.txt old.txt
	rm -f $(OBJS) cfg.dot

clean:
	rm -f $(OUT) $(OBJS) cfg.dot
