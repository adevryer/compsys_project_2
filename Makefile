EXE=fetchmail
CFLAGS=-Wall -Wextra -g -O0
LDFLAGS=-lm

$(EXE): main.o commands.o
	cc $(CFLAGS) -o $(EXE) $^ $(LDFLAGS)

main.o: main.c commands.h
	cc $(CFLAGS) -c -o main.o main.c

commands.o: commands.c commands.h
	cc $(CFLAGS) -c -o commands.o commands.c

clean: 
	rm -f *.o $(EXE)

format:
	clang-format -style=file -i *.c