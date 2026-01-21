#include "../include/lobby.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/db.h"

static lobby_player_t ready_players[MAX_READY_PLAYERS];
static int ready_count = 0;

static room_t rooms[MAX_ROOMS];
static challenge_t challenges[MAX_CHALLENGES];

bool lobby_init(void) {
    memset(ready_players, 0, sizeof(ready_players));
    memset(rooms, 0, sizeof(rooms));
    memset(challenges, 0, sizeof(challenges));
    ready_count = 0;
    printf("Lobby initialized\n");
    return true;
}

void lobby_shutdown(void) {
    ready_count = 0;
}

void lobby_set_ready(int user_id, const char* username, int rating, bool ready) {
    if (ready) {

        for (int i = 0; i < ready_count; i++) {
            if (ready_players[i].user_id == user_id) {

                ready_players[i].rating = rating;
                ready_players[i].ready_since = time(NULL);
                printf("[Lobby] Updated ready player: %s (ID: %d)\n", username, user_id);
                return;
            }
        }

        if (ready_count < MAX_READY_PLAYERS) {
            ready_players[ready_count].user_id = user_id;
            strncpy(ready_players[ready_count].username, username, 63);
            ready_players[ready_count].rating = rating;
            ready_players[ready_count].ready = true;
            ready_players[ready_count].ready_since = time(NULL);
            ready_count++;
            printf("[Lobby] Added ready player: %s (ID: %d). Ready count=%d\n", username, user_id, ready_count);
        } else {
            printf("[Lobby] Ready list full, cannot add: %s (ID: %d)\n", username, user_id);
        }
    } else {

        lobby_remove_player(user_id);
    }
}

void lobby_remove_player(int user_id) {
    for (int i = 0; i < ready_count; i++) {
        if (ready_players[i].user_id == user_id) {

            for (int j = i; j < ready_count - 1; j++) {
                ready_players[j] = ready_players[j + 1];
            }
            ready_count--;
            break;
        }
    }
}

char* lobby_get_ready_list_json(void) {
    char* json = malloc(8192);
    if (!json)
        return NULL;

    char* ptr = json;
    ptr += sprintf(ptr, "[");

    for (int i = 0; i < ready_count; i++) {
        if (i > 0)
            ptr += sprintf(ptr, ",");
        ptr += sprintf(ptr, "{\"user_id\":%d,\"username\":\"%s\",\"rating\":%d}", ready_players[i].user_id,
                       ready_players[i].username, ready_players[i].rating);
    }

    sprintf(ptr, "]");
    return json;
}

bool lobby_find_random_match(int user_id, int* out_opponent_id) {

    for (int i = 0; i < ready_count; i++) {
        if (ready_players[i].user_id != user_id) {
            *out_opponent_id = ready_players[i].user_id;

            lobby_remove_player(user_id);
            lobby_remove_player(*out_opponent_id);
            return true;
        }
    }
    return false;
}

bool lobby_find_rated_match(int user_id, int rating, int tolerance, int* out_opponent_id) {
    int best_opponent = -1;
    int best_diff = tolerance + 1;

    for (int i = 0; i < ready_count; i++) {
        if (ready_players[i].user_id != user_id) {
            int diff = abs(ready_players[i].rating - rating);
            if (diff <= tolerance && diff < best_diff) {
                best_diff = diff;
                best_opponent = ready_players[i].user_id;
            }
        }
    }

    if (best_opponent != -1) {
        *out_opponent_id = best_opponent;
        lobby_remove_player(user_id);
        lobby_remove_player(best_opponent);
        return true;
    }

    return false;
}

char* lobby_create_room(int host_user_id, const char* room_name, const char* password, bool rated) {
    (void)room_name;

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].occupied) {
            sprintf(rooms[i].room_id, "room_%d_%ld", i, time(NULL));
            sprintf(rooms[i].room_code, "%04X%04X", rand() % 0xFFFF, rand() % 0xFFFF);
            rooms[i].host_user_id = host_user_id;
            if (password)
                strncpy(rooms[i].password, password, 63);
            rooms[i].rated = rated;
            rooms[i].occupied = true;
            rooms[i].created_at = time(NULL);

            return strdup(rooms[i].room_code);
        }
    }
    return NULL;
}

void lobby_cleanup_expired_challenges(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].challenge_id[0] != '\0' && now > challenges[i].expires_at) {
            memset(&challenges[i], 0, sizeof(challenge_t));
        }
    }
}

bool lobby_join_room(const char* room_code, const char* password, int user_id, int* out_host_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].occupied && strcmp(rooms[i].room_code, room_code) == 0) {

            if (rooms[i].password[0] != '\0') {
                if (!password || strcmp(rooms[i].password, password) != 0) {
                    return false;
                }
            }

            if (rooms[i].guest_user_id != 0) {
                return false;
            }

            rooms[i].guest_user_id = user_id;
            if (out_host_id) {
                *out_host_id = rooms[i].host_user_id;
            }
            return true;
        }
    }
    return false;
}

bool lobby_close_room(const char* room_code, int user_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].occupied && strcmp(rooms[i].room_code, room_code) == 0) {

            if (rooms[i].host_user_id != user_id) {
                return false;
            }

            memset(&rooms[i], 0, sizeof(room_t));
            return true;
        }
    }
    return false;
}

room_t* lobby_get_room(const char* room_code) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].occupied && strcmp(rooms[i].room_code, room_code) == 0) {
            return &rooms[i];
        }
    }
    return NULL;
}

char* lobby_get_rooms_json(void) {
    char* json = malloc(16384);
    if (!json)
        return NULL;

    char* ptr = json;
    ptr += sprintf(ptr, "[");

    int first = 1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].occupied) {
            if (!first)
                ptr += sprintf(ptr, ",");
            first = 0;

            char host_username[64] = "Unknown";
            db_get_user_by_id(rooms[i].host_user_id, host_username, NULL, NULL, NULL, NULL, NULL);

            ptr += sprintf(ptr,
                           "{\"room_code\":\"%s\",\"host_id\":%d,\"host_name\":\"%s\","
                           "\"has_password\":%s,\"rated\":%s,\"has_guest\":%s}",
                           rooms[i].room_code, rooms[i].host_user_id, host_username,
                           rooms[i].password[0] != '\0' ? "true" : "false", rooms[i].rated ? "true" : "false",
                           rooms[i].guest_user_id != 0 ? "true" : "false");
        }
    }

    sprintf(ptr, "]");
    return json;
}

bool lobby_leave_room(const char* room_code, int user_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].occupied && strcmp(rooms[i].room_code, room_code) == 0) {

            if (rooms[i].guest_user_id == user_id) {
                rooms[i].guest_user_id = 0;
                return true;
            }

            if (rooms[i].host_user_id == user_id) {
                memset(&rooms[i], 0, sizeof(room_t));
                return true;
            }
            return false;
        }
    }
    return false;
}

char* lobby_create_challenge(int from_user_id, int to_user_id, bool rated) {

    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (challenges[i].challenge_id[0] == '\0') {
            sprintf(challenges[i].challenge_id, "ch_%d_%ld", i, time(NULL));
            challenges[i].from_user_id = from_user_id;
            challenges[i].to_user_id = to_user_id;
            challenges[i].rated = rated;
            challenges[i].status = 0;
            challenges[i].created_at = time(NULL);
            challenges[i].expires_at = time(NULL) + 60;

            return strdup(challenges[i].challenge_id);
        }
    }
    return NULL;
}

challenge_t* lobby_get_challenge(const char* challenge_id) {
    for (int i = 0; i < MAX_CHALLENGES; i++) {
        if (strcmp(challenges[i].challenge_id, challenge_id) == 0) {
            return &challenges[i];
        }
    }
    return NULL;
}

bool lobby_accept_challenge(const char* challenge_id, int user_id) {
    challenge_t* ch = lobby_get_challenge(challenge_id);
    if (!ch) {
        return false;
    }

    if (ch->to_user_id != user_id) {
        return false;
    }

    if (time(NULL) > ch->expires_at) {
        return false;
    }

    ch->status = 1;
    return true;
}

bool lobby_decline_challenge(const char* challenge_id, int user_id) {
    challenge_t* ch = lobby_get_challenge(challenge_id);
    if (!ch) {
        return false;
    }

    if (ch->to_user_id != user_id) {
        return false;
    }

    ch->status = 2;
    memset(ch, 0, sizeof(challenge_t));
    return true;
}

int lobby_get_ready_users(int* user_ids, int max_count) {
    int count = 0;
    for (int i = 0; i < ready_count && count < max_count; i++) {
        user_ids[count++] = ready_players[i].user_id;
    }
    return count;
}

void lobby_cleanup_rooms_for_user(int user_id) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].occupied)
            continue;

        if (rooms[i].host_user_id == user_id) {
            printf("[Lobby] Cleaning up room %s (host %d disconnected)\n", rooms[i].room_code, user_id);
            memset(&rooms[i], 0, sizeof(room_t));
        } else if (rooms[i].guest_user_id == user_id) {
            printf("[Lobby] Removing guest %d from room %s\n", user_id, rooms[i].room_code);
            rooms[i].guest_user_id = 0;
        }
    }
}
