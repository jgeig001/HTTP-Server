# HTTP-Server
Der HTTP-Server beantwortet GET anfragen nach statischen Dokumenten. Unterstützte MIME-Types sind: text/html, text/plain, image/jpeg, image/png, image/gif und image/x-icon  
Der Webserver ist in C(C99) programmiert.

**Starten:**

Mithilfe des Makefiles kann eine ausführbare Datei erstellt werden.  
Zum starten des Servers: 

    ./webserver [basisverzeichnis] [port] 
    z.B.: ./webserver ~/home/my_webdocs 8085  
    
Wird kein Port angegeben, wird Port 8080 verwendet. 
Wird kein Basisverzeichnis angegeben wird das aktuelle Verzeichnis genommen.

**Beenden:**

Der Server kann mit ctrl-c oder mit Aufruf von "./webserver stop" im selben Verzeichnis beendet werden.

**Bausteine:** 

* sockserv.c:  
  * sockserv.do_serve() erzeugt pro Request einen eigenen Childprozesse in dem die Anfragen abgearbeitet wird. 
  * Jede Anfrage wird in einem Logfile gesichert(sockserve.server_log())  
  * Die pid des Webservers wird in einem pidFile.pid gespeichert, damit der Server per "./webserver stop" mit Hilfe der pid beendet werden kann.

* process_request.c: 
  * process_request(int infd, int outfd, char *basis) list Request von infd und schreibt Antwort auf outfd.
  
* prozent.c:  
  * URL-Pfade können Prozent-codierte Zeichen enthalten. Diese werden mit prozent.prozentdecode() decodiert.
  
* do_get.c:  
  * do_get() sucht Content von Basisverzeichnis aus und schreibt Antwort inklusive HTTP_Header auf outfd.  
  * Handelt es sich um ein Verzeichnis wird ein html-Dokument mit einer Liste von Links auf die Inhalte gesendet.  
  * Bezeichnet die angefragete URL ein ausführbares Programm wird dies gestartet und dessen stdout als HTTP_Response gesendet.  
  * Ist die URL ungültig, wird eine HTTP_Fehlerseite(404) zurückgegeben.
 
