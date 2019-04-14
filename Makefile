sockserv: sockserv.c
	gcc -g -std=c99 -Wall -pedantic sockserv.c process_request.c do_get.c prozent.c process_request.h do_get.h prozent.h -o webserver
