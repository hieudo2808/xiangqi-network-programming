#include "../include/match.h"

#include "../include/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static match_t matches[MAX_MATCHES];
static int match_count = 0;

static timeout_info_t pending_timeouts[MAX_MATCHES];
static int pending_timeout_count = 0;

bool match_init(void) {
    memset(matches, 0, sizeof(matches));
    memset(pending_timeouts, 0, sizeof(pending_timeouts));
    match_count = 0;
    pending_timeout_count = 0;
    printf("Match manager initialized\n");
    return true;
}

void match_shutdown(void) {
    match_count = 0;
    pending_timeout_count = 0;
}

char* match_create(int red_user_id, int black_user_id, bool rated, int time_ms) {
    if (match_count >= MAX_MATCHES) {
        return NULL;
    }

    match_t* match = NULL;
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (!matches[i].active && matches[i].match_id[0] == '\0') {
            match = &matches[i];
            match_count++;
            break;
        }
    }

    if (!match)
        return NULL;

    sprintf(match->match_id, "match_%d_%ld", match_count, time(NULL));
    match->red_user_id = red_user_id;
    match->black_user_id = black_user_id;
    strcpy(match->current_turn, "red");
    match->move_count = 0;
    match->rated = rated;
    match->red_time_ms = time_ms;
    match->black_time_ms = time_ms;
    match->started_at = time(NULL);
    match->last_move_at = time(NULL);
    match->active = true;
    strcpy(match->result, "ongoing");

    return strdup(match->match_id);
}

match_t* match_get(const char* match_id) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (strcmp(matches[i].match_id, match_id) == 0) {
            return &matches[i];
        }
    }
    return NULL;
}

bool is_valid_position(int row, int col) {
    return (row >= 0 && row <= 9 && col >= 0 && col <= 8);
}

bool is_correct_turn(match_t* match, int user_id) {
    if (strcmp(match->current_turn, "red") == 0) {
        return (user_id == match->red_user_id);
    } else {
        return (user_id == match->black_user_id);
    }
}

bool match_validate_move(const char* match_id, int user_id, int from_row, int from_col, int to_row, int to_col) {
    match_t* match = match_get(match_id);
    if (!match || !match->active)
        return false;

    if (!is_correct_turn(match, user_id))
        return false;

    if (!is_valid_position(from_row, from_col) || !is_valid_position(to_row, to_col)) {
        return false;
    }

    if (from_row == to_row && from_col == to_col)
        return false;

    return true;
}

bool match_add_move(const char* match_id, const move_t* move) {
    match_t* match = match_get(match_id);
    if (!match || !match->active)
        return false;

    if (match->move_count >= MAX_MOVES_PER_MATCH)
        return false;

    match->moves[match->move_count++] = *move;
    match->last_move_at = time(NULL);

    if (strcmp(match->current_turn, "red") == 0) {
        strcpy(match->current_turn, "black");
    } else {
        strcpy(match->current_turn, "red");
    }

    match->red_time_ms = 600000;
    match->black_time_ms = 600000;

    return true;
}

bool match_end(const char* match_id, const char* result, const char* reason) {
    match_t* match = match_get(match_id);
    if (!match)
        return false;

    match->active = false;
    strncpy(match->result, result, 15);
    strncpy(match->end_reason, reason, 31);

    return true;
}

char* match_get_json(const char* match_id) {
    match_t* match = match_get(match_id);
    if (!match)
        return NULL;

    char* json = malloc(65536);
    if (!json)
        return NULL;

    char* ptr = json;

    ptr += sprintf(ptr, "{\"match_id\":\"%s\",", match->match_id);
    ptr += sprintf(ptr, "\"red_user_id\":%d,", match->red_user_id);
    ptr += sprintf(ptr, "\"black_user_id\":%d,", match->black_user_id);
    ptr += sprintf(ptr, "\"red_time_ms\":%d,", match->red_time_ms);
    ptr += sprintf(ptr, "\"black_time_ms\":%d,", match->black_time_ms);
    ptr += sprintf(ptr, "\"result\":\"%s\",", match->result);
    ptr += sprintf(ptr, "\"moves\":[");

    for (int i = 0; i < match->move_count; i++) {
        if (i > 0)
            ptr += sprintf(ptr, ",");
        ptr += sprintf(ptr,
                       "{\"move_id\":%d,\"from\":{\"row\":%d,\"col\":%d},"
                       "\"to\":{\"row\":%d,\"col\":%d}}",
                       match->moves[i].move_id, match->moves[i].from_row, match->moves[i].from_col,
                       match->moves[i].to_row, match->moves[i].to_col);
    }

    sprintf(ptr, "]}");

    return json;
}

match_t* match_find_by_id(const char* match_id) {
    return match_get(match_id);
}

match_t* match_find_by_user(int user_id) {
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].active && (matches[i].red_user_id == user_id || matches[i].black_user_id == user_id)) {
            return &matches[i];
        }
    }
    return NULL;
}

bool match_is_checkmate(match_t* match) {
    (void)match;

    return false;
}

int match_get_opponent_id(const match_t* match, int user_id) {
    if (match->red_user_id == user_id) {
        return match->black_user_id;
    } else {
        return match->red_user_id;
    }
}

char* match_get_moves_json(const match_t* match) {
    if (!match)
        return NULL;

    char* json = malloc(32768);
    if (!json)
        return NULL;

    char* ptr = json;
    ptr += sprintf(ptr, "[");

    for (int i = 0; i < match->move_count; i++) {
        if (i > 0)
            ptr += sprintf(ptr, ",");
        ptr +=
            sprintf(ptr,
                    "{\"from\":{\"row\":%d,\"col\":%d},"
                    "\"to\":{\"row\":%d,\"col\":%d}}",
                    match->moves[i].from_row, match->moves[i].from_col, match->moves[i].to_row, match->moves[i].to_col);
    }

    sprintf(ptr, "]");
    return json;
}

bool match_add_spectator(const char* match_id, int user_id) {
    match_t* match = match_get(match_id);
    if (!match || !match->active)
        return false;

    for (int i = 0; i < match->spectator_count; i++) {
        if (match->spectator_ids[i] == user_id) {
            return true;
        }
    }

    if (match->spectator_count >= MAX_SPECTATORS_PER_MATCH) {
        return false;
    }

    match->spectator_ids[match->spectator_count++] = user_id;
    return true;
}

bool match_remove_spectator(const char* match_id, int user_id) {
    match_t* match = match_get(match_id);
    if (!match)
        return false;

    for (int i = 0; i < match->spectator_count; i++) {
        if (match->spectator_ids[i] == user_id) {

            for (int j = i; j < match->spectator_count - 1; j++) {
                match->spectator_ids[j] = match->spectator_ids[j + 1];
            }
            match->spectator_count--;
            return true;
        }
    }
    return false;
}

bool match_is_spectator(const match_t* match, int user_id) {
    if (!match)
        return false;

    for (int i = 0; i < match->spectator_count; i++) {
        if (match->spectator_ids[i] == user_id) {
            return true;
        }
    }
    return false;
}

char* match_get_live_matches_json(void) {
    char* json = malloc(65536);
    if (!json)
        return NULL;

    char* ptr = json;
    ptr += sprintf(ptr, "[");

    int first = 1;
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].active) {
            if (!first)
                ptr += sprintf(ptr, ",");
            first = 0;

            ptr += sprintf(ptr,
                           "{\"match_id\":\"%s\","
                           "\"red_user_id\":%d,"
                           "\"black_user_id\":%d,"
                           "\"move_count\":%d,"
                           "\"spectator_count\":%d,"
                           "\"current_turn\":\"%s\","
                           "\"started_at\":%ld}",
                           matches[i].match_id, matches[i].red_user_id, matches[i].black_user_id, matches[i].move_count,
                           matches[i].spectator_count, matches[i].current_turn, (long)matches[i].started_at);
        }
    }

    sprintf(ptr, "]");
    return json;
}

bool match_update_timer(const char* match_id) {
    match_t* match = match_get(match_id);
    if (!match || !match->active)
        return false;

    time_t now = time(NULL);
    int elapsed_ms = (int)((now - match->last_move_at) * 1000);

    if (strcmp(match->current_turn, "red") == 0) {
        match->red_time_ms -= elapsed_ms;
        if (match->red_time_ms < 0)
            match->red_time_ms = 0;
    } else {
        match->black_time_ms -= elapsed_ms;
        if (match->black_time_ms < 0)
            match->black_time_ms = 0;
    }

    match->last_move_at = now;
    return true;
}

bool match_check_timeout(const char* match_id) {
    match_t* match = match_get(match_id);
    if (!match || !match->active)
        return false;

    time_t now = time(NULL);
    int elapsed_ms = (int)((now - match->last_move_at) * 1000);

    if (strcmp(match->current_turn, "red") == 0) {
        return (match->red_time_ms - elapsed_ms) <= 0;
    } else {
        return (match->black_time_ms - elapsed_ms) <= 0;
    }
}

char* match_get_timer_json(const char* match_id) {
    match_t* match = match_get(match_id);
    if (!match)
        return NULL;

    time_t now = time(NULL);
    int elapsed_ms = (int)((now - match->last_move_at) * 1000);

    int red_remaining = match->red_time_ms;
    int black_remaining = match->black_time_ms;

    if (match->active) {
        if (strcmp(match->current_turn, "red") == 0) {
            red_remaining -= elapsed_ms;
            if (red_remaining < 0)
                red_remaining = 0;
        } else {
            black_remaining -= elapsed_ms;
            if (black_remaining < 0)
                black_remaining = 0;
        }
    }

    char* json = malloc(256);
    if (!json)
        return NULL;

    sprintf(json,
            "{\"match_id\":\"%s\","
            "\"red_time_ms\":%d,"
            "\"black_time_ms\":%d,"
            "\"current_turn\":\"%s\","
            "\"active\":%s}",
            match->match_id, red_remaining, black_remaining, match->current_turn, match->active ? "true" : "false");

    return json;
}

void match_check_all_timeouts(void) {
    time_t now = time(NULL);

    for (int i = 0; i < MAX_MATCHES; i++) {
        if (matches[i].active) {
            int elapsed_ms = (int)((now - matches[i].last_move_at) * 1000);

            bool timeout = false;
            const char* winner;

            if (strcmp(matches[i].current_turn, "red") == 0) {
                if (matches[i].red_time_ms - elapsed_ms <= 0) {
                    timeout = true;
                    winner = "black_win";
                }
            } else {
                if (matches[i].black_time_ms - elapsed_ms <= 0) {
                    timeout = true;
                    winner = "red_win";
                }
            }

            if (timeout) {

                matches[i].active = false;
                strncpy(matches[i].result, winner, sizeof(matches[i].result) - 1);
                strncpy(matches[i].end_reason, "timeout", sizeof(matches[i].end_reason) - 1);

                if (pending_timeout_count < MAX_MATCHES) {
                    timeout_info_t* ti = &pending_timeouts[pending_timeout_count++];
                    snprintf(ti->match_id, sizeof(ti->match_id), "%s", matches[i].match_id);
                    snprintf(ti->result, sizeof(ti->result), "%s", winner);
                    ti->red_user_id = matches[i].red_user_id;
                    ti->black_user_id = matches[i].black_user_id;
                }

                printf("[Match] Timeout detected: %s -> %s\n", matches[i].match_id, winner);
            }
        }
    }
}

int match_get_pending_timeouts(timeout_info_t* timeouts, int max_count) {
    int count = (pending_timeout_count < max_count) ? pending_timeout_count : max_count;

    for (int i = 0; i < count; i++) {
        timeouts[i] = pending_timeouts[i];
    }

    pending_timeout_count = 0;

    return count;
}

bool match_persist(const char* match_id) {
    match_t* match = match_get(match_id);
    if (!match || !match->active) {
        return false;
    }

    char* moves_json = match_get_moves_json(match);
    if (!moves_json) {
        moves_json = strdup("[]");
    }

    bool result = db_save_active_match(match->match_id, match->red_user_id, match->black_user_id, match->current_turn,
                                       match->red_time_ms, match->black_time_ms, match->move_count, moves_json,
                                       match->rated, match->started_at, match->last_move_at);

    free(moves_json);

    if (result) {
        printf("[Match] Persisted match %s to database\n", match_id);
    } else {
        printf("[Match] Failed to persist match %s\n", match_id);
    }

    return result;
}

bool match_restore_all(void) {
    printf("[Match] Checking for active matches in database...\n");

    int count = db_load_all_active_matches(NULL, 0);
    if (count == 0) {
        printf("[Match] No active matches to restore\n");
        return true;
    }

    printf("[Match] Found %d active matches - restoration requires client "
           "reconnect\n",
           count);

    return true;
}

match_t* match_load_from_db(const char* match_id) {
    if (!match_id)
        return NULL;

    match_t* existing = match_get(match_id);
    if (existing && existing->active) {
        printf("[Match] Match %s already in memory\n", match_id);
        return existing;
    }

    int red_user_id, black_user_id, red_time_ms, black_time_ms, move_count;
    char current_turn[6] = {0};
    char moves_json[60000] = {0};
    bool rated;
    time_t started_at, last_move_at;

    bool loaded =
        db_get_active_match(match_id, &red_user_id, &black_user_id, current_turn, &red_time_ms, &black_time_ms,
                            &move_count, moves_json, sizeof(moves_json), &rated, &started_at, &last_move_at);

    if (!loaded) {
        printf("[Match] Match %s not found in database\n", match_id);
        return NULL;
    }

    match_t* match = NULL;
    for (int i = 0; i < MAX_MATCHES; i++) {
        if (!matches[i].active && matches[i].match_id[0] == '\0') {
            match = &matches[i];
            match_count++;
            break;
        }
    }

    if (!match) {
        printf("[Match] No empty slot available to load match\n");
        return NULL;
    }

    strncpy(match->match_id, match_id, sizeof(match->match_id) - 1);
    match->match_id[sizeof(match->match_id) - 1] = '\0';
    match->red_user_id = red_user_id;
    match->black_user_id = black_user_id;
    snprintf(match->current_turn, sizeof(match->current_turn), "%s", current_turn);
    match->red_time_ms = red_time_ms;
    match->black_time_ms = black_time_ms;
    match->move_count = move_count;
    match->rated = rated;
    match->started_at = started_at;
    match->last_move_at = last_move_at;
    match->active = true;
    strcpy(match->result, "ongoing");
    match->spectator_count = 0;

    printf("[Match] Loaded match %s from DB (red=%d, black=%d, moves=%d)\n", match_id, red_user_id, black_user_id,
           move_count);

    return match;
}
