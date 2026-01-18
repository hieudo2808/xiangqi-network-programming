#ifndef HANDLERS_H
#define HANDLERS_H

#include "protocol.h"
#include "server.h"

void handle_register(server_t* server, client_t* client, message_t* msg);
void handle_login(server_t* server, client_t* client, message_t* msg);
void handle_logout(server_t* server, client_t* client, message_t* msg);
void handle_set_ready(server_t* server, client_t* client, message_t* msg);
void handle_find_match(server_t* server, client_t* client, message_t* msg);
void handle_move(server_t* server, client_t* client, message_t* msg);
void handle_resign(server_t* server, client_t* client, message_t* msg);
void handle_draw_offer(server_t* server, client_t* client, message_t* msg);
void handle_draw_response(server_t* server, client_t* client, message_t* msg);
void handle_game_over(server_t* server, client_t* client, message_t* msg);
void handle_challenge(server_t* server, client_t* client, message_t* msg);
void handle_challenge_response(server_t* server, client_t* client, message_t* msg);
void handle_get_match(server_t* server, client_t* client, message_t* msg);
void handle_leaderboard(server_t* server, client_t* client, message_t* msg);
void handle_heartbeat(server_t* server, client_t* client, message_t* msg);
void handle_chat_message(server_t* server, client_t* client, message_t* msg);

void handle_create_room(server_t* server, client_t* client, message_t* msg);
void handle_join_room(server_t* server, client_t* client, message_t* msg);
void handle_leave_room(server_t* server, client_t* client, message_t* msg);
void handle_get_rooms(server_t* server, client_t* client, message_t* msg);
void handle_start_room_game(server_t* server, client_t* client, message_t* msg);

void handle_rematch_request(server_t* server, client_t* client, message_t* msg);
void handle_rematch_response(server_t* server, client_t* client, message_t* msg);

void handle_match_history(server_t* server, client_t* client, message_t* msg);

void handle_get_live_matches(server_t* server, client_t* client, message_t* msg);
void handle_join_spectate(server_t* server, client_t* client, message_t* msg);
void handle_leave_spectate(server_t* server, client_t* client, message_t* msg);

void handle_get_profile(server_t* server, client_t* client, message_t* msg);

void handle_get_timer(server_t* server, client_t* client, message_t* msg);

void dispatch_handler(server_t* server, client_t* client, message_t* msg);

#endif
