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

#include "process_request.h"
#include "prozent.h"
#include "do_get.h"

#define MAX_GET_REQUEST_LENGHT 8192


/*
 * Der Signalhandler faengt das Signal SIGCHLD ab
 */
void signalHandler(int sig)
{
    int *status = malloc(1);
    int pid = wait(status);
    free(status);
}

/*
 * Die Methode schreibt für jeden Request einen Logeintrag mit Zeitstempel
 * in "sever.log".
 */
void server_log(char *requestBuffer)
{
	char *logBuffer = calloc(strlen(requestBuffer)+80, sizeof(char));
    char timeBuffer[80];
    time_t rawtime;
    struct tm *info;
    time( &rawtime );
    info = localtime(&rawtime);
    strftime(timeBuffer,80,"%x - %X", info);
    
    strcat(logBuffer, timeBuffer);
    strcat(logBuffer, " # ");
    
	// liest die erste Zeile der Request ein
    char *ptr = requestBuffer;
    while(strncmp(ptr, "\n", 1) != 0){
		strncat(logBuffer, ptr, 1);
        ptr+=1;
    }

	//oeffnen
    int log_fd = open("server.log", O_RDWR | O_CREAT | O_APPEND, 0640);
    if(log_fd < 0){
		perror("open"); exit(1);
    }
    //schreiben
    if(write(log_fd, logBuffer, strlen(logBuffer)) < 0){
		perror("write"); exit(1);
	}
	
	close(log_fd);
	free(logBuffer);	
}

//basis ist das Basisverzeichnis, von dem ausgegangen werden soll
int do_serve(int port, char *basis)
{
    int sockfd, newsockfd, clientlen, childpid;
    struct sockaddr_in servaddr, clientaddr;
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        exit(-1);
    }
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(-1);
    }
    
    while(1){
        clientlen = sizeof(struct sockaddr);
        newsockfd = accept(sockfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (newsockfd < 0) { perror("accept"); exit(-1); }
        if((childpid = fork()) < 0){
            perror("server: fork");
            exit(0);
        }else if(childpid == 0){// Sohnprozess
            close(sockfd);// schliesst Socket des Vaterprozess
            // liest den originalen HTTP-Request von newsockfd
            char *originRequest[MAX_GET_REQUEST_LENGHT];
            if(read(newsockfd, originRequest, MAX_GET_REQUEST_LENGHT) < 0){
                perror("read");
                exit(0);
            }
            // finalen Request(inkl. basis) in requestBuffer zwischenspeichern
            int bufferLenght = MAX_GET_REQUEST_LENGHT+strlen(basis);
            char requestBuffer[bufferLenght];
            
            // schreibt in requestBuffer
            // erst nur "GET "...
            strncat(requestBuffer, (char *)originRequest, 4);
            // dann die basis...
            strcat(requestBuffer, basis);
            // und den Rest
            char *originRequestRest;
            int lenghtOfGET = 4;
            originRequestRest = (char *)originRequest;
            originRequestRest+=lenghtOfGET;
            strcat(requestBuffer, originRequestRest);
            
            // schreibt requestBuffer in "request.dat"
            int fd_request = open("request.dat", O_RDWR | O_CREAT | O_TRUNC, 0666);
            if(fd_request < 0){
                perror("open");
                exit(0);
            }
            if(write(fd_request, requestBuffer, bufferLenght) < 0){
                perror("write");
                exit(0);
            }
            
            printf("\nrequestBuffer:%s\n", requestBuffer);
            
            process_request(fd_request, newsockfd);
            
			server_log((char *)requestBuffer);

            close(fd_request);
            exit(0);  
        } else {//Vaterprozess
            close(newsockfd);// schliesst Socket des Sohnprozess
            signal(SIGCHLD, signalHandler);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int max_pid_lenght = 10;
    char *pidBuffer[max_pid_lenght];
    
    // killt Webserver
    if(argc == 2 && !strcmp(argv[1], "stop")){
        
        int fd_pid = open("pid.pid", O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fd_pid < 0){
            perror("open");
            exit(0);
        }
        if(read(fd_pid, pidBuffer, max_pid_lenght) < 0){
            perror("write");
            exit(0);
        }
        kill();
    
    // normal starten
    }else{
        
        // schreibt pid in "pid.pid" im ausgeführtem Verzeichnis
        int fd_pid = open("pid.pid", O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fd_pid < 0){
            perror("open");
            exit(0);
        }
        // vorher Länge rausfindn...
        sprintf(pidBuffer, "%d", getpid());
        
        if(write(fd_pid, pidBuffer, max_pid_lenght) < 0){
            perror("write");
            exit(0);
        }
        kill(pidBuffer, 1);
        
        // Einlesen von Port und Basisverzeichnis
        int port;
        char *basis;
        if(argc == 2){
            char *tmp = argv[1];
            if(tmp[0] >= '0' && tmp[0] <= '9'){
                port = atoi(argv[1]);
                basis = ".";
            }else{
                port = 8080;
                basis = argv[1];
            }
        }else if(argc == 3){
            port = atoi(argv[2]);
            basis = argv[1];
        }else if(argc == 1){
            port = 8080;
            basis = ".";
        }
        
        do_serve(port, basis);
    }
}

