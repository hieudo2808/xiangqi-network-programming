import * as conn from './connectionManager.js';
import * as api from './apiMethods.js';

class CClientBridge {
    constructor() {
        this.connected = false;
        this.seqCounter = 1;
        this.sessionToken = null;
        this.eventListeners = new Map();
        this.pendingRequests = new Map();
        this._messagePollInterval = null;

        this._heartbeatInterval = null;
        this._heartbeatPingInterval = 10000;
        this._heartbeatTimeout = 30000;
        this._lastPongTime = null;
    }

    async connect(host, port) {
        return conn.connect(this, host, port);
    }

    async disconnect() {
        return conn.disconnect(this);
    }

    handleMessage(data) {
        try {
            let message = typeof data === 'string' ? JSON.parse(data) : data;

            if (!message || typeof message !== 'object') {
                console.warn('[CClientBridge] Invalid message:', message);
                return;
            }

            if (message.payload && typeof message.payload === 'string') {
                try {
                    message.payload = JSON.parse(message.payload);
                } catch (e) {
                    console.log('Failed to parse message payload:', message.payload);
                }
            }

            const msgType = message.type;

            if (msgType === 'response' || msgType === 'error') {
                const seq = message.seq;
                if (seq && this.pendingRequests.has(seq)) {
                    const { resolve, reject } = this.pendingRequests.get(seq);
                    this.pendingRequests.delete(seq);

                    if (msgType === 'error') {
                        reject(new Error(message.message || 'Server error'));
                    } else {
                        resolve(message);
                    }
                    return;
                }
            }

            if (msgType === 'response' && message.message === 'pong') {
                conn.handlePongResponse(this);
                return;
            }

            if (msgType === 'connection_status') {
                conn.handleConnectionStatus(this, message.payload);
                return;
            }

            this.emit(msgType, message);
        } catch (error) {
            console.error('[CClientBridge] Message handling error:', error);
        }
    }

    async send(type, payload = {}, expectedResponse = null) {
        if (!this.connected) {
            throw new Error('Not connected');
        }

        if (!window.pywebview?.api) {
            throw new Error('pywebview API not available');
        }

        const seq = this.seqCounter++;
        const message = {
            type,
            seq,
            token: this.sessionToken,
            payload,
        };

        const jsonStr = JSON.stringify(message);
        const result = await window.pywebview.api.send(jsonStr);

        if (!result.success) {
            throw new Error(result.message || 'Send failed');
        }

        if (expectedResponse) {
            return new Promise((resolve, reject) => {
                const timeout = setTimeout(() => {
                    this.pendingRequests.delete(seq);
                    reject(new Error('Request timeout'));
                }, 10000);

                this.pendingRequests.set(seq, {
                    resolve: (response) => {
                        clearTimeout(timeout);
                        resolve(response);
                    },
                    reject: (error) => {
                        clearTimeout(timeout);
                        reject(error);
                    },
                });
            });
        }

        return { success: true };
    }

    async sendAndWait(type, payload = {}, expectedResponse = null) {
        return this.send(type, payload, expectedResponse || type);
    }

    on(event, callback) {
        if (!this.eventListeners.has(event)) {
            this.eventListeners.set(event, []);
        }
        this.eventListeners.get(event).push(callback);
    }

    off(event, callback) {
        if (this.eventListeners.has(event)) {
            const callbacks = this.eventListeners.get(event);
            const index = callbacks.indexOf(callback);
            if (index > -1) callbacks.splice(index, 1);
        }
    }

    emit(event, data) {
        if (this.eventListeners.has(event)) {
            this.eventListeners.get(event).forEach((callback) => {
                try {
                    callback(data);
                } catch (error) {
                    console.error(`[CClientBridge] Event error for ${event}:`, error);
                }
            });
        }
    }

    async register(username, email, password) {
        return api.register(this, username, email, password);
    }

    async login(username, password) {
        return api.login(this, username, password);
    }

    async hashPassword(password) {
        return api.hashPassword(password);
    }

    async findMatch(rated = true) {
        return api.findMatch(this, rated);
    }

    async cancelMatch() {
        return api.cancelMatch(this);
    }

    async setReady(ready = true) {
        return api.setReady(this, ready);
    }

    async joinMatch(matchId) {
        return api.joinMatch(this, matchId);
    }

    async sendMove(matchId, fromRow, fromCol, toRow, toCol) {
        return api.sendMove(this, matchId, fromRow, fromCol, toRow, toCol);
    }

    async resign(matchId) {
        return api.resign(this, matchId);
    }

    async offerDraw(matchId) {
        return api.offerDraw(this, matchId);
    }

    async respondDraw(matchId, accept) {
        return api.respondDraw(this, matchId, accept);
    }

    async sendGameOver(matchId, result, reason = 'normal') {
        return api.sendGameOver(this, matchId, result, reason);
    }

    async getTimer(matchId) {
        return api.getTimer(this, matchId);
    }

    async sendChatMessage(matchId, message) {
        return api.sendChatMessage(this, matchId, message);
    }

    async createRoom(name, password, rated) {
        return api.createRoom(this, name, password, rated);
    }

    async joinRoom(roomCode, password) {
        return api.joinRoom(this, roomCode, password);
    }

    async leaveRoom(roomCode) {
        return api.leaveRoom(this, roomCode);
    }

    async getRooms() {
        return api.getRooms(this);
    }

    async startRoomGame(roomCode) {
        return api.startRoomGame(this, roomCode);
    }

    async joinSpectate(matchId) {
        return api.joinSpectate(this, matchId);
    }

    async leaveSpectate(matchId) {
        return api.leaveSpectate(this, matchId);
    }

    async getLiveMatches() {
        return api.getLiveMatches(this);
    }

    async getProfile(userId = null) {
        return api.getProfile(this, userId);
    }

    async getMatchHistory(limit = 20, offset = 0) {
        return api.getMatchHistory(this, limit, offset);
    }

    async getMatch(matchId) {
        return api.getMatch(this, matchId);
    }

    async getLeaderboard(limit = 10, offset = 0) {
        return api.getLeaderboard(this, limit, offset);
    }

    async sendChallenge(opponentId, rated = true) {
        return api.sendChallenge(this, opponentId, rated);
    }

    async respondChallenge(challengeId, accept) {
        return api.respondChallenge(this, challengeId, accept);
    }

    async requestRematch(matchId) {
        return api.requestRematch(this, matchId);
    }

    async respondRematch(matchId, accept) {
        return api.respondRematch(this, matchId, accept);
    }
}

export default CClientBridge;

if (typeof window !== 'undefined') {
    window.CClientBridge = CClientBridge;
}
