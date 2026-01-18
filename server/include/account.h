#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdbool.h>

typedef struct {
    int user_id;
    char username[64];
    char email[128];
    char password_hash[65];
    int rating;
    int wins;
    int losses;
    int draws;
    char created_at[32];
} user_t;

bool account_register(const char* username, const char* email, const char* password_hash, int* out_user_id);
bool account_login(const char* username, const char* password_hash, user_t* out_user);
bool account_get_by_id(int user_id, user_t* out_user);
bool account_update_rating(int user_id, int new_rating);
bool account_update_stats(int user_id, int wins, int losses, int draws);

bool validate_username(const char* username);
bool validate_email(const char* email);
bool username_exists(const char* username);
bool email_exists(const char* email);

#endif
