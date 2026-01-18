#include "handlers/handlers_auth.c"
#include "handlers/handlers_common.c"
#include "handlers/handlers_lobby.c"
#include "handlers/handlers_match.c"
#include "handlers/handlers_query.c"
#include "handlers/handlers_rematch.c"
#include "handlers/handlers_room.c"
#include "handlers/handlers_social.c"
#include "handlers/handlers_spectate.c"

#include "../include/handlers.h"
#include "../include/log.h"
#include "../include/protocol.h"
#include "../include/server.h"

#include <string.h>

typedef void (*handler_fn)(server_t*, client_t*, message_t*);

typedef struct {
    const char* type;
    handler_fn handler;
} handler_entry_t;

static void handle_ping(server_t* server, client_t* client, message_t* msg) {
    send_response(server, client, msg->seq, true, "pong", NULL);
}

static const handler_entry_t handler_table[] = {{"register", handle_register},
                                                {"login", handle_login},
                                                {"logout", handle_logout},
                                                {"validate_token", handle_validate_token},

                                                {"set_ready", handle_set_ready},
                                                {"find_match", handle_find_match},

                                                {"move", handle_move},
                                                {"resign", handle_resign},
                                                {"draw_offer", handle_draw_offer},
                                                {"draw_response", handle_draw_response},
                                                {"game_over", handle_game_over},
                                                {"join_match", handle_join_match},
                                                {"get_match", handle_get_match},
                                                {"get_timer", handle_get_timer},

                                                {"create_room", handle_create_room},
                                                {"join_room", handle_join_room},
                                                {"leave_room", handle_leave_room},
                                                {"get_rooms", handle_get_rooms},
                                                {"start_room_game", handle_start_room_game},

                                                {"challenge", handle_challenge},
                                                {"challenge_response", handle_challenge_response},
                                                {"chat_message", handle_chat_message},

                                                {"join_spectate", handle_join_spectate},
                                                {"leave_spectate", handle_leave_spectate},

                                                {"rematch_request", handle_rematch_request},
                                                {"rematch_response", handle_rematch_response},
                                                {"match_history", handle_match_history},
                                                {"get_live_matches", handle_get_live_matches},
                                                {"get_profile", handle_get_profile},
                                                {"leaderboard", handle_leaderboard},

                                                {"heartbeat", handle_heartbeat},
                                                {"ping", handle_ping},

                                                {NULL, NULL}};

void dispatch_handler(server_t* server, client_t* client, message_t* msg) {
    if (!msg->type) {
        LOG_ERROR("No message type");
        return;
    }

    LOG_DEBUG("Dispatch: type=%s seq=%d user=%d", msg->type, msg->seq, client->user_id);

    for (const handler_entry_t* e = handler_table; e->type != NULL; e++) {
        if (strcmp(msg->type, e->type) == 0) {
            e->handler(server, client, msg);
            return;
        }
    }

    LOG_WARN("Unknown message type: %s", msg->type);
    send_response(server, client, msg->seq, false, "Unknown message type", NULL);
}
