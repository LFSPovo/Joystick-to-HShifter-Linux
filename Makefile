OBJS=main.o

all: joy4shift

joy4shift: $(OBJS)
	gcc -Wall $(CCFLAGS) -g3 -o joy4shift $(OBJS) $(LDFLAGS) -lm

main.o: main.c
	gcc -Wall $(CCFLAGS) -c main.c -lm

clean:
	rm -rf joy4shift $(OBJS)
