#include <stdio.h>      
#include <sys/types.h>
#include <sys/socket.h>   
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <ctype.h>

//TODO: implementare controllo lato client e server per lo spazio prima di newline

/**
 * Controlla se il messaggio è ben formato, con il corretto utilizzo di spazi e carattere newline
 */
int checkSpacesNewline(char *buffer, int *simpleChildSocket) {
    // check newline alla fine del messaggio ricevuto dal client
    if (buffer[strlen(buffer)-1] != '\n') {
        // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
        sprintf(buffer, "ERROR I messaggi inviati dal client devono terminare con il carattere newline\n");
        write(*simpleChildSocket, buffer, strlen(buffer));
        return 1;
    }

    // check whitespaces nel messaggio ricevuto dal client
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
                // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
                sprintf(buffer, "ERROR Troppi spazi rilevati nel messaggio ricevuto dal client\n");
                write(*simpleChildSocket, buffer, strlen(buffer));
                return 1;
            }
        }
    }
    return 0;
}


int main(int argc, char *argv[]) {

    int simpleSocket = 0;
    int simplePort = 0;
    int returnStatus = 0;
    int maxWords = 10; // numero massimo di parole gestibili per messaggio dal server. Default=10
    int argvCheck; // variabile per memorizzare argv[2], ovvero [<max parole>]
    char buffer[512] = "";
    char cpyBuffer[512] = ""; // copia del buffer, utile nella sprintf per aggiungere parole e stringhe al buffer
    char *errorMessage = "Il numero di parole inserite eccede il numero massimo di parole accettabili dal server";
    char whitespace;
    struct sockaddr_in simpleServer;

    /*    PUNTO 1    */

    if (2 != argc && 3 != argc) {

        fprintf(stderr, "Usage: %s <port> [<max words>]\n", argv[0]);
        exit(1);

    }

    /*    PUNTO 2    */

    /* gestisco il caso in cui venga indicato il numero massimo di parole gestibili per messaggio (maxWords).
    Altrimenti, la variabile è inizializzata di default al valore 10 */
    if (3 == argc) {
        argvCheck = atoi(argv[2]);
        if (argvCheck<1 || argvCheck>30) {
            fprintf(stderr, "Usage: %s <server> <port> [<max words>]\n -> <max words> può assumere valore tra 1 e 30\n", argv[0]);
            exit(1);
        }
        maxWords = argvCheck;
    }

    simpleSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (simpleSocket == -1) {

        fprintf(stderr, "Could not create a socket!\n");
        exit(1);

    }
    else {
	    fprintf(stdout, "Socket created!\n");
    }

    /* retrieve the port number for listening */
    simplePort = atoi(argv[1]);

    /* setup the address structure */
    /* use INADDR_ANY to bind to all local addresses  */
    memset(&simpleServer, '\0', sizeof(simpleServer)); 
    simpleServer.sin_family = AF_INET;
    simpleServer.sin_addr.s_addr = htonl(INADDR_ANY);
    simpleServer.sin_port = htons(simplePort);

    /*  bind to the address and port with our socket  */
    returnStatus = bind(simpleSocket,(struct sockaddr *)&simpleServer,sizeof(simpleServer));

    if (returnStatus == 0) {
	    fprintf(stdout, "Bind completed!\n");
    }
    else {
        fprintf(stderr, "Could not bind to address!\n");
        close(simpleSocket);
        exit(1);
    }

    /* lets listen on the socket for connections      */
    returnStatus = listen(simpleSocket, 5);

    if (returnStatus == -1) {
        fprintf(stderr, "Cannot listen on socket!\n");
	close(simpleSocket);
        exit(1);
    }

    while (1)

    {

        struct sockaddr_in clientName = { 0 };
        int simpleChildSocket = 0;
        int clientNameLength = sizeof(clientName);
        char clientAddress[INET_ADDRSTRLEN]; // stringa per memorizzare l'indirizzo IP del client

        // n° di parole indicate dal client nel messaggio "<Numero_parole> <parola1> <parola2> <parolaN>"
        int nWordsClient = 0;

        //n° di parole lette dal server
        int nWordsServer = 0;

        /* vettore posizionale lungo 12, utilizzato per memorizzare
        le lunghezze delle parole ricevute con le relative istanze */
        int instancesLengths[12] = {0};

        /* flag per iterare un nuovo ciclo del do-while.
        Se newLoop=1 itero un nuovo ciclo, altrimenti esco */
        int newLoop = 0;

        // flag per monitorare lo stato di ALERT (NAK, ERROR)
        int ALERT = 0;

        char words[512] = ""; // variabile per memorizzare "<parola1> <parola2> <parolaN>"

        // buffer dedicato alla memorizzazione delle parole per l'histo
        char histoBuffer[512] = "";

        // copia di histoBuffer, utile nella sprintf per aggiungere parole e stringhe al histoBuffer
        char cpyHistoBuffer[512] = "";

        char space;

        /* wait here */

        simpleChildSocket = accept(simpleSocket,(struct sockaddr *)&clientName, &clientNameLength);

        /*    PUNTO 4    */

        // memorizzo l'indirizzo IP del client clientName nella stringa clientAddress
        inet_ntop(AF_INET, &clientName.sin_addr, clientAddress, sizeof(clientAddress));

        // scrivo il messaggio "OK <Max Parole> <Messaggio>" sul buffer
        sprintf(buffer, "OK %d Benvenuto %s. Io calcolo istogrammi!\n", maxWords, clientAddress);

        if (simpleChildSocket == -1) {

            fprintf(stderr, "Cannot accept connections!\n");
            close(simpleSocket);
            exit(1);

        }

        // mando al client il messaggio di benvenuto nel formato "OK <Max Parole> <Messaggio>"
        write(simpleChildSocket, buffer, strlen(buffer));

        do {

            // setto a false il flag per fare un nuovo ciclo
            newLoop=0;

            int errorSpacesNewline = 0;

            /*    PUNTO 5    */

            // pulisco il buffer
            memset(buffer, 0, sizeof(buffer));
            // leggo il messaggio del client nel formato "<Numero_parole> <parola1> <parola2> <parolaN>"
            returnStatus = read(simpleChildSocket, buffer, sizeof(buffer));

            if (returnStatus<0) {
                exit(0);
            }
            // gestisco il caso in cui il client faccia ctrl+C per terminare il processo client
            if (returnStatus==0) {
                /* esco dal do while(), per cui smetto di interagire con il client che è terminato,
                ma resto in ascolto per altri client */
                break;
            }

            errorSpacesNewline = checkSpacesNewline(buffer, &simpleChildSocket);

            // se sono stati rilevati troppi spazi, interrompo il do while
            if (errorSpacesNewline!=0) break;

            // memorizzo <Numero_parole> in nWordsClient, e <parola1> <parola2> <parolaN> in words
            returnStatus = sscanf(buffer, "%d%c%[^\t\n]", &nWordsClient, &whitespace, words);

            // pulisco il buffer
            memset(buffer, 0, sizeof(buffer));

            if (nWordsClient==0) {
                // se nWordsClient==0, mi aspetto che words non sia istanziata
                if (returnStatus!=1) {
                    // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
                    sprintf(buffer, "ERROR Per interrompere la trasmissione dati, mandare 0\n");
                    write(simpleChildSocket, buffer, strlen(buffer));
                    break;
                }
            }
            else {
                // altrimenti, mi aspetto che whitespace e words siano istanziate
                if (returnStatus!=3) {
                    // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
                    sprintf(buffer, "ERROR Mandare il messaggio nel formato <Numero_parole> <parola1> <parola2> <parolaN>\n");
                    write(simpleChildSocket, buffer, strlen(buffer));
                    break;
                }
                // controllo che in whitespace sia correttamente istanziato un carattere space
                if (!isspace(whitespace)) {
                    // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
                    sprintf(buffer, "ERROR Mandare il messaggio nel formato <Numero_parole> <parola1> <parola2> <parolaN>\n\n");
                    write(simpleChildSocket, buffer, strlen(buffer));
                    break;
                }

                /*
                // controllo che words termini con newline
                if (words[strlen(words)-1] != '\n') {
                    // scrivo sul buffer "ERROR <Messaggio>" in modo che il client legga correttamente il messaggio
                    sprintf(buffer, "ERROR I messaggi inviati dal client devono terminare con il carattere newline\n");
                    write(simpleChildSocket, buffer, strlen(buffer));
                    break;
                }

                // elimino newline sostiuendolo con \0
                words[strlen(words)-1] = '\0';
                */
            }

            /*    PUNTO 6    */

            if (nWordsClient>0) {

                // TODO: rimuovere memset buffer commentati di seguito
                
                // caso 6a
                if (nWordsClient<=maxWords) {

                    nWordsServer=0; // n° di parole lette dal server
                    // itero words, ovvero la serie di parole <parola1> <parola2> <parolaN>
                    for (int i=0; i<=strlen(words); i++) {
                        // ogni volta che incontro uno spazio, aumento il contatore nWordsServer
                        if (isspace(words[i]) || words[i]=='\0') nWordsServer++;
                    }

                    // caso 6a i, caso 7
                    if (nWordsServer == nWordsClient) {

                        // se histoBuffer è vuoto, ovvero words è la prima stringa mandata dal server
                        if (strcmp("",histoBuffer)==0) {
                            // aggiungo words a histoBuffer
                            sprintf(histoBuffer, "%s", words);
                        }
                        // altrimenti, se in histoBuffer sono già state inserite una o più stringhe
                        else {
                            // copio histoBuffer in cpyHistoBuffer
                            strcpy(cpyHistoBuffer, histoBuffer);
                            // inserisco le stringhe già presenti nel histoBuffer e la nuova stringa
                            sprintf(histoBuffer, "%s %s", cpyHistoBuffer, words);
                        }

                        // aumento il contatore per l'ultima parola prima di \n
                        //nWordsServer++;

                        //TODO: rimuovere
                        // pulisco il buffer
                        //memset(buffer, 0, sizeof(buffer));

                        // scrivo nel buffer il messaggio nel formato "ACK <numero_parole_lette>"
                        sprintf(buffer, "ACK %d\n", nWordsServer);
                        // faccio ripartire il ciclo
                        newLoop=1;

                    }
                    // caso 6a ii, caso 8
                    else {
                        // scrivo nel buffer il messaggio nel formato "ERROR <Messaggio>"
                        sprintf(buffer, "ERROR Il numero di parole estratte dal server non corrisponde a quello indicato\n");
                        // esco dal ciclo
                        newLoop=0;
                    }

                }
                // caso 6b, caso 7
                else {
                    if (ALERT==0) {
                        // entro nello stato di ALERT
                        ALERT=1;

                        //TODO: rimuovere
                        // pulisco il buffer
                        //memset(buffer, 0, sizeof(buffer));

                        // scrivo nel buffer il messaggio nel formato "NAK <Max Parole> <Messaggio>"
                        sprintf(buffer, "NAK %d %s\n", maxWords, errorMessage);
                        // faccio ripartire il ciclo
                        newLoop=1;
                    }
                    //caso 6c, caso 8
                    else {

                        //TODO: rimuovere
                        // pulisco il buffer
                        //memset(buffer, 0, sizeof(buffer));
                        
                        // scrivo nel buffer il messaggio nel formato "ERROR <Messaggio>"
                        sprintf(buffer, "ERROR %s\n", errorMessage);
                        // esco dal ciclo
                        newLoop=0;
                    }
                }
                // mando al client il messaggio contenente ACK / NAK / ERROR
                write(simpleChildSocket, buffer, strlen(buffer));
            }

            // caso 6d, caso 9
            else if (nWordsClient==0) { 

                //printf("%s\n", histoBuffer);              

                // se il buffer è vuoto, mando l'errore. Altrimenti calcolo l'istogramma.
                if (strcmp(histoBuffer,"")==0) {
                    sprintf(buffer, "ERROR Non è stato possibile calcolare l'istogramma correttamente\n");
                }
                else {

                /* per tenere traccia delle varie lunghezze con le rispettive istanze, ho deciso di utilizzare
                un vettore posizionale (int instancesLengths[12]). Dal momento che gli indici del vettore
                vanno da 0 a 11, salverò il numero di parole di lunghezza n nella posizione n-1.
                Es: se ho 5 parole di lunghezza 7, l'istanza 5 verra salvata in posizione 7-1=6 */
                    
                    /* indice che si posiziona ogni volta all'inizio della parola,
                    in modo da poterne calcolare la lunghezza */
                    int lastIndex=0;
                    // indice per iterare histoBuffer
                    int i=0;
                    // itero tutto histoBuffer
                    for (i=0; i<strlen(histoBuffer)/*histoBuffer[i]!='\0'*/; i++) {
                        // quando incontro lo spazio alla fine di una parola
                        if (isspace(histoBuffer[i]) /*histoBuffer[i]==' '*/ ) {
                            /* se la lunghezza della parola, calcolata con (i-lastIndex),
                            è maggiore di 12 (massimo intervallo di lunghezza) */
                            if ((i-lastIndex)>12) {
                                /* aumento le instanze del massimo intervallo di lunghezza 12,
                                memorizzate in 12-1=11 */
                                instancesLengths[11]++;
                            }
                            else {
                                // altrimenti aumento le istanze di length-1=(i-lastIndex-1)
                                instancesLengths[i-lastIndex-1]++;
                            }
                            // incremento lastIndex in modo che punti sempre all'inizio della prossima parola
                            lastIndex=i+1;
                        }
                    }
                    /* stesso procedimento visto prima. Lo svolgo un'ultima volta fuori dal ciclo
                    per considerare l'ultima parola del buffer */
                    if ((i-lastIndex)>12) {
                        instancesLengths[11]++;
                    }
                    else {
                        instancesLengths[i-lastIndex-1]++;
                    }
                    lastIndex=i+1;

                    // pulisco il buffer
                    memset(buffer, 0, sizeof(buffer));
                    
                    int nLengths = 0;

                    // itero instancesLengths[12]
                    for (int i=0; i<12; i++) {
                        // se in i ci sono salvate delle istanze
                        if (instancesLengths[i]!=0) {
                            // incremento il contatore nLengths, relativo al numero di lunghezze diverse
                            nLengths++;
                            // se è la prima istanza che trovo nel vettore
                            if (nLengths==1) {
                                // copio nel buffer la lunghezza (i+1) e le istanze ad essa relative (instancesLengths[i])
                                sprintf(buffer, "%d %d", i+1, instancesLengths[i]);
                            }
                            // altrimenti, se il buffer è già stato popolato da altri valori
                            else {
                                // copio il buffer in cpyBuffer
                                strcpy(cpyBuffer, buffer);
                                // copio nel buffer <cpyBuffer lunghezza instanze>
                                sprintf(buffer, "%s %d %d", cpyBuffer, i+1, instancesLengths[i]);
                            }
                        }
                    }

                    // dopo aver inserito tutti i valori, aggiungo all'inizio del buffer HISTO <numero lunghezze>
                    strcpy(cpyBuffer, buffer);
                    sprintf(buffer, "HISTO %d %s\n", nLengths, cpyBuffer);
                }
                // mando al client il messaggio HISTO o ERROR
                write(simpleChildSocket, buffer, strlen(buffer));
                newLoop=0;
            }
        
        } while (newLoop==1); // se nel punto 6 ho settato newLoop a 1, itero un nuovo ciclo

        close(simpleChildSocket);

    }

    close(simpleSocket);
    return 0;

}
