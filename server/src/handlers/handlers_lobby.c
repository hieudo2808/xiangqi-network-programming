#include "handlers_common.h"

void handle_set_ready(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    bool ready = json_get_bool(msg->payload_json, "ready");

    char username[64];
    int rating;
    if (!db_get_user_by_id(user_id, username, NULL, &rating, NULL, NULL, NULL)) {
        send_response(server, client, msg->seq, false, "User not found", NULL);
        return;
    }

    lobby_set_ready(user_id, username, rating, ready);

    char* ready_list = lobby_get_ready_list_json();
    if (ready_list) {
        char broadcast_msg[8192];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"ready_list_update\",\"payload\":%s}\n", ready_list);
        broadcast_to_lobby(server, broadcast_msg);
        free(ready_list);
    }

    send_response(server, client, msg->seq, true, ready ? "Ready set" : "Ready removed", NULL);
}

void handle_find_match(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    LOG_INFO("handle_find_match called: user_id=%d, seq=%d", user_id, msg->seq);

    bool rated = json_get_bool(msg->payload_json, "rated");

    {
        char username[64];
        int rating;
        if (db_get_user_by_id(user_id, username, NULL, &rating, NULL, NULL, NULL)) {
            lobby_set_ready(user_id, username, rating, true);
            printf("[Handler] Marked user_id=%d as ready (auto)\n", user_id);

            char* ready_list = lobby_get_ready_list_json();
            if (ready_list) {
                char broadcast_msg[8192];
                snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"ready_list_update\",\"payload\":%s}\n",
                         ready_list);
                broadcast_to_lobby(server, broadcast_msg);
                free(ready_list);
            }
        } else {
            printf("[Handler] Warning: failed to lookup user %d before queuing\n", user_id);
        }
    }

    int opponent_id;
    bool found = false;

    if (rated) {

        int rating;
        db_get_user_by_id(user_id, NULL, NULL, &rating, NULL, NULL, NULL);
        found = lobby_find_rated_match(user_id, rating, 200, &opponent_id);
    } else {
        found = lobby_find_random_match(user_id, &opponent_id);
    }

    if (!found) {

        printf("[Handler] No opponent currently for user_id=%d — player queued\n", user_id);
        send_response(server, client, msg->seq, true, "Queued for match", "{\"status\":\"queued\"}");
        return;
    }

    if (!is_user_connected(server, user_id)) {
        printf("[Handler] Aborting match: requester user_id=%d not connected\n", user_id);
        send_response(server, client, msg->seq, false, "You are not connected", NULL);
        return;
    }
    if (!is_user_connected(server, opponent_id)) {

        printf("[Handler] Opponent %d not connected; keeping user %d queued\n", opponent_id, user_id);
        send_response(server, client, msg->seq, true, "Queued for match", "{\"status\":\"queued\"}");

        lobby_remove_player(opponent_id);
        return;
    }

    char* match_id = match_create(user_id, opponent_id, rated, 600000);
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Failed to create match", NULL);
        return;
    }
    match_persist(match_id);

    char user_name[64], opp_name[64];
    db_get_user_by_id(user_id, user_name, NULL, NULL, NULL, NULL, NULL);
    db_get_user_by_id(opponent_id, opp_name, NULL, NULL, NULL, NULL, NULL);

    char payload_a[512];
    char payload_b[512];

    snprintf(payload_a, sizeof(payload_a),
             "{\"match_id\":\"%s\",\"red_user\":\"%s\",\"black_user\":\"%s\","
             "\"your_color\":\"%s\",\"time_per_player\":600000}",
             match_id, user_name, opp_name, "red");

    snprintf(payload_b, sizeof(payload_b),
             "{\"match_id\":\"%s\",\"red_user\":\"%s\",\"black_user\":\"%s\","
             "\"your_color\":\"%s\",\"time_per_player\":600000}",
             match_id, user_name, opp_name, "black");

    char notify_a[1024];
    char notify_b[1024];
    snprintf(notify_a, sizeof(notify_a), "{\"type\":\"match_found\",\"payload\":%s}\n", payload_a);
    snprintf(notify_b, sizeof(notify_b), "{\"type\":\"match_found\",\"payload\":%s}\n", payload_b);

    bool sent_a = send_to_user(server, user_id, notify_a);
    bool sent_b = send_to_user(server, opponent_id, notify_b);

    if (!sent_a || !sent_b) {
        printf("[Handler] Warning: match notify failed (sent_a=%d, sent_b=%d). "
               "Rolling back match %s\n",
               sent_a, sent_b, match_id);

        match_end(match_id, "aborted", "notify_failed");

        int rating_a = 0, rating_b = 0;
        db_get_user_by_id(user_id, NULL, NULL, &rating_a, NULL, NULL, NULL);
        db_get_user_by_id(opponent_id, NULL, NULL, &rating_b, NULL, NULL, NULL);

        if (is_user_connected(server, user_id)) {
            lobby_set_ready(user_id, user_name, rating_a, true);
        }
        if (is_user_connected(server, opponent_id)) {
            lobby_set_ready(opponent_id, opp_name, rating_b, true);
        }

        send_response(server, client, msg->seq, true, "Queued for match", "{\"status\":\"queued\"}");

        free(match_id);
        return;
    }

    send_response(server, client, msg->seq, true, "Match found", payload_a);

    free(match_id);
    printf("[Handler] Match created: %s vs %s (sent to user %d: %d, opponent %d: "
           "%d)\n",
           user_name, opp_name, user_id, sent_a, opponent_id, sent_b);
}
