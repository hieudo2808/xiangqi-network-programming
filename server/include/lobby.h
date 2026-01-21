#ifndef LOBBY_H
#define LOBBY_H

#include <stdbool.h>
#include <time.h>

#include "account.h"

#define MAX_READY_PLAYERS 100
#define MAX_ROOMS 50
#define MAX_CHALLENGES 100

typedef struct {
    int user_id;
    char username[64];
    int rating;
    bool ready;
    time_t ready_since;
} lobby_player_t;

typedef struct {
    char room_id[32];
    char room_code[16];
    int host_user_id;
    int guest_user_id;
    char password[64];
    bool rated;
    bool occupied;
    time_t created_at;
} room_t;

typedef struct {
    char challenge_id[32];
    int from_user_id;
    int to_user_id;
    bool rated;
    int status;
    time_t created_at;
    time_t expires_at;
} challenge_t;

bool lobby_init(void);
void lobby_shutdown(void);

void lobby_set_ready(int user_id, const char* username, int rating, bool ready);
void lobby_remove_player(int user_id);
char* lobby_get_ready_list_json(void);

bool lobby_find_random_match(int user_id, int* out_opponent_id);
bool lobby_find_rated_match(int user_id, int rating, int tolerance, int* out_opponent_id);

char* lobby_create_room(int host_user_id, const char* room_name, const char* password, bool rated);
bool lobby_join_room(const char* room_code, const char* password, int user_id, int* out_host_id);
bool lobby_close_room(const char* room_code, int user_id);
bool lobby_leave_room(const char* room_code, int user_id);
room_t* lobby_get_room(const char* room_code);
char* lobby_get_rooms_json(void);

char* lobby_create_challenge(int from_user_id, int to_user_id, bool rated);
challenge_t* lobby_get_challenge(const char* challenge_id);
bool lobby_accept_challenge(const char* challenge_id, int user_id);
bool lobby_decline_challenge(const char* challenge_id, int user_id);
void lobby_cleanup_expired_challenges(void);

int lobby_get_ready_users(int* user_ids, int max_count);
void lobby_cleanup_rooms_for_user(int user_id);

#endif
