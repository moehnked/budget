CC=gcc
CFLAGS=-g -Wshadow -pthread -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement -Wno-return-local-addr -Wuninitialized
#DEPS = 
#OBJ =

%.0: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: budget

budget: budget.o
	$(CC) $(CFLAGS) -o budget budget.c

budget.o: budget.c
	$(CC) -c budget.c

clean:
	rm -rf budget *.o *~ \#*