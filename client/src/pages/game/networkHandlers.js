import * as timer from './timerManager.js';
import * as chat from './chatManager.js';

export function setupNetworkListeners(gameController, matchData, userData, callbacks) {
    if (!gameController.network) return;

    const { updateTurnIndicator, showResult, exitToLobby, oldRating } = callbacks;

    gameController.network.on('opponent_move', (data) => {
        let payload = data.payload || data;
        if (typeof payload === 'string') {
            try {
                payload = JSON.parse(payload);
            } catch (e) {}
        }

        if (payload.red_time_ms !== undefined && payload.black_time_ms !== undefined) {
            timer.syncTimerFromServer(payload.red_time_ms, payload.black_time_ms);
        }

        gameController.isMyTurn = true;
        updateTurnIndicator();
    });

    gameController.network.on('match_found', (msg) => {
        const data = msg.payload || msg;
        document.getElementById('rematch-modal')?.classList.remove('active');
        document.getElementById('rematch-waiting-modal')?.classList.remove('active');
        document.getElementById('result-modal')?.classList.remove('active');
        localStorage.setItem('current_match', JSON.stringify(data));
        window.location.reload();
    });

    gameController.network.on('turn_changed', (data) => {
        gameController.isMyTurn = data.isMyTurn;
        updateTurnIndicator();
    });

    gameController.network.on('chat_message', (data) => {
        let payload = data.payload || data;
        if (typeof payload === 'string') {
            try {
                payload = JSON.parse(payload);
            } catch (e) {}
        }

        if (payload.username === userData.username) return;

        let senderDisplay = payload.username;
        if (!senderDisplay || senderDisplay === 'Unknown') {
            senderDisplay = matchData?.opponent_name || 'Đối thủ';
        }

        chat.addChatMessage(senderDisplay, payload.message, false);
    });

    gameController.network.on('draw_offer', () => {
        document.getElementById('draw-modal')?.classList.add('active');
    });

    gameController.network.on('timer_sync', (data) => {
        if (data.red_time_ms !== undefined && data.black_time_ms !== undefined) {
            timer.syncTimerFromServer(data.red_time_ms, data.black_time_ms);
        }
    });

    gameController.network.on('reconnected', async (data) => {
        const storedUserData = localStorage.getItem('xiangqi_user');
        if (storedUserData) {
            try {
                const user = JSON.parse(storedUserData);
                if (user.token) {
                    gameController.token = user.token;
                    gameController.network.sessionToken = user.token;
                }
            } catch (e) {
                console.warn('[Game] Failed to refresh token:', e);
            }
        }

        if (matchData && matchData.match_id) {
            try {
                const joinResult = await gameController.network.joinMatch(matchData.match_id);

                if (joinResult && joinResult.payload) {
                    const state =
                        typeof joinResult.payload === 'string' ? JSON.parse(joinResult.payload) : joinResult.payload;

                    if (state.is_my_turn !== undefined) {
                        gameController.isMyTurn = state.is_my_turn;
                        updateTurnIndicator();
                    }

                    if (state.red_time_ms !== undefined && state.black_time_ms !== undefined) {
                        timer.syncTimerFromServer(state.red_time_ms, state.black_time_ms);
                    }
                }
            } catch (error) {
                console.error('[Game] Failed to rejoin match:', error);
                alert('Không thể kết nối lại trận đấu: ' + error.message);
                localStorage.removeItem('match_data');
                window.location.href = 'lobby.html';
            }
        }
    });

    gameController.network.on('game_end', (data) => {
        let payload = data.payload || data;
        if (typeof payload === 'string') {
            try {
                payload = JSON.parse(payload);
            } catch (e) {}
        }

        const durationStr = timer.getMatchDurationString();
        const resultTime = document.getElementById('result-time');
        if (resultTime) resultTime.textContent = durationStr;

        if (payload.red_rating && payload.black_rating) {
            let newRating = matchData.your_color === 'red' ? payload.red_rating : payload.black_rating;
            let oppRating = matchData.your_color === 'red' ? payload.black_rating : payload.red_rating;

            if (newRating > 0) {
                const ratingChange = newRating - oldRating;
                const changeStr = ratingChange >= 0 ? `+${ratingChange}` : `${ratingChange}`;

                const resultRatingChange = document.getElementById('result-rating-change');
                if (resultRatingChange) {
                    resultRatingChange.textContent = changeStr;
                    resultRatingChange.style.color = ratingChange >= 0 ? '#4ade80' : '#f87171';
                }

                const resultNewRating = document.getElementById('result-new-rating');
                if (resultNewRating) resultNewRating.textContent = newRating;

                userData.rating = newRating;
                localStorage.setItem('xiangqi_user', JSON.stringify(userData));

                const ratingDisplay = document.getElementById('player-rating');
                if (ratingDisplay) ratingDisplay.textContent = `${newRating}`;

                const oppRatingDisplay = document.getElementById('opponent-rating');
                if (oppRatingDisplay) oppRatingDisplay.textContent = `${oppRating}`;
            }
        }

        let result = 'draw';
        let reason = payload.reason || 'Kết thúc';

        if (payload.result === 'red_win') {
            result = matchData.your_color === 'red' ? 'win' : 'loss';
        } else if (payload.result === 'black_win') {
            result = matchData.your_color === 'black' ? 'win' : 'loss';
        } else if (payload.result === 'draw') {
            result = 'draw';
        }

        showResult(result, reason);
    });
}

export function setupButtonHandlers(gameController, callbacks) {
    const { isGameActive, setGameActive, exitToLobby } = callbacks;

    const btnLeave = document.getElementById('btn-leave');
    if (btnLeave) {
        btnLeave.addEventListener('click', async () => {
            if (!isGameActive()) {
                exitToLobby();
                return;
            }

            if (confirm('Bạn chắc chắn muốn đầu hàng và rời trận?')) {
                try {
                    await gameController.resign();
                } catch (e) {
                    console.error(e);
                } finally {
                    setTimeout(exitToLobby, 500);
                }
            }
        });
    }

    const btnDrawOffer = document.getElementById('btn-draw-offer');
    if (btnDrawOffer) {
        btnDrawOffer.addEventListener('click', async () => {
            if (!isGameActive()) return;
            try {
                await gameController.offerDraw();
                alert('Đã gửi lời mời hòa!');
            } catch (error) {
                console.error(error);
                alert('Lỗi: ' + error.message);
            }
        });
    }

    const btnResign = document.getElementById('btn-resign');
    if (btnResign) {
        btnResign.addEventListener('click', async () => {
            if (!isGameActive()) return;
            if (confirm('Bạn muốn đầu hàng?')) {
                await gameController.resign();
            }
        });
    }

    document.getElementById('btn-accept-draw')?.addEventListener('click', async () => {
        document.getElementById('draw-modal')?.classList.remove('active');
        await gameController.network.respondDraw(gameController.matchId, true);
    });

    document.getElementById('btn-decline-draw')?.addEventListener('click', async () => {
        document.getElementById('draw-modal')?.classList.remove('active');
        await gameController.network.respondDraw(gameController.matchId, false);
    });

    document.getElementById('btn-new-game')?.addEventListener('click', exitToLobby);
    document.getElementById('btn-to-lobby')?.addEventListener('click', exitToLobby);
}

export function setupRematchHandlers(gameController, callbacks) {
    const { exitToLobby, getPendingRematchMatchId, setPendingRematchMatchId } = callbacks;

    document.getElementById('btn-rematch')?.addEventListener('click', async () => {
        const matchId = gameController.matchId;

        if (!matchId) {
            alert('Không thể yêu cầu đấu lại - thiếu mã trận đấu');
            return;
        }

        if (!gameController.network || !gameController.network.connected) {
            alert('Không thể yêu cầu đấu lại - mất kết nối server');
            return;
        }

        try {
            document.getElementById('result-modal')?.classList.remove('active');
            document.getElementById('rematch-waiting-modal')?.classList.add('active');

            await gameController.network.requestRematch(matchId);
            setPendingRematchMatchId(matchId);
        } catch (e) {
            console.error('[Rematch] Error:', e);
            document.getElementById('rematch-waiting-modal')?.classList.remove('active');
            document.getElementById('result-modal')?.classList.add('active');
            alert('Lỗi: ' + e.message);
        }
    });

    document.getElementById('btn-cancel-rematch')?.addEventListener('click', () => {
        document.getElementById('rematch-waiting-modal')?.classList.remove('active');
        document.getElementById('result-modal')?.classList.add('active');
        setPendingRematchMatchId(null);
    });

    document.getElementById('btn-accept-rematch')?.addEventListener('click', async () => {
        const matchId = getPendingRematchMatchId() || gameController.matchId;
        document.getElementById('rematch-modal')?.classList.remove('active');

        try {
            gameController.network.respondRematch(matchId, true);
        } catch (e) {
            console.error('Accept rematch error:', e);
        }
    });

    document.getElementById('btn-decline-rematch')?.addEventListener('click', async () => {
        const matchId = getPendingRematchMatchId() || gameController.matchId;
        document.getElementById('rematch-modal')?.classList.remove('active');

        try {
            gameController.network.respondRematch(matchId, false);
        } catch (e) {
            console.error('Decline rematch error:', e);
        }
        setPendingRematchMatchId(null);
    });

    gameController.on('rematch_request', (data) => {
        setPendingRematchMatchId(data.match_id);
        const fromUsername = data.from_username || 'Đối thủ';

        const requestText = document.getElementById('rematch-request-text');
        if (requestText) {
            requestText.textContent = `${fromUsername} muốn đấu lại. Bạn đồng ý không?`;
        }

        document.getElementById('result-modal')?.classList.remove('active');
        document.getElementById('rematch-waiting-modal')?.classList.remove('active');
        document.getElementById('rematch-modal')?.classList.add('active');
    });

    gameController.on('rematch_declined', (data) => {
        document.getElementById('rematch-waiting-modal')?.classList.remove('active');
        document.getElementById('result-modal')?.classList.add('active');
        alert('Đối thủ đã từ chối đấu lại');
        setPendingRematchMatchId(null);
    });
}

export function setupMoveHistoryHandler(gameController) {
    gameController.on('move-made', (move) => {
        const list = document.getElementById('moves-list');
        if (!list) return;

        if (list.children.length > 0 && list.children[0].innerText.includes('Chưa có')) {
            list.innerHTML = '';
        }

        const item = document.createElement('div');
        item.className = 'move-record';

        item.style.display = 'flex';
        item.style.justifyContent = 'space-between';
        item.style.borderBottom = '1px solid #333';
        item.style.padding = '5px 0';

        const turnNum = list.children.length + 1;
        const isRed = move.color === 'red';
        const colorClass = isRed ? '#ff4444' : '#aaaaaa';
        const colorName = isRed ? 'Đỏ' : 'Đen';

        const moveText = `${colorName}: (${move.from.row}, ${move.from.col}) ➔ (${move.to.row}, ${move.to.col})`;

        item.innerHTML = `
            <span style="color: #666; width: 30px;">${turnNum}.</span>
            <span style="color: ${colorClass}; font-weight: 500; text-align: right; flex: 1;">
                ${moveText}
            </span>
        `;

        list.appendChild(item);
        list.scrollTop = list.scrollHeight;
    });
}
