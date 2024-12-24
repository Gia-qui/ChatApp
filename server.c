#include "./lib/utils.h"

// *********************************************** VARIABILI GLOBALI E DEFINIZIONI ************************************************
#define BUF_LEN 1024
#define REQUEST_LEN 4 // REQ\0

fd_set master;
//testa della lista di connessioni del server
connessione* connessioni_server = NULL;
//testa della lista di utenti che sfruttano il server per chattare con un utente offline
offline_chat* testa = NULL;



// *********************************************************** MAIN ***************************************************************

int main(int argc, char* argv[]){
    
    int ret, newfd, listener, addrlen, i;
    bool result;
    fd_set read_fds;
    int fdmax;
    struct sockaddr_in my_addr, device_addr;
    uint16_t my_port, dev_port, dev_port_net;
    char istruzione[10], user[41], password[41], comando[10], target_user[41], buffer[BUF_LEN];
    uint8_t ack;

    /* Creazione socket */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Creazione indirizzo di bind */
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;

    my_port = (argc != 2)? 4242 : atoi(argv[1]);

    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(listener, (struct sockaddr*)&my_addr, sizeof(my_addr));
    
    if(ret < 0){
        perror("Bind non riuscita\n");
        exit(0);
    }
    
    listen(listener, 10);
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(listener, &master);
    //metto STDIN tra i file in ascolto per poter scrivere
    FD_SET(STDIN_FILENO, &master);
    fdmax = listener;


    system("clear");
    print_serv_menu(my_port);
    //IL CICLO CHE GESTISCE IL SERVER
    for(;;){
        // Inizializzo il set read_fds, manipolato dalla select()
        read_fds = master; 
        ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);       
        if(ret < 0){
            perror("ERRORE SELECT");
            exit(1);
        }
        

        // Spazzolo i descrittori
        for(i = 0; i <= fdmax; i++) {
            memset(comando, 0, 10);
            memset(istruzione, 0, 10);
            memset(user, 0, 41);
            memset(target_user, 0, 41);
            memset(password, 0, 41);

            // se i non è pronto continuo
            if(!FD_ISSET(i, &read_fds))
                continue;


            //SE IL SOCKET PRONTO E' IL LISTENER E DEVO ACCETTARE UNA CONNESSIONE CON UN DEVICE
            if(i == listener) {
                fflush(stdout);
                addrlen = sizeof(device_addr);
                // faccio accept() e creo il socket connesso 'newfd'
                newfd = accept(listener,(struct sockaddr *)&device_addr, (socklen_t *)&addrlen);
                // Aggiungo il descrittore al set dei socket monitorati
                FD_SET(newfd, &master);
                printf("%s %i \n", "-NOTIFICA: Connessione con il socket", newfd);
                // Aggiorno l'ID del massimo descrittore 
                if(newfd > fdmax)
                    fdmax = newfd; 
                continue;
            }


            //SE IL SOCKET PRONTO E' QUELLO CHE GESTISCE L'INPUT DELLA CONSOLE
            else if(i == STDIN_FILENO){
                ret = read(STDIN_FILENO, (void*)comando, 10);
                if(ret == -1){
                    printf("%s \n", "ERRORE NEL RICEVERE INPUT DA CONSOLE"); 
                    continue;
                }
                //printf("%s \n", comando);
                if(strcmp(comando, "esc\n") == 0){
                    printf("TERMINAZIONE DEL PROCESSO SERVER\n");
                    exit(0);
                }
                if(strcmp(comando, "list\n") == 0)
                    stampa_connessioni(connessioni_server);

                if(strcmp(comando, "help\n") == 0)
                    stampa_help();
                
                continue;
            }


            //(GESTIONE MESSAGGI PENDENTI LATO SERVER)
            //SE IL SOCKET PRONTO E' UN DEVICE CHE STA SFRUTTANDO IL SERVER PER MEMORIZZARE MESSAGGI RIVOLTI AD UN DESTINATARIO OFFLINE
            if(sd_is_using_offline_chat(i, testa)){
                memset(buffer, 0, 650);
                result = recv_wrap(i, (void*)buffer, 650, "Errore di ricezione messaggio pending da Device");
                if(result == false){
                    find_user_by_sd(i, connessioni_server, user);
                    disconnetti_user(i, &connessioni_server);
                    registra_logout(user, false, -1);
                    FD_CLR(i, &master);
                    printf("%s %i \n", "-NOTIFICA: Chiusa connessione con il socket", i);
                    close_offline_chat(i, &testa);
                    continue;
                }
                //se al server arriva questo comando vuol dire che il device che gli sta scrivendo ha chiuso la chat
                if(strcmp(buffer, "\\q\n") == 0)
                    close_offline_chat(i, &testa);
                //altrimenti il server memorizza il messaggio su file per recapitarlo in futuro quando il destinatario lo richiederà
                else
                    save_pending_msg(buffer, i, testa);
                continue;
            }


            //SE IL SOCKET PRONTO E' UNA DELLE CONNESSIONI CON I DEVICE
            result = recv_wrap(i, (void*)istruzione, 10, "Errore di ricezione comando");
            //la recv_wrap ritorna inoltre false se ha identificato la chiusura di una connessione, rimuovo quindi i dal master
            if(result == false){
                //se l'utente era loggato va rimosso dalla lista delle connessioni
                if(find_user_by_sd(i, connessioni_server, user) == true){
                    disconnetti_user(i, &connessioni_server);
                    registra_logout(user, false, -1);
                }
                if(sd_is_using_offline_chat(i, testa))
                    close_offline_chat(i, &testa);
                FD_CLR(i, &master);
                printf("%s %i \n", "-NOTIFICA: Chiusa connessione con il socket", newfd);
                continue;
            }


            //SE IL SOCKET PRONTO E' UN DEVICE CHE STA TENTANDO DI REGISTRARSI
            if(strcmp("signup", istruzione) == 0){
                recv_wrap(i, (void*)user, 40, "Errore di ricezione username");
                recv_wrap(i, (void*)password, 40, "Errore di ricezione password");

                //mi assicuro che l'username non sia già registrato
                if(user_already_exist(user))
                    ack = 255;
                else{
                    registra_utente(user, password);
                    ack = 1;
                }

                //invio l'esito della signup al device
                send_ack(i, (void*)&ack);
                continue;
            }


            //UN DEVICE STA TENTANDO DI EFFETTUARE IL LOGIN
            else if(strcmp("in", istruzione) == 0){
                recv_wrap(i, (void*)user, 40, "Errore di ricezione username");
                recv_wrap(i, (void*)password, 40, "Errore di ricezione password");
                ret = recv(i, &dev_port_net, sizeof(uint16_t), 0);
                if(ret == -1){
                    perror("Errore di ricezione porta utente ");
                    exit(-1);
                }
                dev_port = ntohs(dev_port_net);

                //mi assicuro che i dati inviati dall'utente per la login esistano
                if(!account_already_exist(user, password)){
                    ack = 2;
                    send_ack(i, (void*)&ack);
                    continue;
                }
                //mi assicuro che l'utente non sia già loggato col server su un'altra porta
                if(check_user_connected(user, connessioni_server) == true){
                    ack = 3;
                    send_ack(i, (void*)&ack);
                    continue;
                }
                else { 
                    ack = 1;
                    connetti_user(user, i, dev_port, &connessioni_server);
                    //invio l'esito della signup al device
                    send_ack(i, (void*)&ack);
                    printf("%s %s %s \n", "-NOTIFICA: L'utente", user, "ha effettuato il login");
                    //invio eventuali ack di messaggi dell'utente appena loggato che il server ha recapitato mentre l'utente era offline
                    send_ack_info(i, user);
                    //questa chiamata a registra_logout si occupa di ricevere eventuali timestamp di logout di user non salvati
                    registra_logout(user, true, i);
                    //scriviamo nel registro degli accessi
                    registra_login(connessioni_server);
                    continue;
                }
            }


            //UN UTENTE STA RICHIEDENTO I MESSAGGI PENDENTI CON IL COMANDO SHOW
            else if(strcmp("show", istruzione) == 0){
                recv_wrap(i, (void*)user, 40, "Errore nella ricezione username");
                //target_user è l'utente che richiede i messaggi pending di user
                find_user_by_sd(i, connessioni_server, target_user); 
                printf("-NOTIFICA: Invio dei messaggi pending di %s a %s\n", user, target_user);
                send_pending_msg(i, target_user, user);
                //se l'utente che aveva mandato quei messaggi è online gli invio gli ack di recapito
                if(check_user_online(user, connessioni_server) == true){
                    newfd = find_sd_by_user(user, connessioni_server);
                    memset(buffer, 0, 300);
                    strcpy(buffer, target_user);
                    send_wrap(newfd, (void*)buffer, 300, "Errore nell'avvisare il recapito di messaggi");
                }
                //se è offline il server memorizza gli ack di recapito
                else
                    save_ack_info(user, target_user);
                continue;
            }


            //UN UTENTE STA RICHIEDENTO I MESSAGGI PENDENTI CON IL COMANDO SHOW
            else if(strcmp("hanging", istruzione) == 0){
                //target_user è l'utente che richiede la lista di hanging
                find_user_by_sd(i, connessioni_server, target_user); 
                printf("-NOTIFICA: Gestisco la richiesta di Hanging di %s\n", target_user);
                handle_hanging(i, target_user);
                continue;
            }


            //UN UTENTE STA RICHIEDENTO LA LISTA DI UTENTI ONLINE TRA QUELLI NELLA SUA RUBRICA
            else if(strcmp("\\u\n", istruzione) == 0){
                handle_show_online(i, connessioni_server);
                continue;
            }

            //UN UTENTE STA RICHIEDENTO LA PORTA DI UN ALTRO UTENTE PER AGGIUNGERLO AD UN GRUPPO
            else if(strcmp("\\a", istruzione) == 0){
                //ricevo l'username
                recv_wrap(i, (void*)user, 40, "Errore nella ricezione username");
                //se l'utente destinatario è online devo mandare la sua porta all'utente mittente
                if(check_user_online(user, connessioni_server) == true){
                    ack = 1;
                    send_ack(i, (void*)&ack);
                    dev_port = find_port_by_user(user, connessioni_server);
                    dev_port_net = htons(dev_port);
                    ret = send(i, &dev_port_net, sizeof(uint16_t), 0);
                    if(ret < 0){
                        perror("Errore in fase di invio porta\n");
                        exit(1);
                    }
                }
                else{
                    ack = 0;
                    send_ack(i, (void*)&ack);
                }
                continue;
            }


            //UN UTENTE STA RICHIEDENDO DI CHATTARE CON UN ALTRO UTENTE
            else if(strcmp("chat", istruzione) == 0){

                recv_wrap(i, (void*)user, 40, "Errore nella ricezione username");
                //target_user è l'utente che richiede di chattare con user
                find_user_by_sd(i, connessioni_server, target_user);
                //se l'utente destinatario è online devo mandare la sua porta all'utente mittente; non ci devono essere pending msgs
                if(check_user_online(user, connessioni_server) == true && check_pending_msg(target_user, user) == false){
                    ack = 1;
                    send_ack(i, (void*)&ack);
                    dev_port = find_port_by_user(user, connessioni_server);
                    dev_port_net = htons(dev_port);
                    ret = send(i, &dev_port_net, sizeof(uint16_t), 0);
                    if(ret < 0){
                        perror("Errore in fase di invio porta\n");
                        exit(1);
                    }
                }
                //l'utente destinatario è online ma ci sono messaggi pending che il richiedente deve prima scaricare
                else if(check_user_online(user, connessioni_server) == true && check_pending_msg(target_user, user) == true){
                    ack = 2;
                    send_ack(i, (void*)&ack);
                    send_pending_msg(i, target_user, user);
                    //avviso l'user di cui è stata fatta richiesta (che è online) che i suoi messaggi sono stati recapitati
                    newfd = find_sd_by_user(user, connessioni_server);
                    memset(buffer, 0, 300);
                    strcpy(buffer, target_user);
                    send_wrap(newfd, (void*)buffer, 300, "Errore nell'avvisare il recapito di messaggi");
                    //dopo aver inviato i pending msgs posso inviare anche la porta che serve al richiedente
                    dev_port = find_port_by_user(user, connessioni_server);
                    dev_port_net = htons(dev_port);
                    ret = send(i, &dev_port_net, sizeof(uint16_t), 0);
                    if(ret < 0){
                        perror("Errore in fase di invio porta\n");
                        exit(1);
                    }
                }
                //l'utente destinatario è offline, ci sono inoltre messaggi pending che il richiedente deve prima scaricare
                else if(check_user_online(user, connessioni_server) == false && check_pending_msg(target_user, user) == true){
                    printf("-NOTIFICA: Invio dei messaggi pending di %s a %s\n", user, target_user);
                    ack = 254;
                    send_ack(i, (void*)&ack);
                    send_pending_msg(i, target_user, user);
                    //memorizzo nel file acks le informazioni relative ai messaggi recapitati
                    printf("-NOTIFICA: Invio dei messaggi pending di %s a %s\n", user, target_user);
                    save_ack_info(user, target_user);
                    printf("-NOTIFICA: L'utente %s ha aperto una chat offline con %s\n", target_user, user);
                    new_offline_chat(i, user, connessioni_server, &testa);
                }
                //l'utente destinatario è offline, il server lo notifica e sarà quest'ultimo a memorizzare i messaggi
                else{
                    ack = 255;
                    send_ack(i, (void*)&ack);
                    printf("-NOTIFICA: L'utente %s ha aperto una chat offline con %s\n", target_user, user);
                    new_offline_chat(i, user, connessioni_server, &testa);
                }
            }

            else{
                printf("ERRORE CRITICO, QUESTO CASE NON DOVREBBE ESISTERE \n");
                exit(1);
            }
        }
    }



    exit(0);
    fflush(stdout);
    close(listener);
}
