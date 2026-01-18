#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <time.h>

#define SESSION_TIMEOUT 86400

typedef struct {
    char token[65];
    int user_id;
    time_t created_at;
    time_t last_activity;
} session_t;

char* session_create(int user_id);
bool session_validate(const char* token, int* out_user_id);
void session_update_activity(const char* token);
void session_destroy(const char* token);
void session_cleanup_expired(void);

bool session_init(void);
void session_shutdown(void);

#endif
