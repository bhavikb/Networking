all: client server
client: clientUDP.c
	gcc -o client clientUDP.c
	
server: serverUDP.c
	gcc -o server serverUDP.c
	
clean:
	rm -rf server client