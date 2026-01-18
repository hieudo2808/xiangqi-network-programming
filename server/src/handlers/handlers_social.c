#include "handlers_common.h"

void handle_challenge(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    int opponent_id = json_get_int(msg->payload_json, "opponent_id");
    bool rated = json_get_bool(msg->payload_json, "rated");

    if (opponent_id <= 0) {
        send_response(server, client, msg->seq, false, "Invalid opponent_id", NULL);
        return;
    }

    char* challenge_id = lobby_create_challenge(user_id, opponent_id, rated);
    if (!challenge_id) {
        send_response(server, client, msg->seq, false, "Failed to create challenge", NULL);
        return;
    }

    char payload[256];
    snprintf(payload, sizeof(payload), "{\"challenge_id\":\"%s\",\"from_user_id\":%d,\"rated\":%s}", challenge_id,
             user_id, rated ? "true" : "false");
    char notify[512];
    snprintf(notify, sizeof(notify), "{\"type\":\"challenge_received\",\"payload\":%s}\n", payload);
    send_to_user(server, opponent_id, notify);

    char resp_payload[256];
    snprintf(resp_payload, sizeof(resp_payload), "{\"challenge_id\":\"%s\"}", challenge_id);
    send_response(server, client, msg->seq, true, "Challenge sent", resp_payload);

    free(challenge_id);
}

void handle_challenge_response(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* challenge_id = json_get_string(msg->payload_json, "challenge_id");
    bool accept = json_get_bool(msg->payload_json, "accept");

    if (!challenge_id) {
        send_response(server, client, msg->seq, false, "Missing challenge_id", NULL);
        return;
    }

    if (accept) {
        if (!lobby_accept_challenge(challenge_id, user_id)) {
            send_response(server, client, msg->seq, false, "Failed to accept challenge", NULL);
            return;
        }

        challenge_t* ch = lobby_get_challenge(challenge_id);
        if (!ch) {
            send_response(server, client, msg->seq, false, "Challenge not found", NULL);
            return;
        }

        char* match_id = match_create(ch->from_user_id, ch->to_user_id, ch->rated, 600000);
        if (!match_id) {
            send_response(server, client, msg->seq, false, "Failed to create match", NULL);
            return;
        }
        match_persist(match_id);

        char payload[512];
        snprintf(payload, sizeof(payload), "{\"match_id\":\"%s\"}", match_id);
        char notify[1024];
        snprintf(notify, sizeof(notify), "{\"type\":\"match_start\",\"payload\":%s}\n", payload);
        send_to_user(server, ch->from_user_id, notify);
        send_to_user(server, ch->to_user_id, notify);

        send_response(server, client, msg->seq, true, "Challenge accepted", payload);
        free(match_id);
    } else {
        lobby_decline_challenge(challenge_id, user_id);
        send_response(server, client, msg->seq, true, "Challenge declined", NULL);
    }
}
void handle_chat_message(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* message = json_get_string(msg->payload_json, "message");
    const char* match_id = json_get_string(msg->payload_json, "match_id");

    if (!message || !match_id) {
        send_response(server, client, msg->seq, false, "Missing message or match_id", NULL);
        return;
    }

    if (strlen(message) > 500) {
        send_response(server, client, msg->seq, false, "Message too long (max 500 chars)", NULL);
        return;
    }

    match_t* match = match_get(match_id);
    if (!match) {
        send_response(server, client, msg->seq, false, "Match not found", NULL);
        return;
    }

    if (match->red_user_id != user_id && match->black_user_id != user_id) {
        send_response(server, client, msg->seq, false, "Not in this match", NULL);
        return;
    }

    char username[64] = {0};
    if (!db_get_username(user_id, username, sizeof(username))) {
        strcpy(username, "Unknown");
    }

    char chat_payload[1024];
    snprintf(chat_payload, sizeof(chat_payload),
             "{\"match_id\":\"%s\",\"user_id\":%d,\"username\":\"%s\",\"message\":"
             "\"%s\",\"timestamp\":%ld}",
             match_id, user_id, username, message, (long)time(NULL));

    char notification[MAX_MESSAGE_SIZE];
    snprintf(notification, sizeof(notification), "{\"type\":\"chat_message\",\"payload\":%s}\n", chat_payload);

    client_t* red_client = server_get_client_by_user_id(server, match->red_user_id);
    if (red_client) {
        send_to_client(server, red_client->fd, notification);
    }

    client_t* black_client = server_get_client_by_user_id(server, match->black_user_id);
    if (black_client) {
        send_to_client(server, black_client->fd, notification);
    }

    send_response(server, client, msg->seq, true, "Message sent", NULL);

    printf("[Handler] Chat message from user %d in match %s\n", user_id, match_id);
}
