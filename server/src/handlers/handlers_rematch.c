#include "handlers_common.h"

void handle_rematch_request(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Match ID required", NULL);
        return;
    }

    match_t* match = match_get(match_id);
    if (!match) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    if (match->red_user_id != user_id && match->black_user_id != user_id) {
        send_response(server, client, msg->seq, false, "Not in this match", NULL);
        return;
    }

    int opponent_id = (match->red_user_id == user_id) ? match->black_user_id : match->red_user_id;

    char username[64] = {0};
    db_get_username(user_id, username, sizeof(username));

    client_t* opponent_client = server_get_client_by_user_id(server, opponent_id);
    if (!opponent_client) {
        send_response(server, client, msg->seq, false, "Opponent not online", NULL);
        return;
    }

    char notification[512];
    snprintf(notification, sizeof(notification),
             "{\"type\":\"rematch_request\",\"payload\":{\"match_id\":\"%s\","
             "\"from_user_id\":%d,\"from_username\":\"%s\"}}\n",
             match_id, user_id, username);
    send_to_client(server, opponent_client->fd, notification);

    send_response(server, client, msg->seq, true, "Rematch request sent", NULL);
}

void handle_rematch_response(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    bool accept = json_get_bool(msg->payload_json, "accept");

    if (!match_id) {
        send_response(server, client, msg->seq, false, "Match ID required", NULL);
        return;
    }

    match_t* old_match = match_get(match_id);
    if (!old_match) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    int opponent_id = (old_match->red_user_id == user_id) ? old_match->black_user_id : old_match->red_user_id;
    client_t* opponent_client = server_get_client_by_user_id(server, opponent_id);

    if (!accept) {

        send_response(server, client, msg->seq, true, "Rematch declined", NULL);

        if (opponent_client) {
            char notification[256];
            snprintf(notification, sizeof(notification),
                     "{\"type\":\"rematch_declined\",\"payload\":{\"match_id\":\"%s\"}}\n", match_id);
            send_to_client(server, opponent_client->fd, notification);
        }
        printf("[Handler] Rematch declined by user %d\n", user_id);
        return;
    }

    int new_red = old_match->black_user_id;
    int new_black = old_match->red_user_id;
    bool rated = old_match->rated;
    int time_ms = old_match->red_time_ms > 0 ? old_match->red_time_ms : 600000;

    char* new_match_id = match_create(new_red, new_black, rated, time_ms);
    if (!new_match_id) {
        send_response(server, client, msg->seq, false, "Failed to create rematch", NULL);
        return;
    }
    match_persist(new_match_id);

    char red_username[64] = {0}, black_username[64] = {0};
    int red_rating = 1500, black_rating = 1500;
    db_get_user_by_id(new_red, red_username, NULL, &red_rating, NULL, NULL, NULL);
    db_get_user_by_id(new_black, black_username, NULL, &black_rating, NULL, NULL, NULL);

    char red_payload[512];
    snprintf(red_payload, sizeof(red_payload),
             "{\"match_id\":\"%s\",\"your_color\":\"red\",\"opponent_id\":%d,"
             "\"opponent_name\":\"%s\",\"opponent_rating\":%d,\"rated\":%s,"
             "\"rematch\":true}",
             new_match_id, new_black, black_username, black_rating, rated ? "true" : "false");

    client_t* red_client = server_get_client_by_user_id(server, new_red);
    client_t* black_client = server_get_client_by_user_id(server, new_black);

    if (red_client) {
        char red_msg[MAX_MESSAGE_SIZE];
        snprintf(red_msg, sizeof(red_msg), "{\"type\":\"match_found\",\"payload\":%s}\n", red_payload);
        send_to_client(server, red_client->fd, red_msg);
    }

    char black_payload[512];
    snprintf(black_payload, sizeof(black_payload),
             "{\"match_id\":\"%s\",\"your_color\":\"black\",\"opponent_id\":%d,"
             "\"opponent_name\":\"%s\",\"opponent_rating\":%d,\"rated\":%s,"
             "\"rematch\":true}",
             new_match_id, new_red, red_username, red_rating, rated ? "true" : "false");

    if (black_client) {
        char black_msg[MAX_MESSAGE_SIZE];
        snprintf(black_msg, sizeof(black_msg), "{\"type\":\"match_found\",\"payload\":%s}\n", black_payload);
        send_to_client(server, black_client->fd, black_msg);
    }

    send_response(server, client, msg->seq, true, "Rematch accepted", NULL);
    printf("[Handler] Rematch created: %s (colors swapped)\n", new_match_id);
    free(new_match_id);
}

void handle_match_history(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    int limit = json_get_int(msg->payload_json, "limit");
    int offset = json_get_int(msg->payload_json, "offset");

    if (limit <= 0)
        limit = 20;
    if (limit > 100)
        limit = 100;
    if (offset < 0)
        offset = 0;

    char history_json[16384];
    if (!db_get_match_history(user_id, limit, offset, history_json, sizeof(history_json))) {
        send_response(server, client, msg->seq, false, "Failed to get match history", NULL);
        return;
    }

    char payload[16500];
    snprintf(payload, sizeof(payload), "{\"matches\":%s}", history_json);
    send_response(server, client, msg->seq, true, "Match history", payload);

    printf("[Handler] Match history for user %d (limit=%d, offset=%d)\n", user_id, limit, offset);
}

void handle_get_live_matches(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    char* live_matches_json = match_get_live_matches_json();
    if (!live_matches_json) {
        send_response(server, client, msg->seq, false, "Failed to get live matches", NULL);
        return;
    }

    char* payload = malloc(strlen(live_matches_json) + 128);
    if (!payload) {
        free(live_matches_json);
        send_response(server, client, msg->seq, false, "Memory allocation failed", NULL);
        return;
    }

    sprintf(payload, "{\"matches\":%s}", live_matches_json);
    send_response(server, client, msg->seq, true, "Live matches", payload);

    free(live_matches_json);
    free(payload);

    printf("[Handler] Get live matches for user %d\n", user_id);
}

void handle_get_profile(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    int target_user_id = json_get_int(msg->payload_json, "user_id");
    if (target_user_id <= 0) {
        target_user_id = user_id;
    }

    char profile_json[2048];
    if (!db_get_user_profile(target_user_id, profile_json, sizeof(profile_json))) {
        send_response(server, client, msg->seq, false, "User not found", NULL);
        return;
    }

    char payload[2200];
    snprintf(payload, sizeof(payload), "{\"profile\":%s}", profile_json);

    send_response(server, client, msg->seq, true, "Profile data", payload);

    printf("[Handler] Get profile for user %d (requested by %d)\n", target_user_id, user_id);
}

void handle_get_timer(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id || strlen(match_id) == 0) {

        match_t* match = match_find_by_user(user_id);
        if (!match) {
            send_response(server, client, msg->seq, false, "No active match", NULL);
            return;
        }
        match_id = match->match_id;
    }

    char* timer_json = match_get_timer_json(match_id);
    if (!timer_json) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload), "{\"timer\":%s}", timer_json);
    send_response(server, client, msg->seq, true, "Timer data", payload);

    free(timer_json);
}
