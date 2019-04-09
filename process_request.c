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

void process_request(int infd, int outfd)
{       
        int running=1;
        char *requestzeile = malloc(MAX_GET_REQUEST_LENGHT);
        char *zeichen = malloc(1);
        char *vierZeichen = malloc(4);
        int counter = 1, fdread;
        int basis = SEEK_SET;
        // Iteration mit lseek()
        //laeuft solange, bis "\r\n\r\n" eingelesen wird
        while(running){
            fdread = read(infd, zeichen, 1);
            if(fdread  < 0){
                perror("process_request: read()"); exit(1);
            }
            
            strcat(requestzeile, zeichen);
            
            if(lseek(infd, counter, basis) < 0){
                perror("process_request: fehler bei lseek()"); exit(1);
            }
            //vierZeichen-Check
            char tmpVier[4];
            tmpVier[0]=vierZeichen[1];
            tmpVier[1]=vierZeichen[2];
            tmpVier[2]=vierZeichen[3];
            tmpVier[3]=zeichen[0];
            if(!strcmp(tmpVier, "\r\n\r\n")){
                running = 0;
            }
            vierZeichen=tmpVier;
            counter+=1;
        }
        close(infd);       
        
        // ersetzt Prozent-codierte Zeichen durch ASCII-Aequivalente
        char *requestdecoded = prozentdecode(requestzeile);
                
        do_get(requestdecoded, outfd);
        
        free(requestdecoded);
        free(requestzeile);
        free(zeichen);
}
