default: all

all: sender.o receiver.o
sender.o:
	gcc -o sender sender.c
receiver.o:
	gcc -o receiver receiver.c

clean:
	rm -rf *.o
