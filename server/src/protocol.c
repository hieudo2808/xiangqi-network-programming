#include "../include/protocol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* json_get_string(const char* json, const char* key) {
    if (!json || !key)
        return NULL;

    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* pos = strstr(json, search);
    if (!pos)
        return NULL;

    pos += strlen(search);
    while (*pos && isspace(*pos))
        pos++;

    if (*pos != '"')
        return NULL;
    pos++;

    const char* end = strchr(pos, '"');
    if (!end)
        return NULL;

    size_t len = end - pos;
    char* result = malloc(len + 1);
    if (!result)
        return NULL;

    memcpy(result, pos, len);
    result[len] = '\0';

    return result;
}

int json_get_int(const char* json, const char* key) {
    if (!json || !key)
        return 0;

    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* pos = strstr(json, search);
    if (!pos)
        return 0;

    pos += strlen(search);
    while (*pos && isspace(*pos))
        pos++;

    return atoi(pos);
}

bool json_get_bool(const char* json, const char* key) {
    if (!json || !key)
        return false;

    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char* pos = strstr(json, search);
    if (!pos)
        return false;

    pos += strlen(search);
    while (*pos && isspace(*pos))
        pos++;

    return (strncmp(pos, "true", 4) == 0);
}

static char* extract_payload(const char* json) {
    const char* start = strstr(json, "\"payload\":");
    if (!start)
        return NULL;

    start += 10;
    while (*start && isspace(*start))
        start++;

    if (*start != '{')
        return NULL;

    int depth = 0;
    const char* p = start;
    while (*p) {
        if (*p == '{')
            depth++;
        else if (*p == '}') {
            depth--;
            if (depth == 0) {
                size_t len = p - start + 1;
                char* payload = malloc(len + 1);
                if (!payload)
                    return NULL;
                memcpy(payload, start, len);
                payload[len] = '\0';
                return payload;
            }
        }
        p++;
    }

    return NULL;
}

message_t* parse_message(const char* json) {
    if (!json)
        return NULL;

    message_t* msg = calloc(1, sizeof(message_t));
    if (!msg)
        return NULL;

    msg->type = json_get_string(json, "type");
    msg->seq = json_get_int(json, "seq");
    msg->token = json_get_string(json, "token");
    msg->payload_json = extract_payload(json);

    if (!msg->type || !msg->payload_json) {
        free_message(msg);
        return NULL;
    }

    return msg;
}

void free_message(message_t* msg) {
    if (!msg)
        return;
    free(msg->type);
    free(msg->token);
    free(msg->payload_json);
    free(msg);
}

char* create_response(const char* type, int seq, const char* token, const char* payload_json) {
    char* response = malloc(4096);
    if (!response)
        return NULL;

    snprintf(response, 4096, "{\"type\":\"%s\",\"seq\":%d,\"token\":%s,\"payload\":%s}", type, seq,
             token ? (char[]){'"', '\0'} : "null", payload_json);

    if (token) {
        char temp[4096];
        snprintf(temp, sizeof(temp), "{\"type\":\"%s\",\"seq\":%d,\"token\":\"%s\",\"payload\":%s}", type, seq, token,
                 payload_json);
        strcpy(response, temp);
    }

    return response;
}

char* create_error(int seq, const char* error_code, const char* message, bool fatal) {
    char payload[512];
    snprintf(payload, sizeof(payload), "{\"error_code\":\"%s\",\"message\":\"%s\",\"fatal\":%s}", error_code, message,
             fatal ? "true" : "false");

    return create_response("error", seq, NULL, payload);
}

char* json_escape(const char* str) {
    if (!str)
        return NULL;

    size_t len = strlen(str);
    char* escaped = malloc(len * 2 + 1);
    if (!escaped)
        return NULL;

    char* dst = escaped;
    for (const char* src = str; *src; src++) {
        switch (*src) {
            case '"':
                *dst++ = '\\';
                *dst++ = '"';
                break;
            case '\\':
                *dst++ = '\\';
                *dst++ = '\\';
                break;
            case '\n':
                *dst++ = '\\';
                *dst++ = 'n';
                break;
            case '\r':
                *dst++ = '\\';
                *dst++ = 'r';
                break;
            case '\t':
                *dst++ = '\\';
                *dst++ = 't';
                break;
            default:
                *dst++ = *src;
                break;
        }
    }
    *dst = '\0';

    return escaped;
}

int extract_messages(const char* buffer, size_t len, char*** out_messages, int* count) {
    if (!buffer || !out_messages || !count)
        return -1;

    *count = 0;
    *out_messages = NULL;

    int num_messages = 0;
    for (size_t i = 0; i < len; i++) {
        if (buffer[i] == '\n')
            num_messages++;
    }

    if (num_messages == 0)
        return 0;

    char** messages = malloc(num_messages * sizeof(char*));
    if (!messages)
        return -1;

    const char* start = buffer;
    int idx = 0;
    for (size_t i = 0; i < len && idx < num_messages; i++) {
        if (buffer[i] == '\n') {
            size_t msg_len = &buffer[i] - start;
            if (msg_len > 0) {
                messages[idx] = malloc(msg_len + 1);
                if (messages[idx]) {
                    memcpy(messages[idx], start, msg_len);
                    messages[idx][msg_len] = '\0';
                    idx++;
                }
            }
            start = &buffer[i + 1];
        }
    }

    *out_messages = messages;
    *count = idx;

    return 0;
}
