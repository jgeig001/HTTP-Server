#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "process_request.h"
#include "prozent.h"
#include "do_get.h"

#define MAX_GET_REQUEST_LENGHT 8192
#define MAX_PID_LENGTH 50

/*
 * Der Signalhandler faengt das Signal SIGCHLD ab
 */
void signalHandler(int sig)
{
    int status;
    wait(&status);
    printf("ends with status:%d\n", status);
}

/*
 * Die Methode schreibt für jeden Request einen Logeintrag mit Zeitstempel
 * in "sever.log".
 */
void server_log(char *requestBuffer)
{
	char *logBuffer = calloc(strlen(requestBuffer) + 80, sizeof(char));
    char timeBuffer[80];
    time_t rawtime;
    struct tm *info;
    time( &rawtime );
    info = localtime(&rawtime);
    strftime(timeBuffer,80,"%x - %X", info);
    
    strcat(logBuffer, timeBuffer);
    strcat(logBuffer, " # ");
    
	// liest die erste Zeile des Requests ein
    char *ptr = requestBuffer;
	while (*ptr != '\n' && *ptr != '\0') {
		strncat(logBuffer, ptr, 1);
        ptr += 1;
    }

	//oeffnen
    int log_fd = open("server.log", O_WRONLY | O_CREAT | O_APPEND, 0640);
    if(log_fd < 0){
		perror("open failed"); exit(1);
    }
    //schreiben
    if(write(log_fd, logBuffer, strlen(logBuffer)) < 0){
		perror("write failed"); exit(1);
	}
	
	close(log_fd);
	free(logBuffer);	
}
/*
 * Started eine neuen SohnProzess zum abarbeiten des Requests.
 * "basis" ist das Basisverzeichnis, von dem ausgegangen werden soll.
 */
int do_serve(int port, char *basis)
{
    int sockfd, newsockfd, clientlen, childpid;
    struct sockaddr_in servaddr, clientaddr;
    
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&clientaddr, 0, sizeof(clientaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind error");
        exit(-1);
    }
    if (listen(sockfd, 15) < 0) {
        perror("listen error");
        exit(-1);
    }
    
    while (1) {
		
		// auf eingehende Verbindzung warten und entgegenehmen
		do {
		    clientlen = sizeof(clientaddr);
		    newsockfd = accept(sockfd, (struct sockaddr *) &clientaddr, (socklen_t *) &clientlen);
		} while (newsockfd == -1 && errno == EINTR);
        if (newsockfd < 0) {
            perror("accept error");
			exit(-1);
        }

		// child-process erzeugen
        if ((childpid = fork()) < 0) {
            perror("server: fork failed");
            exit(-1);
        } else if (childpid == 0) {

			/* >>> Sohnprozess <<< */

            close(sockfd);// schliesst Socket des Vaterprozess

            // liest den originalen HTTP-Request von newsockfd
            char *originRequest = calloc(MAX_GET_REQUEST_LENGHT, sizeof(char));
            if(read(newsockfd, originRequest, MAX_GET_REQUEST_LENGHT) < 0){
                perror("read");
                exit(0);
            }
			
            // finalen Request(inkl. basis als Einschub!!!) in requestBuffer zwischenspeichern
            int bufferLenght = MAX_GET_REQUEST_LENGHT + strlen(basis);
            char *requestBuffer = calloc(bufferLenght, sizeof(char));
            // schreibt in requestBuffer
            // erst nur "GET "...
            int lenghtOfGET = 4; /* ^= strlen("GET ") */
            strncat(requestBuffer, (char *)originRequest, lenghtOfGET);
            // dann die basis einschieben ...
            strcat(requestBuffer, basis);
            // und den Rest
            strcat(requestBuffer, (char *)originRequest + lenghtOfGET);
			printf("%s", requestBuffer);
          	
            // schreibt requestBuffer in "request.dat"
            int fd_request = open("request.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
            if(fd_request < 0){
                perror("open failed");
                exit(-1);
            }
            if(write(fd_request, requestBuffer, bufferLenght) < 0){
                perror("write failed");
                exit(-1);
            }
            
			/* entry in logfile */
			server_log((char *)requestBuffer);

			/* Request beantworten */
            process_request(fd_request, newsockfd, basis);

			free(originRequest);
			free(requestBuffer);
            close(fd_request);
            exit(0);
			
        } else {

			/* >>> Vaterprozess <<< */			

            close(newsockfd);// schliesst Socket des Sohnprozess

            signal(SIGCHLD, signalHandler);
			
        }
		
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char pidBuffer[MAX_PID_LENGTH] = {0};

    if (argc == 2 && !strcmp(argv[1], "stop")) {

		/* killt Webserver */
        
        int fd_pid = open("pidFile.pid", O_RDONLY | O_CREAT , 0640);
        if (fd_pid < 0) {
            perror("open failed");
            exit(-1);
        }
        if (read(fd_pid, (char *)pidBuffer, MAX_PID_LENGTH) < 0) {
            perror("read failed");
            exit(-1);
        }
		if (close(fd_pid) != 0) {
			perror("close failed");
			exit(-1);
		}
		kill((pid_t)atoi(pidBuffer), SIGINT);
		printf("killed pid:%s", pidBuffer);
		
    } else {

    	/* normal starten */
    
        snprintf(pidBuffer, MAX_PID_LENGTH, "%ld\n", (long)getpid());

        // schreibt pid in "pidFile.pid" im ausgeführtem Verzeichnis
        int fd_pid = open("pidFile.pid", O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if(fd_pid < 0){
            perror("open failed");
            exit(-1);
        }
        if (write(fd_pid, pidBuffer, MAX_PID_LENGTH) < 0) {
            perror("write error!");
            exit(-1);
        }
		if (close(fd_pid) != 0) {
			perror("close failed");
			exit(-1);
		}
		printf("server-pid:%s\n", pidBuffer);
        
        // Einlesen von Port und Basisverzeichnis
        int port;
        char *basis;
        if (argc == 2) {
            char *tmp = argv[1];
            if(tmp[0] >= '0' && tmp[0] <= '9') {
                port = atoi(argv[1]);
                basis = ".";
            } else {
                port = 8080;
                basis = argv[1];
            }
        } else if (argc == 3) {
            port = atoi(argv[2]);
            basis = argv[1];
        } else if (argc == 1) {
            port = 8080;
            basis = ".";
        }
        
		// respond
        do_serve(port, basis);
    }
	
	return 0;
}

