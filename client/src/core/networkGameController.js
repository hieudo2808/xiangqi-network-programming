import { GameController } from './gameController.js';
import { OPENING_POSITION, parsePosition } from './config.js';
import CClientBridge from '../network/cClientBridge.js';

class NetworkGameController extends GameController {
    constructor(boardContainerIdOrConfig = null) {
        let boardContainerId = null;
        let config = {};

        if (typeof boardContainerIdOrConfig === 'string') {
            boardContainerId = boardContainerIdOrConfig;
        } else if (boardContainerIdOrConfig && typeof boardContainerIdOrConfig === 'object') {
            config = boardContainerIdOrConfig;
            boardContainerId = config.boardContainerId || null;
        }

        const initialPosition = config.initialPosition || parsePosition(OPENING_POSITION);
        super(boardContainerId, initialPosition);

        this.network = null;
        this.token = null;
        this.userId = null;
        this.matchId = null;
        this.myColor = null;
        this.isOnlineMode = false;
        this.isMyTurn = false;
        this.isSpectator = false;

        this.onMatchFound = null;
        this.onOpponentMove = null;
        this.onGameEnd = null;
        this.onConnectionError = null;
        this.onChatMessage = null;
        this.onDrawOffer = null;
        this.onChallengeReceived = null;

        this.on('game-over', (data) => {
            if (this.isOnlineMode && this.matchId && !this.isSpectator) {
                let result = '';
                if (data.winner === 'red') result = 'red_win';
                else if (data.winner === 'black') result = 'black_win';
                else result = 'draw';

                this.network
                    .sendGameOver(this.matchId, result, data.reason)
                    .catch((e) => console.error('Failed to report game over:', e));
            }
        });
    }

    initBoardUI(boardContainerId) {
        return this.initUI(boardContainerId);
    }

    async connectToServer(host, port) {
        this.network = new CClientBridge();
        try {
            await this.network.connect(host, port);
            this.setupNetworkListeners();
            return true;
        } catch (error) {
            console.error('[NetworkGame] Connection failed:', error);
            if (this.onConnectionError) this.onConnectionError(error);
            return false;
        }
    }

    setupNetworkListeners() {
        this.network.on('match_found', (msg) => this.handleMatchFound(msg.payload));
        this.network.on('opponent_move', (msg) => this.handleOpponentMove(msg.payload));
        this.network.on('game_end', (msg) => this.handleGameEnd(msg.payload));

        this.network.on('draw_offer', (msg) => {
            this.handleDrawOffer(msg.payload);
        });

        this.network.on('challenge_received', (msg) => {
            this.handleChallengeReceived(msg.payload);
        });

        this.network.on('chat_message', (msg) => this.handleChatMessage(msg.payload));
        this.network.on('ready_list_update', (msg) => this.handleReadyListUpdate(msg.payload));

        this.network.on('rematch_request', (msg) => {
            this.emit('rematch_request', msg.payload);
        });

        this.network.on('rematch_declined', (msg) => {
            this.emit('rematch_declined', msg.payload);
        });
    }

    async register(username, email, password) {
        try {
            const response = await this.network.register(username, email, password);
            return {
                status: 'success',
                message: response.message || 'Registration successful',
            };
        } catch (error) {
            console.error('[NetworkGame] Registration failed:', error);
            throw error;
        }
    }

    async login(username, password) {
        try {
            const response = await this.network.login(username, password);

            let payload = response.payload;
            if (typeof payload === 'string') {
                payload = JSON.parse(payload);
            }

            this.token = payload.token;
            this.userId = payload.user_id;

            return {
                status: 'success',
                username: payload.username,
                token: payload.token,
                rating: payload.rating,
                user_id: payload.user_id,
            };
        } catch (error) {
            console.error('[NetworkGame] Login failed:', error);
            throw error;
        }
    }

    async setReady(ready = true) {
        return this.network.setReady(ready);
    }

    async joinMatch(matchId) {
        try {
            const response = await this.network.sendAndWait('join_match', { match_id: matchId });
            return response;
        } catch (error) {
            console.warn('[NetworkGame] Join match failed:', error);
            return null;
        }
    }

    async joinSpectate(matchId) {
        try {
            const response = await this.network.joinSpectate(matchId);

            this.isSpectator = true;
            this.matchId = matchId;
            this.isOnlineMode = true;
            this.isMyTurn = false;

            return response;
        } catch (error) {
            console.error('[NetworkGame] Join spectate failed:', error);
            throw error;
        }
    }

    async findMatch(rated = true) {
        try {
            const response = await this.network.findMatch(rated);
            return response;
        } catch (error) {
            console.error('[NetworkGame] Find match failed:', error);
            throw error;
        }
    }

    handleMatchFound(payload) {
        let data = payload;
        if (typeof data === 'string') {
            try {
                data = JSON.parse(data);
            } catch (e) {}
        }

        this.matchId = data.match_id;
        this.myColor = data.your_color;
        this.isOnlineMode = true;
        this.isMyTurn = this.myColor === 'red';

        if (data.opponent && typeof data.opponent === 'object') {
            data.opponent_name = data.opponent.username;
            data.opponent_rating = data.opponent.rating;
        }

        if (this.ui) {
            this.setupBoard({ flipped: this.myColor === 'black' });
        }

        if (this.onMatchFound) {
            this.onMatchFound(data);
        }
    }

    async executeMove(newRowOrFrom, newColOrTo) {
        let from, to;

        if (typeof newRowOrFrom === 'object' && newRowOrFrom !== null) {
            from = newRowOrFrom;
            to = newColOrTo;
        } else {
            if (!this.chessboard.curPiece) {
                console.warn('[NetworkGame] No piece selected');
                return false;
            }
            from = { row: this.chessboard.curPiece.row, col: this.chessboard.curPiece.col };
            to = { row: newRowOrFrom, col: newColOrTo };
        }

        if (this.isOnlineMode && !this.isMyTurn) {
            console.warn('[NetworkGame] Not your turn!');
            return false;
        }

        if (this.isSpectator) {
            console.warn('[NetworkGame] Spectators cannot move pieces');
            return false;
        }

        const isValid = this.validateMove(from, to);
        if (!isValid) {
            console.warn('[NetworkGame] Invalid move');
            return false;
        }

        if (this.isOnlineMode) {
            try {
                const response = await this.network.sendMove(this.matchId, from.row, from.col, to.row, to.col);
                const result = super.executeMove(to.row, to.col);

                if (result) {
                    this.isMyTurn = false;
                    this.network.emit('turn_changed', { isMyTurn: false });

                    if (response && response.payload) {
                        let timerData = response.payload;
                        if (typeof timerData === 'string') {
                            try {
                                timerData = JSON.parse(timerData);
                            } catch (e) {}
                        }
                        if (timerData.red_time_ms !== undefined && timerData.black_time_ms !== undefined) {
                            this.network.emit('timer_sync', timerData);
                        }
                    }
                }

                return result;
            } catch (error) {
                console.error('[NetworkGame] Failed to send move:', error);
                if (error.message && error.message.includes('Not your turn')) {
                    this.isMyTurn = false;
                }
                return false;
            }
        } else {
            return super.executeMove(to.row, to.col);
        }
    }

    handleOpponentMove(payload) {
        let fromRow, fromCol, toRow, toCol;

        if (payload.from && typeof payload.from === 'object') {
            fromRow = payload.from.row;
            fromCol = payload.from.col;
            toRow = payload.to.row;
            toCol = payload.to.col;
        } else {
            fromRow = parseInt(payload.from_row);
            fromCol = parseInt(payload.from_col);
            toRow = parseInt(payload.to_row);
            toCol = parseInt(payload.to_col);
        }

        const piece = this.chessboard.board[fromRow][fromCol];
        if (!piece) {
            console.error(`Sync Error: No piece at ${fromRow},${fromCol}`);
            return;
        }

        const originalTurn = this.chessboard.turn;
        this.chessboard.turn = piece.color;
        this.chessboard.curPiece = piece;

        super.executeMove(toRow, toCol);

        this.isMyTurn = true;
        this.ui.updateTurn(this.chessboard.turn);
    }

    async resign() {
        if (!this.isOnlineMode || !this.matchId) {
            console.warn('[NetworkGame] No active match');
            return;
        }

        try {
            await this.network.resign(this.matchId);
        } catch (error) {
            console.error('[NetworkGame] Resign failed:', error);
        }
    }

    async offerDraw() {
        if (!this.isOnlineMode || !this.matchId) {
            console.warn('[NetworkGame] No active match');
            return;
        }

        try {
            await this.network.offerDraw(this.matchId);
        } catch (error) {
            console.error('[NetworkGame] Draw offer failed:', error);
        }
    }

    async respondDraw(accept) {
        if (!this.isOnlineMode || !this.matchId) return;
        try {
            await this.network.respondDraw(this.matchId, accept);
        } catch (error) {
            console.error('[NetworkGame] Draw response failed:', error);
        }
    }

    async sendChatMessage(message) {
        if (!this.isOnlineMode || !this.matchId) {
            console.warn('[NetworkGame] No active match');
            return;
        }

        try {
            await this.network.sendChatMessage(this.matchId, message);
        } catch (error) {
            console.error('[NetworkGame] Chat message failed:', error);
            throw error;
        }
    }

    handleDrawOffer(payload) {
        if (this.onDrawOffer) {
            this.onDrawOffer(payload);
        } else {
        }
    }

    handleGameEnd(payload) {
        this.isOnlineMode = false;
        this.isMyTurn = false;
        if (this.onGameEnd) {
            this.onGameEnd(payload);
        }
    }

    async getLeaderboard(limit = 10, offset = 0) {
        try {
            const leaderboard = await this.network.getLeaderboard(limit, offset);
            return leaderboard;
        } catch (error) {
            console.error('[NetworkGame] Get leaderboard failed:', error);
            throw error;
        }
    }

    async getMatch(matchId) {
        try {
            const response = await this.network.getMatch(matchId);
            return response.payload;
        } catch (error) {
            console.error('[NetworkGame] Get match failed:', error);
            throw error;
        }
    }

    handleReadyListUpdate(payload) {}

    async sendChallenge(opponentId, rated = true) {
        try {
            await this.network.sendChallenge(opponentId, rated);
        } catch (error) {
            console.error('[NetworkGame] Send challenge failed:', error);
        }
    }

    handleChallengeReceived(payload) {
        if (this.onChallengeReceived) {
            this.onChallengeReceived(payload);
        } else {
        }
    }

    handleChatMessage(payload) {
        if (this.onChatMessage) {
            this.onChatMessage(payload);
        }
    }

    disconnect() {
        if (this.network) {
            this.network.disconnect();
            this.network = null;
        }

        this.token = null;
        this.userId = null;
        this.matchId = null;
        this.isOnlineMode = false;
    }
}

export default NetworkGameController;
