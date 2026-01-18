#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024
#define MAX_CLIENTS 1000
#define BUFFER_SIZE 8192
#define MAX_MESSAGE_SIZE 16384

typedef struct {
    int fd;
    char recv_buffer[MAX_MESSAGE_SIZE];
    size_t recv_len;
    char send_buffer[MAX_MESSAGE_SIZE];
    size_t send_len;
    size_t send_offset;
    char* session_token;
    int user_id;
    bool authenticated;
    time_t last_heartbeat;
} client_t;

typedef struct {
    int listen_fd;
    int epoll_fd;
    client_t* clients[MAX_CLIENTS];
    int client_count;
    bool running;
} server_t;

int server_init(server_t* server, int port);
void server_run(server_t* server);
void server_shutdown(server_t* server);

client_t* client_create(int fd);
void client_destroy(client_t* client);
int client_send(client_t* client, const char* json);
void client_disconnect(server_t* server, client_t* client);
client_t* server_get_client_by_user_id(server_t* server, int user_id);

void handle_new_connection(server_t* server);
void handle_client_read(server_t* server, client_t* client);
void handle_client_write(server_t* server, client_t* client);

void process_message(server_t* server, client_t* client, const char* json);

#endif
