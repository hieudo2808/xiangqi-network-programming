let spectateData = null;
let gameController = null;

export function initSpectateManager(controller, data) {
    gameController = controller;
    spectateData = data;

    if (!data) return;

    controller.matchId = data.match_id;
    controller.isOnlineMode = true;
    controller.isSpectator = true;
    controller.isMyTurn = false;
}

export function setupSpectateUI() {
    const gameActions = document.querySelector('.game-actions');
    if (gameActions) {
        gameActions.innerHTML = `
            <span style="color: #28a745; font-weight: 600; margin-right: 10px;">Đang xem trận đấu</span>
            <button class="btn btn-secondary" id="btn-leave-spectate">🚪 Rời Xem</button>
        `;
        document.getElementById('btn-leave-spectate')?.addEventListener('click', leaveSpectate);
    }

    const gameTitle = document.querySelector('.game-title');
    if (gameTitle) {
        gameTitle.textContent = 'Xem Trận Đấu - Cờ Tướng Online';
    }
}

export async function joinSpectate(syncTimerCallback, updateTurnCallback) {
    if (!spectateData || !gameController) {
        throw new Error('Spectate not initialized');
    }

    try {
        const joinResult = await gameController.joinSpectate(spectateData.match_id);

        if (joinResult && joinResult.payload && joinResult.payload.match_data) {
            const matchData =
                typeof joinResult.payload.match_data === 'string'
                    ? JSON.parse(joinResult.payload.match_data)
                    : joinResult.payload.match_data;

            if (!gameController.chessboard) {
                throw new Error('Chessboard not initialized');
            }

            if (!gameController.chessboard.board) {
                throw new Error('Board not initialized');
            }

            if (!gameController.ui) {
                gameController.initBoardUI('xiangqi-board');
            }

            gameController.setupBoard({ flipped: false });

            if (matchData.moves && matchData.moves.length > 0) {
                for (const move of matchData.moves) {
                    if (!isValidMove(move)) {
                        console.warn('[Spectate] Invalid move coordinates:', move);
                        continue;
                    }

                    const piece = gameController.chessboard.board[move.from.row][move.from.col];
                    if (piece) {
                        gameController.chessboard.board[move.from.row][move.from.col] = null;
                        gameController.chessboard.board[move.to.row][move.to.col] = piece;
                        piece.row = move.to.row;
                        piece.col = move.to.col;

                        gameController.chessboard.turn = gameController.chessboard.turn === 'red' ? 'black' : 'red';
                        gameController.chessboard.turnCnt++;
                    } else {
                        console.warn('[Spectate] No piece at position:', move.from);
                    }
                }

                if (gameController.ui) {
                    gameController.ui.renderBoard(gameController.chessboard.board);
                }
            }

            if (matchData.red_time_ms !== undefined && matchData.black_time_ms !== undefined) {
                syncTimerCallback?.(matchData.red_time_ms, matchData.black_time_ms);
            }
            if (joinResult.payload.current_turn) {
                const currentTurn = joinResult.payload.current_turn;
                gameController.isMyTurn = currentTurn === gameController.myColor;
                updateTurnCallback?.();
            }

            const playerName = document.querySelector('#player-name');
            const opponentName = document.querySelector('#opponent-name');
            if (playerName) playerName.textContent = `🔴 Player #${matchData.red_user_id}`;
            if (opponentName) opponentName.textContent = `⚫ Player #${matchData.black_user_id}`;
        }

        return joinResult;
    } catch (error) {
        console.error('[Spectate] Failed to join spectate:', error);
        throw error;
    }
}

export async function leaveSpectate() {
    try {
        if (gameController?.network && spectateData) {
            await gameController.network.leaveSpectate(spectateData.match_id);
        }
    } catch (e) {
        console.warn('Error leaving spectate:', e);
    }
    localStorage.removeItem('spectate_match');
    window.location.href = 'lobby.html';
}

function isValidMove(move) {
    return (
        move.from &&
        move.to &&
        move.from.row >= 0 &&
        move.from.row <= 9 &&
        move.from.col >= 0 &&
        move.from.col <= 8 &&
        move.to.row >= 0 &&
        move.to.row <= 9 &&
        move.to.col >= 0 &&
        move.to.col <= 8
    );
}

export function setupSpectateListeners(controller, syncTimerCallback) {
    if (!controller.network) return;

    controller.network.on('spectate_move', (data) => {
        const payload = data.payload || data;
        if (payload.from && payload.to) {
            const piece = controller.chessboard.board[payload.from.row][payload.from.col];
            if (piece) {
                controller.chessboard.board[payload.from.row][payload.from.col] = null;
                controller.chessboard.board[payload.to.row][payload.to.col] = piece;
                piece.row = payload.to.row;
                piece.col = payload.to.col;
                controller.chessboard.turn = controller.chessboard.turn === 'red' ? 'black' : 'red';

                if (controller.ui) {
                    controller.ui.renderBoard(controller.chessboard.board);
                }
            }
        }

        if (payload.red_time_ms !== undefined && payload.black_time_ms !== undefined) {
            syncTimerCallback?.(payload.red_time_ms, payload.black_time_ms);
        }
    });

    controller.network.on('spectate_game_over', (data) => {
        const payload = data.payload || data;
        controller.emit('game-over', {
            winner: payload.winner,
            reason: payload.reason,
        });
    });
}
