#include "utils.h"


// ********************************************* FUNZIONI DI UTILITY ********************************************************

//APRE UNA CONNESSIONE TCP CON LA PORTA SPECIFICATA
int openTCP(uint16_t portaDest){
    int ret, sd;
    struct sockaddr_in dest_addr;

    /* Creazione socket */
    sd = socket(AF_INET,SOCK_STREAM,0);

    /* Creazione indirizzo del server */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(portaDest);
    inet_pton(AF_INET, "127.0.0.1", &dest_addr.sin_addr);

    /* connessione */
    ret = connect(sd, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

    if(ret < 0){
        perror("Errore in fase di connessione: \n");
        exit(1);
    }
    return sd;
}



//QUESTA SEND SI ACCERTA DI INVIARE TUTTI I BYTE E INCLUDE L'EVENTUALE MESSAGGIO DI ERRORE
void send_wrap(int socket, void *buffer, size_t size, const char *error){
    int ret;
    void *remaining = (void*) buffer;
    
    //ciclo assicurandomi di inviare il messaggio per intero
    while (size > 0){
        ret = send(socket, remaining, size, 0);
        if(ret == -1){
            printf("%s \n", error);
            exit(-1);
        }
        //aumento la partenza di tanti byte quanti ne ho inviati e la dimensione da inviare diminuisce analogamente
        remaining += ret; 
        size -= ret; 
    }
    return;
}



//QUESTA RECEIVE SI ACCERTA DI RICEVERE TUTTI I BYTE E INCLUDE IL MESSAGGIO DI ERRORE. SI OCCUPA ANCHE DI CHIUDERE EVENTUALMENTE LA CONNESSIONE
bool recv_wrap(int socket, void *buffer, size_t size, const char *error){
    int ret;
    void *remaining = (void*) buffer;

    //ciclo assicurandomi di ricevere il messaggio per intero
    while (size > 0){
        ret = recv(socket, remaining, size, 0);
        if(ret == -1){
            printf("%s \n", error);
            exit(-1);
        }
        if(ret == 0){
            close(socket);
            return false;
        }
        //aumento la partenza di tanti byte quanti ne ho ricevuti e la dimensione da ricevere diminuisce analogamente
        remaining += ret; 
        size -= ret; 
    }
    return true;
}



//QUESTA SEND INVIA L'ACK DI 1 BYTE
void send_ack(int socket, void *ack){
    int ret;
    ret = send(socket, ack, sizeof(uint8_t), 0);
    if(ret == -1){
        printf("%s \n", "Errore nell'invio dell'ack");
        exit(-1);
    }
}



//QUESTA RECEIVE RICEVE L'ACK DI 1 BYTE
void recv_ack(int socket, void *ack){
    int ret;
    ret = recv(socket, ack, sizeof(uint8_t), 0);
    if(ret == -1){
        printf("%s \n", "Errore: il server ha tardato a rispondere");
        exit(-1);
    }
}



//QUESTA FUNZIONE RESTITUISCE TRUE SE IL CAMPO IN INGRESSO E' TROPPO CORTO (MENO DI 3 CARATTERI)
bool entry_too_short(char* entry){
    if(strlen(entry) < 3){
        printf("%s \n", "-SERVER: La lunghezza minima è di 3 caratteri!");
        return true;
    }
    return false;
}



//QUESTA FUNZIONE VIENE CHIAMATA DOPO UNA signup CHE HA SUPERATO I CONTROLLI; AGGIUNGE L'UTENTE AL FILE USERS
void registra_utente(char* user, char* password){
    FILE* file_users;
    char concat[82];

    file_users = fopen("./fileServer/users.txt", "a");
	if (file_users == NULL) exit(1);

    strcpy(concat, user);
    strcat(concat, " ");
    strcat(concat, password);
    strcat(concat, "\n");
    fputs(concat, file_users);

	fclose(file_users);
    printf("%s %s \n", "-NOTIFICA: Registrato l'utente:", user);
    return;
}



//QUESTA FUNZIONE CERCA NEL FILE users UN USERNAME; RESTITUISCE TRUE SE LO TROVA
bool user_already_exist(char* username){
    FILE* file_users;
    char entry[82], user[41], password[41];


    file_users = fopen("./fileServer/users.txt", "r");
    if (file_users == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_users)){
        if(fgets(entry, 82, file_users) == NULL)
            break;			
        sscanf(entry, "%40s %40s", user, password);

		if(strcmp(user, username) == 0){
            fclose(file_users);
			return true;
		}
    }

    fclose(file_users);
	return false;
}



//QUESTA FUNZIONE CERCA NEL FILE users UN ACCOUNT; RESTITUISCE TRUE SE LO TROVA
bool account_already_exist(char* username, char* password){
    FILE* file_users;
    char entry[82], user[41], passw[41];


    file_users = fopen("./fileServer/users.txt", "r");
    if (file_users == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_users)){
        if(fgets(entry, 82, file_users) == NULL)
            break;			
        sscanf(entry, "%40s %40s", user, passw);

		if(strcmp(user, username) == 0 && strcmp(passw, password) == 0){
            fclose(file_users);
			return true;
		}
    }

    fclose(file_users);
	return false;
}



//QUESTA FUNZIONE GESTISCE PER IL DEVICE L'ESITO DEL SUO TENTATIVO DI signup IN BASE ALLA RISPOSTA RICEVUTA DAL SERVER
void esito_signup(uint8_t ack){
    if(ack == 1){
        printf("%s \n", "-SERVER: Ti sei registrato con successo, adesso effettua il login");
        return;
    }
    else{
        printf("%s \n", "-SERVER: Il nome utente è già registrato");
        return;
    }
}



//QUESTA FUNZIONE GESTISCE PER IL DEVICE L'ESITO DEL SUO TENTATIVO DI in IN BASE ALLA RISPOSTA RICEVUTA DAL SERVER
bool esito_login(uint8_t ack){
    if(ack == 1){
        printf("%s \n", "-SERVER: Hai effettuato il login con successo");
        return true;
    }
    if(ack == 2){
        printf("%s \n", "-SERVER: I dati inseriti sono errati!");
        return false;
    }
    if(ack == 3){
        printf("%s \n", "-SERVER: Sei già loggato su un'altra porta!");
        return false;
    }
    return false;
}



//QUESTA FUNZIONE AGGIUNGE L'UTENTE APPENA LOGGATO NELLA LISTA DELLE CONNESSIONI POSSEDUTA DAL SERVER (inserimento in testa)
connessione* connetti_user(char* username, int socket, uint16_t port, connessione** testa){
    connessione* temp = (connessione*) malloc(sizeof(connessione));
    temp->next = NULL;
    strcpy(temp->username, username);
    temp->conn_port = port;
    temp->conn_sd = socket;
    temp->logout_time = NOT_SET;
    temp->login_time = time(NULL);

    temp->next = *testa;
    *testa = temp;

    return *testa;
}



//QUESTA FUNZIONE RIMUOVE L'UTENTE CON IL SOCKET SPECIFICATO DALLA LISTA DELLE CONNESSIONI
connessione* disconnetti_user(int socket, connessione** testa){
    connessione* temp = *testa;
    connessione* next = NULL;

    if(temp->conn_sd == socket){
        printf("\r%s %s %s\n", "-NOTIFICA: L'utente", temp->username, "si è disconnesso");
        *testa = temp->next;
        free(temp);
        return *testa;
    }

    for(; temp != NULL; temp = temp->next){
        next = temp->next;
        if(next->conn_sd == socket){
            printf("\r%s %s %s\n", "-NOTIFICA: L'utente", next->username, "si è disconnesso");
            temp->next = next->next;
            free(next);
            return *testa;
        }
    }
    return temp;
}



//QUESTA FUNZIONE STAMPA USERNAME, TIMESTAMP DI CONNESSIONE E NUMERO DI PORTA DI TUTTE LE CONNESSIONI
void stampa_connessioni(connessione* testa){
    connessione* temp = testa;
    char timestamp[260];
    printf("\x1b[1F");
    printf("-LISTA DELLE CONNESSIONI ATTIVE:\n");
    if(testa == NULL){
        printf("%s \n", "-AVVISO: Non ci sono connessioni attive");
        return;
    }

    while(temp != NULL){
        if(temp->login_time != NOT_SET){
            strcpy(timestamp, asctime(localtime(&temp->login_time)));
            removeChar(timestamp, '\n');
            printf("%s*%s*%i\n", temp->username, timestamp, temp->conn_port);
        }
        temp = temp->next;
    }
}



//STAMPA LA LISTA DEI COMANDI DEL SERVER E RELATIVE DESCRIZIONI
void stampa_help(){
    printf("LISTA DEI COMANDI DEL SERVER:\n");
    printf("-help: è il comando appena chiamato, fornisce una descrizione dei comandi\n");
    printf("-list: elenca le connessioni attive del server indicando user, timestamp di login e porta\n");
    printf("-esc: termina il processo del server\n");
}



//CERCA ALL'INTERNO DELLA RUBRICA DEL CHIAMANTE PER ASSICURARSI CHE L'UTENTE CON CUI SI VUOLE CHATTARE SIA SALVATO
bool user_is_friend(char* user_src, char* user_dest){
    FILE* rubrica;
    char user[41], path[100];

    strcpy(path, "./rubriche/rubrica-");
    strcat(path, user_src);
    strcat(path, ".txt");

    rubrica = fopen(path, "r");
    if (rubrica == NULL){
        printf("%s\n", "RUBRICA NON ESISTENTE");
        exit(1);
    }

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(rubrica)){
        if(fgets(user, 41, rubrica) == NULL)
            break;			
        sscanf(user, "%40s", user);

		if(strcmp(user, user_dest) == 0){
            fclose(rubrica);
			return true;
		}
    }

    fclose(rubrica);
	return false;
}



//QUESTA FUNZIONE E' CHIAMATA DAL SERVER PER VERIFICARE SE L'UTENTE IN INPUT E' ONLINE
bool check_user_online(char* user, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (strcmp(user, aux->username) == 0 && aux->logout_time == NOT_SET){
            //printf("%s\n", user);
            //printf("%s\n", aux->username);
            return true;
        }
        
        aux = aux->next;
    }

    return false;
}



//QUESTA FUNZIONE E' CHIAMATA DAL DEVICE PER VERIFICARE SE L'UTENTE IN INPUT E' CONNESSO
bool check_user_connected(char* user, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (strcmp(user, aux->username) == 0)
            return true;
        
        aux = aux->next;
    }

    return false;
}



//FUNZIONE CHE ANNUNCIA L'APERTURA DI UNA CHAT
void chat_title(char* dest, char* mitt){
    FILE* chat_file;
    char path[200], entry[650];
    char *entry_mitt, *msg, *ack;

    system("clear");
    printf("---------------------------------------------------------------------------\n");
    printf("%s %s\n", "                 CHAT CON L'UTENTE:", dest);
    printf("---------------------------------------------------------------------------\n");

    strcpy(path, "./fileDevice/chat/");
    strcat(path, mitt);
    strcat(path, "/");
    strcat(path, dest);
    strcat(path, ".txt");

    chat_file = fopen(path, "a");
    fclose(chat_file);
    chat_file = fopen(path, "r");
    if (chat_file == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(chat_file)){
        if(fgets(entry, 650, chat_file) == NULL)
            break;	
	
        entry_mitt = strtok(entry, "|");
        msg = strtok(NULL, "|");
        ack = strtok(NULL, "|");
        strtok(NULL, "|");
        strtok(NULL, "|\n");	

        if(strcmp(entry_mitt, mitt) == 0)
		    printf("-%s:%s %s\n", entry_mitt, ack, msg);
        else
            printf("-%s: %s\n", entry_mitt, msg);
    }

    fclose(chat_file);

    return;
}



//QUESTA FUNZIONE TROVA L'USERNAME DI UN UTENTE CONNESSO CON TE TRAMITE IL SOCKET (E LO SALVA SULLA STRINGA PASSATA)
bool find_user_by_sd(int sd, connessione* testa, char* string){
    connessione* aux = testa; 

    while (aux != NULL){
        if (aux->conn_sd == sd){
            strcpy(string, aux->username);
            return true;
        }
        aux = aux->next;
    }
    return false;
}



//QUESTA FUNZIONE TROVA IL SOCKET DESCRIPTOR DI UN UTENTE CONNESSO CON TE TRAMITE IL SUO USERNAME
int find_sd_by_user(char* user, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (strcmp(user, aux->username) == 0)
            return aux->conn_sd;
        aux = aux->next;
    }
    return 0;
}



//QUESTA FUNZIONE MEMORIZZA SU FILE UN MESSAGGIO RICEVUTO. FORMATO: mittente|messaggio|*/**|destinatario|timestamp\n
void handle_msg_rec(char* msg, char* receiver, int sd, connessione* testa){
    FILE* chat_file;
    char mitt[41], path[200], timestr[256], concat[650];
    time_t timestamp = time(NULL);
    find_user_by_sd(sd, testa, mitt);

    // printf("ENTRO NELLA RECEIVE che salva solo il file\n");
    // printf("%s", msg);
    // printf("%s\n", mitt);
    // printf("%s\n", receiver);
    // printf("%i\n", sd);

    strcpy(path, "./fileDevice/chat/");
    strcat(path, receiver);
    strcat(path, "/");
    strcat(path, mitt);
    strcat(path, ".txt");

    chat_file = fopen(path, "a");
    if (chat_file == NULL){
        printf("%s\n", "IMPOSSIBILE MEMORIZZARE IL MESSAGGIO RICEVUTO");
        exit(1);
    }

    strcpy(concat, mitt);
    strcat(concat, "|");
    strcat(concat, msg);
    strcat(concat, "|");
    strcat(concat, "**");
    strcat(concat, "|");
    strcat(concat, receiver);
    strcat(concat, "|");
    sprintf(timestr, "%lld", (long long int)timestamp);
    strcat(concat, timestr);
    strcat(concat, "\n");

    fputs(concat, chat_file);

    fclose(chat_file);
}



//QUESTA FUNZIONE RESTITUISCE TRUE SE USER1 HA MESSAGGI PENDENTI DA USER2 mittente|messaggio|*/**|destinatario|timestamp\n
bool check_pending_msg(char* user1, char* user2){
    FILE* file_pending;
    char entry[650];
    char *mitt, *dest;


    file_pending = fopen("./fileServer/pending.txt", "r");
    if (file_pending == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_pending)){
        if(fgets(entry, 650, file_pending) == NULL)
            break;			
        mitt = strtok(entry, "|");
        strtok(NULL, "|");
        strtok(NULL, "|");
        dest = strtok(NULL, "|");
        strtok(NULL, "|\n");

		if(strcmp(dest, user1) == 0 && strcmp(mitt, user2) == 0){
            fclose(file_pending);
			return true;
		}
    }

    fclose(file_pending);
	return false;
}



//QUESTA FUNZIONE RESTITUISCE LA PORTA DI UN UTENTE CONNESSO TRAMITE L'USERNAME
uint16_t find_port_by_user(char* user, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (strcmp(user, aux->username) == 0)
            return aux->conn_port;
        aux = aux->next;
    }
    return 0;
}



//QUESTA FUNZIONE MANDA TUTTI I PENDING MESSAGES CHE L'UTENTE dest CON SOCKET sd DEVE RICEVERE DALL'UTENTE mitt
void send_pending_msg(int sd, char* dest, char* mitt){
    FILE* file_pending;
    char entry[650];
    char *entry_mitt, *entry_dest;
    uint8_t ackk = 1;
    line* testa = NULL;
    line* aux = NULL;
    line* current = NULL;
    char* copy = NULL;

    file_pending = fopen("./fileServer/pending.txt", "r");
    if (file_pending == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_pending)){
        memset(entry, 0, 650);
        if(fgets(entry, 650, file_pending) == NULL)
            break;	
        copy = strdup(entry);   
  
        entry_mitt = strtok(entry, "|");
        strtok(NULL, "|");
        strtok(NULL, "|");
        entry_dest = strtok(NULL, "|");
        strtok(NULL, "|\n");
    
        //se ho trovato un pending message lo invio al device richiedente chat o pending messages
		if(strcmp(entry_dest, dest) == 0 && strcmp(entry_mitt, mitt) == 0){
            send_ack(sd, (void*)&ackk);
            send_wrap(sd, copy, 650, "Errore in fase di invio messaggi pending"); 
		}

        //se il messaggio non è un suo pending message lo copio in una lista (dovrò ricreare il file pending depennando i messaggi inviati)
        else{
            aux = (line*)malloc(sizeof(line));

            strcpy(aux->entry, copy);
            aux->next = NULL; 

            if(testa == NULL)
                testa = aux;

            else{
                current = testa;
                while(true){ 
                    if(current->next == NULL){
                        current->next = aux;
                        break;
                    }
                    current = current->next;
                }
            }
        }
    }

    //l'ack = 0 indica che sono finiti i messaggi pending da ricevere per il device
    ackk = 0;
    send_ack(sd, (void*)&ackk);
    fclose(file_pending);

    //riapro il file pending in write mode così da pulirlo e riscriverlo senza i messaggi appena inviati
    file_pending = fopen("./fileServer/pending.txt", "w");
    current = testa;

    while(current != NULL){ 
        fputs(current->entry, file_pending);
        current = current->next;
    }
    fclose(file_pending);
    
    //ho riscritto il file pending, adesso posso svuotare la memoria
    while(testa != NULL){
        current = testa->next;
        free(testa);
        testa = current;
    }
}



//QUESTA FUNZIONE MEMORIZZA SU FILE UN MESSAGGIO PENDING RICEVUTO DAL SERVER. FORMATO: mittente|messaggio|*/**|destinatario|timestamp\n
void handle_msg_pen(char* entry, char* receiver, char* mitt){
    FILE* chat_file;
    char path[200];

    strcpy(path, "./fileDevice/chat/");
    strcat(path, receiver);
    strcat(path, "/");
    strcat(path, mitt);
    strcat(path, ".txt");

    chat_file = fopen(path, "a");
    if (chat_file == NULL){
        printf("%s\n", "IMPOSSIBILE MEMORIZZARE IL MESSAGGIO PENDING");
        exit(1);
    }
    
    fputs(entry, chat_file);
    fclose(chat_file);
}



//FUNZIONE CHE MODIFICA IL FILE RELATIVO ALLA CHAT CON dest SEGNALANDO CHE GLI SONO STATI RECAPITATI TUTTI I MESSAGGI PENDING
void aggiorna_ack(char* mitt, char* dest){
    FILE* chat_file;
    char path[200];
    char entry[650];
    char *entry_mitt, *entry_dest, *msg, *timestr;
    line* testa = NULL;
    line* aux = NULL;
    line* current = NULL;

    strcpy(path, "./fileDevice/chat/");
    strcat(path, mitt);
    strcat(path, "/");
    strcat(path, dest);
    strcat(path, ".txt");

    chat_file = fopen(path, "r");
    if (chat_file == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(chat_file)){
        if(fgets(entry, 650, chat_file) == NULL)
            break;	
        entry_mitt = strtok(entry, "|");
        msg = strtok(NULL, "|");
        strtok(NULL, "|");
        entry_dest = strtok(NULL, "|");
        timestr = strtok(NULL, "|\n");

        //estraggo ogni messaggio e lo inserisco in una lista settando ogni ack di recapito a **
        aux = (line*)malloc(sizeof(line));

        strcpy(aux->entry, entry_mitt);
        strcat(aux->entry, "|");
        strcat(aux->entry, msg);
        strcat(aux->entry, "|");
        strcat(aux->entry, "**");
        strcat(aux->entry, "|");
        strcat(aux->entry, entry_dest);
        strcat(aux->entry, "|");
        strcat(aux->entry, timestr);
        strcat(aux->entry, "\n");
        aux->next = NULL; 

        if(testa == NULL)
            testa = aux;

        else{
            current = testa;
            while(true){ 
                if(current->next == NULL){
                    current->next = aux;
                    break;
                }
                current = current->next;
            }
        }
    }

    fclose(chat_file);

    //riapro il file chat in write mode così da pulirlo e riscriverlo con gli ack aggiornati
    chat_file = fopen(path, "w");
    current = testa;

    while(current != NULL){ 
        fputs(current->entry, chat_file);
        current = current->next;
    }
    
    //ho riscritto il file chat, adesso posso svuotare la memoria
    while(testa != NULL){
        current = testa->next;
        free(testa);
        testa = current;
    }

    fclose(chat_file);
}



//QUESTA FUNZIONE INVIA AL DEVICE APPENA LOGGATO TUTTI GLI ACK RELATIVI AI MESSAGGI CHE AVEVA INVIATO E CHE SONO STATI RECAPITATI
void send_ack_info(int sd, char* user){
    char mitt[41], dest[41], entry[90];
    uint8_t ack;
    shortline* testa = NULL;
    shortline* aux = NULL;
    FILE* file_ack = NULL;
    shortline* current = NULL;
    ack = 3;

    file_ack = fopen("./fileServer/acks.txt", "r");
    if (file_ack == NULL){
        printf("Errore nell'aprire il file ack\n");
        exit(1);
    }

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_ack)){
        if(fgets(entry, 90, file_ack) == NULL)
            	break;		
        sscanf(entry, "%s %s", mitt, dest);

        //se ho trovato una chat scaricata lo comunico a user, inviandogli l'utente che ha ricevuto i messaggi
		if(strcmp(mitt, user) == 0){
            send_ack(sd, (void*)&ack);
            send_wrap(sd, dest, 40, "Errore in fase di invio ack di recapito");
		}

        //altrimento copio in una lista la entry (dovrò ricreare il file ack depennando gli ack inviati)
        else{
            aux = (shortline*)malloc(sizeof(shortline));

            strcpy(aux->entry, entry);
            aux->next = NULL; 

            if(testa == NULL)
                testa = aux;

            else{
                current = testa;
                while(true){ 
                    if(current->next == NULL){
                        current->next = aux;
                        break;
                    }
                    current = current->next;
                }
            }
        }
    }

    fclose(file_ack);

    //segnalo al device che sono finiti gli utenti di cui deve ricevere ack di conferma recapito
    ack = 4;
    send_ack(sd, (void*)&ack);

    //riapro il file ack in write mode così da pulirlo e riscriverlo con gli ack aggiornati
    file_ack = fopen("./fileServer/acks.txt", "w");
    if (file_ack == NULL){
        printf("Errore nell'aprire il file ack\n");
        exit(1);
    }
    current = testa;

    while(current != NULL){ 
        strcat(current->entry, "\n");
        fputs(current->entry, file_ack);
        current = current->next;
    }
    
    //ho riscritto il file ack, adesso posso svuotare la memoria
    while(testa != NULL){
        current = testa->next;
        free(testa);
        testa = current;
    }

    fclose(file_ack);

}



//QUESTA FUNZIONE MEMORIZZA SU FILE UN MESSAGGIO INVIATO. FORMATO: mittente|messaggio|*/**|destinatario|timestamp\n
//INVIA INOLTRE IL MESSAGGIO AL DESTINATARIO O AL SERVER SE DEST E' OFFLINE. INFINE STAMPA A VIDEO IL MESSAGGIO
void handle_msg_sent(char* msg, char* mitt, char* dest, int sd, bool ack){
    FILE* chat_file;
    char path[200], timestr[256], concat[650];
    time_t timestamp = time(NULL);
    // printf("ENTRO NELLA SEND\n");
    // printf("%s", msg);
    // printf("%s\n", mitt);
    // printf("%s\n", dest);
    // printf("%i\n", sd);
    // printf("%i\n", ack);

    strcpy(path, "./fileDevice/chat/");
    strcat(path, mitt);
    strcat(path, "/");
    strcat(path, dest);
    strcat(path, ".txt");

    chat_file = fopen(path, "a");
    if (chat_file == NULL){
        printf("%s\n", "IMPOSSIBILE MEMORIZZARE IL MESSAGGIO INVIATO");
        exit(1);
    }

    strcpy(concat, mitt);
    strcat(concat, "|");
    removeChar(msg, '\n');
    strcat(concat, msg);
    strcat(concat, "|");
    //se l'ack è true sto scrivendo ad un utente online, quindi il messaggio è recapitato
    if(ack == true)
        strcat(concat, "**");
    else
        strcat(concat, "*");
    strcat(concat, "|");
    strcat(concat, dest);
    strcat(concat, "|");
    sprintf(timestr, "%lld", (long long int)timestamp);
    strcat(concat, timestr);
    strcat(concat, "\n");

    fputs(concat, chat_file);

    fclose(chat_file);

    //se l'ack è true il destinatario è online e gli invio solo il messaggio
    if(ack == true){
        send_wrap(sd, msg, 300, "Errore nell'invio del messaggio");
        printf("\x1b[1F");
        printf("\r%s%s%s%s\n", "-", mitt, ":** ", msg);
    }
    //se è false è il server a memorizzare, invio direttamente il messaggio formattato pronto per essere salvato su file
    else{
        send_wrap(sd, concat, 650, "Errore nell'invio del messaggio");
        printf("\x1b[1F");
        printf("\r%s%s%s%s\n", "-", mitt, ":* ", msg);
    }
}



//FUNZIONE CHE AGGIUNGE IN UNA STRUCT DEL SERVER DUE UTENTI; IL SERVER DOVRA' MEMORIZZARE I MESSAGGI CHE
//L'UTENTE CON SOCKET sd VUOLE MANDARE ALL'UTENTE CON USERNAME user, POICHE' QUEST'ULTIMO E' OFFLINE
void new_offline_chat(int sd, char* user, connessione* testa, offline_chat** head){
    offline_chat* temp = (offline_chat*) malloc(sizeof(offline_chat));
    char mitt[41];
    find_user_by_sd(sd, testa, mitt);

    temp->next = NULL;
    temp->sd = sd;
    strcpy(temp->mitt, mitt);
    strcpy(temp->dest, user);

    temp->next = *head;
    *head = temp;
}



//QUESTA FUNZIONE RESTITUISCE TRUE SE L'UTENTE CON SOCKET sd STA USANDO IL SERVER PER SCRIVERE AD UN DESTINATARIO OFFLINE
bool sd_is_using_offline_chat(int sd, offline_chat* testa){
    offline_chat* aux = testa; 

    while (aux != NULL){
        if (aux->sd == sd)
            return true;
        aux = aux->next;
    }
    return false;
}



//QUESTA FUNZIONE RIMUOVE UN UTENTE DALLA LISTA DEGLI UTENTI CHE STANNO USANDO IL SERVER PERCHE' IL DESTINATARIO E' OFFLINE
void close_offline_chat(int sd, offline_chat** testa){
    offline_chat* temp = *testa;
    offline_chat* next = NULL;

    if(temp->sd == sd){
        printf("%s %s %s\n", "-NOTIFICA: L'utente", temp->mitt, "ha terminato con l'utilizzo della offline chat");
        *testa = temp->next;
        free(temp);
        return;
    }

    for(; temp != NULL; temp = temp->next){
        next = temp->next;
        if(next->sd == sd){
            printf("%s %s %s\n", "-NOTIFICA: L'utente", next->mitt, "ha terminato con l'utilizzo della offline chat");
            temp->next = next->next;
            free(next);
        }
    }
}



//QUESTA FUNZIONE SI OCCUPA DI MEMORIZZARE NEL FILE PENDING DEL SERVER UN MESSAGGIO CHE ANDRA' RECAPITATO
void save_pending_msg(char* msg, int sd, offline_chat* testa){
    FILE* file_pending = NULL;

    file_pending = fopen("./fileServer/pending.txt", "a");
    if (file_pending == NULL){
        printf("%s\n", "IMPOSSIBILE MEMORIZZARE IL MESSAGGIO RICEVUTO");
        exit(1);
    }

    fputs(msg, file_pending);

    fclose(file_pending);
}



//FUNZIONE DI UTILITY: RIMUOVE UN CARATTERE IN INPUT DA UNA STRINGA; SERVE PER RIMUOVERE LA NEWLINE NEI MESSAGGI INVIATI
void removeChar(char * str, char charToRemmove){
    int i, j;
    int len = strlen(str);
    for(i=0; i<len; i++){
        if(str[i] == charToRemmove){
            for(j=i; j<len; j++)
                str[j] = str[j+1];
            len--;
            i--;
        }
    }
}



//SALVA NEL FILE ACKS LE INFORMAZIONI RIGUARDANTI AI MESSAGGI RECAPITATI DAL SERVER (CIOE' CHI AVEVA MANDATO IL MESSAGGIO E CHI LO HA RICEVUTO)
void save_ack_info(char* mitt, char* dest){
    FILE* file_acks;
    char concat[82];

    file_acks = fopen("./fileServer/acks.txt", "a");
	if (file_acks == NULL) exit(1);

    strcpy(concat, mitt);
    strcat(concat, " ");
    strcat(concat, dest);
    strcat(concat, "\n");
    fputs(concat, file_acks);

	fclose(file_acks);
    return;
}



//STAMPA IL MENU DEL DEVICE
void print_dev_menu(char* user, uint16_t port){
    printf("---------------------------------------------------------------------------\n");
    printf("                       HAI EFFETTUATO L'ACCESSO\n");
    printf("                    USERNAME: %s | PORTA: %i\n", user, port);
    printf("---------------------------------------------------------------------------\n");
}



//STAMPA IL MENU DEL SERVER
void print_serv_menu(uint16_t port){
    printf("---------------------------------------------------------------------------\n");
    printf("                 SERVER IN ESECUZIONE ALLA PORTA %i\n", port);
    printf("---------------------------------------------------------------------------\n");
}



//QUESTA FUNZIONE DEL SERVER GESTISCE LA RICHIESTA DI HANGING DI UN DEVICE MANDANDOGLI LA LISTA ENTRY PER ENTRY SOTTOFORMA DI STRINGHE
void handle_hanging(int sd, char* user){
    FILE* file_pending;
    char entry[650];
    char hanging_elem[450];
    char number[10];
    char *entry_mitt, *entry_dest, *timestr;
    uint8_t ackk = 1;
    hanging_entry* testa = NULL;
    hanging_entry* aux = NULL;
    hanging_entry* current = NULL;

    file_pending = fopen("./fileServer/pending.txt", "r");
    if (file_pending == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(file_pending)){
        memset(entry, 0, 650);
        if(fgets(entry, 650, file_pending) == NULL)
            break;	  
  
        entry_mitt = strtok(entry, "|");
        strtok(NULL, "|");
        strtok(NULL, "|");
        entry_dest = strtok(NULL, "|");
        timestr = strtok(NULL, "|\n");
    
        //se ho trovato un pending message lo invio al device richiedente chat o pending messages
		if(strcmp(entry_dest, user) == 0){
            if(testa == NULL){
                aux = (hanging_entry*)malloc(sizeof(hanging_entry));
                aux->next = NULL; 
                testa = aux;
                strcpy(aux->user, entry_mitt);
                aux->count = 1;
                aux->timestamp = atoll(timestr);
            }

            else{
                current = testa;
                while(true){ 
                    if(strcmp(entry_mitt, current->user) == 0){
                        current->count++;
                        current->timestamp = atoll(timestr);
                        break;
                    }
                    if(current->next == NULL){
                        aux = (hanging_entry*)malloc(sizeof(hanging_entry));
                        aux->next = NULL; 
                        strcpy(aux->user, entry_mitt);
                        aux->count = 1;
                        aux->timestamp = atoll(timestr);
                        current->next = aux;
                        break;
                    }
                    current = current->next;
                }
            }
		}
    }

    //ora compongo la entry hanging e le invio

    current = testa; 

    while (current != NULL){
        strcpy(hanging_elem, "-");
        strcat(hanging_elem, current->user);
        strcat(hanging_elem, ": Hai ");
        sprintf(number, "%d", current->count); 
        strcat(hanging_elem, number);
        strcat(hanging_elem, " messaggi pendenti. Ultimo recapitato: ");
        strcat(hanging_elem, asctime(localtime(&current->timestamp)));
        
        aux = current;
        current = current->next;
        free(aux);
        send_ack(sd, (void*)&ackk);
        send_wrap(sd, hanging_elem, 450, "Errore in fase di invio messaggi pending"); 
    }

    ackk = 0;
    send_ack(sd, (void*)&ackk);
    fclose(file_pending);
}



//FUNZIONE DEL DEVICE CHE STAMPA GLI UTENTI ONLINE E IN RUBRICA IN SEGUITO ALLA DIGITAZIONE DI \u
void show_online_users(int sd, char* user, connessione* testa){
    FILE* rubrica;
    char friend[41], path[100];
    uint8_t ack;
    uint8_t ack_dev = 1;

    strcpy(path, "./rubriche/rubrica-");
    strcat(path, user);
    strcat(path, ".txt");

    rubrica = fopen(path, "r");
    if (rubrica == NULL){
        printf("%s\n", "RUBRICA NON ESISTENTE");
        exit(1);
    }

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(rubrica)){
        if(fgets(friend, 41, rubrica) == NULL)
            break;			
        sscanf(friend, "%40s", friend);
        //controllo se l'amico prelevato è già connesso con me (evito in caso una richiesta al server)
        if(check_user_connected(friend, testa) == true)
            printf("%s\n", friend);
        //altrimenti chiedo al server
        else{
            send_ack(sd, (void*)&ack_dev);
            send_wrap(sd, friend, 40, "Impossibile richiedere se l'utente è online al server");
            recv_ack(sd, (void*)&ack);
            if(ack == 1)
                printf("%s\n", friend);
        }
    }
    //segnalo la fine delle richieste al server
    ack_dev = 0;
    send_ack(sd, (void*)&ack_dev);

    fclose(rubrica); 
}



//QUESTA FUNZIONE DEL SERVER SEGNALA AL DEVICE RICHIEDENTE QUALI UTENTI DELLA SUA RUBRICA SONO ONLINE (IN SEGUITO AL COMANDO \u)
void handle_show_online(int sd, connessione* testa){
    uint8_t ack_dev;
    uint8_t ack;
    char user[41];

    while(true){
        recv_ack(sd, (void*)&ack_dev);
        if(ack_dev == 1){
            recv_wrap(sd, (void*)user, 40, "Errore nel ricevere username per verificare se online");
            if(check_user_online(user, testa) == true){
                ack = 1;
                send_ack(sd, (void*)&ack);
            }
            else{
                ack = 0;
                send_ack(sd, (void*)&ack);
            }
        }

        else
            break;
    }
}



//QUESTA FUNZIONE INVIA UN MESSAGGIO A TUTTI GLI UTENTI DEL GRUPPO
void handle_msg_sent_group(char* msg, char* user, connessione* ghead){
    connessione* aux = ghead;
    if(strcmp(msg, "\\q\n") != 0){
        printf("\x1b[1F");
        printf("\r%s%s%s%s", "-", user, ": ", msg);    
    }    

    while (aux != NULL){
        send_wrap(aux->conn_sd, msg, 300, "Errore nell'invio del messaggio");
        aux = aux->next;
    }
}



//QUESTA FUNZIONE INVIA UN MESSAGGIO A TUTTI GLI UTENTI DEL GRUPPO
void handle_msg_rec_group(char* msg, int sd, connessione* ghead){
    char user[41];
    find_user_by_sd(sd, ghead, user);

    printf("\r%s%s%s%s", "-", user, ": ", msg);
}



//QUESTA FUNZIONE E' CHIAMATA DAL DEVICE PER VERIFICARE SE IL SOCKET IN INPUT E' CONNESSO (NEL GRUPPO ATTUALMENTE ATTIVO)
bool check_sd_connected(int sd, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (sd == aux->conn_sd)
            return true;
        
        aux = aux->next;
    }

    return false;
}



//QUESTA FUNZIONE VIENE CHIAMATA QUANDO SI ABBANDONA UN GRUPPO, SVUOTA LA MEMORIA DALLA LISTA ALLOCATA
void leave_group(connessione** ghead){
    connessione* testa = *ghead;
    connessione* current = NULL;
    connessione* aux = NULL;
    current = testa;

    while(current != NULL){
        aux = current;
        current = current->next;
        free(aux);
        testa = current;
    }
    *ghead = NULL;
}



//QUESTA FUNZIONE AGGIUNGE UN UTENTE AD UN GRUPPO GIA' ESISTENTE
bool add_to_group(char* user, connessione** ghead, connessione* testa){
    char buffer[300];
    uint16_t porta, porta_net;
    int ret;
    uint8_t ack;
    connessione* aux = *ghead;
    connessione* temp = NULL;

    //devo verificare la disponibilità di user ad entrare in un gruppo
    strcpy(buffer, "group_request");
    send_wrap(find_sd_by_user(user, testa), buffer, 300, "Errore nel far entrare il vecchio destinatario nel gruppo\n");
    recv_ack(find_sd_by_user(user, testa) ,(void*)&ack);
    if(ack != 1){
        printf("-AVVISO: Impossibile creare il gruppo, %s è impegnato\n", user);
        return false;
    }


    while (aux != NULL){
        //invio nuovamente il segnale di group request ai vecchi utenti del gruppo per fargli aggiungere il nuovo arrivato
        send_wrap(aux->conn_sd, buffer, 300, "Errore nell'invio del segnale di group request");
        //invio poi agli altri utenti tutte le info sul nuovo arrivato
        porta = find_port_by_user(user, testa);
        porta_net = htons(porta);
        ret = send(aux->conn_sd, &porta_net, sizeof(uint16_t), 0);
        if(ret < 0){
            perror("Errore in fase di invio porta\n");
            exit(1);
        }
        send_wrap(aux->conn_sd, user, 40, "Errore nell'invio dell'username agli utenti del gruppo");
        aux = aux->next;
    }

    //aggiungo new_user alla lista del gruppo
    temp = (connessione*) malloc(sizeof(connessione));
    temp->next = NULL;
    strcpy(temp->username, user);
    temp->conn_port = find_port_by_user(user, testa);
    temp->conn_sd = find_sd_by_user(user, testa);
    temp->logout_time = NOT_SET;
    temp->login_time = time(NULL);
    temp->next = *ghead;
    *ghead = temp;

    return true;
}



//STAMPA LA SCHERMATA INIZIALE DI UNA CHAT DI GRUPPO
void group_chat_title(){
    system("clear");
    printf("---------------------------------------------------------------------------\n");
    printf("                             CHAT DI GRUPPO\n");
    printf("   NOTA: Il comando 'list' è abilitato per i device nelle chat di gruppo\n");
    printf("---------------------------------------------------------------------------\n");
}



//CONTROLLA CHE LA PORTA NON SIA GIA' CONNESSA
bool check_port_connected(uint16_t porta, connessione* testa){
    connessione* aux = testa; 

    while (aux != NULL){
        if (porta == aux->conn_port)
            return true;
        
        aux = aux->next;
    }

    return false;
}



//QUESTA SEND SI OCCUPA DI INVIARE UN FILE
bool send_file(char* nomeFile, char* user, int sd){
    FILE* target = NULL;
    char buffer[1024];
    uint8_t ack;

    strcpy(buffer, "./fileDevice/files/");
    strcat(buffer, user);
    strcat(buffer, "/");
    strcat(buffer, nomeFile);
    strcat(buffer, ".txt");

    target = fopen(buffer, "r");
    if(target == NULL){
        printf("\x1b[1F");
        printf("-AVVISO: File non trovato. Digita il nome di un file valido\n");
        return false;
    }

    memset(buffer, 0, 1024);
    //aviso il destinatario che gli sto per inviare un file
    strcpy(buffer, "_file_");
    send_wrap(sd, buffer, 300, "Errore nell'avvisare dell'imminente invio di un file");
    memset(buffer, 0, 10);
    //invio al destinatario il nome del file
    send_wrap(sd, nomeFile, 300, "Errore nell'avvisare dell'imminente invio di un file");
    ack = 1;

    //inizio con l'invio del file
    while(fgets(buffer, 1024, target) != NULL) {
        send_ack(sd, (void*)&ack);
        send_wrap(sd, buffer, 1024, "Errore nell'inviare un chuck di file");
        memset(buffer, 0, 1024);
    }

    //avviso della fine dell'invio del file
    ack = 0;
    send_ack(sd, (void*)&ack);
    fclose(target);
    return true;
}




//QUESTA SEND SI OCCUPA DI INVIARE UN FILE A TUTTI GLI UTENTI IN UN GRUPPO
void send_file_group(char* nomeFile, char* user, connessione* ghead){
    FILE* target = NULL;
    char buffer[400];
    connessione* aux = ghead;

    strcpy(buffer, "./fileDevice/files/");
    strcat(buffer, user);
    strcat(buffer, "/");
    strcat(buffer, nomeFile);
    strcat(buffer, ".txt");

    target = fopen(buffer, "r");
    if(target == NULL){
        printf("\x1b[1F");
        printf("\r-AVVISO: File non trovato. Digita il nome di un file valido\n");
        return;
    }
    fclose(target);       

    while (aux != NULL){
        send_file(nomeFile, user, aux->conn_sd);
        aux = aux->next;
    }
    printf("\x1b[1F");
    printf("-AVVISO: Hai correttamente inviato il file al gruppo\n");
}



//QUESTA FUNZIONE SI OCCUPA DI RICEVERE UN FILE, CHE SIA UNA CHAT DI GRUPPO O NORMALE
void recv_file(char* user, int sd){
    char buffer[1024];
    char nomeFile[300];
    FILE* target = NULL;
    uint8_t ack;

    recv_wrap(sd, (void*)nomeFile, 300, "Errore nel ricevere nome del file");
    strcpy(buffer, "./fileDevice/files/");
    strcat(buffer, user);
    strcat(buffer, "/");
    strcat(buffer, nomeFile);
    strcat(buffer, ".txt");

    target = fopen(buffer, "a");
    fclose(target);
    target = fopen(buffer, "w");

    while (1) {
        recv_ack(sd, (void*)&ack);
        if(ack == 0)
            break;
        recv_wrap(sd, (void*)buffer, 1024, "Errore nel ricevere chuck di file");
        fprintf(target, "%s", buffer);
        memset(buffer, 0, 1024);
    }
    fclose(target);
}



//QUESTA FUNZIONE STAMPA USERNAME DI TUTTI GLI UTENTI NEL GRUPPO
void stampa_gruppo(connessione* ghead){
    connessione* temp = ghead;
    printf("\x1b[1F");
    printf("\r-LISTA DEGLI UTENTI NEL GRUPPO:\n");
    if(ghead == NULL){
        printf("%s \n", "-AVVISO: Non ci sono connessioni attive");
        return;
    }

    while(temp != NULL){
        printf("%s\n", temp->username);
        temp = temp->next;
    }
}



//QUESTA FUNZIONE SALVA NEL REGISTRO DEGLI ACCESSI UNA ENTRY DI LOGIN
void registra_login(connessione* testa){
    //si nota che utilizzando inserimenti in testa, quest'ultima conterrà esattamente i dati da scrivere nel registro
    FILE* registro = NULL;
    char entry[90];
    char number[20];
    memset(entry, 0, 90);
    memset(number, 0, 20);

    registro = fopen("./fileServer/registroAccessi.txt", "a");
    if(registro == NULL) exit(1);

    strcpy(entry, testa->username);
    strcat(entry, " ");

    sprintf(number, "%u", testa->conn_port);
    strcat(entry, number);
    memset(number, 0, 20);
    strcat(entry, " ");

    sprintf(number, "%lld", (long long int)testa->login_time);
    strcat(entry, number);
    memset(number, 0, 20);
    strcat(entry, " ");

    sprintf(number, "%lld", (long long int)testa->logout_time);
    strcat(entry, number);
    strcat(entry, "\n");

    fputs(entry, registro);

	fclose(registro);
}



//QUESTA FUNZIONE MODIFICA NEL REGISTRO DEGLI ACCESSI UNA ENTRY, CIOE' MODIFICA IL TIMESTAMP DI LOGOUT
void registra_logout(char* user, bool start, int sd){
    //bool start indica se stiamo usando la funzione per ricevere un timestamp di logout non salvato da un utente appena
    //questo effettua il login (start = true); int sd è quindi il socket descriptor di tale utente, pertanto int sd sarà 
    //non significativo se la funzione viene chiamata nell'altro scenario (start = false), cioè se viene chiamata quando
    //il server online rileva il logout di un utente ed è quindi il server a generare il timestamp di logout
    char entry[90], entry_user[41], porta[10], timestamp_login[20], timestamp_logout[20];
    char timestr[20];
    FILE* registro = NULL;
    shortline* testa = NULL;
    shortline* aux = NULL;
    shortline* current = NULL;
    time_t now = time(NULL);
    uint8_t ack;

    //se start è true allora siamo al login di un utente, ci sta comunicando un eventuale timestamp di logout non salvato
    if(start == true){
        recv_ack(sd, (void*)&ack);
        if(ack == 0) 
            return;
        if(ack == 1){
            memset(timestr, 0, 20);
            recv_wrap(sd, (void*)timestr, 20, "Errore nel ricevere timestamp logout");
        }
    }

    registro = fopen("./fileServer/registroAccessi.txt", "r");
    if (registro == NULL){
        printf("Errore nell'aprire il file registroAccessi\n");
        exit(1);
    }

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(registro)){
        memset(entry, 0, 90);
        if(fgets(entry, 90, registro) == NULL)
            break;		
        sscanf(entry, "%s %s %s %s", entry_user, porta, timestamp_login, timestamp_logout);

        //creo una lista di entry che userò per riscrivere il file registroAccessi
        aux = (shortline*)malloc(sizeof(shortline));
        aux->next = NULL; 
        //se ho trovato una entry in cui corrisponde l'username e in cui non è settato il logout time setto quest'ultimo
		if(strcmp(entry_user, user) == 0 && strcmp(timestamp_logout, "0") == 0){
            memset(aux->entry, 0, 90);
            strcpy(aux->entry, entry_user);
            strcat(aux->entry, " ");
            strcat(aux->entry, porta);
            strcat(aux->entry, " ");
            strcat(aux->entry, timestamp_login);
            strcat(aux->entry, " ");
            //se start è false, cioè se la funzione non è stata chiamata al login di un device ma al logout,
            //il timestamp lo genera il server
            if(start == false){
                memset(timestr, 0, 20);
                sprintf(timestr, "%lld", (long long int)now);
            }
            strcat(aux->entry, timestr);
            strcat(aux->entry, "\n");
		}
        //altrimenti lascio tutto così com'è e inserisco in lista
        else
            strcpy(aux->entry, entry);

        if(testa == NULL)
            testa = aux;

        else{
            current = testa;
            while(true){ 
                if(current->next == NULL){
                    current->next = aux;
                    break;
                }
                current = current->next;
            }
        }
    }

    fclose(registro);


    //riapro il file registroAccessi in write mode così da pulirlo e riscriverlo con gli ack aggiornati
    registro = fopen("./fileServer/registroAccessi.txt", "w");
    if (registro == NULL){
        printf("Errore nell'aprire il file registroAccessi\n");
        exit(1);
    }
    current = testa;

    while(current != NULL){ 
        fputs(current->entry, registro);
        current = current->next;
    }
    
    //ho riscritto il file registroAccessi, adesso posso svuotare la memoria
    while(testa != NULL){
        current = testa->next;
        free(testa);
        testa = current;
    }

    fclose(registro);
}



//QUESTA FUNZIONE FA INVIARE AL DEVICE APPENA LOGGATO UN SUO EVENTUALE TIMESTAMP DI LOGOUT MEMORIZZATO AL SERVER
void send_logout_time(char* user, int sd){
    FILE* last_logout = NULL;
    char path[70], timestr[20];
    uint8_t ack = 1;

    //apro prima il file in append cosi in caso non esista viene creato vuoto
    strcpy(path, "./fileDevice/files/");
    strcat(path, user);
    strcat(path, "/last_logout.txt");
    last_logout = fopen(path, "a");
    if(last_logout == NULL) exit(1);
    fclose(last_logout);
    //poi lo apro in read mode per leggere un eventuale ultimo logout salvato
    last_logout = fopen(path, "r");
    if(last_logout == NULL) exit(1);

    //Analizziamo il file fino a che non è rilevata la fine dello stesso
    while(!feof(last_logout)){
        //se trovo qualcosa c'è un timestamp di logout, invio ack = 1 e la stringa. Altrimenti non c'è e invio ack = 0
        if(fgets(timestr, 20, last_logout) == NULL){
            ack = 0;
            send_ack(sd, (void*)&ack);
            break;
        }

        send_ack(sd, (void*)&ack);
        send_wrap(sd, timestr, 20, "Errore nell'inviare timestamp di logout");
	}

    //chiudo il file e lo riapro in write mode per svuotarlo
    fclose(last_logout);
    last_logout = fopen(path, "w");
    if(last_logout == NULL) exit(1);
    fclose(last_logout);
}



//QUESTA FUNZIONE FA SALVARE AL DEVICE IL TIMESTAMP DI LOGOUT PRIMA DI SCONNETTERSI SE IL SERVER E' OFFLINE
void save_logout_time(char* user){
    FILE* last_logout = NULL;
    char path[70], timestr[20];
    time_t now = time(NULL);

    //apro prima il file in append cosi in caso non esista viene creato vuoto
    strcpy(path, "./fileDevice/files/");
    strcat(path, user);
    strcat(path, "/last_logout.txt");
    last_logout = fopen(path, "a");
    if(last_logout == NULL) exit(1);

    memset(timestr, 0, 20);
    sprintf(timestr, "%lld", (long long int)now);
    fputs(timestr, last_logout);

    fclose(last_logout);
}