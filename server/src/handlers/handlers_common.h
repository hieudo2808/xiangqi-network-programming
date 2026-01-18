#ifndef HANDLERS_COMMON_H
#define HANDLERS_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../include/handlers.h"
#include "../../include/broadcast.h"
#include "../../include/db.h"
#include "../../include/lobby.h"
#include "../../include/log.h"
#include "../../include/match.h"
#include "../../include/protocol.h"
#include "../../include/rating.h"
#include "../../include/server.h"
#include "../../include/session.h"

void escape_json_string(const char* src, char* dst, size_t dst_size);
void send_response(server_t* server, client_t* client, int seq, bool success, 
                   const char* message, const char* payload);
bool validate_token_and_get_user(const char* token, int* out_user_id);


#define REQUIRE_AUTH(server, client, msg) \
    int user_id; \
    if (!validate_token_and_get_user((msg)->token, &user_id)) { \
        send_response((server), (client), (msg)->seq, false, "Invalid or expired token", NULL); \
        return; \
    } \
    (client)->user_id = user_id; \
    (client)->authenticated = true;

#endif
