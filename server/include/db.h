#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include <sql.h>
#include <sqlext.h>

extern SQLHENV g_db_env;
extern SQLHDBC g_db_conn;
extern SQLHSTMT g_db_stmt;

#define DB_PREPARE(stmt, sql)                                                                                          \
    SQLHSTMT stmt;                                                                                                     \
    SQLRETURN db_ret;                                                                                                  \
    SQLLEN db_indicator;                                                                                               \
    do {                                                                                                               \
        db_ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &(stmt));                                                  \
        if (db_ret != SQL_SUCCESS)                                                                                     \
            return false;                                                                                              \
        db_ret = SQLPrepare((stmt), (SQLCHAR*)(sql), SQL_NTS);                                                         \
        if (db_ret != SQL_SUCCESS) {                                                                                   \
            SQLFreeHandle(SQL_HANDLE_STMT, (stmt));                                                                    \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#define DB_EXECUTE(stmt)                                                                                               \
    do {                                                                                                               \
        db_ret = SQLExecute(stmt);                                                                                     \
        if (db_ret != SQL_SUCCESS && db_ret != SQL_SUCCESS_WITH_INFO) {                                                \
            SQLFreeHandle(SQL_HANDLE_STMT, (stmt));                                                                    \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#define DB_EXECUTE_OR_FAIL(stmt, cleanup_stmt)                                                                         \
    do {                                                                                                               \
        db_ret = SQLExecute(stmt);                                                                                     \
        if (db_ret != SQL_SUCCESS && db_ret != SQL_SUCCESS_WITH_INFO) {                                                \
            SQLFreeHandle(SQL_HANDLE_STMT, (cleanup_stmt));                                                            \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#define DB_CLEANUP(stmt) SQLFreeHandle(SQL_HANDLE_STMT, (stmt))

bool db_init(const char* connection_string);
void db_shutdown(void);

bool db_create_user(const char* username, const char* email, const char* password_hash, int* out_user_id);
bool db_get_user_by_username(const char* username, int* out_user_id, char* out_password_hash, int* out_rating);
bool db_get_user_by_id(int user_id, char* out_username, char* out_email, int* out_rating, int* out_wins,
                       int* out_losses, int* out_draws);
bool db_update_user_rating(int user_id, int new_rating);
bool db_update_user_stats(int user_id, int wins, int losses, int draws);

bool db_save_match(const char* match_id, int red_user_id, int black_user_id, const char* result, const char* moves_json,
                   const char* started_at, const char* ended_at);
bool db_get_match(const char* match_id, char* out_json, size_t json_size);
bool db_get_match_history(int user_id, int limit, int offset, char* out_json, size_t json_size);

bool db_get_user_profile(int user_id, char* out_json, size_t json_size);

bool db_get_leaderboard(int limit, int offset, char* out_json, size_t json_size);

bool db_execute(const char* sql);
bool db_check_username_exists(const char* username);
bool db_check_email_exists(const char* email);
bool db_get_username(int user_id, char* out_username, size_t username_size);

bool db_save_active_match(const char* match_id, int red_user_id, int black_user_id, const char* current_turn,
                          int red_time_ms, int black_time_ms, int move_count, const char* moves_json, bool rated,
                          time_t started_at, time_t last_move_at);
bool db_get_active_match(const char* match_id, int* red_user_id, int* black_user_id, char* current_turn,
                         int* red_time_ms, int* black_time_ms, int* move_count, char* moves_json,
                         size_t moves_json_size, bool* rated, time_t* started_at, time_t* last_move_at);
bool db_delete_active_match(const char* match_id);
int db_load_all_active_matches(void* matches_array, int max_matches);

/* Session management - persisted in DB */
bool db_session_create(const char* token, int user_id, int expires_hours);
bool db_session_validate(const char* token, int* out_user_id);
bool db_session_destroy(const char* token);
bool db_session_cleanup_expired(void);

void db_print_error(SQLHANDLE handle, SQLSMALLINT type, const char* msg);

#endif
