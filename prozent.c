#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * gibt eine dynamisch allokierte Zeichenkette zurück, in der
 * Prozent-codierte Zeichen durch ASCII-Aequivalente erstzt werden
 * Aufrufer muss Zeichenkette nach Gebrauch mit free() freigeben
 */
char *prozentdecode(const char* eingabe)
{   
    char *head = (char *)eingabe;
	int decodeAnz=0;
	//Speicherplatz berechnen
	while(*head){
		if(*head == '%' && *head+1 != '%'){
			decodeAnz+=1;
		}
        head++;
	}
    head = (char *)eingabe;
    int len = strlen(eingabe)-(decodeAnz*2);
    //ersetzen
    char *resultHead = malloc(len);
    char *resultPtr = resultHead;//return
    while(*head){
		if(*head == '%'){
            char tmp[2];
            strncpy(tmp, head+1, 2);
            int number = strtol(tmp, NULL, 16);  
            char c = (char)number;
            *resultHead = c;
            resultHead+=1;
            head+=2;
		}else if (*head != '%'){
            char tmp[1];
            char *tmpPtr = (char *)&tmp;
            strncpy(tmpPtr, head, 1);
            *resultHead = *tmpPtr;
            resultHead+=1;
        }
        head++;
	}
	return resultPtr;
}

/*
 * Wandelt eine Dezimalzahl in eine Hexadezimalzahl um
 * und gibt diese zurueck
 * Rueckgabe mit free() freigeben  
 */
char *hexConverter(char* c)
{    
    char letter[2];
    char *out = malloc(1);
    letter[0] = c[0];
    sprintf(out, "%02X", letter[0]);
    return out;
}

/*
 * Gibt die 'eingabe'-Zeichenkette zurueck, wobei alle Zeichen 
 * Prozent-codiert werden, die im zweiten Parameter 'zucodieren' 
 * aufgezaehlt sind.
 */
char *prozentencode(const char* eingabe, const char* zucodieren)
{   
    char *head = (char *)eingabe;
    char *ptr = (char *)zucodieren;
    int encodeAnz=0;
    //Speicherplatz berechnen
    while(*head){
        while(*ptr){
            if(*head == *ptr){
                encodeAnz+=1;
            }
            ptr++;
        }
        ptr = (char *)zucodieren;//zurücksetzen
        head++;
    }
    head = (char *)eingabe;//zurücksetzen
    int len = strlen(eingabe)+(encodeAnz*2);
    //ersetzen
    char *resultHead = malloc(len*sizeof(char)+1);
    char *resultPtr = resultHead;//return
    int flag;
    while(*head){
        //handelt es sich um einen der gesuchten Buchstaben
        //-> wenn ja: setzte flag
        while(*ptr){
            if(*head == *ptr){
                flag=1;
                break;
            }else{
                flag=0;
            }
            ptr++;
        }
        //kopiere entspechend nach flag
        //1:Sonderzeichen, 0:uebernehmen
        if(flag==1){
            *resultHead = '%';
            resultHead+=1;
            char *hex = hexConverter(head);
            *resultHead = *hex;
            resultHead+=1;
            hex+=1;
            *resultHead = *hex;
            hex--;
            free(hex);
        }else{
            *resultHead = *head;
        }
        resultHead+=1;
        ptr = (char *)zucodieren;//zuruecksetzen
        head++;
    }
    return resultPtr;
}
