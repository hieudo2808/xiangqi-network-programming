#include "../include/db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SQLHENV g_db_env = NULL;
SQLHDBC g_db_conn = NULL;
SQLHSTMT g_db_stmt = NULL;

void db_print_error(SQLHANDLE handle, SQLSMALLINT type, const char* msg) {
    SQLCHAR sql_state[6];
    SQLCHAR error_msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native_error;
    SQLSMALLINT msg_len;

    if (msg) {
        fprintf(stderr, "[DB Error] %s\n", msg);
    }

    SQLGetDiagRec(type, handle, 1, sql_state, &native_error, error_msg, sizeof(error_msg), &msg_len);

    fprintf(stderr, "[SQL Server] State: %s, Error: %d, Message: %s\n", sql_state, (int)native_error, error_msg);
}

bool db_init(const char* connection_string) {
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &g_db_env);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        fprintf(stderr, "Failed to allocate environment handle\n");
        return false;
    }

    ret = SQLSetEnvAttr(g_db_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(g_db_env, SQL_HANDLE_ENV, "Failed to set ODBC version");
        SQLFreeHandle(SQL_HANDLE_ENV, g_db_env);
        return false;
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, g_db_env, &g_db_conn);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(g_db_env, SQL_HANDLE_ENV, "Failed to allocate connection handle");
        SQLFreeHandle(SQL_HANDLE_ENV, g_db_env);
        return false;
    }

    ret = SQLDriverConnect(g_db_conn, NULL, (SQLCHAR*)connection_string, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(g_db_conn, SQL_HANDLE_DBC, "Failed to connect to database");
        SQLFreeHandle(SQL_HANDLE_DBC, g_db_conn);
        SQLFreeHandle(SQL_HANDLE_ENV, g_db_env);
        return false;
    }

    printf("[DB] Connected to SQL Server successfully\n");

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &g_db_stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(g_db_conn, SQL_HANDLE_DBC, "Failed to allocate statement handle");
        db_shutdown();
        return false;
    }

    return true;
}

void db_shutdown(void) {
    if (g_db_stmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, g_db_stmt);
        g_db_stmt = NULL;
    }

    if (g_db_conn) {
        SQLDisconnect(g_db_conn);
        SQLFreeHandle(SQL_HANDLE_DBC, g_db_conn);
        g_db_conn = NULL;
    }

    if (g_db_env) {
        SQLFreeHandle(SQL_HANDLE_ENV, g_db_env);
        g_db_env = NULL;
    }

    printf("[DB] Disconnected from SQL Server\n");
}

bool db_execute(const char* sql) {
    SQLHSTMT stmt;
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(g_db_conn, SQL_HANDLE_DBC, "Failed to allocate statement");
        return false;
    }

    ret = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(stmt, SQL_HANDLE_STMT, sql);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool db_create_user(const char* username, const char* email, const char* password_hash, int* out_user_id) {
    const char* sql = "INSERT INTO Users (username, email, password_hash, rating, wins, "
                      "losses, draws) "
                      "VALUES (?, ?, ?, 1200, 0, 0, 0); SELECT SCOPE_IDENTITY();";
    int user_id;

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)username, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 128, 0, (SQLCHAR*)email, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 128, 0, (SQLCHAR*)password_hash, 0, NULL);

    db_ret = SQLExecute(stmt);
    if (db_ret != SQL_SUCCESS && db_ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(stmt, SQL_HANDLE_STMT, "Failed to execute INSERT");
        DB_CLEANUP(stmt);
        return false;
    }

    SQLMoreResults(stmt);
    db_ret = SQLFetch(stmt);
    if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(stmt, 1, SQL_C_SLONG, &user_id, 0, &db_indicator);
        if (out_user_id)
            *out_user_id = user_id;
    }

    DB_CLEANUP(stmt);
    return true;
}

bool db_get_user_by_username(const char* username, int* out_user_id, char* out_password_hash, int* out_rating) {
    const char* sql = "SELECT user_id, password_hash, rating FROM Users WHERE username = ?";
    int user_id, rating;
    char password_hash[128];

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)username, 0, NULL);
    DB_EXECUTE(stmt);

    db_ret = SQLFetch(stmt);
    if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(stmt, 1, SQL_C_SLONG, &user_id, 0, &db_indicator);
        SQLGetData(stmt, 2, SQL_C_CHAR, password_hash, sizeof(password_hash), &db_indicator);
        SQLGetData(stmt, 3, SQL_C_SLONG, &rating, 0, &db_indicator);

        if (out_user_id)
            *out_user_id = user_id;
        if (out_password_hash)
            strcpy(out_password_hash, password_hash);
        if (out_rating)
            *out_rating = rating;

        DB_CLEANUP(stmt);
        return true;
    }

    DB_CLEANUP(stmt);
    return false;
}

bool db_get_user_by_id(int user_id, char* out_username, char* out_email, int* out_rating, int* out_wins,
                       int* out_losses, int* out_draws) {
    const char* sql = "SELECT username, email, rating, wins, losses, draws FROM Users WHERE user_id = ?";
    char username[64], email[128];
    int rating, wins, losses, draws;

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);
    DB_EXECUTE(stmt);

    db_ret = SQLFetch(stmt);
    if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(stmt, 1, SQL_C_CHAR, username, sizeof(username), &db_indicator);
        SQLGetData(stmt, 2, SQL_C_CHAR, email, sizeof(email), &db_indicator);
        SQLGetData(stmt, 3, SQL_C_SLONG, &rating, 0, &db_indicator);
        SQLGetData(stmt, 4, SQL_C_SLONG, &wins, 0, &db_indicator);
        SQLGetData(stmt, 5, SQL_C_SLONG, &losses, 0, &db_indicator);
        SQLGetData(stmt, 6, SQL_C_SLONG, &draws, 0, &db_indicator);

        if (out_username)
            strcpy(out_username, username);
        if (out_email)
            strcpy(out_email, email);
        if (out_rating)
            *out_rating = rating;
        if (out_wins)
            *out_wins = wins;
        if (out_losses)
            *out_losses = losses;
        if (out_draws)
            *out_draws = draws;

        DB_CLEANUP(stmt);
        return true;
    }

    DB_CLEANUP(stmt);
    return false;
}

bool db_update_user_rating(int user_id, int new_rating) {
    const char* sql = "UPDATE Users SET rating = ? WHERE user_id = ?";

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &new_rating, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);

    db_ret = SQLExecute(stmt);
    bool success = (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO);
    DB_CLEANUP(stmt);
    return success;
}

bool db_update_user_stats(int user_id, int wins, int losses, int draws) {
    const char* sql = "UPDATE Users SET wins = ?, losses = ?, draws = ? WHERE user_id = ?";

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &wins, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &losses, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &draws, 0, NULL);
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);

    db_ret = SQLExecute(stmt);
    bool success = (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO);
    DB_CLEANUP(stmt);
    return success;
}

bool db_save_match(const char* match_id, int red_user_id, int black_user_id, const char* result, const char* moves_json,
                   const char* started_at, const char* ended_at) {
    const char* sql = "INSERT INTO Matches (match_id, red_user_id, black_user_id, result, "
                      "moves_json, started_at, ended_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)match_id, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &red_user_id, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &black_user_id, 0, NULL);
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 16, 0, (SQLCHAR*)result, 0, NULL);
    SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 8000, 0, (SQLCHAR*)moves_json, 0, NULL);
    SQLBindParameter(stmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 32, 0, (SQLCHAR*)started_at, 0, NULL);
    SQLBindParameter(stmt, 7, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 32, 0, (SQLCHAR*)ended_at, 0, NULL);

    db_ret = SQLExecute(stmt);
    bool success = (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO);
    DB_CLEANUP(stmt);
    return success;
}

bool db_get_match(const char* match_id, char* out_json, size_t json_size) {
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLLEN indicator;
    char red_username[64], black_username[64], result[16], moves[8000], started[32], ended[32];

    const char* sql = "SELECT m.result, m.moves_json, m.started_at, m.ended_at, "
                      "u1.username as red_name, u2.username as black_name "
                      "FROM Matches m "
                      "JOIN Users u1 ON m.red_user_id = u1.user_id "
                      "JOIN Users u2 ON m.black_user_id = u2.user_id "
                      "WHERE m.match_id = ?";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS) {
        return false;
    }

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)match_id, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    ret = SQLFetch(stmt);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(stmt, 1, SQL_C_CHAR, result, sizeof(result), &indicator);
        SQLGetData(stmt, 2, SQL_C_CHAR, moves, sizeof(moves), &indicator);
        SQLGetData(stmt, 3, SQL_C_CHAR, started, sizeof(started), &indicator);
        SQLGetData(stmt, 4, SQL_C_CHAR, ended, sizeof(ended), &indicator);
        SQLGetData(stmt, 5, SQL_C_CHAR, red_username, sizeof(red_username), &indicator);
        SQLGetData(stmt, 6, SQL_C_CHAR, black_username, sizeof(black_username), &indicator);

        snprintf(out_json, json_size,
                 "{\"match_id\":\"%s\",\"red_user\":\"%s\",\"black_user\":\"%s\","
                 "\"result\":\"%s\",\"moves\":%s,\"started_at\":\"%s\",\"ended_at\":"
                 "\"%s\"}",
                 match_id, red_username, black_username, result, moves, started, ended);

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return true;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return false;
}

bool db_get_leaderboard(int limit, int offset, char* out_json, size_t json_size) {
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLLEN indicator;
    char username[64];
    int rating, wins, losses, draws;
    char buffer[512];

    const char* sql = "SELECT username, rating, wins, losses, draws FROM Users "
                      "ORDER BY rating DESC OFFSET ? ROWS FETCH NEXT ? ROWS ONLY";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS) {
        return false;
    }

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &offset, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &limit, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    strcpy(out_json, "[");
    bool first = true;

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_CHAR, username, sizeof(username), &indicator);
        SQLGetData(stmt, 2, SQL_C_SLONG, &rating, 0, &indicator);
        SQLGetData(stmt, 3, SQL_C_SLONG, &wins, 0, &indicator);
        SQLGetData(stmt, 4, SQL_C_SLONG, &losses, 0, &indicator);
        SQLGetData(stmt, 5, SQL_C_SLONG, &draws, 0, &indicator);

        snprintf(buffer, sizeof(buffer),
                 "%s{\"username\":\"%s\",\"rating\":%d,\"wins\":%d,\"losses\":%"
                 "d,\"draws\":%d}",
                 first ? "" : ",", username, rating, wins, losses, draws);

        if (strlen(out_json) + strlen(buffer) + 2 < json_size) {
            strcat(out_json, buffer);
            first = false;
        }
    }

    strcat(out_json, "]");

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool db_get_match_history(int user_id, int limit, int offset, char* out_json, size_t json_size) {
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLLEN indicator;
    char match_id[64], result[16], started[32], ended[32];
    char red_username[64], black_username[64];
    int red_user_id, black_user_id;
    char buffer[512];

    const char* sql = "SELECT m.match_id, m.red_user_id, m.black_user_id, m.result, "
                      "m.started_at, m.ended_at, "
                      "u1.username as red_name, u2.username as black_name "
                      "FROM Matches m "
                      "JOIN Users u1 ON m.red_user_id = u1.user_id "
                      "JOIN Users u2 ON m.black_user_id = u2.user_id "
                      "WHERE m.red_user_id = ? OR m.black_user_id = ? "
                      "ORDER BY m.ended_at DESC "
                      "OFFSET ? ROWS FETCH NEXT ? ROWS ONLY";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS) {
        return false;
    }

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &offset, 0, NULL);
    SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &limit, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    strcpy(out_json, "[");
    bool first = true;

    while (SQLFetch(stmt) == SQL_SUCCESS) {
        SQLGetData(stmt, 1, SQL_C_CHAR, match_id, sizeof(match_id), &indicator);
        SQLGetData(stmt, 2, SQL_C_SLONG, &red_user_id, 0, &indicator);
        SQLGetData(stmt, 3, SQL_C_SLONG, &black_user_id, 0, &indicator);
        SQLGetData(stmt, 4, SQL_C_CHAR, result, sizeof(result), &indicator);
        SQLGetData(stmt, 5, SQL_C_CHAR, started, sizeof(started), &indicator);
        SQLGetData(stmt, 6, SQL_C_CHAR, ended, sizeof(ended), &indicator);
        SQLGetData(stmt, 7, SQL_C_CHAR, red_username, sizeof(red_username), &indicator);
        SQLGetData(stmt, 8, SQL_C_CHAR, black_username, sizeof(black_username), &indicator);

        const char* user_result = "unknown";
        if (strcmp(result, "red_win") == 0) {
            user_result = (user_id == red_user_id) ? "win" : "loss";
        } else if (strcmp(result, "black_win") == 0) {
            user_result = (user_id == black_user_id) ? "win" : "loss";
        } else if (strcmp(result, "draw") == 0) {
            user_result = "draw";
        }

        const char* opponent = (user_id == red_user_id) ? black_username : red_username;
        const char* my_color = (user_id == red_user_id) ? "red" : "black";

        snprintf(buffer, sizeof(buffer),
                 "%s{\"match_id\":\"%s\",\"opponent\":\"%s\",\"my_color\":\"%s\","
                 "\"result\":\"%s\",\"started_at\":\"%s\",\"ended_at\":\"%s\"}",
                 first ? "" : ",", match_id, opponent, my_color, user_result, started, ended);

        if (strlen(out_json) + strlen(buffer) + 2 < json_size) {
            strcat(out_json, buffer);
            first = false;
        }
    }

    strcat(out_json, "]");

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

bool db_get_user_profile(int user_id, char* out_json, size_t json_size) {
    SQLHSTMT stmt;
    SQLRETURN ret;
    SQLLEN indicator;
    char username[64], email[128], created_at[32];
    int rating, wins, losses, draws;
    int total_matches;

    const char* sql = "SELECT username, email, rating, wins, losses, draws, created_at "
                      "FROM Users WHERE user_id = ?";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS) {
        return false;
    }

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    if (SQLFetch(stmt) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLGetData(stmt, 1, SQL_C_CHAR, username, sizeof(username), &indicator);
    SQLGetData(stmt, 2, SQL_C_CHAR, email, sizeof(email), &indicator);
    SQLGetData(stmt, 3, SQL_C_SLONG, &rating, 0, &indicator);
    SQLGetData(stmt, 4, SQL_C_SLONG, &wins, 0, &indicator);
    SQLGetData(stmt, 5, SQL_C_SLONG, &losses, 0, &indicator);
    SQLGetData(stmt, 6, SQL_C_SLONG, &draws, 0, &indicator);
    SQLGetData(stmt, 7, SQL_C_CHAR, created_at, sizeof(created_at), &indicator);

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    total_matches = wins + losses + draws;

    double win_rate = 0.0;
    if (total_matches > 0) {
        win_rate = (double)wins / total_matches * 100.0;
    }

    const char* rank_title;
    if (rating >= 2400)
        rank_title = "Đại Kiện Tướng";
    else if (rating >= 2200)
        rank_title = "Kiện Tướng Quốc Tế";
    else if (rating >= 2000)
        rank_title = "Kiện Tướng";
    else if (rating >= 1800)
        rank_title = "Cao Thủ";
    else if (rating >= 1600)
        rank_title = "Chuyên Gia";
    else if (rating >= 1400)
        rank_title = "Thành Thạo";
    else if (rating >= 1200)
        rank_title = "Nghiệp Dư";
    else
        rank_title = "Tân Thủ";

    snprintf(out_json, json_size,
             "{"
             "\"user_id\":%d,"
             "\"username\":\"%s\","
             "\"email\":\"%s\","
             "\"rating\":%d,"
             "\"rank_title\":\"%s\","
             "\"wins\":%d,"
             "\"losses\":%d,"
             "\"draws\":%d,"
             "\"total_matches\":%d,"
             "\"win_rate\":%.1f,"
             "\"created_at\":\"%s\""
             "}",
             user_id, username, email, rating, rank_title, wins, losses, draws, total_matches, win_rate, created_at);

    return true;
}

bool db_check_username_exists(const char* username) {
    const char* sql = "SELECT COUNT(*) FROM Users WHERE username = ?";
    int count = 0;

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)username, 0, NULL);

    db_ret = SQLExecute(stmt);
    if (db_ret == SQL_SUCCESS) {
        SQLFetch(stmt);
        SQLGetData(stmt, 1, SQL_C_SLONG, &count, 0, &db_indicator);
    }

    DB_CLEANUP(stmt);
    return count > 0;
}

bool db_check_email_exists(const char* email) {
    const char* sql = "SELECT COUNT(*) FROM Users WHERE email = ?";
    int count = 0;

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 128, 0, (SQLCHAR*)email, 0, NULL);

    db_ret = SQLExecute(stmt);
    if (db_ret == SQL_SUCCESS) {
        SQLFetch(stmt);
        SQLGetData(stmt, 1, SQL_C_SLONG, &count, 0, &db_indicator);
    }

    DB_CLEANUP(stmt);
    return count > 0;
}

bool db_get_username(int user_id, char* out_username, size_t username_size) {
    const char* sql = "SELECT username FROM Users WHERE user_id = ?";
    char username[64] = {0};

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);

    db_ret = SQLExecute(stmt);
    if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
        db_ret = SQLFetch(stmt);
        if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
            SQLGetData(stmt, 1, SQL_C_CHAR, username, sizeof(username), &db_indicator);
            strncpy(out_username, username, username_size - 1);
            out_username[username_size - 1] = '\0';
            DB_CLEANUP(stmt);
            return true;
        }
    }

    DB_CLEANUP(stmt);
    return false;
}

bool db_save_active_match(const char* match_id, int red_user_id, int black_user_id, const char* current_turn,
                          int red_time_ms, int black_time_ms, int move_count, const char* moves_json, bool rated,
                          time_t started_at, time_t last_move_at) {
    SQLHSTMT stmt;
    SQLRETURN ret;

    char started_str[32], last_move_str[32];
    strftime(started_str, sizeof(started_str), "%Y-%m-%d %H:%M:%S", localtime(&started_at));
    strftime(last_move_str, sizeof(last_move_str), "%Y-%m-%d %H:%M:%S", localtime(&last_move_at));

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret == SQL_SUCCESS) {
        char delete_sql[256];
        snprintf(delete_sql, sizeof(delete_sql), "DELETE FROM active_matches WHERE match_id = '%s'", match_id);
        SQLExecDirect(stmt, (SQLCHAR*)delete_sql, SQL_NTS);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS)
        return false;

    char* insert_sql = malloc(65536);
    if (!insert_sql) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    const char* safe_moves = (moves_json && moves_json[0]) ? moves_json : "[]";

    snprintf(insert_sql, 65536,
             "INSERT INTO active_matches (match_id, red_user_id, black_user_id, "
             "current_turn, red_time_ms, black_time_ms, move_count, moves_json, "
             "rated, started_at, last_move_at) VALUES ('%s', %d, %d, '%s', %d, "
             "%d, %d, "
             "N'%s', %d, '%s', '%s')",
             match_id, red_user_id, black_user_id, current_turn, red_time_ms, black_time_ms, move_count, safe_moves,
             rated ? 1 : 0, started_str, last_move_str);

    ret = SQLExecDirect(stmt, (SQLCHAR*)insert_sql, SQL_NTS);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        db_print_error(stmt, SQL_HANDLE_STMT, "db_save_active_match");
    } else {
        printf("[DB] Active match saved: %s\n", match_id);
    }

    free(insert_sql);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

bool db_delete_active_match(const char* match_id) {
    SQLHSTMT stmt;
    SQLRETURN ret;

    const char* sql = "DELETE FROM active_matches WHERE match_id = ?";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS)
        return false;

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)match_id, 0, NULL);

    ret = SQLExecute(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

bool db_get_active_match(const char* match_id, int* red_user_id, int* black_user_id, char* current_turn,
                         int* red_time_ms, int* black_time_ms, int* move_count, char* moves_json,
                         size_t moves_json_size, bool* rated, time_t* started_at, time_t* last_move_at) {
    SQLHSTMT stmt;
    SQLRETURN ret;

    const char* sql = "SELECT red_user_id, black_user_id, current_turn, "
                      "red_time_ms, black_time_ms, "
                      "move_count, moves_json, rated, started_at, last_move_at "
                      "FROM active_matches WHERE match_id = ?";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS)
        return false;

    ret = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)match_id, 0, NULL);

    ret = SQLExecute(stmt);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        if (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLLEN indicator;
            int rated_int = 0;
            char started_str[32] = {0}, last_move_str[32] = {0};

            SQLGetData(stmt, 1, SQL_C_SLONG, red_user_id, 0, &indicator);
            SQLGetData(stmt, 2, SQL_C_SLONG, black_user_id, 0, &indicator);
            SQLGetData(stmt, 3, SQL_C_CHAR, current_turn, 6, &indicator);
            SQLGetData(stmt, 4, SQL_C_SLONG, red_time_ms, 0, &indicator);
            SQLGetData(stmt, 5, SQL_C_SLONG, black_time_ms, 0, &indicator);
            SQLGetData(stmt, 6, SQL_C_SLONG, move_count, 0, &indicator);
            SQLGetData(stmt, 7, SQL_C_CHAR, moves_json, moves_json_size, &indicator);
            SQLGetData(stmt, 8, SQL_C_SLONG, &rated_int, 0, &indicator);
            SQLGetData(stmt, 9, SQL_C_CHAR, started_str, sizeof(started_str), &indicator);
            SQLGetData(stmt, 10, SQL_C_CHAR, last_move_str, sizeof(last_move_str), &indicator);

            *rated = (rated_int != 0);

            *started_at = time(NULL);
            *last_move_at = time(NULL);

            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            return true;
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return false;
}

int db_load_all_active_matches(void* matches_array, int max_matches) {

    (void)matches_array;
    (void)max_matches;

    SQLHSTMT stmt;
    SQLRETURN ret;
    int count = 0;

    const char* sql = "SELECT COUNT(*) FROM active_matches";

    ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS)
        return 0;

    ret = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        if (SQLFetch(stmt) == SQL_SUCCESS) {
            SQLLEN indicator;
            SQLGetData(stmt, 1, SQL_C_SLONG, &count, 0, &indicator);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    printf("[DB] Found %d active matches in database\n", count);
    return count;
}

bool db_session_create(const char* token, int user_id, int expires_hours) {
    const char* sql = "INSERT INTO Sessions (session_token, user_id, expires_at) "
                      "VALUES (?, ?, DATEADD(HOUR, ?, GETDATE()))";

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)token, 0, NULL);
    SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &user_id, 0, NULL);
    SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &expires_hours, 0, NULL);

    db_ret = SQLExecute(stmt);
    bool success = (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO);
    DB_CLEANUP(stmt);

    if (success) {
        printf("[DB] Session created for user %d (expires in %d hours)\n", user_id, expires_hours);
    }
    return success;
}

bool db_session_validate(const char* token, int* out_user_id) {
    if (!token || !out_user_id)
        return false;

    const char* sql = "SELECT user_id FROM Sessions WHERE session_token = ? AND expires_at > GETDATE()";
    int user_id = 0;

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)token, 0, NULL);
    DB_EXECUTE(stmt);

    db_ret = SQLFetch(stmt);
    if (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO) {
        SQLGetData(stmt, 1, SQL_C_SLONG, &user_id, 0, &db_indicator);
        *out_user_id = user_id;
        DB_CLEANUP(stmt);
        return true;
    }

    DB_CLEANUP(stmt);
    return false;
}

bool db_session_destroy(const char* token) {
    if (!token)
        return false;

    const char* sql = "DELETE FROM Sessions WHERE session_token = ?";

    DB_PREPARE(stmt, sql);
    SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 64, 0, (SQLCHAR*)token, 0, NULL);

    db_ret = SQLExecute(stmt);
    bool success = (db_ret == SQL_SUCCESS || db_ret == SQL_SUCCESS_WITH_INFO);
    DB_CLEANUP(stmt);

    if (success) {
        printf("[DB] Session destroyed\n");
    }
    return success;
}

bool db_session_cleanup_expired(void) {
    const char* sql = "DELETE FROM Sessions WHERE expires_at < GETDATE()";

    SQLHSTMT stmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, g_db_conn, &stmt);
    if (ret != SQL_SUCCESS)
        return false;

    ret = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
    bool success = (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);

    if (success) {
        SQLLEN rows_affected = 0;
        SQLRowCount(stmt, &rows_affected);
        if (rows_affected > 0) {
            printf("[DB] Cleaned up %ld expired sessions\n", (long)rows_affected);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return success;
}
