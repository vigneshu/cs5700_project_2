default: all

all: sender.o receiver.o
sender.o:
	gcc   -pthread  -o sender  sender.c -lm 
receiver.o:
	gcc  -pthread   -o receiver receiver.c -lm 

clean:
	rm -rf *.o
