#include <stdio.h>      
#include <sys/types.h>
#include <sys/socket.h>   
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

/**
 * @author: Elia Marisio
 */

/**
 * Controlla se il messaggio è ben formato, con il corretto utilizzo di spazi e carattere newline
 */
void checkSpacesNewline(char *buffer) {
    // check newline alla fine del messaggio ricevuto dal server
    if (buffer[strlen(buffer)-1] != '\n') {
        fprintf(stdout, "I messaggi inviati dal server devono terminare con il carattere newline\n");
        exit(1);
    }

    // check whitespaces nel messaggio ricevuto dal server
    /* controllo che non ci sia uno spazio eccessivo all'inizio della stringa, o più spazi
    consecutivi all'interno, o uno spazio eccessivo alla fine della stringa */
    // indice al primo carattere
    int j=0;
    // itero la stringa. i è l'indice al secondo carattere
    for (int i=1; i<strlen(buffer); i++) {
        j=i-1;
        // se in j c'è uno spazio
        if (isspace(buffer[j])) {
            /* se j è il primo carattere (spazio eccessivo all'inizio),
            oppure se in i=j+1 c'è uno spazio (spazi consecutivi),
            oppure se in i=j+1 c'è newline (spazio eccessivo alla fine) */
            if (j==0 || isspace(buffer[i]) || buffer[i]=='\n') {
                fprintf(stdout, "Troppi spazi rilevati nel messaggio ricevuto dal server\n");
                exit(1);
            }
        }
    }
}


int main(int argc, char *argv[]) {

    int simpleSocket = 0;
    int simplePort = 0;
    int returnStatus = 0;
    int maxWords; // numero massimo di parole gestibili per messaggio dal server
    int inputMethod; // variabile per memorizzare il metodo di input
    int inputParameter; // variabile per memorizzare il criterio di input
    int nWordsClient; // numero di parole scritte dal client nella stringa
    int nWordsServer; // numero di parole lette dal server
    
    /* flag per iterare un nuovo ciclo del do-while.
    Se newLoop=1 itero un nuovo ciclo, altrimenti esco */
    int newLoop = 0;

    /* flag per indicare se ho ancora dati da trasmettere.
    Se endData=1 ho finito, altrimenti chiedo un nuovo input */
    int endData = 0;

    // contatore del numero di lunghezze diverse (primo valore dell'output HISTO)
    int nLengths = 0;

    // variabile per memorizzare le singole lunghezze dell'istogramma
    int histoLength = 0;

    // varibile per memorizzare le singole istanze dell'istogramma
    int histoInstance = 0;

    /* flag per gestire il caso in cui, ricevuto NAK, ritrasmetto il contenuto
    del messaggio errato, rispettando i requisiti del server.
    Se ho ricevuto NAK alla scorsa iterazione, setto correctedNAK=1 e non chiedo l'input all'utente
    perché ho in memoria nel buffer il NAK corretto, che spedisco al server.
    Se correctedNAK=0, procedo normalmente */
    int correctedNAK = 0;

    char word[25]; // variabile per memorizzare singole parole ricevute in input
    char buffer[512] = "";
    char cpyBuffer[512] = ""; // copia del buffer, utile nella sprintf per aggiungere parole e stringhe al buffer
    char welcomeMessage[25]; // variabile per memorizzare il messaggio di benvenuto del server
    char fileName[25]; // variabile per memorizzare il nome del file che si desidera aprire per l'input
    char answer[5]; // variabile per memorizzare ACK / NAK / ERROR / HISTO
    char other[512]; // variabile per memorizzare la parte del messaggio successiva ad answer[5]
    char endDatachar; // char che può assumere valori 'y' (yes) o 'n' (no)
    char whitespace1; // variabile per memorizzare il carattere whitespace durante la sscanf dei messaggi del server
    char whitespace2; // variabile per memorizzare il carattere whitespace durante la sscanf dei messaggi del server
    char *token; // variabile per memorizzare il token restituito dalla strtok
    FILE *fPtr; // puntatore a file
    struct sockaddr_in simpleServer;

    /*    PUNTO 1    */

    if (3 != argc) {

        fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
        exit(1);

    }

    /* create a streaming socket      */
    simpleSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (simpleSocket == -1) {

        fprintf(stderr, "Could not create a socket!\n");
        exit(1);

    }
    else {
	    fprintf(stdout, "Socket created!\n");
    }

    /* retrieve the port number for connecting */
    simplePort = atoi(argv[2]);

    /* setup the address structure */
    /* use the IP address sent as an argument for the server address  */
    //bzero(&simpleServer, sizeof(simpleServer)); 
    memset(&simpleServer, '\0', sizeof(simpleServer));
    simpleServer.sin_family = AF_INET;
    //inet_addr(argv[2], &simpleServer.sin_addr.s_addr);
    simpleServer.sin_addr.s_addr=inet_addr(argv[1]);
    simpleServer.sin_port = htons(simplePort);

    /*  connect to the address and port with our socket  */
    returnStatus = connect(simpleSocket, (struct sockaddr *)&simpleServer, sizeof(simpleServer));

    if (returnStatus == 0) {
	    fprintf(stdout, "Connect successful!\n");
    }
    else {
        fprintf(stderr, "Could not connect to address!\n");
        close(simpleSocket);
        exit(1);
    }

    /*    PUNTO 3    */

    /* get the message from the server   */

    // leggo il messaggio del server nel formato "OK <Max Parole> <Messaggio>"
    returnStatus = read(simpleSocket, buffer, sizeof(buffer));

    if ( returnStatus <= 0 ) {
        fprintf(stderr, "Return Status = %d \n", returnStatus);
    }

    checkSpacesNewline(buffer);

    /*    PUNTO 4    */

    /* memorizzo maxWords e welcomeMessage, ignoro OK
     * grazie a %[^\t] memorizzo in welcomeMessage qualsiasi carattere tranne \t */
    returnStatus = sscanf(buffer, "OK%c%d%c%[^\t]", &whitespace1, &maxWords, &whitespace2, welcomeMessage);

    // controllo che la sscanf abbia correttamente memorizzato 4 variabili
    if (returnStatus!=4) {
        fprintf(stderr, "Mandare il messaggio nel formato OK <Max Parole> <Messaggio>\n");
        exit(1);
    }
    // controllo che in whitespace sia correttamente istanziato un carattere space
    if (!(isspace(whitespace1) && isspace(whitespace2))) {
        fprintf(stderr, "Mancanza di spazi nel messaggio ricevuto dal server\n");
        exit(1);
    }

    fprintf(stdout, "Welcome message from server: %s", welcomeMessage);
    fprintf(stdout, "Numero massimo di parole gestibili dal server in ogni singolo messaggio: %d\n", maxWords);

    do {

        // Se correctedNAK=0, procedo normalmente (andare alla definizione della varibile per maggiori info)
        if (correctedNAK==0) {

            // se entro nel ciclo di nuovo, chiedo se si hanno ancora dati da trasmettere
            if (newLoop!=0) {
                fprintf(stdout, "Si desidera trasmettere altri dati?\nInserire 'y' per accettare e continuare, oppure 'n' per declinare e terminare: ");
                scanf("%c%*c", &endDatachar);

                if (endDatachar=='y') endData=0;
                else if (endDatachar=='n') {
                    endData=1;
                }
                else {
                    fprintf(stderr, "Input indefinito. Inserire 'y' o 'n'\n");
                    exit(1);
                }
            }
            
            newLoop=0;
            memset(buffer, 0, sizeof(buffer));

            if (endData==1) {
                sprintf(buffer, "%d\n", 0);
                write(simpleSocket, buffer, strlen(buffer));
            }
            else {
                
                /*    PUNTO 5    */

                fprintf(stdout, "Metodi di input disponibili\n");
                fprintf(stdout, " 1. Tastiera\n 2. File\nInserire 1 o 2 e premere invio: ");
                scanf("%d%*c", &inputMethod);
                if (inputMethod!=1 && inputMethod!=2) {
                    fprintf(stderr, "Input indefinito. Inserire 1 o 2\n");
                    exit(1);        
                }

                fprintf(stdout, "\nCriterio di inserimento parole:\n");
                fprintf(stdout, " 1. Una alla volta [parola invio parola invio]\n");
                fprintf(stdout, " 2. Tutte insieme [parola parola parola invio]\n");
                fprintf(stdout, "Inserire 1 o 2 e premere invio: ");   
                scanf("%d%*c", &inputParameter);
                if (inputParameter!=1 && inputParameter!=2) {
                    fprintf(stderr, "Input indefinito. Inserire 1 o 2\n");
                    exit(1);        
                }

                nWordsClient=0;

                if (inputMethod == 1) {
                    if (inputParameter == 1) {
                        fprintf(stdout, "Al termine, digitare <STOP> e premere invio\n");

                        do {
                            fprintf(stdout, "Inserire parola: ");
                            // grazie a "%s%*c" consumo con %*c il carattere newline
                            scanf("%s%*c", word);
                            // incremento il contatore
                            nWordsClient++;

                            // se è la prima parola, la aggiungo al buffer
                            if (strcmp(buffer,"")==0) sprintf(buffer, "%s", word);
                            else {
                                // altrimenti, devo copiare anche le parole già presenti sul buffer
                                strcpy(cpyBuffer, buffer);
                                // se la parola è diversa da STOP la aggiungo
                                if (strcmp(word, "STOP")!=0) sprintf(buffer, "%s %s", cpyBuffer, word);
                                // altrimenti aggiungo \n
                                else sprintf(buffer, "%s\n", cpyBuffer);
                            }
                        } while (strcmp(word, "STOP")!=0);

                        // decremento il contatore, che considera anche la parola STOP
                        nWordsClient--;
                    }
                    else {
                        fprintf(stdout, "Inserire le parole separate da uno spazio. Premere invio alla fine:\n");
                        fgets(buffer, sizeof(buffer), stdin);
                        for (int i=0; i<strlen(buffer); i++) {
                            /* ogni volta che incontro uno spazio, o il carattere finale newline,
                               aumento il contatore nWordsClient */
                            if (isspace(buffer[i]) || buffer[i]=='\n') nWordsClient++;
                        }
                    }
                }
                else {
                    fprintf(stdout, "Inserire nome file presente della stessa directory: ");
                    scanf("%s%*c", fileName);

                    fPtr = fopen(fileName, "r");

                    if (fPtr==NULL) {
                        fprintf(stderr, "Impossibile accedere al file\n");
                        exit(1);
                    }

                    // finché non viene raggiunta la fine del file
                    while (!feof(fPtr)) {
                        if (inputParameter == 1) {
                            // se è la prima parola, la aggiungo direttamente del buffer
                            if (nWordsClient==0) fscanf(fPtr, "%s\n", buffer);
                            // altrimenti, devo copiare anche le parole già presenti sul buffer
                            else {
                                strcpy(cpyBuffer, buffer);
                                fscanf(fPtr, "%s\n", word);
                                sprintf(buffer, "%s %s", cpyBuffer, word);
                            }             
                        }
                        else {
                            if (nWordsClient==0) fscanf(fPtr, "%s%*c", buffer);
                            else {
                                strcpy(cpyBuffer, buffer);
                                fscanf(fPtr, "%s%*c", word);
                                sprintf(buffer, "%s %s", cpyBuffer, word);
                            }
                        }
                        // aumento il contatore per l'ultima parola prima di \n
                        nWordsClient++;
                    }
                    strcpy(cpyBuffer, buffer);
                    // aggiungo alla fine del buffer \n
                    sprintf(buffer, "%s\n", cpyBuffer);

                    fclose(fPtr);
                }

                // copio il contenuto del buffer in cpyBuffer
                strcpy(cpyBuffer, buffer);
                // aggiungo all'inizio del buffer nWordsClient
                sprintf(buffer, "%d %s", nWordsClient, cpyBuffer);
            
                /*    PUNTO 6    */

                // mando al server il messaggio nel formato "<Numero_parole> <parola1> <parola2> <parolaN>"
                write(simpleSocket, buffer, strlen(buffer));
            }
        }
        /* se ho ricevuto NAK alla scorsa iterazione, setto correctedNAK=1 e non chiedo l'input all'utente
        perché ho in memoria nel buffer il NAK corretto, che spedisco al server. */
        else {
            /*    PUNTO 6    */

            // mando al server il messaggio nel formato "<Numero_parole> <parola1> <parola2> <parolaN>"
            write(simpleSocket, buffer, strlen(buffer));

            correctedNAK=0;
        }

        /*    PUNTO 7    */

        // pulisco il buffer
        memset(buffer, 0, sizeof(buffer));
        // leggo il messaggio del server contenente ACK / NAK / ERROR / HISTO
        returnStatus = read(simpleSocket, buffer, sizeof(buffer));

        if ( returnStatus <= 0 ) {
            fprintf(stderr, "Return Status = %d \n", returnStatus);
        }

        checkSpacesNewline(buffer);

        returnStatus = sscanf(buffer, "%s%c%[^\t]", answer, &whitespace1, other);

        if (returnStatus!=3) {
            fprintf(stderr, "Messaggi accettabili:\n ACK <numero>\n NAK <numero>\n HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>\n ERROR <Messaggio>\n");
            exit(1);
        }
        if (!isspace(whitespace1)) {
            fprintf(stderr, "Mancanza di spazi nel messaggio ricevuto dal server\n");
            exit(1);
        }

        // caso 7a, caso 8
        if (strcmp("ACK", answer)==0) {
            // salvo il n° di parole calcolate dal server
            returnStatus = sscanf(other, "%d", &nWordsServer);

            if (returnStatus!=1) {
                fprintf(stderr, "Messaggio accettabile: ACK <numero>\n");
                exit(1);
            }

            // in ogni caso faccio ripartire il ciclo
            newLoop=1;
            // caso 9
            if (nWordsServer!=nWordsClient) {
                // setto il flag per indicare che non ho più dati da trasmettere
                endData=1;
                fprintf(stderr, "Il numero di dati ricevuti è diverso dal numero di dati trasmessi\n");
            }
        }

        // caso 7b, caso 10
        /* ritrasmetto il contenuto del messaggio errato, rispettando i requisiti del server,
        ovvero ottenendo una substringa di lunghezza maxWords */
        if (strcmp("NAK", answer)==0) {
            // in ogni caso faccio ripartire il ciclo
            newLoop=1;
            // azzero il contatore del numero di parole scritte dal client
            nWordsClient=0;
            int i=0;
            // cpyBuffer contiene ancora la serie di parole
            // itero cpyBuffer, tenendo traccia dell'indice quando finisco la decima parola (maxWords)
            for (i=0; i<strlen(cpyBuffer) && nWordsClient<maxWords; i++) {
                // ogni volta che incontro uno spazio, aumento il contatore nWordsClient
                if (isspace(cpyBuffer[i])) nWordsClient++;
            }
            /* dichiaro una substring lunga i+1, perché newline occupa 2 caratteri
            ed i punta al primo carattere (spazio) dopo l'ultima parola */
            char sub[512];

            /* copio in sub la substringa di cpyBuffer dall'indice 0 ad i-1
            (per escludere l'ultimo spazio) dopo l'ultima parola */
            memcpy(sub, cpyBuffer, i-1);

            // pulisco il buffer
            memset(buffer, 0, sizeof(buffer));
            // aggiungo all'inizio del buffer nWordsClient
            sprintf(buffer, "%d %s\n", nWordsClient, sub);

            /* setto il flag per indicare che, alla prossima iterazione, devo direttamente ritrasmettere
            al server il messaggio corretto, senza chiede una nuova stringa in input */
            correctedNAK=1;
        }

        /*    PUNTO 11    */

        if (strcmp("HISTO", answer)==0) {
            // memorizzo <numero lunghezze> in nLengths, e <lunghezza1> <istanze1> <lunghezza2> <istanze2> in other
            returnStatus = sscanf(buffer, "HISTO%c%d%c%[^\t]", &whitespace1, &nLengths, &whitespace2, other);

            if (returnStatus!=4) {
                fprintf(stderr, "Messaggio accettabile: HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>\n");
                exit(1);
            }
            if (!(isspace(whitespace1) && isspace(whitespace2))) {
                fprintf(stderr, "Mancanza di spazi nel messaggio ricevuto dal server\n");
                exit(1);
            }

            if (nLengths==1) {
                fprintf(stdout, "Ho ricevuto dal server %d sola lunghezza diversa:\n", nLengths);
            }
            else fprintf(stdout, "Ho ricevuto dal server %d lunghezze diverse:\n", nLengths);
            
            token = strtok(buffer, " "); // scarto HISTO
            token = strtok(NULL, " "); // scarto nLengths
            
            for (int i=0; i<nLengths; i++) {
                // estraggo <lunghezza_i> e la memorizzo in histoLength
                token = strtok(NULL, " ");
                histoLength = atoi(token);
                // estraggo <istanza_i> e la memorizzo in histoInstance
                token = strtok(NULL, " ");
                histoInstance = atoi(token);

                if (histoLength==0 || histoInstance==0) {
                    fprintf(stderr, "Messaggio accettabile: HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>");
                    exit(1);
                }

                if (histoInstance==1) fprintf(stdout, "• %d parola di lunghezza %d\n", histoInstance, histoLength);
                else fprintf(stdout, "• %d parole di lunghezza %d\n", histoInstance, histoLength);
            }

            newLoop=0;
        }

        /*    PUNTO 12    */

        if (strcmp("ERROR", answer)==0) {
            fprintf(stderr, "%s", other);
            newLoop=0;
        }

    } while (newLoop==1);

    close(simpleSocket);
    return 0;

}
