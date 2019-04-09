#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h> 
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#define MAX_GET_REQUEST_LENGHT 8192

char *mime_type(char *str)
{
    if(!strcmp(str, "html")){
        return "text/html";
    }else if(!strcmp(str, "txt")){
        return "text/plain";
    }else if(!strcmp(str, "png")){
        return "image/png";
    }
    return 0;
}

/*
 * Gibt die Anzahl an Ziffer einer Zahl zurueck.
 */
int ziffernAnz(int i)
{   
    int anz=0;
    while(i>0){
        i/=10;
        anz+=1;
    }
    return anz; 
}

/*
 * Gibt Fehlermeldung aus und beendet Programm,
 * gdw der Filediscriptor < 0
 */
void schreibfehlerTest(int fd_write)
{
    if(fd_write < 0){
        perror("write");
        exit(2);
    }
}

char *getHost(char *request)
{
    char *ptr = request;
    while(strncmp(ptr, "\r", 1) != 0){
        ptr+=1;
    }
    ptr+=8; // ptr zeigt dann auf den Beginn von Host 
    char *host = malloc(MAX_GET_REQUEST_LENGHT);
    while(strncmp(ptr, "\n", 1) != 0){
        strncat(host, ptr, 1);
        ptr+=1;
    }    
    
    return host;
}

/*
 * 
 * 
 */
void do_get(const char *respath, int outfd)
{   
    // printf("respath: %s\n", respath);
    // Dateipfad rauslesen        
    char *path = malloc(strlen(respath));
    char *pathPtr = (char *)respath;
    // G"ET " ueberspringen /*G am Anfang geht verloren?!*/
    pathPtr+=3;
    // strcat bis Leerzeichen bzw. Ende der Pfades
    while(strncmp(pathPtr, " ", 1) != 0){
        strncat(path, pathPtr, 1);
        pathPtr+=1;
    }
 
    //exists oder isDirectory
    int exists=0, isDir=0;
    if(access(path, F_OK) != -1){
        exists=1;
        // pruefen ob Directory und in isDir speichern
        struct stat statStruct;
        stat(path, &statStruct);
        isDir = S_ISDIR(statStruct.st_mode);
        //String nach letztem Slash = Dateiname/Verzeichnisname
        char *behindfLastSlash;
        char *pathPtr = (char *)path;
        for(int i=0; i<strlen(path); i++){
            if(*(pathPtr-1) == '/'){
                behindfLastSlash = pathPtr;
            }
            pathPtr++;
        }
        const int eintragLenght = strlen(behindfLastSlash);
        char eintrag[eintragLenght]; // <<<datei/directory-Name
        strcpy(eintrag, behindfLastSlash);
        
        /*Datei*/
        if(!isDir){
			
			//wenn die Datei ausfürbar ist..
			if(access(path, X_OK) != -1){
                dup2(outfd, 0);
                dup2(outfd, 1);
                execl(path, behindfLastSlash, "-1", NULL);
                
			//sonst...
			}else{
				int contentLenght = (int)statStruct.st_size;
				//Dateiname
				const int eintragLenght = strlen(behindfLastSlash);
				char eintrag[eintragLenght];
				strcpy(eintrag, behindfLastSlash);
				//Dateityp
				char *contentType = mime_type(strpbrk(eintrag, ".")+1);
				char *contentLenghtString[ziffernAnz((int)contentLenght)];
				sprintf((char *)contentLenghtString, "%d", contentLenght);      
				//file einlesen
				int bufferSize = statStruct.st_size;
				char *buffer[bufferSize]; // hier wird der Dateiinhalt gespeichert
				int fd, fd_read;
				fd = open(path, O_RDONLY);
				fd_read = read(fd, buffer, bufferSize);
				if(fd_read < 0 || fd < 0){
					perror("Fehler beim Einlesen der Datei.");
					exit(1);
				}
				close(fd);
				close(fd_read);            
				//schreiben
                char *string1 = "HTTP/1.0 200 OK\r\nContentType: ";
				char *string2 = "\r\nContent-Lenght: ";
				char *string3 = "\r\n\n";
                          
                schreibfehlerTest(write(outfd, string1, strlen(string1)));
                schreibfehlerTest(write(outfd, contentType, strlen(contentType)));
                schreibfehlerTest(write(outfd, string2, strlen(string2)));
                schreibfehlerTest(write(outfd, contentLenghtString, strlen((char *)contentLenghtString)));
                schreibfehlerTest(write(outfd, string3, strlen(string3)));
                schreibfehlerTest(write(outfd, buffer, bufferSize));
			}
        }
            
        /*Ordner*/
        if(isDir){
            DIR *dir;
            struct dirent *eintrag;
            struct stat statbuf;
            char pfadbuffer[PATH_MAX], *pfadptr;
            int write_fd;
                        
            dir = opendir(path);
            if(dir == NULL){
                perror("opendir() kaputt");
                exit(3);
            }
            strcpy(pfadbuffer, path);
            strcat(pfadbuffer, "/");
            pfadptr = pfadbuffer +strlen(pfadbuffer);
            write_fd = write(outfd, "<UI>", 4);
            schreibfehlerTest(write_fd);
            char *host = getHost((char *)respath);
            while(1){
                eintrag = readdir(dir);
                if(eintrag == NULL) break;
                if(strcmp(eintrag->d_name, ".")==0 ||
                    strcmp(eintrag->d_name, "..")==0){continue;}
                strcpy(pfadptr, eintrag->d_name);
                
                if(stat(pfadbuffer, &statbuf) == -1){
                    perror(pfadbuffer);
                }else{
                    
                    write_fd = write(outfd, "<LI><a href=&quot;", 18);
                    schreibfehlerTest(write_fd);
                    
                    write_fd = write(outfd, host, strlen(host));
                    schreibfehlerTest(write_fd);
                    
                    write_fd = write(outfd, "&quot;>", 6);
                    schreibfehlerTest(write_fd);
                    
                    write_fd = write(outfd, pfadbuffer, strlen(pfadbuffer));
                    schreibfehlerTest(write_fd);                    
                    
					write_fd = write(outfd, "</a></LI>", 9);
                    schreibfehlerTest(write_fd);
                    
                }
            }
            free(host);
            write_fd = write(outfd, "</UI>", 5);
            schreibfehlerTest(write_fd);
            closedir(dir);
        }
    }
    
    /*Nicht vorhanden*/
    if(!isDir && !exists){
        int fd_write;
        char *output = "HTTP/1.0 404 NOT Found \r\nContent-Type:text/html\r\nContent-Lenght: 52\r\n\n<html>\n<h1>Error 404</h1>\n<h3>Diese Seite gibt es nicht :(</h3>\n</html>\n";
        fd_write = write(outfd, output, strlen(output));
        schreibfehlerTest(fd_write);
    } 
    
    free(path);
    
}
