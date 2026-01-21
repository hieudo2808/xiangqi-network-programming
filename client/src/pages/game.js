import NetworkGameController from '../core/networkGameController.js';
import { waitForPywebview } from '../utils/pywebview.js';
import * as timer from './game/timerManager.js';
import * as chat from './game/chatManager.js';
import * as spectate from './game/spectateManager.js';
import {
    setupNetworkListeners,
    setupButtonHandlers,
    setupRematchHandlers,
    setupMoveHistoryHandler,
} from './game/networkHandlers.js';

const userDataStr = localStorage.getItem('xiangqi_user');
const matchDataStr = localStorage.getItem('current_match');
const spectateDataStr = localStorage.getItem('spectate_match');

const urlParams = new URLSearchParams(window.location.search);
const isSpectateMode = urlParams.get('mode') === 'spectate';

if (!userDataStr) {
    window.location.href = '../index.html';
}

const userData = JSON.parse(userDataStr);
let matchData = matchDataStr ? JSON.parse(matchDataStr) : null;
let spectateData = spectateDataStr ? JSON.parse(spectateDataStr) : null;

const gameController = new NetworkGameController('xiangqi-board');
window.gameController = gameController;

let isGameActive = true;
let oldRating = userData.rating || 1500;
let pendingRematchMatchId = null;

const initialTimeMs = matchData?.time_per_player || 600000;

timer.initTimerManager({
    initialTimeMs,
    onTimeout: handleTimeout,
    getIsMyTurn: () => gameController.isMyTurn,
    getIsSpectator: () => gameController.isSpectator,
    getBoardTurn: () => gameController.chessboard?.turn || 'red',
    getMatchData: () => matchData,
});

async function handleTimeout(who) {}

gameController.on('game-over', (data) => {
    let result = 'draw';
    const myColor = matchData ? matchData.your_color : gameController.myColor || 'red';

    if (data.winner) {
        result = data.winner === myColor ? 'win' : 'loss';
    }

    const durationStr = timer.getMatchDurationString();
    const resultTime = document.getElementById('result-time');
    if (resultTime) resultTime.textContent = durationStr;

    showResult(result, data.reason);
    isGameActive = false;
    timer.setGameActive(false);
});

if (isSpectateMode && spectateData) {
    spectate.initSpectateManager(gameController, spectateData);
    spectate.setupSpectateUI();
} else if (matchData) {
    gameController.matchId = matchData.match_id;
    gameController.myColor = matchData.your_color;
    gameController.isOnlineMode = true;
    gameController.isMyTurn = matchData.your_color === 'red';

    const playerAvatar = document.getElementById('player-avatar');
    const opponentAvatar = document.getElementById('opponent-avatar');
    if (playerAvatar && opponentAvatar) {
        playerAvatar.classList.remove('avatar-red', 'avatar-black');
        opponentAvatar.classList.remove('avatar-red', 'avatar-black');

        if (matchData.your_color === 'red') {
            playerAvatar.textContent = '🔴';
            opponentAvatar.textContent = '⚫';
        } else {
            playerAvatar.textContent = '⚫';
            opponentAvatar.textContent = '🔴';
        }
    }
}

function updateTurnIndicator() {
    const turnText = document.getElementById('turn-text');
    const playerPanel = document.getElementById('player-panel');
    const opponentPanel = document.getElementById('opponent-panel');

    if (gameController.isMyTurn) {
        turnText.innerHTML = '<span style="color:#27ae60">Đến lượt bạn</span>';
        playerPanel?.classList.add('active');
        opponentPanel?.classList.remove('active');
    } else {
        turnText.innerHTML = '<span style="color:#f39c12">Đối thủ đang nghĩ...</span>';
        playerPanel?.classList.remove('active');
        opponentPanel?.classList.add('active');
    }
}

function showResult(result, reason) {
    isGameActive = false;
    timer.setGameActive(false);
    gameController.isOnlineMode = false;

    const modal = document.getElementById('result-modal');
    const title = document.getElementById('result-title');

    if (result === 'win') {
        title.textContent = 'CHIẾN THẮNG!';
        title.style.color = '#ffd700';
    } else if (result === 'loss') {
        title.textContent = 'THẤT BẠI';
        title.style.color = '#ff4444';
    } else {
        title.textContent = '🤝 HÒA';
        title.style.color = '#4a90e2';
    }

    modal?.classList.add('active');

    let countdown = 5;
    const btnLobby = document.getElementById('btn-to-lobby');

    const autoExitInterval = setInterval(() => {
        countdown--;
        if (btnLobby) btnLobby.textContent = `Về Lobby (${countdown}s)`;
        if (countdown <= 0) {
            clearInterval(autoExitInterval);
            exitToLobby();
        }
    }, 1000);
}

function exitToLobby() {
    localStorage.removeItem('current_match');
    if (window.gameController) window.gameController.disconnect();
    window.location.href = 'lobby.html';
}

(async () => {
    try {
        await waitForPywebview();

        const serverInfoStr = localStorage.getItem('xiangqi_server');
        if (!serverInfoStr) {
            alert('Mất thông tin server, vui lòng đăng nhập lại.');
            exitToLobby();
            return;
        }

        const serverInfo = JSON.parse(serverInfoStr);
        const connected = await gameController.connectToServer(serverInfo.host, serverInfo.port);

        if (!connected) {
            throw new Error('Không thể kết nối Server Game');
        }

        if (userData.token) {
            gameController.token = userData.token;
            if (gameController.network) gameController.network.sessionToken = userData.token;
        }

        // Spectate mode
        if (isSpectateMode && spectateData) {
            try {
                await spectate.joinSpectate(timer.syncTimerFromServer, updateTurnIndicator);
                spectate.setupSpectateListeners(gameController, timer.syncTimerFromServer);
            } catch (error) {
                alert('Không thể vào xem trận đấu: ' + error.message);
                window.location.href = 'lobby.html';
            }
        }
        // Normal match mode
        else if (matchData) {
            const joinResult = await gameController.joinMatch(matchData.match_id);

            if (joinResult && joinResult.payload) {
                const state =
                    typeof joinResult.payload === 'string' ? JSON.parse(joinResult.payload) : joinResult.payload;
                if (state.is_my_turn !== undefined) {
                    gameController.isMyTurn = state.is_my_turn;
                    updateTurnIndicator();
                }
            }

            const shouldFlip = matchData.your_color === 'black';
            gameController.setupBoard({ flipped: shouldFlip });
            if (gameController.ui && gameController.ui.flipBoard) {
                gameController.ui.flipBoard(shouldFlip);
            }
        }

        // Setup network event listeners
        setupNetworkListeners(gameController, matchData, userData, {
            updateTurnIndicator,
            showResult,
            exitToLobby,
            oldRating,
        });

        // Initialize chat
        chat.initChatManager(gameController, userData.username);
    } catch (e) {
        console.error('Setup failed:', e);
        alert('Lỗi kết nối: ' + e.message);
        exitToLobby();
    }
})();

document.getElementById('player-name').textContent = userData.username;
document.getElementById('player-rating').textContent = `${userData.rating || 1500}`;

if (matchData) {
    let opponentName = 'Đối thủ';
    if (matchData.your_color === 'red') {
        opponentName = matchData.black_user || matchData.opponent_name || 'Đối thủ';
    } else {
        opponentName = matchData.red_user || matchData.opponent_name || 'Đối thủ';
    }
    document.getElementById('opponent-name').textContent = opponentName;
    document.getElementById('opponent-rating').textContent = `${matchData.opponent_rating || 1500}`;
}

setupButtonHandlers(gameController, {
    isGameActive: () => isGameActive,
    setGameActive: (val) => {
        isGameActive = val;
    },
    exitToLobby,
});

setupRematchHandlers(gameController, {
    exitToLobby,
    getPendingRematchMatchId: () => pendingRematchMatchId,
    setPendingRematchMatchId: (val) => {
        pendingRematchMatchId = val;
    },
});

setupMoveHistoryHandler(gameController);
timer.setupPeriodicSync(gameController, gameController.matchId, isSpectateMode);
timer.startTimer();
updateTurnIndicator();
