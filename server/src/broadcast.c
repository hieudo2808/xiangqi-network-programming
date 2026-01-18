#include "../include/broadcast.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/lobby.h"
#include "../include/match.h"
#include "../include/server.h"

bool send_to_client(server_t* server, int client_fd, const char* message) {
    if (!server || !message || client_fd < 0) {
        return false;
    }

    client_t* client = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] && server->clients[i]->fd == client_fd) {
            client = server->clients[i];
            break;
        }
    }

    if (!client) {
        fprintf(stderr, "[Broadcast] Client fd %d not found\n", client_fd);
        return false;
    }

    char buffer[MAX_MESSAGE_SIZE];
    snprintf(buffer, sizeof(buffer), "%s%s", message, (message[strlen(message) - 1] == '\n') ? "" : "\n");

    ssize_t sent = send(client_fd, buffer, strlen(buffer), MSG_NOSIGNAL);
    if (sent < 0) {
        perror("[Broadcast] send() failed");
        return false;
    }

    printf("[Broadcast] Sent to fd %d: %.*s\n", client_fd, (int)(strlen(buffer) - 1), buffer);
    return true;
}

bool send_to_user(server_t* server, int user_id, const char* message) {
    if (!server || !message || user_id <= 0) {
        return false;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] && server->clients[i]->user_id == user_id) {
            return send_to_client(server, server->clients[i]->fd, message);
        }
    }

    fprintf(stderr, "[Broadcast] User %d not connected\n", user_id);

    fprintf(stderr, "[Broadcast] Connected clients (user_id:fd):");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_t* c = server->clients[i];
        if (c) {
            fprintf(stderr, " %d:%d", c->user_id, c->fd);
        }
    }
    fprintf(stderr, "\n");
    return false;
}

bool is_user_connected(server_t* server, int user_id) {
    if (!server || user_id <= 0)
        return false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] && server->clients[i]->user_id == user_id)
            return true;
    }
    return false;
}

void broadcast_to_match(server_t* server, const char* match_id, const char* message) {
    if (!server || !match_id || !message) {
        return;
    }

    match_t* match = match_find_by_id(match_id);
    if (!match) {
        fprintf(stderr, "[Broadcast] Match %s not found\n", match_id);
        return;
    }

    send_to_user(server, match->red_user_id, message);
    send_to_user(server, match->black_user_id, message);

    for (int i = 0; i < match->spectator_count; i++) {
        send_to_user(server, match->spectator_ids[i], message);
    }

    printf("[Broadcast] Sent to match %s (players: %d, %d, spectators: %d)\n", match_id, match->red_user_id,
           match->black_user_id, match->spectator_count);
}

void broadcast_to_lobby(server_t* server, const char* message) {
    if (!server || !message) {
        return;
    }

    int ready_users[MAX_CLIENTS];
    int count = lobby_get_ready_users(ready_users, MAX_CLIENTS);

    for (int i = 0; i < count; i++) {
        send_to_user(server, ready_users[i], message);
    }

    printf("[Broadcast] Sent to %d ready users in lobby\n", count);
}

void broadcast_to_all(server_t* server, const char* message) {
    if (!server || !message) {
        return;
    }

    int sent_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] && send_to_client(server, server->clients[i]->fd, message)) {
            sent_count++;
        }
    }

    printf("[Broadcast] Sent to %d/%d clients\n", sent_count, server->client_count);
}
