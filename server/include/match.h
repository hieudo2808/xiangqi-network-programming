#ifndef MATCH_H
#define MATCH_H

#include <stdbool.h>
#include <time.h>

#define MAX_MATCHES 500
#define MAX_MOVES_PER_MATCH 300
#define MAX_SPECTATORS_PER_MATCH 50

typedef struct {
    int move_id;
    char from_row;
    char from_col;
    char to_row;
    char to_col;
    char piece[16];
    char capture[16];
    char notation[32];
    time_t timestamp;
    int red_time_ms;
    int black_time_ms;
} move_t;

typedef struct {
    char match_id[32];
    int red_user_id;
    int black_user_id;
    char current_turn[6];
    int move_count;
    move_t moves[MAX_MOVES_PER_MATCH];
    bool rated;
    int red_time_ms;
    int black_time_ms;
    time_t started_at;
    time_t last_move_at;
    bool active;
    char result[16];
    char end_reason[32];

    int spectator_ids[MAX_SPECTATORS_PER_MATCH];
    int spectator_count;
} match_t;

bool match_init(void);
void match_shutdown(void);

char* match_create(int red_user_id, int black_user_id, bool rated, int time_ms);
match_t* match_get(const char* match_id);
match_t* match_find_by_id(const char* match_id);
match_t* match_find_by_user(int user_id);
bool match_validate_move(const char* match_id, int user_id, int from_row, int from_col, int to_row, int to_col);
bool match_add_move(const char* match_id, const move_t* move);
bool match_end(const char* match_id, const char* result, const char* reason);
char* match_get_json(const char* match_id);
char* match_get_moves_json(const match_t* match);
int match_get_opponent_id(const match_t* match, int user_id);
bool match_is_checkmate(match_t* match);

bool is_valid_position(int row, int col);
bool is_correct_turn(match_t* match, int user_id);

bool match_add_spectator(const char* match_id, int user_id);
bool match_remove_spectator(const char* match_id, int user_id);
bool match_is_spectator(const match_t* match, int user_id);
char* match_get_live_matches_json(void);

bool match_update_timer(const char* match_id);
bool match_check_timeout(const char* match_id);
char* match_get_timer_json(const char* match_id);
void match_check_all_timeouts(void);

typedef struct {
    char match_id[32];
    char result[16];
    int red_user_id;
    int black_user_id;
} timeout_info_t;

int match_get_pending_timeouts(timeout_info_t* timeouts, int max_count);

bool match_add_spectator(const char* match_id, int user_id);
bool match_remove_spectator(const char* match_id, int user_id);
int match_get_spectator_count(const char* match_id);

bool match_persist(const char* match_id);
bool match_restore_all(void);
match_t* match_load_from_db(const char* match_id);

#endif
