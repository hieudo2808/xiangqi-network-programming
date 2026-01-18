#ifndef BROADCAST_H
#define BROADCAST_H

#include <stdbool.h>

#include "server.h"

void broadcast_to_match(server_t* server, const char* match_id, const char* message);

void broadcast_to_lobby(server_t* server, const char* message);

bool send_to_user(server_t* server, int user_id, const char* message);

bool send_to_client(server_t* server, int client_fd, const char* message);

bool is_user_connected(server_t* server, int user_id);

void broadcast_to_all(server_t* server, const char* message);

#endif
