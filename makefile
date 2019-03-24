all: clean par-shell par-shell-terminal

par-shell: par-shell.o funcs.o commandlinereader.o 
	gcc -lpthread -pthread  -o par-shell commandlinereader.o par-shell.o funcs.o

par-shell-terminal: par-shell-terminal.c errors.h
	gcc  -o par-shell-terminal par-shell-terminal.c

comandlinereader.o: commandlinereader.c commandlinereader.h
	gcc -Wall  -g -c commandlinereader.c

funcs.o: funcs.c funcs.h errors.h
	gcc -Wall  -g -c funcs.c

par-shell.o: par-shell.c commandlinereader.c
	gcc -Wall  -g -c par-shell.c

clean:
	rm -rf *.o *.txt par-shell par-shell-terminal /tmp/par-shell*
