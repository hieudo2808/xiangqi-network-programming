#include "handlers_common.h"

void handle_create_room(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* room_name = json_get_string(msg->payload_json, "room_name");
    const char* password = json_get_string(msg->payload_json, "password");
    bool rated = json_get_bool(msg->payload_json, "rated");

    char* room_code = lobby_create_room(user_id, room_name, password, rated);
    if (!room_code) {
        send_response(server, client, msg->seq, false, "Failed to create room", NULL);
        return;
    }

    char username[64] = {0};
    db_get_username(user_id, username, sizeof(username));

    char payload[256];
    snprintf(payload, sizeof(payload), "{\"room_code\":\"%s\",\"host_id\":%d,\"host_name\":\"%s\",\"rated\":%s}",
             room_code, user_id, username, rated ? "true" : "false");
    send_response(server, client, msg->seq, true, "Room created", payload);

    char* rooms_json = lobby_get_rooms_json();
    if (rooms_json) {
        char broadcast_msg[16384];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"rooms_update\",\"payload\":%s}\n", rooms_json);
        broadcast_to_all(server, broadcast_msg);
        free(rooms_json);
    }

    printf("[Handler] Room created: %s by user %d\n", room_code, user_id);
    free(room_code);
}

void handle_join_room(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* room_code = json_get_string(msg->payload_json, "room_code");
    const char* password = json_get_string(msg->payload_json, "password");

    if (!room_code) {
        send_response(server, client, msg->seq, false, "Room code required", NULL);
        return;
    }

    int host_id;
    if (!lobby_join_room(room_code, password, user_id, &host_id)) {
        send_response(server, client, msg->seq, false, "Cannot join room (wrong password, full, or not found)", NULL);
        return;
    }

    char host_username[64] = {0};
    char guest_username[64] = {0};
    int host_rating = 1500, guest_rating = 1500;
    db_get_user_by_id(host_id, host_username, NULL, &host_rating, NULL, NULL, NULL);
    db_get_user_by_id(user_id, guest_username, NULL, &guest_rating, NULL, NULL, NULL);

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"room_code\":\"%s\",\"host_id\":%d,\"host_name\":\"%s\",\"host_"
             "rating\":%d}",
             room_code, host_id, host_username, host_rating);
    send_response(server, client, msg->seq, true, "Joined room", payload);

    client_t* host_client = server_get_client_by_user_id(server, host_id);
    if (host_client) {
        char notification[512];
        snprintf(notification, sizeof(notification),
                 "{\"type\":\"room_guest_joined\",\"payload\":{\"room_code\":\"%s\","
                 "\"guest_id\":%d,\"guest_name\":\"%s\",\"guest_rating\":%d}}\n",
                 room_code, user_id, guest_username, guest_rating);
        send_to_client(server, host_client->fd, notification);
    }

    char* rooms_json = lobby_get_rooms_json();
    if (rooms_json) {
        char broadcast_msg[16384];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"rooms_update\",\"payload\":%s}\n", rooms_json);
        broadcast_to_all(server, broadcast_msg);
        free(rooms_json);
    }

    printf("[Handler] User %d joined room %s\n", user_id, room_code);
}

void handle_leave_room(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* room_code = json_get_string(msg->payload_json, "room_code");
    if (!room_code) {
        send_response(server, client, msg->seq, false, "Room code required", NULL);
        return;
    }

    room_t* room = lobby_get_room(room_code);
    if (!room) {
        send_response(server, client, msg->seq, false, "Room not found", NULL);
        return;
    }

    int host_id = room->host_user_id;
    int guest_id = room->guest_user_id;
    bool is_host = (user_id == host_id);

    if (!lobby_leave_room(room_code, user_id)) {
        send_response(server, client, msg->seq, false, "Cannot leave room", NULL);
        return;
    }

    send_response(server, client, msg->seq, true, "Left room", NULL);

    if (is_host && guest_id != 0) {
        client_t* guest_client = server_get_client_by_user_id(server, guest_id);
        if (guest_client) {
            char notification[256];
            snprintf(notification, sizeof(notification),
                     "{\"type\":\"room_closed\",\"payload\":{\"room_code\":\"%s\","
                     "\"reason\":\"host_left\"}}\n",
                     room_code);
            send_to_client(server, guest_client->fd, notification);
        }
    }

    if (!is_host) {
        client_t* host_client = server_get_client_by_user_id(server, host_id);
        if (host_client) {
            char notification[256];
            snprintf(notification, sizeof(notification),
                     "{\"type\":\"room_guest_left\",\"payload\":{\"room_code\":\"%s\"}}\n", room_code);
            send_to_client(server, host_client->fd, notification);
        }
    }

    char* rooms_json = lobby_get_rooms_json();
    if (rooms_json) {
        char broadcast_msg[16384];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"rooms_update\",\"payload\":%s}\n", rooms_json);
        broadcast_to_all(server, broadcast_msg);
        free(rooms_json);
    }

    printf("[Handler] User %d left room %s\n", user_id, room_code);
}

void handle_get_rooms(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    char* rooms_json = lobby_get_rooms_json();
    if (!rooms_json) {
        send_response(server, client, msg->seq, false, "Failed to get rooms", NULL);
        return;
    }

    char payload[16384];
    snprintf(payload, sizeof(payload), "{\"rooms\":%s}", rooms_json);
    send_response(server, client, msg->seq, true, "Rooms list", payload);

    free(rooms_json);
}

void handle_start_room_game(server_t* server, client_t* client, message_t* msg) {
    REQUIRE_AUTH(server, client, msg);

    const char* room_code = json_get_string(msg->payload_json, "room_code");
    if (!room_code) {
        send_response(server, client, msg->seq, false, "Room code required", NULL);
        return;
    }

    room_t* room = lobby_get_room(room_code);
    if (!room) {
        send_response(server, client, msg->seq, false, "Room not found", NULL);
        return;
    }

    if (room->host_user_id != user_id) {
        send_response(server, client, msg->seq, false, "Only host can start game", NULL);
        return;
    }

    if (room->guest_user_id == 0) {
        send_response(server, client, msg->seq, false, "Need an opponent to start", NULL);
        return;
    }

    int host_id = room->host_user_id;
    int guest_id = room->guest_user_id;
    bool rated = room->rated;

    char* match_id = match_create(host_id, guest_id, rated, 600000);
    if (!match_id) {
        send_response(server, client, msg->seq, false, "Failed to create match", NULL);
        return;
    }
    match_persist(match_id);

    char host_username[64] = {0}, guest_username[64] = {0};
    int host_rating = 1500, guest_rating = 1500;
    db_get_user_by_id(host_id, host_username, NULL, &host_rating, NULL, NULL, NULL);
    db_get_user_by_id(guest_id, guest_username, NULL, &guest_rating, NULL, NULL, NULL);

    char host_payload[512];
    snprintf(host_payload, sizeof(host_payload),
             "{\"match_id\":\"%s\",\"your_color\":\"red\",\"opponent_id\":%d,"
             "\"opponent_name\":\"%s\",\"opponent_rating\":%d,\"rated\":%s}",
             match_id, guest_id, guest_username, guest_rating, rated ? "true" : "false");

    char host_msg[MAX_MESSAGE_SIZE];
    snprintf(host_msg, sizeof(host_msg), "{\"type\":\"match_found\",\"payload\":%s}\n", host_payload);
    send_to_client(server, client->fd, host_msg);

    char guest_payload[512];
    snprintf(guest_payload, sizeof(guest_payload),
             "{\"match_id\":\"%s\",\"your_color\":\"black\",\"opponent_id\":%d,"
             "\"opponent_name\":\"%s\",\"opponent_rating\":%d,\"rated\":%s}",
             match_id, host_id, host_username, host_rating, rated ? "true" : "false");

    client_t* guest_client = server_get_client_by_user_id(server, guest_id);
    if (guest_client) {
        char guest_msg[MAX_MESSAGE_SIZE];
        snprintf(guest_msg, sizeof(guest_msg), "{\"type\":\"match_found\",\"payload\":%s}\n", guest_payload);
        send_to_client(server, guest_client->fd, guest_msg);
    }

    lobby_close_room(room_code, host_id);

    char* rooms_json = lobby_get_rooms_json();
    if (rooms_json) {
        char broadcast_msg[16384];
        snprintf(broadcast_msg, sizeof(broadcast_msg), "{\"type\":\"rooms_update\",\"payload\":%s}\n", rooms_json);
        broadcast_to_all(server, broadcast_msg);
        free(rooms_json);
    }

    printf("[Handler] Room game started: %s -> match %s\n", room_code, match_id);
    free(match_id);
}
