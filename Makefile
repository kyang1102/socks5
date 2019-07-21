server:server.c cregex.c
	gcc server.c cregex.c -o server

clean:
	rm server
