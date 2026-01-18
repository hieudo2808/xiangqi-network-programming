#include "handlers_common.h"

void handle_join_spectate(server_t* server, client_t* client, message_t* msg) {

    int user_id = 0;
    if (msg->token && strlen(msg->token) > 0) {
        validate_token_and_get_user(msg->token, &user_id);
    }

    if (user_id > 0) {
        client->user_id = user_id;
        client->authenticated = true;
    }

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    match_t* match = match_find_by_id(match_id);
    if (!match || !match->active) {
        send_response(server, client, msg->seq, false, "Match not found or ended", NULL);
        return;
    }

    bool is_red_turn = (match->move_count % 2 == 0);
    const char* current_turn = is_red_turn ? "red" : "black";

    char* match_json = match_get_json(match_id);
    if (!match_json) {
        send_response(server, client, msg->seq, false, "Failed to get match state", NULL);
        return;
    }

    if (!match_add_spectator(match_id, user_id)) {
        send_response(server, client, msg->seq, false, "Failed to add spectator", NULL);
        free(match_json);
        return;
    }

    char payload[4096];
    snprintf(payload, sizeof(payload),
             "{\"match_id\":\"%s\",\"move_count\":%d,\"current_turn\":\"%s\","
             "\"is_spectator\":true,\"match_data\":%s}",
             match_id, match->move_count, current_turn, match_json);

    send_response(server, client, msg->seq, true, "Joined as spectator", payload);

    printf("[Handler] User %d spectating match %s (move_count=%d)\n", user_id, match_id, match->move_count);

    free(match_json);
}

void handle_leave_spectate(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    if (match_remove_spectator(match_id, user_id)) {
        send_response(server, client, msg->seq, true, "Left spectate mode", NULL);
    } else {
        send_response(server, client, msg->seq, false, "Not spectating this match", NULL);
    }
}

void handle_heartbeat(server_t* server, client_t* client, message_t* msg) {
    send_response(server, client, msg->seq, true, "pong", NULL);
}
