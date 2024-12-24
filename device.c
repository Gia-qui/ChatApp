#include "./lib/utils.h"

// *********************************************** VARIABILI GLOBALI E DEFINIZIONI ************************************************

#define BUF_LEN 1024

//username e password del device
char user[41], password[41];
//porta, porta network del device e porta server
uint16_t my_port, my_port_net, porta_srv;
//testa della lista che contiene le connessioni del device
connessione* connessioni_device = NULL;
//testa della lista che contiene le connessioni del device in un gruppo, cioè gli utenti del gruppo
connessione* group_users = NULL;
//booleano che indica se il server è online o no
bool server_on = false;
//booleano che indica se siamo o no in un gruppo
bool group = false;
fd_set master;
//socket descriptor del server
int sd;
//socket descriptor relativo all'utente con cui stiamo chattando: -1 se non siamo in una chat privata
int current_chat_sd = -1;

// *********************************************************** MAIN ***************************************************************

int main(int argc, char* argv[]){
    int ret, listener, fdmax, i, addrlen, newfd;
    char comando[BUF_LEN];
    char buffer[BUF_LEN], aux_buffer[300];
    char dest[41], mitt[41], group_user[41], istruzione[10];
    uint16_t porta_net, porta;
    uint8_t ack;
    fd_set read_fds;
    struct sockaddr_in my_addr, conn_addr;
    




    if(argc != 2){
        printf("Per favore avviare il Device specificando correttamente una porta\n");
        exit(1);
    }
    system("clear");
    printf("---------------------------------------------------------------------------\n");
    printf("- EFFETTUA IL LOGIN CON      'in [porta srv] [username] [password]'       -\n");
    printf("- REGISTRATI CON             'signup [porta srv] [username] [password]'   -\n");
    printf("---------------------------------------------------------------------------\n");


    //la fase iniziale di login e registrazione è gestita separatamente rispetto al ciclo di vita del device
    for(;;){
        memset(comando, 0, BUF_LEN);
        memset(istruzione, 0, 10);
        memset(user, 0, 41);
        memset(password, 0, 41);
        fgets(comando, BUF_LEN, stdin);
        sscanf(comando, "%10s %hd %40s %40s", istruzione, &porta_srv, user, password);

        //SE L'UTENTE STA TENTANDO DI EFFETTUARE LA signup
        if(strcmp("signup", istruzione) == 0){
            if(entry_too_short(user) || entry_too_short(password))
                continue;
            sd = openTCP(porta_srv);
            send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
            send_wrap(sd, user, 40, "Errore in fase di invio username");
            send_wrap(sd, password, 40, "Errore in fase di invio password");

            //ricevo l'ack dal server
            recv_ack(sd, (void*)&ack);
            //scopro l'esito della signup
            esito_signup(ack);
            close(sd);
        }

        //SE L'UTENTE STA TENTANDO DI EFFETTUARE IL LOGIN
        else if(strcmp("in", istruzione) == 0){
            sd = openTCP(porta_srv);
            send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
            send_wrap(sd, user, 40, "Errore in fase di invio username");
            send_wrap(sd, password, 40, "Errore in fase di invio password");
            //invio la porta che l'utente desidera usare
            my_port = atoi(argv[1]);
            my_port_net = htons(my_port);
            ret = send(sd, &my_port_net, sizeof(uint16_t), 0);
            if(ret < 0){
                perror("Errore in fase di invio porta: \n");
                exit(1);
            }

            //ricevo l'ack dal server
            recv_ack(sd, (void*)&ack);

            //scopro l'esito del login
            if(esito_login(ack) == true){
                //memorizzo che il server è online, d'altronde ha appena gestito una mia richiesta
                server_on = true;
                //se il login è riuscito esco dal ciclo, posso usare il device
                break;
            }
            else{
                close(sd);
                continue;
            }
        }

        //SE L'UTENTE DIGITA UN COMANDO SCONOSCIUTO
        else{
            printf("Comando Sconosciuto\n");
        }
    }


    //APPENA EFFETTUATO L'ACCESSO CI SI PREPARA A RICEVERE UN ACK DAL SERVER CHE CI AVVISA DI QUALI
    //DEGLI EVENTUALI MESSAGGI INVIATI IN PASSATO A UTENTI OFFLINE SONO STATI RECAPITATI
    while(true){
        recv_ack(sd, (void*)&ack);
        if(ack == 4) break;
        if(ack == 3){
            recv_wrap(sd, (void*)dest, 40, "Errore nella ricezione ack di recapito dal server");
            aggiorna_ack(user, dest);
            memset(dest, 0, 41);
        }
    }
    //mando al server un eventuale timestamp di logout salvato mentre era offline
    send_logout_time(user, sd);



    system("clear");
    print_dev_menu(user, my_port);
    //DA QUA IN POI VENGONO GESTITE TUTTE LE FUNZIONI A CUI UN DEVICE LOGGATO HA ACCESSO
    /* Creazione socket */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Creazione indirizzo di bind */
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;

    my_port = atoi(argv[1]);

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
    fdmax = listener;
    //metto il socket del server tra quelli in ascolto
    FD_SET(sd, &master);
    if(sd > fdmax)
        fdmax = sd; 
    //metto STDIN tra i file in ascolto per poter scrivere
    FD_SET(STDIN_FILENO, &master);
    fdmax = listener;



    //IL CICLO CHE GESTISCE IL DEVICE DOPO AVER EFFETTUATO IL LOGIN
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

            //SE NESSUN SOCKET E' PRONTO
            if(!FD_ISSET(i, &read_fds))
                continue;


            memset(comando, 0, 301);
            memset(istruzione, 0, 10);
            memset(buffer, 0, 650);


            //SE IL SOCKET PRONTO E' IL LISTENER E DEVO ACCETTARE UNA CONNESSIONE CON UN DEVICE
            if(i == listener) {
                fflush(stdout);
                addrlen = sizeof(conn_addr);
                // faccio accept() e creo il socket connesso 'newfd'
                newfd = accept(listener,(struct sockaddr *)&conn_addr, (socklen_t *)&addrlen);
                // Aggiungo il descrittore al set dei socket monitorati
                FD_SET(newfd, &master);
                // Aggiorno l'ID del massimo descrittore 
                if(newfd > fdmax)
                    fdmax = newfd; 
                // Il protocollo prevede la ricezione di username e porta in modo da salvare nella lista la connessione i dati
                recv_wrap(newfd, (void*)mitt, 40, "Errore di ricezione username");
                ret = recv(newfd, &porta_net, sizeof(uint16_t), 0);
                if(ret == -1){
                    perror("Errore di ricezione porta utente ");
                    exit(-1);
                }
                porta = ntohs(porta_net);
                //salvo in lista connessione l'utente appena connesso
                connetti_user(mitt, newfd, porta, &connessioni_device);
                if(group == false)
                    printf("%s %s %s\n", "-NOTIFICA: L'utente", mitt, "ha aperto una chat con te");
                // Controllo insidioso: se l'utente che ha richiesto di chattare con me è lo stesso utente con cui stavo chattando
                // in questo esatto momento (prima che lui si connettesse) ma mentre lui era offline. Da ora riceverà lui i messaggi
                // e non più il server, che va avvisato con il comando \q
                if(current_chat_sd == sd){
                    if(strcmp(mitt, dest) == 0){
                        strcpy(buffer, "\\q\n");
                        send_wrap(sd, buffer, 650, "Errore nell'avvisare il server dell'uscita dalla chat");
                        current_chat_sd = newfd;
                    }
                }
                memset(mitt, 0, 41);
                continue;
            }

            
            //SE UN QUALSIASI SOCKET E' PRONTO MENTRE NON SONO IN UNA CHAT APERTA
            if(current_chat_sd == -1 && group == false){
            

                //SE IL SOCKET PRONTO E' QUELLO CHE GESTISCE L'INPUT DELLA CONSOLE E NON SONO IN UNA CHAT
                if(i == STDIN_FILENO){       
                    ret = read(STDIN_FILENO, (void*)comando, 60);
                    if(ret == -1){
                        printf("%s \n", "ERRORE NEL RICEVERE INPUT DA CONSOLE"); 
                        continue;
                    }
                    sscanf(comando, "%10s %40s", istruzione, dest);


                    //SE L'UTENTE STA CHIUDENDO IL DEVICE
                    if(strcmp(istruzione, "out") == 0){
                        //se il server è offline salvo il timestamp di logout su un file
                        if(server_on == false)
                            save_logout_time(user);
                        system("clear");
                        exit(0);
                    }


                    //SE L'UTENTE STA RICHIEDENDO AL SERVER LA LISTA DI HANGING
                    if(strcmp(istruzione, "hanging") == 0){
                        if(server_on){
                            send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
                            printf("\x1b[1F");
                            printf("-LISTA DEI MESSAGGI HANGING:\n");
                            while(true){
                                recv_ack(sd, (void*)&ack);
                                //se l'ack è 1 sto per ricevere una entry degli hanging
                                if(ack == 1){
                                    //il server compone la stringa in modo che sia già pronta a essere stamata dal device
                                    recv_wrap(sd, (void*)buffer, 450, "Errore nella ricezione dei messaggi pending");
                                    printf("%s", buffer);
                                    memset(buffer, 0, 450);
                                }
                                //se l'ack è 0 sono finite le entry
                                if(ack == 0)
                                    break;
                            }
                        }
                    }


                    //SE L'UTENTE STA TENTANDO DI SCARICARE I MESSAGGI PENDENTI DI UN QUALCHE UTENTE
                    if(strcmp(istruzione, "show") == 0){
                        //controllo che l'utente sia in rubrica
                        if(user_is_friend(user, dest) == false){
                            printf("-AVVISO: L'utente inserito non è nella tua rubrica\n");
                            continue;
                        }
                        //se il server è online richiedo e scarico i messaggi, che poi vengono salvati su file
                        if(server_on){
                            send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
                            send_wrap(sd, dest, 40, "Errore in fase di invio username");
                            while(true){
                                recv_ack(sd, (void*)&ack);
                                //se l'ack è 1 sto per ricevere un pending msg
                                if(ack == 1){
                                    recv_wrap(sd, (void*)buffer, 650, "Errore nella ricezione dei messaggi pending");
                                    handle_msg_pen(buffer, user, dest);
                                }
                                //se l'ack è 0 sono finiti i pending msg
                                if(ack == 0)
                                    break;
                            }
                            printf("\x1b[1F");
                            printf("-AVVISO: Download dei messaggi pendenti di %s completato\n", dest);
                            continue;
                        }
                        //se invece il server è offline non se ne fa nulla
                        else{
                            printf("\x1b[1F");
                            printf("-AVVISO: Il server è offline, impossibile inoltrare la richiesta\n");
                            continue;
                        }
                    }


                    //SE L'UTENTE STA CERCANDO DI AVVIARE UNA CHAT CON UN ALTRO UTENTE
                    if(strcmp(istruzione, "chat") == 0){
                        //controllo che l'utente sia nella mia rubrica
                        if(user_is_friend(user, dest)){
                            //controllo se l'utente ha già una connessione aperta con me, in tal caso non devo ricorrere al server
                            if(check_user_connected(dest, connessioni_device) == true){
                                current_chat_sd = find_sd_by_user(dest, connessioni_device);
                                chat_title(dest, user);
                                continue;
                            }

                            // L'utente è in rubrica ma non connesso con me, richiedo al server la sua porta se quest'ultimo è online
                            if(server_on){
                                send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
                                send_wrap(sd, dest, 40, "Errore in fase di invio username");
                                recv_ack(sd, (void*)&ack);
                                
                                // Se l'ack=1 il destinatario è online, ricevo la porta, mi connetto seguendo il protocollo, apro una conn. TCP e chatto
                                if(ack == 1){
                                    ret = recv(sd, &porta_net, sizeof(uint16_t), 0);
                                    if(ret == -1){
                                        perror("Errore di ricezione porta utente ");
                                        exit(-1);
                                    }
                                    porta = ntohs(porta_net);
                                    newfd = openTCP(porta);
                                    FD_SET(newfd, &master);
                                    if(newfd > fdmax)
                                        fdmax = newfd; 
                                    connetti_user(dest, newfd, porta, &connessioni_device);
                                    current_chat_sd = newfd;
                                    send_wrap(newfd, user, 40, "Errore in fase di invio username");
                                    ret = send(newfd, &my_port_net, sizeof(uint16_t), 0);
                                    if(ret < 0){
                                        perror("Errore in fase di invio porta: \n");
                                        exit(1);
                                    }
                                    chat_title(dest, user);
                                    continue;
                                }
                                // Se l'ack è 2 l'utente è raggiungibile ma è prima necessario scaricare i messaggi pendenti
                                if(ack == 2){
                                    printf("-AVVISO: Download dei messaggi pendenti\n");
                                    while(true){
                                        recv_ack(sd, (void*)&ack);
                                        //se l'ack è 1 sto per ricevere un pending msg
                                        if(ack == 1){
                                            recv_wrap(sd, (void*)buffer, 650, "Errore nella ricezione dei messaggi pending");
                                            handle_msg_pen(buffer, user, dest);
                                        }
                                        //se l'ack è 0 sono finiti i pending msg
                                        if(ack == 0)
                                            break;
                                    }

                                    ret = recv(sd, &porta_net, sizeof(uint16_t), 0);
                                    if(ret == -1){
                                        perror("Errore di ricezione porta utente ");
                                        exit(-1);
                                    }
                                    porta = ntohs(porta_net);
                                    newfd = openTCP(porta);
                                    FD_SET(newfd, &master);
                                    if(newfd > fdmax)
                                        fdmax = newfd; 
                                    connetti_user(dest, newfd, porta, &connessioni_device);
                                    current_chat_sd = newfd;
                                    send_wrap(newfd, user, 40, "Errore in fase di invio username");
                                    ret = send(newfd, &my_port_net, sizeof(uint16_t), 0);
                                    if(ret < 0){
                                        perror("Errore in fase di invio porta: \n");
                                        exit(1);
                                    }
                                    chat_title(dest, user);
                                    continue;
                                }
                                //Se l'ack è 254 l'utente cercato è offline, il server può fungere da tramite ma prima vanno scaricati messaggi pendenti
                                if(ack == 254){
                                    printf("-AVVISO: Download dei messaggi pendenti\n");
                                    while(true){
                                        recv_ack(sd, (void*)&ack);
                                        //se l'ack è 1 sto per ricevere un pending msg
                                        if(ack == 1){
                                            recv_wrap(sd, (void*)buffer, 650, "Errore nella ricezione dei messaggi pending");
                                            handle_msg_pen(buffer, user, dest);
                                        }
                                        //se l'ack è 0 sono finiti i pending msg
                                        if(ack == 0)
                                            break;
                                    }

                                    current_chat_sd = sd;
                                    chat_title(dest, user);
                                    continue;
                                }
                                //se l'utente non è online e non ci sono messaggi pendenti si chatta e il server memorizza (ack = 255)
                                else{
                                    current_chat_sd = sd;
                                    chat_title(dest, user);
                                    continue;
                                }
                            }
                            //se l'utente  è in rubrica ma non connesso con me e il server è offline non se ne fa niente
                            if(server_on == false){
                                printf("-AVVISO: Non è possibile contattare l'utente: il server è offline\n");
                                continue;
                            }
                        }
                        //se l'utente non è in rubrica
                        else{
                            printf("-AVVISO: L'utente con cui desideri chattare non è nella tua rubrica\n");
                            continue;
                        }
                    }
                    
                    else{
                        printf("Comando Sconosciuto\n");
                    }
                    
                    continue;
                }


                //CASO IN CUI QUALCUNO STIA INVIANDOCI UN MESSAGGIO MENTRE NON SIAMO IN UNA CHAT APERTA
                ret = recv_wrap(i, (void*)buffer, 300, "Errore di ricezione messaggio in entrata");
                //va gestita la disconnessione di qualcuno, device o server che sia
                if(ret == false){
                    //se si è disconnesso il server memorizzo tale informazione (non potrò piu chattare con utenti offline)
                    if(i == sd){
                        printf("-NOTIFICA: Il server si è disconesso\n");
                        server_on = false;
                    }
                    //se è un qualsiasi altro utente lo rimuovo dalla lista connessioni e lo notifico sull'interfaccia
                    else{
                        disconnetti_user(i, &connessioni_device);
                    }
                    //rimuovo dal socket set
                    FD_CLR(i, &master);
                    continue;
                }
                

                //CASO IN CUI IL SERVER CI STIA INVIANDO UN MESSAGGIO. SUCCEDE SOLO QUANDO CI DEVE INVIARE LA 
                //CONFERMA CHE ALCUNI DEI NOSTRI MESSAGGI INVIATI E RIMASTI PENDING SONO STATI RECAPITATI
                else if(i == sd){
                    aggiorna_ack(user, buffer);
                }

                //CASO IN CUI CI SIA ARRIVATO UN MESSAGGIO DA UN DEVICE
                else{
                    //cerco il suo username nella lista connessione
                    find_user_by_sd(i, connessioni_device, mitt);
                    
                    //controllo che invece di essere un normale messaggio non sia una richiesta di unione ad un gruppo
                    if(strcmp(buffer, "group_request") == 0){
                        ack = 1;
                        group = true;
                        send_ack(i, (void*)&ack);
                        connetti_user(mitt, i, find_port_by_user(mitt, connessioni_device), &group_users);
                        group_chat_title();
                        continue;
                    }
                    //se qualcuno mi sta inviando un file
                    if(strcmp(buffer, "_file_") == 0){
                        recv_file(user, i);
                        printf("-NOTIFICA: Hai ricevuto un file da %s\n", mitt);
                        memset(mitt, 0, 41);
                        continue;
                    }

                    //altrimenti è un normale messaggio
                    printf("%s %s\n", "-NOTIFICA: Hai ricevuto un messaggio da", mitt);
                    handle_msg_rec(buffer, user, i, connessioni_device);
                    memset(mitt, 0, 41);
                }
            }










            //SE UN SOCKET E' PRONTO MENTRE SONO ALL'INTERNO DI UNA CHAT
            else if(current_chat_sd != -1 || group == true){
                memset(buffer, 0, 650);

                //CASO IN CUI IL SOCKET PRONTO SIA QUELLO CHE GESTISCE L'INPUT DELLA CONSOLE MENTRE SONO IN UNA CHAT
                if(i == STDIN_FILENO){       
                    ret = read(STDIN_FILENO, (void*)buffer, 300);
                    if(ret == -1){
                        printf("%s \n", "ERRORE NEL RICEVERE INPUT DA CONSOLE"); 
                        continue;
                    }

                    //in caso di comandi che prevedono un argomento è necessario parsare l'input in due stringhe diverse
                    memset(aux_buffer, 0, 300);
                    sscanf(buffer, "%s %s", comando, aux_buffer);

                    //SE L'UTENTE STA CERCANDO DI USCIRE DALLA CHAT ATTUALMENTE APERTA
                    if(strcmp(buffer, "\\q\n") == 0){
                        //se stiamo uscendo da una chat in cui era il server a memorizzare dei messaggi pending, va avvisato
                        if(current_chat_sd == sd)
                            send_wrap(sd, buffer, 650, "Errore nell'avvisare il server dell'uscita dalla chat");
                        current_chat_sd = -1;
                        //se stiamo uscendo da una chat di gruppo vanno avvisati tutti gli utenti poi va pulita la memoria
                        if(group == true){
                            handle_msg_sent_group(buffer, user, group_users);
                            group = false;
                            leave_group(&group_users);
                        }
                        system("clear");
                        print_dev_menu(user, my_port);
                        continue;
                    }

                    //SE L'UTENTE HA DIGITATO IL COMANDO PER VISUALIZZARE GLI UTENTI ONLINE ATTUALMENTE
                    else if(strcmp(buffer, "\\u\n") == 0){
                        if(server_on){
                            printf("\x1b[1F");
                            printf("-AGGIUNTA AD UN GRUPPO | UTENTI ONLINE E IN RUBRICA:\n");
                            send_wrap(sd, buffer, 10, "Errore nell'inoltrare la richiesta di \\u");
                            show_online_users(sd, user, connessioni_device);
                            continue;
                        }
                        printf("\x1b[1F");
                        printf("AVVISO: Il server è offline, impossibile inoltrare la richiesta\n");
                        continue;
                    }        


                    //SE L'UTENTE HA DIGITATO IL COMANDO PER AGGIUNGERE UN PARTECIPANTE AD UN GRUPPO, NUOVO O PRE-ESISTENTE CHE SIA
                    else if(strcmp(comando, "\\a") == 0){
                        //non si può creare una chat di gruppo se si è in una chat offline con il server
                        if(current_chat_sd == sd){
                            printf("\x1b[1F");
                            printf("-AVVISO: Non puoi iniziare una chat di gruppo mentre chatti con un utente offline\n");
                            continue;
                        }
                        //controllo che un utente burlone non voglia creare un gruppo con l'utente con cui sta già chattando in chat privata
                        if(strcmp(dest, aux_buffer) == 0){
                            printf("\x1b[1F");
                            printf("-AVVISO: Non puoi iniziare una chat di gruppo con l'utente con cui stai già chattando\n");
                            continue;
                        }
                        //ovviamente l'utente che si vuole aggiungere deve essere in rubrica
                        if(user_is_friend(user, aux_buffer) == false){
                            printf("\x1b[1F");
                            printf("-AVVISO: L'utente selezionato non è nella tua rubrica\n");
                            continue;
                        }
                        //se non sono già connesso con l'utente da aggiungere contatto il server e richiedo la porta
                        if(server_on && check_user_connected(aux_buffer, connessioni_device) == false){
                            send_wrap(sd, comando, 10, "Errore ne contattare il server per aggiungere un membro al gruppo");
                            send_wrap(sd, aux_buffer, 40, "Errore ne contattare il server per aggiungere un membro al gruppo");
                            recv_ack(sd, (void*)&ack);  
                            // Se l'ack=1 il destinatario è online, ricevo la porta, apro una conn. TCP e lo aggiungo
                            if(ack == 1){
                                ret = recv(sd, &porta_net, sizeof(uint16_t), 0);
                                if(ret == -1){
                                    perror("Errore di ricezione porta utente ");
                                    exit(-1);
                                }
                                porta = ntohs(porta_net);
                                newfd = openTCP(porta);
                                FD_SET(newfd, &master);
                                if(newfd > fdmax)
                                    fdmax = newfd; 
                                connetti_user(aux_buffer, newfd, porta, &connessioni_device);
                                send_wrap(newfd, user, 40, "Errore in fase di invio username");
                                ret = send(newfd, &my_port_net, sizeof(uint16_t), 0);
                                if(ret < 0){
                                    perror("Errore in fase di invio porta\n");
                                    exit(1);
                                }
                            }
                            else{
                                printf("-AVVISO: L'utente che si vuole aggiungere è offline\n");
                                continue;
                            }
                        }
                        //se sono già connesso con l'utente da aggiungere risparmio una richiesta al server
                        if(check_user_connected(aux_buffer, connessioni_device) == true){
                            //se group è false devo inizializzare la chat di gruppo, fino ad ora ero in una chat privata
                            if(group == false){
                                //caso in cui uno dei due utenti iniziali sia impegnato, abortisco subito l'operazione
                                if(add_to_group(aux_buffer, &group_users, connessioni_device) != true 
                                || add_to_group(dest, &group_users, connessioni_device) != true)
                                {
                                    memset(buffer, 0, 300);
                                    strcpy(buffer, "\\q\n");
                                    handle_msg_sent_group(buffer, user, group_users);
                                    leave_group(&group_users);
                                    continue;
                                }
                                //altrimenti il gruppo è creato con successo
                                group_chat_title();
                                current_chat_sd = -1;
                                memset(dest, 0, 41);
                                group = true;
                                continue;
                            }
                            //se group è true il gruppo è già creato e devo solo aggiungere un utente alla lista e avvisare gli altri
                            else{
                                if(add_to_group(aux_buffer, &group_users, connessioni_device) == true){
                                    printf("\x1b[1F");
                                    printf("\r-AVVISO: Hai aggiunto al gruppo l'utente %s\n", aux_buffer);
                                }
                                continue;
                            }
                        }
                        printf("-AVVISO: Il server è offline, impossibile richiedere porta utente\n");
                    }  



                    //SE STIAMO PROVANDO AD INVIARE UN FILE, CHE SIA IN UN GRUPPO O IN UNA CHAT NORMALE
                    else if(strcmp(comando, "share") == 0){
                        //se stiamo inviando il file all'interno di un gruppo
                        if(group == true)
                            send_file_group(aux_buffer, user, group_users);
                        else{
                            if(send_file(aux_buffer, user, current_chat_sd)){
                                printf("\x1b[1F");
                                printf("\r-AVVISO: Hai correttamente inviato il file\n");
                            }
                        }
                        continue;
                    }



                    //COMANDO DI UTILITY: NEI GRUPPI E' CONCESSO AI DEVICE USARE IL COMANDO list PER VEDERE GLI UTENTI ALL'INTERNO
                    else if(strcmp(buffer, "list\n") == 0){
                        stampa_gruppo(group_users);
                        continue;
                    }



                    //TOLTI TUTTI I CASE SPECIFICI RIMANE SOLO IL CASO GENERALE: STIAMO INVIANDO UN MESSAGGIO IN UNA CHAT DI GRUPPO O PRIVATA
                    else{
                        //SE SIAMO IN UNA CHAT DI GRUPPO
                        if(group == true){
                            handle_msg_sent_group(buffer, user, group_users);
                            continue;
                        }

                        //SE LA CHAT NON E' DI GRUPPO:
                        //se l'utente a cui sto scrivendo è offline ed è il server a memorizzare i messaggi
                        if(current_chat_sd == sd)
                            handle_msg_sent(buffer, user, dest, sd, false);
                        //se l'utente a cui sto scrivendo è online e può ricevere il messaggio
                        else
                            handle_msg_sent(buffer, user, dest, find_sd_by_user(dest, connessioni_device), true);
                    }          
    
                    continue;
                }
                
                

                //CASO IN CUI QUALCUNO STIA INVIANDOCI UN MESSAGGIO MENTRE SIAMO IN UNA CHAT APERTA
                ret = recv_wrap(i, (void*)buffer, 300, "Errore di ricezione messaggio in entrata");
                //guardo che non si sia disconnesso un socket, e gestisco i vari casi
                if(ret == false){
                    //caso in cui sia il server a essersi disconnesso
                    if(i == sd){
                        printf("-NOTIFICA: Il server si è disconesso\n");
                        server_on = false;
                        //se stavo scrivendo ad un utente offline e quindi era il server a memorizzare, ora non posso piu farlo
                        if(current_chat_sd == sd){
                            current_chat_sd = -1;
                            system("clear");
                            print_dev_menu(user, my_port);
                            printf("-AVVISO: Il server si è disconnesso per cui non sarà più possibile continuare la chat\n");
                        }
                    }
                    //caso in cui si sia disconnesso l'utente con cui stavo chattando
                    else if(i == current_chat_sd){
                        disconnetti_user(i, &connessioni_device);
                        //se a seguito di questa disconnessione il server è online da ora memorizzerà lui i messaggi
                        if(server_on){
                            //questa sleep evita una fastidiosa race condition in cui si farebbe richiesta al server di memorizzare
                            sleep(1);
                            strcpy(istruzione, "chat");
                            send_wrap(sd, istruzione, 10, "Errore in fase di invio comando");
                            send_wrap(sd, dest, 40, "Errore in fase di invio comando");
                            recv_ack(sd, (void*)&ack);
                            current_chat_sd = sd;
                        }
                        //altrimenti la chat viene chiusa e non ci sarà piu modo di comunicare con l'utente
                        else{
                            current_chat_sd = -1;
                            system("clear");
                            print_dev_menu(user, my_port);
                            printf("-AVVISO: L'utente destinatario è andato offline, impossibile continuare a chattare (server offline)\n");
                        }
                    }
                    //caso in cui si sia disconnesso un qualsiasi altro utente
                    else{
                        disconnetti_user(i, &connessioni_device);
                        if(check_sd_connected(i, group_users) == true){
                            printf("\x1b[1F");
                            find_user_by_sd(i, group_users, mitt);
                            disconnetti_user(i, &group_users);
                            printf("\x1b[1F");
                            printf("\r%s %s %s\n", "-NOTIFICA: L'utente", mitt, "si è disconnesso dal gruppo");
                            memset(mitt, 0, 41);
                            if(group_users == NULL){
                                group = false;
                                system("clear");
                                print_dev_menu(user, my_port);
                                printf("-AVVISO: Il gruppo è stato sciolto\n");
                            }
                        }
                    }
                    
                    FD_CLR(i, &master);
                    continue;
                }
                

                //CASO IN CUI IL SERVER CI STIA INVIANDO UN MESSAGGIO (SUCCEDE SOLO QUANDO CI DEVE INVIARE LA CONFERMA CHE
                //ALCUNI DEI NOSTRI MESSAGGI INVIATI E RIMASTI PENDING SONO STATI RECAPITATI)
                else if(i == sd){
                    aggiorna_ack(user, buffer);
                    if(strcmp(buffer, dest) == 0){
                        system("clear");
                        chat_title(dest, user);
                    }
                }



                //CASO IN CUI CI SIA ARRIVATO UN MESSAGGIO DA UN UTENTE ALL'INTERNO DEL GRUPPO
                else if(group == true && check_sd_connected(i, group_users) == true){

                    //caso in cui il messaggio ci stia indicando che un utente è uscito dal gruppo
                    if(strcmp(buffer, "\\q\n") == 0){
                        find_user_by_sd(i, group_users, mitt);
                        disconnetti_user(i, &group_users);
                        printf("\x1b[1F");
                        printf("\r%s %s %s\n", "-NOTIFICA: L'utente", mitt, "si è disconnesso dal gruppo");
                        memset(mitt, 0, 41);
                        if(group_users == NULL){
                            group = false;
                            system("clear");
                            print_dev_menu(user, my_port);
                            printf("-AVVISO: Il gruppo è stato sciolto\n");
                        }
                        continue;
                    }

                    //caso in cui un utente del gruppo sta aggiungendo un altro utente
                    if(strcmp(buffer, "group_request") == 0){
                        //devo ricevere porta e username dell'utente che è stato aggiunto da colui che lo ha aggiunto e aggiungerlo alla lista connessione,
                        //poi inviare al nuovo entrato il comando "group_add" così che lui mi aggiunga a sua volta alla sua lista connessione di gruppo
                        ret = recv(i, &porta_net, sizeof(uint16_t), 0);
                        if(ret == -1){
                            perror("Errore di ricezione porta utente");
                            exit(-1);
                        }
                        recv_wrap(i, (void*)group_user, 40, "Errore nel ricevere username utente gruppo");
                        porta = ntohs(porta_net);
                        if(check_port_connected(porta, connessioni_device) == false){
                            newfd = openTCP(porta);
                            FD_SET(newfd, &master);
                            if(newfd > fdmax)
                                fdmax = newfd; 
                            connetti_user(group_user, newfd, porta, &connessioni_device);
                            send_wrap(newfd, user, 40, "Errore in fase di invio username");
                            ret = send(newfd, &my_port_net, sizeof(uint16_t), 0);
                            if(ret < 0){
                                perror("Errore in fase di invio porta: \n");
                                exit(1);
                            }
                        }
                        connetti_user(group_user, find_sd_by_user(group_user, connessioni_device), porta, &group_users);
                        memset(buffer, 0, 50);
                        strcpy(buffer, "group_add");
                        send_wrap(find_sd_by_user(group_user, connessioni_device), buffer, 300, "Errore in fase di invio username");
                        printf("-NOTIFICA: L'utente %s è stato aggiunto al gruppo\n", group_user);
                        memset(group_user, 0, 41);
                        continue;
                    }

                    //caso in cui stiamo per ricevere un file da un utente del gruppo
                    if(strcmp(buffer, "_file_") == 0){
                        recv_file(user, i);
                        find_user_by_sd(i, group_users, group_user);
                        printf("-NOTIFICA: Hai ricevuto un file da %s\n", group_user);
                        memset(group_user, 0, 41);
                        continue;
                    }

                    //caso in cui il messaggio sia un effettivo messaggio normale              
                    handle_msg_rec_group(buffer, i, group_users);
                    continue;
                }

                //SE SIAMO APPENA ENTRATI IN UN GRUPPO E I MEMBRI MI INVIANO IL COMANDO "group_add" PER SEGNALARMI LA LORO PRESENZA NEL GRUPPO
                else if(group == true && strcmp(buffer, "group_add") == 0){
                    //posso ottenere l'username di chi mi ha inviato "group_add" perché ogni utente è salvato nella lista connessioni_device;
                    //il gruppo sfrutta per l'appunto una seconda lista con la stessa struct, per cui copio i dati dalla prima
                    find_user_by_sd(i, connessioni_device, group_user);
                    connetti_user(group_user, i, find_port_by_user(group_user, connessioni_device), &group_users);
                    memset(group_user, 0, 41);
                    continue;
                }



                //CASO IN CUI IN UNA CHAT PRIVATA RICEVA UN MESSAGGIO PROPRIO DALL'UTENTE CON CUI SONO IN CHAT
                else if(i == current_chat_sd){

                    //se l'utente con cui sto chattando ha creato un gruppo ci entro in automatico; è l'unico caso in cui un
                    //utente all'interno di una chat entra in un gruppo, cioè se il gruppo viene creato da colui con cui sono in chat
                    if(strcmp(buffer, "group_request") == 0){
                        ack = 1;
                        send_ack(i, (void*)&ack);
                        current_chat_sd = -1;
                        find_user_by_sd(i, connessioni_device, dest);
                        connetti_user(dest, i, find_port_by_user(dest, connessioni_device), &group_users);
                        group = true;
                        memset(dest, 0, 41);
                        group_chat_title();
                        continue;
                    }

                    find_user_by_sd(i, connessioni_device, mitt);

                    //se l'utente con cui sto chattando mi sta inviando un file
                    if(strcmp(buffer, "_file_") == 0){
                        recv_file(user, i);
                        printf("-NOTIFICA: Hai ricevuto un file da %s\n", mitt);
                        memset(mitt, 0, 41);
                        continue;
                    }
                    
                    //altrimenti sto semplicemente ricevendo un messaggio dall'utente con cui sono in chat
                    printf("%s%s%s%s\n", "-", mitt, ": ", buffer);
                    handle_msg_rec(buffer, user, i, connessioni_device);
                }



                //CASO IN CUI CI SIA ARRIVATO UN MESSAGGIO DA QUALSIASI ALTRO UTENTE
                else{
                    //Se qualcuno mi sta richiedendo di unirmi ad un gruppo mentre sono impegnato in un'altra chat rifiuto
                    if(strcmp(buffer, "group_request") == 0){
                        ack = 0;
                        send_ack(i, (void*)&ack);
                    }
                    find_user_by_sd(i, connessioni_device, mitt);

                    //se qualcuno mi sta inviando un file
                    if(strcmp(buffer, "_file_") == 0){
                        recv_file(user, i);
                        printf("-NOTIFICA: Hai ricevuto un file da %s\n", mitt);
                        memset(mitt, 0, 41);
                        continue;
                    }

                    //caso in cui mi stia scrivendo un utente diverso da quello con cui sono in chat
                    printf("%s %s\n", "-NOTIFICA: Hai ricevuto un messaggio da", mitt);
                    handle_msg_rec(buffer, user, i, connessioni_device);
                }
                
            }

        }
             
    }
    exit(0);
}
