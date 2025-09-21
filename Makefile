all: main

main: main.o mem.o
	gcc -g -o $@ $^

%.o: %.c
	gcc -g -c -I./includes -o $@ $<

clean:
	rm -f *.o main
