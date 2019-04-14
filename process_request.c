#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "prozent.h"
#include "do_get.h"

#define MAX_GET_REQUEST_LENGHT 8192

/* 
 *	Liest HTTP-Request(nur GET) von infd
 *	und schreibt Antwort auf outfd.
 * (ersetzt Prozent-codierte Zeichen durch ASCII-Aequivalente)
 */
void process_request(int infd, int outfd, char *basis)
{       
        int running=1;
        char *requestzeile = calloc(MAX_GET_REQUEST_LENGHT, sizeof(char));
		char zeichen = 0;
        int counter = 1, fdread;
        int base = SEEK_SET;
        // Iteration mit lseek()
        // laeuft solange, bis "\r\n\r\n" eingelesen wird
        while (running) {
            fdread = read(infd, &zeichen, 1);
            if(fdread  < 0){
                perror("process_request: read()"); exit(1);
            }
            
            strncat(requestzeile, &zeichen, 1);
            
            if(lseek(infd, counter, base) < 0){
                perror("process_request: fehler bei lseek()"); exit(1);
            }
			
			if (strstr(requestzeile, "\r\n\r\n") != NULL) {
				running = 0;
			}
			
            counter+=1;
        }
        close(infd);

        // ersetzt Prozent-codierte Zeichen durch ASCII-Aequivalente
        char *requestdecoded = prozentdecode(requestzeile); 

        do_get(requestdecoded, outfd, basis);
        
		free(requestzeile);
        free(requestdecoded);
}

