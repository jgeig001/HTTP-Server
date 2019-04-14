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
    if (!strcmp(str, "html") ){
        return "text/html";
    } else if(!strcmp(str, "txt")) {
        return "text/plain";
    } else if(!strcmp(str, "jpeg") || !strcmp(str, "jpg")) {
        return "image/jpeg";
    } else if(!strcmp(str, "png")) {
        return "image/png";
    } else if(!strcmp(str, "gif")) {
        return "image/gif";
    } else if(!strcmp(str, "ico")) {
        return "image/x-icon";
    }
    return 0;
}

/*
 * Gibt die Anzahl an Ziffer eine positive Dezimalganzzahl zurueck.
 */
int ziffernAnz(int i)
{   
    int anz = 0;
    while (i >= 1) {
        i /= 10;
        anz++;
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
        perror("write failed");
        exit(-1);
    }
}

char *getHost(char *request)
{
    char *ptr = request;
    while(strncmp(ptr, "\r", 1) != 0){
        ptr+=1;
    }
    ptr+=8; // ptr zeigt dann auf den Beginn von Host 
    char *host = calloc(MAX_GET_REQUEST_LENGHT, sizeof(char));
    while(strncmp(ptr, "\n", 1) != 0){
        strncat(host, ptr, 1);
        ptr+=1;
    }
    return host;
}

/*
 * Sendet ERROR_404 als Antwort 
 */
void not_found (int outfd)
{
  	int fd_write;
  	char *output = "HTTP/1.0 404 NOT Found \r\nContent-Type:text/html\r\nContent-Lenght: 52\r\n\n<html>\n<h1>Error 404</h1>\n<h3>Diese Seite gibt es nicht :(</h3>\n</html>\n";
   	fd_write = write(outfd, output, strlen(output));
	schreibfehlerTest(fd_write);
}

void remove_doubleSlash (char *str)
{
    char *ptr = str;
    while (*ptr) {
        while (*ptr == '/' && *ptr == *(ptr+1)) {
            memmove(ptr, ptr+1, strlen(ptr));
        }
        ptr++;
    }
}

/*
 * Sucht Content von Basisverzeichnis aus und schreibt Output auf outfd.
 */
void do_get(const char *respath, int outfd, char *basis)
{   
	remove_doubleSlash(basis);
    // Dateipfad rauslesen
    char *path = calloc(strlen(respath) + 1, sizeof(char));
    char *pathPtr = (char *)respath;
    // G"ET " ueberspringen /*G am Anfang geht verloren?!*/
    pathPtr += 3;
    // strcat bis Leerzeichen bzw. Ende der Pfades
    while (strncmp(pathPtr, " ", 1) != 0) {
        strncat(path, pathPtr, 1);
        pathPtr += 1;
    }
    // exists oder isDirectory
    int exists=0, isDir=0;
    if (access(path, F_OK) != -1) {
        exists=1;
        // pruefen ob Directory und in isDir speichern
        struct stat statStruct;
        stat(path, &statStruct);
        isDir = S_ISDIR(statStruct.st_mode);
        //String nach letztem Slash = Dateiname bzw. Verzeichnisname
        char *behindfLastSlash = strrchr(path, '/') + 1;
		
        const int eintragLenght = strlen(behindfLastSlash);
        char eintrag[eintragLenght]; // <<<datei/directory-Name
        strcpy(eintrag, behindfLastSlash);
        
		/* (1) Datei */
        if (!isDir) {
			// wenn die Datei ausfürbar ist..
			if(access(path, X_OK) != -1){
                dup2(outfd, 0);
                dup2(outfd, 1);
                execl(path, behindfLastSlash, "-1", NULL);
                
			} else {
				// ,sonst... HTTP-Header
				int contentLenght = (int)statStruct.st_size;
				//Dateiname
				const int eintragLenght = strlen(behindfLastSlash);
				char eintrag[eintragLenght];
				strcpy(eintrag, behindfLastSlash);
				//Dateityp
				char *contentType = mime_type(strpbrk(eintrag, ".") + 1);
				if (strlen(contentType) == 0) {
					// wenn kein contentType gefunden wurde wird 404 gesendet.
					printf("wrong contentType:%s\n", contentType);
					not_found(outfd); free(path); return;
				}
				char *contentLenghtString[ziffernAnz((int)contentLenght)];
				sprintf((char *)contentLenghtString, "%d", contentLenght);      
				//file einlesen
				int bufferSize = contentLenght;
				char *buffer = calloc(bufferSize, sizeof(char)); // hier wird der Dateiinhalt gespeichert
				int fd, fd_read;
				fd = open(path, O_RDONLY);
				fd_read = read(fd, buffer, bufferSize);
				if(fd_read < 0 || fd < 0){
					perror("Fehler beim Einlesen der Datei.");
					exit(-1);
				}
				close(fd);
				close(fd_read);            
				//schreiben
                char *string1 = "HTTP/1.0 200 OK\r\nContentType: ";
				char *string2 = "\r\nContent-Lenght: ";
				char *string3 = "\r\n\n";
                        
				// send HTTP-header  
                schreibfehlerTest(write(outfd, string1, strlen(string1)));
                schreibfehlerTest(write(outfd, contentType, strlen(contentType)));
                schreibfehlerTest(write(outfd, string2, strlen(string2)));
                schreibfehlerTest(write(outfd, contentLenghtString, strlen((char *)contentLenghtString)));
                schreibfehlerTest(write(outfd, string3, strlen(string3)));
				// send content
                schreibfehlerTest(write(outfd, buffer, bufferSize));
				
				free(buffer);
			}
        }
            
        /* (2) Ordner */
        if (isDir) {
			/*** create/open and write html-file ***/
			
			DIR *dir;
            struct dirent *eintrag;
            struct stat statbuf;
            char pfadbuffer[PATH_MAX];
			char *pfadptr;
            int write_fd;
                        
            dir = opendir(path);
            if (dir == NULL) {
                perror("opendir failed");
                exit(-1);
            }
            strcpy(pfadbuffer, path);
            strcat(pfadbuffer, "/");
            pfadptr = pfadbuffer + strlen(pfadbuffer);

			// neue Datei erstellen: html schreiben
			// open
			int htmlfile_fd = open("dir_bulletlist.html", O_WRONLY | O_CREAT | O_TRUNC, 0640);
			if (htmlfile_fd < 0) {
				perror("open failed"); exit(-1);
			}
			// write
            write_fd = write(htmlfile_fd, "<html><body><ui>\n", 17);
            schreibfehlerTest(write_fd);
            char *host = getHost((char *)respath);
            while (1) {
                eintrag = readdir(dir);
				// Abbruch, wenn fertig gelesen
                if (eintrag == NULL) break;
				// parentDir und selfDir ignorieren
                if (strcmp(eintrag->d_name, ".")==0 ||
                    strcmp(eintrag->d_name, "..")==0){continue; }

                strcpy(pfadptr, eintrag->d_name);
                
                if (stat(pfadbuffer, &statbuf) == -1) {
                    perror("stat() failed:"); exit(1);
                } else {
                    
                    write_fd = write(htmlfile_fd, "	<li><a href=\"http://", 21);
                    schreibfehlerTest(write_fd);
                    write_fd = write(htmlfile_fd, host, strlen(host)-1);
                    schreibfehlerTest(write_fd);
					
					// pfadbuffer -> relativer Pfad
					char rel_path[PATH_MAX] = {0};
					strcpy(rel_path, pfadbuffer);
					remove_doubleSlash(rel_path);
					// basis loeschen
					strcpy(rel_path, rel_path + strlen(basis)-1);
					
                    write_fd = write(htmlfile_fd, rel_path, strlen(rel_path));
                    schreibfehlerTest(write_fd);

                    write_fd = write(htmlfile_fd, "\">", 2);
                    schreibfehlerTest(write_fd);                                  
                    write_fd = write(htmlfile_fd, strrchr(rel_path, '/')+1, strlen(strrchr(rel_path, '/')+1));
                    schreibfehlerTest(write_fd);
					write_fd = write(htmlfile_fd, "</a></li>\n", 10);
                    schreibfehlerTest(write_fd);
                    
                }
            }
            write_fd = write(htmlfile_fd, "</ui></body></html>", 19);
            schreibfehlerTest(write_fd);
			free(host);
            closedir(dir);
        	// close
			close(htmlfile_fd);

			/*** get content-lenght of file ***/
         	struct stat statStruct;
        	stat(path, &statStruct);
			int contentLenght = statStruct.st_size;
            
			/*** send HTTP-header ***/
			char contentLenghtString[ziffernAnz(contentLenght)];
			sprintf((char *)contentLenghtString, "%d", contentLenght);
			char *string1 = "HTTP/1.0 200 OK\r\nContentType: ";
			char *string2 = "\r\nContent-Lenght: ";
			char *string3 = "\r\n\n";
                          
			schreibfehlerTest(write(outfd, string1, strlen(string1)));
		    schreibfehlerTest(write(outfd, "text/html", strlen("text/html")));
            schreibfehlerTest(write(outfd, string2, strlen(string2)));
            schreibfehlerTest(write(outfd, contentLenghtString, strlen(contentLenghtString)));
            schreibfehlerTest(write(outfd, string3, strlen(string3)));


			/*** send html ***/			
            // open
			htmlfile_fd = open("dir_bulletlist.html", O_RDONLY);
			if (htmlfile_fd < 0) {
				perror("open failed"); exit(-1);
			}
			// read
			char *bulletContent = calloc(contentLenght, sizeof(char));
			if (read(htmlfile_fd, bulletContent, contentLenght) < 0) {
				perror("read failed"); exit(-1);
			}
			// send content
       		schreibfehlerTest(write(outfd, bulletContent, contentLenght));
			// close
			free(bulletContent);
			close(htmlfile_fd);
			
          }  

    }
    
    /* (3) Nicht vorhanden */
    if (!isDir && !exists) {
		not_found(outfd);
    } 
    
    free(path);
    
}

