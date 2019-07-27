all: server1 server

server:server.c dns.c
	gcc server.c dns.c -o server
server1:server1.c dns.c array.c
	gcc server1.c dns.c array.c -o server1


clean:
	rm server server1
