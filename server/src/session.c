#include "../include/session.h"
#include "../include/db.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SESSION_EXPIRES_HOURS 24

static void generate_token(char* token) {
    const char charset[] = "0123456789abcdef";
    unsigned char random_bytes[64];

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t bytes_read = read(fd, random_bytes, sizeof(random_bytes));
        close(fd);

        if (bytes_read == sizeof(random_bytes)) {
            for (int i = 0; i < 64; i++) {
                token[i] = charset[random_bytes[i] % 16];
            }
            token[64] = '\0';
            return;
        }
    }

    fprintf(stderr, "[WARNING] /dev/urandom failed, falling back to rand()\n");
    for (int i = 0; i < 64; i++) {
        token[i] = charset[rand() % 16];
    }
    token[64] = '\0';
}

bool session_init(void) {
    srand(time(NULL));
    db_session_cleanup_expired();
    return true;
}

char* session_create(int user_id) {
    char token[65];
    generate_token(token);

    if (!db_session_create(token, user_id, SESSION_EXPIRES_HOURS)) {
        fprintf(stderr, "[Session] Failed to create session in DB\n");
        return NULL;
    }

    char* token_copy = strdup(token);
    printf("[Session] Created session for user %d\n", user_id);
    return token_copy;
}

bool session_validate(const char* token, int* out_user_id) {
    if (!token)
        return false;
    return db_session_validate(token, out_user_id);
}

void session_update_activity(const char* token) {
    (void)token;
}

void session_destroy(const char* token) {
    if (!token)
        return;
    db_session_destroy(token);
}

void session_cleanup_expired(void) {
    db_session_cleanup_expired();
}

void session_shutdown(void) {
    printf("[Session] Shutdown complete\n");
}
