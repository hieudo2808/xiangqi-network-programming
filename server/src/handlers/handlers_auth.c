#include "handlers_common.h"

void handle_register(server_t* server, client_t* client, message_t* msg) {
    const char* username = json_get_string(msg->payload_json, "username");
    const char* email = json_get_string(msg->payload_json, "email");
    const char* password = json_get_string(msg->payload_json, "password");

    if (!username || !email || !password) {
        send_response(server, client, msg->seq, false, "Missing required fields", NULL);
        return;
    }

    if (db_check_username_exists(username)) {
        send_response(server, client, msg->seq, false, "Username already exists", NULL);
        return;
    }

    if (db_check_email_exists(email)) {
        send_response(server, client, msg->seq, false, "Email already exists", NULL);
        return;
    }

    int user_id;
    if (!db_create_user(username, email, password, &user_id)) {
        send_response(server, client, msg->seq, false, "Failed to create user", NULL);
        return;
    }

    char payload[256];
    snprintf(payload, sizeof(payload), "{\"user_id\":%d,\"username\":\"%s\"}", user_id, username);
    send_response(server, client, msg->seq, true, "Registration successful", payload);

    printf("[Handler] User registered: %s (ID: %d)\n", username, user_id);
}

void handle_login(server_t* server, client_t* client, message_t* msg) {
    const char* username = json_get_string(msg->payload_json, "username");
    const char* password = json_get_string(msg->payload_json, "password");

    if (!username || !password) {
        send_response(server, client, msg->seq, false, "Missing username or password", NULL);
        return;
    }

    int user_id, rating;
    char password_hash[128];

    if (!db_get_user_by_username(username, &user_id, password_hash, &rating)) {
        send_response(server, client, msg->seq, false, "Invalid username or password", NULL);
        return;
    }

    if (strcmp(password, password_hash) != 0) {
        send_response(server, client, msg->seq, false, "Invalid username or password", NULL);
        return;
    }

    const char* token = session_create(user_id);
    if (!token) {
        send_response(server, client, msg->seq, false, "Failed to create session", NULL);
        return;
    }

    client->user_id = user_id;
    client->authenticated = true;

    char payload[512];
    snprintf(payload, sizeof(payload), "{\"token\":\"%s\",\"user_id\":%d,\"username\":\"%s\",\"rating\":%d}", token,
             user_id, username, rating);
    send_response(server, client, msg->seq, true, "Login successful", payload);

    printf("[Handler] User logged in: %s (ID: %d, fd=%d)\n", username, user_id, client->fd);
}

void handle_logout(server_t* server, client_t* client, message_t* msg) {
    if (!client->authenticated) {
        send_response(server, client, msg->seq, false, "Not authenticated", NULL);
        return;
    }

    if (msg->token) {
        session_destroy(msg->token);
    }

    int logged_out_user_id = client->user_id;
    lobby_remove_player(client->user_id);

    client->authenticated = false;
    client->user_id = 0;

    send_response(server, client, msg->seq, true, "Logged out", NULL);
    printf("[Handler] User logged out (ID: %d)\n", logged_out_user_id);
}

void handle_validate_token(server_t* server, client_t* client, message_t* msg) {
    const char* token = json_get_string(msg->payload_json, "token");

    if (!token) {
        send_response(server, client, msg->seq, false, "Missing token", NULL);
        return;
    }

    int user_id;
    if (session_validate(token, &user_id)) {
        char username[64] = {0};
        int rating = 0;
        db_get_user_by_id(user_id, username, NULL, &rating, NULL, NULL, NULL);

        char payload[256];
        snprintf(payload, sizeof(payload), "{\"valid\":true,\"user_id\":%d,\"username\":\"%s\",\"rating\":%d}", user_id,
                 username, rating);

        client->user_id = user_id;
        client->authenticated = true;

        send_response(server, client, msg->seq, true, "Token valid", payload);
        printf("[Handler] Token validated for user %d (%s)\n", user_id, username);
    } else {
        send_response(server, client, msg->seq, false, "Token expired or invalid", "{\"valid\":false}");
    }
}
