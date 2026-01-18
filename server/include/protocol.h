#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char* type;
    int seq;
    char* token;
    char* payload_json;
} message_t;

message_t* parse_message(const char* json);
void free_message(message_t* msg);

char* create_response(const char* type, int seq, const char* token, const char* payload_json);
char* create_error(int seq, const char* error_code, const char* message, bool fatal);

int extract_messages(const char* buffer, size_t len, char*** out_messages, int* count);

char* json_escape(const char* str);
char* json_get_string(const char* json, const char* key);
int json_get_int(const char* json, const char* key);
bool json_get_bool(const char* json, const char* key);

#endif
