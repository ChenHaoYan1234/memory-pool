all: main

main: main.o mem.o
	gcc -o $@ $^

%.o: %.c
	gcc -c -I./includes -o $@ $<

clean:
	rm -f *.o main
