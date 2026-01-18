#include "handlers_common.h"

void escape_json_string(const char* src, char* dst, size_t dst_size) {
    if (!src) {
        dst[0] = '\0';
        return;
    }

    char* d = dst;
    const char* s = src;
    size_t remaining = dst_size - 1;

    while (*s && remaining > 1) {
        if (*s == '"') {
            if (remaining < 2)
                break;
            *d++ = '\\';
            *d++ = '"';
            remaining -= 2;
        } else if (*s == '\\') {
            if (remaining < 2)
                break;
            *d++ = '\\';
            *d++ = '\\';
            remaining -= 2;
        } else if (*s == '\n') {
            if (remaining < 2)
                break;
            *d++ = '\\';
            *d++ = 'n';
            remaining -= 2;
        } else if (*s == '\r') {
            if (remaining < 2)
                break;
            *d++ = '\\';
            *d++ = 'r';
            remaining -= 2;
        } else {
            *d++ = *s;
            remaining--;
        }
        s++;
    }
    *d = '\0';
}

void send_response(server_t* server, client_t* client, int seq, bool success, const char* message,
                   const char* payload) {
    char response[MAX_MESSAGE_SIZE];
    char escaped_msg[512];

    escape_json_string(message, escaped_msg, sizeof(escaped_msg));

    if (payload) {
        snprintf(response, sizeof(response),
                 "{\"type\":\"%s\",\"seq\":%d,\"success\":%s,\"message\":\"%s\","
                 "\"payload\":%s}\n",
                 success ? "response" : "error", seq, success ? "true" : "false", escaped_msg, payload);
    } else {
        snprintf(response, sizeof(response), "{\"type\":\"%s\",\"seq\":%d,\"success\":%s,\"message\":\"%s\"}\n",
                 success ? "response" : "error", seq, success ? "true" : "false", escaped_msg);
    }

    send_to_client(server, client->fd, response);
}

bool validate_token_and_get_user(const char* token, int* out_user_id) {
    if (!token || !out_user_id) {
        return false;
    }
    return session_validate(token, out_user_id);
}
