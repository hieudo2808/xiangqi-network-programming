export async function hashPassword(password) {
    const encoder = new TextEncoder();
    const data = encoder.encode(password);
    const hashBuffer = await crypto.subtle.digest('SHA-256', data);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map((b) => b.toString(16).padStart(2, '0')).join('');
}

export async function register(bridge, username, email, password) {
    const hashedPassword = await hashPassword(password);
    return bridge.sendAndWait('register', { username, email, password: hashedPassword }, 'register');
}

export async function login(bridge, username, password) {
    const hashedPassword = await hashPassword(password);
    const response = await bridge.sendAndWait('login', { username, password: hashedPassword }, 'login');

    if (response.success && response.payload?.token) {
        bridge.sessionToken = response.payload.token;
        localStorage.setItem(
            '_auth_creds',
            JSON.stringify({
                username,
                hashedPassword,
            }),
        );
    }

    return response;
}

export async function findMatch(bridge, rated = true) {
    return bridge.sendAndWait('find_match', { rated: true }, 'match_found');
}

export async function cancelMatch(bridge) {
    return bridge.sendAndWait('cancel_match', {}, 'cancel_match');
}

export async function setReady(bridge, ready = true) {
    return bridge.sendAndWait('set_ready', { ready }, 'set_ready');
}

export async function joinMatch(bridge, matchId) {
    return bridge.sendAndWait('join_match', { match_id: matchId }, 'join_match');
}

export async function sendMove(bridge, matchId, fromRow, fromCol, toRow, toCol) {
    return bridge.sendAndWait(
        'move',
        {
            match_id: matchId,
            from_row: fromRow,
            from_col: fromCol,
            to_row: toRow,
            to_col: toCol,
        },
        'move',
    );
}

export async function resign(bridge, matchId) {
    return bridge.sendAndWait('resign', { match_id: matchId }, 'resign');
}

export async function offerDraw(bridge, matchId) {
    return bridge.sendAndWait('draw_offer', { match_id: matchId }, 'draw_offer');
}

export async function respondDraw(bridge, matchId, accept) {
    return bridge.sendAndWait('draw_response', { match_id: matchId, accept }, 'draw_response');
}

export async function sendGameOver(bridge, matchId, result, reason = 'normal') {
    return bridge.sendAndWait('game_over', { match_id: matchId, result, reason }, 'game_over');
}

export async function getTimer(bridge, matchId) {
    return bridge.sendAndWait('get_timer', { match_id: matchId }, 'get_timer');
}

export async function sendChatMessage(bridge, matchId, message) {
    return bridge.sendAndWait('chat_message', { match_id: matchId, message }, 'chat_message');
}

export async function createRoom(bridge, name, password, rated) {
    return bridge.sendAndWait('create_room', { name, password, rated }, 'create_room');
}

export async function joinRoom(bridge, roomCode, password) {
    return bridge.sendAndWait('join_room', { room_code: roomCode, password }, 'join_room');
}

export async function leaveRoom(bridge, roomCode) {
    return bridge.sendAndWait('leave_room', { room_code: roomCode }, 'leave_room');
}

export async function getRooms(bridge) {
    return bridge.sendAndWait('get_rooms', {}, 'get_rooms');
}

export async function startRoomGame(bridge, roomCode) {
    return bridge.sendAndWait('start_room_game', { room_code: roomCode }, 'start_room_game');
}

export async function joinSpectate(bridge, matchId) {
    return bridge.sendAndWait('join_spectate', { match_id: matchId }, 'join_spectate');
}

export async function leaveSpectate(bridge, matchId) {
    return bridge.sendAndWait('leave_spectate', { match_id: matchId }, 'leave_spectate');
}

export async function getLiveMatches(bridge) {
    return bridge.sendAndWait('get_live_matches', {}, 'get_live_matches');
}

export async function getProfile(bridge, userId = null) {
    return bridge.sendAndWait('get_profile', { user_id: userId }, 'get_profile');
}

export async function getMatchHistory(bridge, limit = 20, offset = 0) {
    return bridge.sendAndWait('match_history', { limit, offset }, 'match_history');
}

export async function getMatch(bridge, matchId) {
    return bridge.sendAndWait('get_match', { match_id: matchId }, 'get_match');
}

export async function getLeaderboard(bridge, limit = 10, offset = 0) {
    const response = await bridge.sendAndWait('leaderboard', { limit, offset }, 'leaderboard');
    return response.payload || [];
}

export async function sendChallenge(bridge, opponentId, rated = true) {
    return bridge.sendAndWait('challenge', { opponent_id: opponentId, rated }, 'challenge');
}

export async function respondChallenge(bridge, challengeId, accept) {
    return bridge.sendAndWait('challenge_response', { challenge_id: challengeId, accept }, 'challenge_response');
}

export async function requestRematch(bridge, matchId) {
    return bridge.sendAndWait('rematch_request', { match_id: matchId }, 'rematch_request');
}

export async function respondRematch(bridge, matchId, accept) {
    return bridge.sendAndWait('rematch_response', { match_id: matchId, accept }, 'rematch_response');
}
