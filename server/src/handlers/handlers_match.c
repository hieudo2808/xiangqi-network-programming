#include "handlers_common.h"

void handle_move(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    int from_row = json_get_int(msg->payload_json, "from_row");
    int from_col = json_get_int(msg->payload_json, "from_col");
    int to_row = json_get_int(msg->payload_json, "to_row");
    int to_col = json_get_int(msg->payload_json, "to_col");

    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    match_t* match = match_find_by_id(match_id);
    if (!match) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    bool is_red_turn = (match->move_count % 2 == 0);
    bool is_red_player = (match->red_user_id == user_id);

    if (is_red_turn != is_red_player) {
        send_response(server, client, msg->seq, false, "Not your turn", NULL);
        return;
    }

    match_update_timer(match_id);

    if (match_check_timeout(match_id)) {
        send_response(server, client, msg->seq, false, "Time expired", NULL);
        return;
    }

    move_t move = {0};
    move.from_row = from_row;
    move.from_col = from_col;
    move.to_row = to_row;
    move.to_col = to_col;
    move.timestamp = time(NULL);

    move.red_time_ms = match->red_time_ms;
    move.black_time_ms = match->black_time_ms;

    if (!match_add_move(match_id, &move)) {
        send_response(server, client, msg->seq, false, "Failed to add move", NULL);
        return;
    }

    char timer_json[128];
    snprintf(timer_json, sizeof(timer_json), "{\"red_time_ms\":%d,\"black_time_ms\":%d}", match->red_time_ms,
             match->black_time_ms);
    send_response(server, client, msg->seq, true, "Move accepted", timer_json);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"match_id\":\"%s\",\"from\":{\"row\":%d,\"col\":%d},\"to\":{"
             "\"row\":%d,\"col\":%d},\"red_time_ms\":%d,\"black_time_ms\":%d}",
             match_id, from_row, from_col, to_row, to_col, match->red_time_ms, match->black_time_ms);

    char broadcast_msg[1024];
    snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"opponent_move\",\"payload\":%s}\n", payload);

    int opponent_id = (match->red_user_id == user_id) ? match->black_user_id : match->red_user_id;
    send_to_user(server, opponent_id, broadcast_msg);

    for (int i = 0; i < match->spectator_count; i++) {
        send_to_user(server, match->spectator_ids[i], broadcast_msg);
    }

    printf("[Handler] Move: %s (%d,%d)->(%d,%d) [Red:%dms, Black:%dms]\n", match_id, from_row, from_col, to_row, to_col,
           match->red_time_ms, match->black_time_ms);

    match_persist(match_id);
}

void handle_resign(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    match_t* match = match_get(match_id);
    if (!match || !match->active) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    const char* result = (user_id == match->red_user_id) ? "black_win" : "red_win";

    match_end(match_id, result, "resign");

    int new_red_rating = 0;
    int new_black_rating = 0;

    if (match->rated) {
        char u1[64], e1[128];
        int r1, w1, l1, d1;
        char u2[64], e2[128];
        int r2, w2, l2, d2;

        db_get_user_by_id(match->red_user_id, u1, e1, &r1, &w1, &l1, &d1);
        db_get_user_by_id(match->black_user_id, u2, e2, &r2, &w2, &l2, &d2);

        rating_change_t rc = rating_calculate(r1, r2, result, DEFAULT_K_FACTOR);

        new_red_rating = r1 + rc.red_change;
        new_black_rating = r2 + rc.black_change;

        if (strcmp(result, "red_win") == 0) {
            w1++;
            l2++;
        } else {
            l1++;
            w2++;
        }

        db_update_user_rating(match->red_user_id, new_red_rating);
        db_update_user_stats(match->red_user_id, w1, l1, d1);

        db_update_user_rating(match->black_user_id, new_black_rating);
        db_update_user_stats(match->black_user_id, w2, l2, d2);

        printf("[Rating] Resign: Red(%d->%d), Black(%d->%d)\n", r1, new_red_rating, r2, new_black_rating);
    }

    char* moves_json = match_get_moves_json(match);
    char started[32], ended[32];
    sprintf(started, "%ld", match->started_at);
    sprintf(ended, "%ld", time(NULL));
    db_save_match(match_id, match->red_user_id, match->black_user_id, result, moves_json, started, ended);
    db_delete_active_match(match_id);
    free(moves_json);

    send_response(server, client, msg->seq, true, "Resigned", NULL);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"match_id\":\"%s\",\"result\":\"%s\",\"red_rating\":%d,\"black_"
             "rating\":%d}",
             match_id, result, new_red_rating, new_black_rating);

    char notify[1024];
    snprintf(notify, sizeof(notify), "{\"type\":\"game_end\",\"payload\":%s}\n", payload);
    broadcast_to_match(server, match_id, notify);
}

void handle_draw_offer(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    match_t* match = match_get(match_id);
    if (!match || !match->active) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    int opponent_id = match_get_opponent_id(match, user_id);
    char payload[256];
    snprintf(payload, sizeof(payload), "{\"match_id\":\"%s\"}", match_id);
    char notify[512];
    snprintf(notify, sizeof(notify), "{\"type\":\"draw_offer\",\"payload\":%s}\n", payload);
    send_to_user(server, opponent_id, notify);

    send_response(server, client, msg->seq, true, "Draw offer sent", NULL);
}

void handle_draw_response(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    bool accept = json_get_bool(msg->payload_json, "accept");

    if (!match_id) {
        send_response(server, client, msg->seq, false, "Missing match_id", NULL);
        return;
    }

    if (accept) {
        match_t* match = match_get(match_id);
        if (!match || !match->active) {
            send_response(server, client, msg->seq, false, "Match not found or ended", NULL);
            return;
        }

        match_end(match_id, "draw", "agreement");

        int new_red_rating = 0;
        int new_black_rating = 0;

        if (match->rated) {
            char u1[64], e1[128];
            int r1, w1, l1, d1;
            char u2[64], e2[128];
            int r2, w2, l2, d2;

            db_get_user_by_id(match->red_user_id, u1, e1, &r1, &w1, &l1, &d1);
            db_get_user_by_id(match->black_user_id, u2, e2, &r2, &w2, &l2, &d2);

            rating_change_t rc = rating_calculate(r1, r2, "draw", DEFAULT_K_FACTOR);

            new_red_rating = r1 + rc.red_change;
            new_black_rating = r2 + rc.black_change;

            d1++;
            d2++;
            db_update_user_rating(match->red_user_id, new_red_rating);
            db_update_user_stats(match->red_user_id, w1, l1, d1);

            db_update_user_rating(match->black_user_id, new_black_rating);
            db_update_user_stats(match->black_user_id, w2, l2, d2);

            printf("[Rating] Draw: Red(%d->%d), Black(%d->%d)\n", r1, new_red_rating, r2, new_black_rating);
        }

        char* moves_json = match_get_moves_json(match);
        char started[32], ended[32];
        sprintf(started, "%ld", match->started_at);
        sprintf(ended, "%ld", time(NULL));
        db_save_match(match_id, match->red_user_id, match->black_user_id, "draw", moves_json, started, ended);
        db_delete_active_match(match_id);
        free(moves_json);

        char payload[512];
        snprintf(payload, sizeof(payload),
                 "{\"match_id\":\"%s\",\"result\":\"draw\",\"red_rating\":%d,"
                 "\"black_rating\":%d}",
                 match_id, new_red_rating, new_black_rating);

        char notify[1024];
        snprintf(notify, sizeof(notify), "{\"type\":\"game_end\",\"payload\":%s}\n", payload);
        broadcast_to_match(server, match_id, notify);

        send_response(server, client, msg->seq, true, "Draw accepted", NULL);
    } else {

        send_response(server, client, msg->seq, true, "Draw declined", NULL);
    }
}

void handle_game_over(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* match_id = json_get_string(msg->payload_json, "match_id");
    const char* result = json_get_string(msg->payload_json, "result");
    const char* reason = json_get_string(msg->payload_json, "reason");

    if (!match_id || !result) {
        send_response(server, client, msg->seq, false, "Missing match_id or result", NULL);
        return;
    }

    match_t* match = match_get(match_id);
    if (!match) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    if (match->active) {
        match_end(match_id, result, reason ? reason : "game_over");

        int new_red_rating = 0;
        int new_black_rating = 0;

        if (match->rated) {
            char u1[64], e1[128];
            int r1, w1, l1, d1;
            char u2[64], e2[128];
            int r2, w2, l2, d2;

            db_get_user_by_id(match->red_user_id, u1, e1, &r1, &w1, &l1, &d1);
            db_get_user_by_id(match->black_user_id, u2, e2, &r2, &w2, &l2, &d2);

            rating_change_t rc = rating_calculate(r1, r2, result, DEFAULT_K_FACTOR);

            new_red_rating = r1 + rc.red_change;
            new_black_rating = r2 + rc.black_change;

            if (strcmp(result, "red_win") == 0) {
                w1++;
                l2++;
            } else if (strcmp(result, "black_win") == 0) {
                l1++;
                w2++;
            } else if (strcmp(result, "draw") == 0) {
                d1++;
                d2++;
            }

            db_update_user_rating(match->red_user_id, new_red_rating);
            db_update_user_stats(match->red_user_id, w1, l1, d1);

            db_update_user_rating(match->black_user_id, new_black_rating);
            db_update_user_stats(match->black_user_id, w2, l2, d2);

            printf("[Rating] Game Over: Red(%d->%d), Black(%d->%d), Reason: %s\n", r1, new_red_rating, r2,
                   new_black_rating, reason);
        }

        char* moves_json = match_get_moves_json(match);
        char started[32], ended[32];
        sprintf(started, "%ld", match->started_at);
        sprintf(ended, "%ld", time(NULL));
        bool save_result =
            db_save_match(match_id, match->red_user_id, match->black_user_id, result, moves_json, started, ended);
        printf("[Handler] db_save_match returned: %s\n", save_result ? "true" : "false");
        db_delete_active_match(match_id);
        free(moves_json);

        char payload[512];
        snprintf(payload, sizeof(payload),
                 "{\"match_id\":\"%s\",\"result\":\"%s\",\"reason\":\"%s\",\"red_"
                 "rating\":%d,\"black_rating\":%d}",
                 match_id, result, reason ? reason : "game_over", new_red_rating, new_black_rating);

        char notify[1024];
        snprintf(notify, sizeof(notify), "{\"type\":\"game_end\",\"payload\":%s}\n", payload);
        broadcast_to_match(server, match_id, notify);

        printf("[Handler] Game over: match %s, result %s\n", match_id, result);
    }

    send_response(server, client, msg->seq, true, "Game ended", NULL);
}
