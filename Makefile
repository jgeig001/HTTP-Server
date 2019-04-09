sockserv: sockserv.c
	gcc -g -std=c99 -Wall sockserv.c process_request.c do_get.c prozent.c -o webserver
