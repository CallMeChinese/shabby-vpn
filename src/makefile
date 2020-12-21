.PHONY: all clean

all: main.c main.h vpn.c vpn.h
	mkdir -p ../bin
	$(CC) -DSEND -DRECV main.c -o ../bin/main -lpthread
	$(CC) vpn.c -o ../bin/vpn -lpthread -lssl -lcrypto

clean:
	rm -rf ../bin/*