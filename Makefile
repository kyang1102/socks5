server1:server1.c dns.c array.c socks5.c
	gcc server1.c dns.c array.c socks5.c -o server1

clean:
	rm server1
