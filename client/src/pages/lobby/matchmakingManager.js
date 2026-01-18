let isSearching = false;

export function isSearchingMatch() {
    return isSearching;
}

export async function startMatchSearch(gameController, showMessage) {
    if (!gameController?.network) {
        showMessage('error', 'Chưa kết nối đến máy chủ! Vui lòng đợi...');
        return false;
    }

    const btnFindMatch = document.getElementById('btn-find-match');
    const searchingDiv = document.getElementById('searching-indicator');

    if (btnFindMatch) btnFindMatch.disabled = true;
    if (searchingDiv) searchingDiv.classList.add('active');
    isSearching = true;

    try {
        try {
            await gameController.setReady(true);
        } catch (e) {
            console.warn('setReady failed:', e.message || e);
        }

        try {
            const response = await gameController.findMatch();
            if (response && response.payload) {
                let payload = response.payload;
                if (typeof payload === 'string') {
                    try {
                        payload = JSON.parse(payload);
                    } catch (e) {}
                }
                if (payload && (payload.match_id || payload.matchId)) {
                    handleMatchFound(payload);
                    return true;
                }
            }
            showMessage('info', 'Đang tìm đối thủ...');
            return true;
        } catch (err) {
            if (err && err.message && err.message.toLowerCase().includes('no opponent')) {
                showMessage('info', 'Đang tìm đối thủ...');
                return true;
            } else {
                throw err;
            }
        }
    } catch (error) {
        showMessage('error', `Lỗi: ${error.message}`);
        cancelMatchSearch(gameController);
        return false;
    }
}

export async function cancelMatchSearch(gameController, showMessage = null) {
    const btnFindMatch = document.getElementById('btn-find-match');
    const searchingDiv = document.getElementById('searching-indicator');

    if (searchingDiv) searchingDiv.classList.remove('active');
    if (btnFindMatch) btnFindMatch.disabled = false;
    isSearching = false;

    if (showMessage) {
        showMessage('info', 'Đã hủy tìm kiếm');
    }

    try {
        if (gameController && gameController.network) {
            await gameController.setReady(false);
        }
    } catch (e) {
        console.warn('Failed to unset ready:', e.message || e);
    }
}

export function handleMatchFound(matchData) {
    const searchingDiv = document.getElementById('searching-indicator');
    const matchFoundDiv = document.getElementById('match-found');

    if (searchingDiv) searchingDiv.classList.remove('active');
    if (matchFoundDiv) matchFoundDiv.classList.add('active');
    isSearching = false;

    const oppName = matchData.opponent_name || matchData.opponent || 'Unknown';
    const oppRating = matchData.opponent_rating || 1500;

    const opponentInfo = document.getElementById('opponent-info');
    if (opponentInfo) {
        opponentInfo.innerHTML = `Đối thủ: <strong>${oppName}</strong> (Rating: ${oppRating})`;
    }

    localStorage.setItem('current_match', JSON.stringify(matchData));
}

export function setupMatchFoundCallback(gameController) {
    gameController.onMatchFound = (payload) => {
        try {
            let data = payload;
            if (typeof data === 'string') {
                try {
                    data = JSON.parse(data);
                } catch (e) {}
            }
            handleMatchFound(data || {});

            setTimeout(() => {
                window.location.href = 'game.html';
            }, 400);
        } catch (e) {
            console.error('onMatchFound handler failed:', e);
        }
    };
}
