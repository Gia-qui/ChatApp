#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>


#define NOT_SET 0

// TODO
// Quando si salva una connessione in lista rimuovere dalla data la newline che appare per qualche strano motivo

// Gestire l'intera parte del programma che riguarda la chat aperta


//STRUTTURA DATI CHE MEMORIZZA TUTTO CIO' CHE SERVE NELLE CONNESSIONI SERVER-DEVICE O DEVICE-DEVICE
struct connessione{
    char username[41];
    uint16_t conn_port;
    int conn_sd;
    time_t login_time;
    time_t logout_time;
    struct connessione* next;
} typedef connessione;

//STRUTTURA DATI UTILIZZATA IN ALCUNE OCCASIONI PER MEMORIZZARE IN UNA LISTA TUTTE LE INFO DEI MESSAGGI
struct messaggio{
    time_t timestamp;
    char mitt[41];
    char dest[41];
    char msg[300];
    bool sent;
    struct messaggio* next;
} typedef messaggio;

//STRUTTURA DATI UTILIZZATA PER LA MEMORIZZAZIONE IN LISTA DI GRANDI STRINGE PRIMA DI RISCRIVERE UN FILE MODIFICATO
struct line{
    char entry[650];
    struct line* next;
} typedef line;

//STRUTTURA DATI UTILIZZATA PER LA MEMORIZZAZIONE IN LISTA DI PICCOLE STRINGE PRIMA DI RISCRIVERE UN FILE MODIFICATO
struct shortline{
    char entry[90];
    struct shortline* next;
} typedef shortline;

//STRUTTURA DATI UTILIZZATA DAL SERVER PER GESTIRE GLI UTENTI CHE GLI INVIANO MESSAGGI
//POICHE' STANNO IN REALTA' SCRIVENDO AD UN UTENTE OFFLINE E IL SERVER DEVE MEMORIZZARE
struct offline_chat{
    int sd;
    char mitt[41];
    char dest[41];
    struct offline_chat* next;
} typedef offline_chat;

//STRUTTURA DATI UTILIZZATA DAL SERVER PER GESTIRE LE RICHIESTE DI HANGING
struct hanging_entry{
    char user[41];
    int count;
    time_t timestamp;
    struct hanging_entry* next;
} typedef hanging_entry;



int openTCP(uint16_t);
void send_wrap(int, void*, size_t, const char*);
bool recv_wrap(int, void*, size_t, const char*);
void send_ack(int, void*);
void recv_ack(int, void*);
bool entry_too_short(char*);
void registra_utente(char*, char*);
bool user_already_exist(char*);
bool account_already_exist(char*, char*);
void esito_signup(uint8_t);
bool esito_login(uint8_t);
connessione* connetti_user(char*, int, uint16_t, connessione**);
connessione* disconnetti_user(int, connessione**);
void stampa_connessioni(connessione*);
void stampa_help();
bool user_is_friend(char*, char*);
bool check_user_online(char*, connessione*);
bool check_user_connected(char*, connessione*);
void chat_title(char*, char*);
bool find_user_by_sd(int, connessione*, char*);
int find_sd_by_user(char*, connessione*);
void handle_msg_rec(char*, char*, int, connessione*);
bool check_pending_msg(char*, char*);
uint16_t find_port_by_user(char*, connessione*);
void send_pending_msg(int, char*, char*);
void handle_msg_pen(char*, char*, char*);
void aggiorna_ack(char*, char*);
void send_ack_info(int, char*);
void handle_msg_sent(char*, char*, char*, int, bool);
void new_offline_chat(int, char*, connessione*, offline_chat**);
bool sd_is_using_offline_chat(int, offline_chat*);
void close_offline_chat(int, offline_chat**);
void save_pending_msg(char*, int, offline_chat*);
void removeChar(char *, char);
void save_ack_info(char*, char*);
void print_dev_menu(char*, uint16_t);
void print_serv_menu(uint16_t);
void handle_hanging(int, char*);
void show_online_users(int, char*, connessione*);
void handle_show_online(int, connessione*);
void handle_msg_sent_group(char*, char*, connessione*);
void handle_msg_rec_group(char*, int, connessione*);
bool check_sd_connected(int, connessione*);
void leave_group(connessione**);
bool add_to_group(char*, connessione**, connessione*);
void group_chat_title();
bool check_port_connected(uint16_t, connessione*);
bool send_file(char*, char*, int);
void send_file_group(char*, char*, connessione*);
void recv_file(char*, int);
void stampa_gruppo(connessione*);
void registra_login(connessione*);
void registra_logout(char*, bool, int);
void send_logout_time(char*, int);
void save_logout_time(char*);