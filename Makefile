server:server.c dns.c array.c socks5.c
	gcc server.c dns.c array.c socks5.c -o server

clean:
	rm server
