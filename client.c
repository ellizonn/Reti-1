#include <stdio.h>      
#include <sys/types.h>
#include <sys/socket.h>   
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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

    // vettore per memorizzare le lunghezze dell'istogramma
    int histoLengths[12] = {0};

    // vettore per memorizzare le istanze dell'istogramma
    int histoInstances[12] = {0};

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

    /*    PUNTO 4    */

    /* memorizzo maxWords e welcomeMessage, ignoro OK
     * grazie a %[^\t] memorizzo in welcomeMessage qualsiasi carattere tranne \t */
    returnStatus = sscanf(buffer, "OK %d %[^\t]", &maxWords, welcomeMessage);

    // controllo che la sscanf abbia correttamente memorizzato 2 variabili
    if (returnStatus!=2) {
        fprintf(stderr, "Mandare il messaggio nel formato OK <Max Parole> <Messaggio>");
        exit(1);
    }

    // controllo che welcomeMessage termini con newline
    if (welcomeMessage[strlen(welcomeMessage)-1] != '\n') {
        fprintf(stderr, "I messaggi inviati dal server devono terminare con il carattere newline\n");
        exit(1);
    }

    fprintf(stdout, "Welcome message from server: %s", welcomeMessage);
    fprintf(stdout, "Numero massimo di parole gestibili dal server in ogni singolo messaggio: %d\n", maxWords);

    do {

        // se 
        if (correctedNAK==0) {

            // se entro nel ciclo di nuovo, chiedo se si hanno ancora dati da trasmettere
            if (newLoop!=0) {
                //TODO: rimuovere

                //endDatachar=' ';
                printf("Si desidera trasmettere altri dati?\nInserire 'y' per accettare e continuare, oppure 'n' per declinare e terminare: ");
                //scanf("%d", &endData);
                scanf("%c%*c", &endDatachar);
                /*getchar();
                endDatachar = getchar();
                getchar();*/
                /*if (endDatachar!='y' && endDatachar!='n') {
                    fprintf(stderr, "Input indefinito. Inserire 'y' o 'n'\n");
                }*/
                if (endDatachar=='y') endData=0;
                else if (endDatachar=='n') {
                    endData=1;
                    //TODO: rimuovere printf("Spedisco '0' al server\n");
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

                //TODO: rimuovere
                //break;
    /*
                memset(buffer, 0, sizeof(buffer));
                returnStatus = read(simpleSocket, buffer, sizeof(buffer));

                if ( returnStatus <= 0 ) {
                    fprintf(stderr, "Return Status = %d \n", returnStatus);
                }

                fprintf(stdout, "Message from server: %s", buffer);
                sscanf(buffer, "%s %[^\t]", answer, other);

                if (strcmp("HISTO", answer)==0) {
                    printf("%s", other);
                    //newLoop=0;
                    break;
                }
                if (strcmp("ERROR", answer)==0) {
                    printf("%s", other);
                    //newLoop=0;
                    break;
                }
    */
            }
            else {
                
                /*    PUNTO 5    */

                printf("Metodi di input disponibili\n");
                printf(" 1. Tastiera\n 2. File\nInserire 1 o 2 e premere invio: ");
                scanf("%d%*c", &inputMethod);
                if (inputMethod!=1 && inputMethod!=2) {
                    fprintf(stderr, "Input indefinito. Inserire 1 o 2\n");
                    exit(1);        
                }

                printf("\nCriterio di inserimento parole:\n");
                printf(" 1. Una alla volta [parola invio parola invio]\n");
                printf(" 2. Tutte insieme [parola parola parola invio]\n");
                printf("Inserire 1 o 2 e premere invio: ");   
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
                            printf("Inserire parola: ");
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

                        //TODO: rimuovere
                        /*printf("Inserire n° di parole: ");
                        scanf("%d", &nWordsClient);
                        for (int i=0; i<nWordsClient; i++) {
                            printf("Inserire parola: ");
                            scanf("%s", word);
                            strcpy(cpyBuffer, buffer);
                            // se è la prima parola, la aggiungo al buffer
                            if (i==0) sprintf(buffer, "%s", word);
                            // altrimenti, devo copiare anche le parole già presenti sul buffer
                            else sprintf(buffer, "%s %s", cpyBuffer, word);
                        }
                        getchar();
                        strcpy(cpyBuffer, buffer);
                        // aggiungo \n alla fine
                        sprintf(buffer, "%s\n", cpyBuffer);*/
                    }
                    else {
                        //TODO: rimuovere
                        //getchar();
                        printf("Inserire le parole separate da uno spazio. Premere invio alla fine:\n");
                        fgets(buffer, sizeof(buffer), stdin);
                        for (int i=0; i<strlen(buffer); i++) {
                            // ogni volta che incontro uno spazio, aumento il contatore nWordsClient
                            if (buffer[i]==' ' || buffer[i]=='\n') nWordsClient++;
                        }
                        // quando premo invio aumento il contatore per l'ultima parola
                        //nWordsClient++;
                    }
                }
                else {
                    printf("Inserire nome file presente della stessa directory: ");
                    scanf("%s%*c", fileName);
                    //getchar();
                    fPtr = fopen(fileName, "r"); 

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
                            if (nWordsClient==0) fscanf(fPtr, "%s ", buffer);
                            else {
                                strcpy(cpyBuffer, buffer);
                                fscanf(fPtr, "%s ", word);
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
                //TODO: rimuovere
            /*
                if (nWordsClient > maxWords) {
                    fprintf(stderr, "Il numero di parole inserite eccede il numero massimo di parole accettabili da server\n");
                    exit(1);
                }
            */
                // copio il contenuto del buffer in cpyBuffer
                strcpy(cpyBuffer, buffer);
                // aggiungo all'inizio del buffer nWordsClient
                sprintf(buffer, "%d %s", nWordsClient, cpyBuffer);
            
                //TODO: rimuovere printf("Buffer: %s", buffer);

                /*    PUNTO 6    */

                // mando al server il messaggio nel formato "<Numero_parole> <parola1> <parola2> <parolaN>"
                write(simpleSocket, buffer, strlen(buffer));
            }
        }
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
        //TODO: rimuovere printf("%s", buffer);

        if ( returnStatus <= 0 ) {
            fprintf(stderr, "Return Status = %d \n", returnStatus);
        }

        //TODO: rimuovere fprintf(stdout, "Message from server: %s", buffer);
        returnStatus = sscanf(buffer, "%s %[^\t]", answer, other);

        if (returnStatus!=2) {
            fprintf(stderr, "Messaggi accettabili:\n ACK <numero>\n NAK <numero>\n HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>\n ERROR <Messaggio>");
            exit(1);
        }

        // controllo che other termini con newline
        if (other[strlen(other)-1] != '\n') {
            fprintf(stderr, "I messaggi inviati dal server devono terminare con il carattere newline\n");
            exit(1);
        }

        // caso 7a, caso 8
        if (strcmp("ACK", answer)==0) {
            // salvo il n° di parole calcolate dal server
            returnStatus = sscanf(buffer, "ACK %d", &nWordsServer);

            if (returnStatus!=1) {
                fprintf(stderr, "Messaggio accettabile: ACK <numero>");
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
                if (cpyBuffer[i]==' ') nWordsClient++;
            }
            /* dichiaro una substring lunga i+1, perché newline occupa 2 caratteri
            ed i punta al primo carattere (spazio) dopo l'ultima parola */
            char sub[512];

            // copio in sub la substringa di cpyBuffer dall'indice 0 ad i
            memcpy(sub, cpyBuffer, i);

            //TODO: rimuovere printf("%d = %lu\n", i+1, strlen(sub));

            // pulisco il buffer
            memset(buffer, 0, sizeof(buffer));
            // aggiungo all'inizio del buffer nWordsClient
            sprintf(buffer, "%d %s\n", nWordsClient, sub);

            //TODO: rimuovere printf("%s", buffer);

            /* setto il flag per indicare che, alla prossima iterazione, devo direttamente ritrasmettere
            al server il messaggio corretto, senza chiede una nuova stringa in input */
            correctedNAK=1;

            //TODO: rimuovere
            /*
            int j=0;
            // salvo in sub il contenuto di cpyBuffer, tenendo conto i requisiti del server (maxWords)
            for (j=0; j<nWordsClient; j++) {
                sub[j] = cpyBuffer[j];
            }
            // aggiungo \n alla fine di sub
            sub[j] = '\n';
            */
        }
        if (strcmp("HISTO", answer)==0) {
            // memorizzo <numero lunghezze> in nLengths, e <lunghezza1> <istanze1> <lunghezza2> <istanze2> in other
            returnStatus = sscanf(buffer, "HISTO %d %[^\t]", &nLengths, other);

            if (returnStatus!=2) {
                fprintf(stderr, "Messaggio accettabile: HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>");
                exit(1);
            }

            fprintf(stdout, "Ho ricevuto dal server %d lunghezze diverse: \n", nLengths);
            // contatore posizioni histoLengths e histoInstances
            int index=0;
            // contatore che itera <lunghezza1> <lunghezza2> ...
            int j=0;
            // i è il contatore che itera <istanze1> <istanze2> ...
            for (int i=2; i<strlen(other); i+=4) {
                j=i-2;
                histoLengths[index] = atoi(&other[j]);
                histoInstances[index] = atoi(&other[i]);
                if (histoLengths[index]==0 || histoInstances[index]==0) {
                    fprintf(stderr, "Messaggio accettabile: HISTO <numero lunghezze> <lunghezza1> <istanze1> <lunghezza2> <istanze2>");
                    exit(1);
                }
                if (histoInstances[index]==1) fprintf(stdout, "• %d parola di lunghezza %d\n", histoInstances[index], histoLengths[index]);
                else fprintf(stdout, "• %d parole di lunghezza %d\n", histoInstances[index], histoLengths[index]);
                index++;
            }
            newLoop=0;
        }
        if (strcmp("ERROR", answer)==0) {
            fprintf(stderr, "%s", other);
            newLoop=0;
        }

    } while (newLoop==1);

    close(simpleSocket);
    return 0;

}

