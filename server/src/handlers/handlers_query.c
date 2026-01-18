#include "handlers_common.h"

void handle_get_match(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    char match_json[65536];
    if (!db_get_match(match_id, match_json, sizeof(match_json))) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    send_response(server, client, msg->seq, true, "Match found", match_json);
}

void handle_leaderboard(server_t* server, client_t* client, message_t* msg) {
    if (!msg->payload_json) {
        send_response(server, client, msg->seq, false, "Invalid request payload", NULL);
        return;
    }

    int limit = json_get_int(msg->payload_json, "limit");
    int offset = json_get_int(msg->payload_json, "offset");

    if (limit <= 0)
        limit = 10;
    if (offset < 0)
        offset = 0;

    size_t buffer_size = 16384;
    char* leaderboard_json = (char*)malloc(buffer_size);

    if (!leaderboard_json) {
        perror("malloc failed in handle_leaderboard");
        send_response(server, client, msg->seq, false, "Server memory error", NULL);
        return;
    }

    if (!db_get_leaderboard(limit, offset, leaderboard_json, buffer_size)) {
        send_response(server, client, msg->seq, false, "Failed to get leaderboard", NULL);
        free(leaderboard_json);
        return;
    }

    send_response(server, client, msg->seq, true, "Leaderboard", leaderboard_json);
    free(leaderboard_json);
}

void handle_join_match(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    match_t* match = match_find_by_id(match_id);

    if (!match || !match->active) {
        match = match_load_from_db(match_id);
    }

    if (!match || !match->active) {
        send_response(server, client, msg->seq, false, "Match not found or ended", NULL);
        return;
    }

    if (match->red_user_id != user_id && match->black_user_id != user_id) {
        send_response(server, client, msg->seq, false, "Not a player in this match", NULL);
        return;
    }

    bool is_red_turn = (match->move_count % 2 == 0);
    const char* current_turn = is_red_turn ? "red" : "black";
    bool is_my_turn =
        (is_red_turn && match->red_user_id == user_id) || (!is_red_turn && match->black_user_id == user_id);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"match_id\":\"%s\",\"move_count\":%d,\"current_turn\":\"%s\","
             "\"is_my_turn\":%s}",
             match_id, match->move_count, current_turn, is_my_turn ? "true" : "false");

    send_response(server, client, msg->seq, true, "Joined match", payload);

    printf("[Handler] User %d joined match %s (move_count=%d, is_my_turn=%d)\n", user_id, match_id, match->move_count,
           is_my_turn);
}
