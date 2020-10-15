# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2020-21

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -I../
LDFLAGS=-lm
PTH = -pthread

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs

tecnicofs: fs/state.o fs/operations.o main.o
	$(LD) $(CFLAGS) $(LDFLAGS) $(PTH) -o tecnicofs fs/state.o fs/operations.o main.o

fs/state.o: fs/state.c fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) $(PTH) -o fs/state.o -c fs/state.c

fs/operations.o: fs/operations.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) $(PTH) -o fs/operations.o -c fs/operations.c

main.o: main.c fs/operations.h fs/state.h tecnicofs-api-constants.h
	$(CC) $(CFLAGS) $(PTH) -o main.o -c main.c



clean:
	@echo Cleaning...
	rm -f fs/*.o *.o tecnicofs

run: tecnicofs
	./tecnicofs inputs/test1.txt outputs/test1.txt 1 nosync
