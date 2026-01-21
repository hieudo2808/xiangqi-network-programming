import NetworkGameController from '../core/networkGameController.js';
import { Board } from '../core/board.js';
import { UI } from '../ui/renderer.js';

let gameController = null;
let currentMatchData = null;
let moves = [];
let currentMoveIndex = -1;
let isPlaying = false;
let playInterval = null;
let board = null;
let userData = null;
let replayBoard = null;
let replayUI = null;

const OPENING_POSITION = [
    ['chariot', 'red', 9, 0],
    ['horse', 'red', 9, 1],
    ['elephant', 'red', 9, 2],
    ['advisor', 'red', 9, 3],
    ['general', 'red', 9, 4],
    ['advisor', 'red', 9, 5],
    ['elephant', 'red', 9, 6],
    ['horse', 'red', 9, 7],
    ['chariot', 'red', 9, 8],
    ['cannon', 'red', 7, 1],
    ['cannon', 'red', 7, 7],
    ['pawn', 'red', 6, 0],
    ['pawn', 'red', 6, 2],
    ['pawn', 'red', 6, 4],
    ['pawn', 'red', 6, 6],
    ['pawn', 'red', 6, 8],

    ['chariot', 'black', 0, 0],
    ['horse', 'black', 0, 1],
    ['elephant', 'black', 0, 2],
    ['advisor', 'black', 0, 3],
    ['general', 'black', 0, 4],
    ['advisor', 'black', 0, 5],
    ['elephant', 'black', 0, 6],
    ['horse', 'black', 0, 7],
    ['chariot', 'black', 0, 8],
    ['cannon', 'black', 2, 1],
    ['cannon', 'black', 2, 7],
    ['pawn', 'black', 3, 0],
    ['pawn', 'black', 3, 2],
    ['pawn', 'black', 3, 4],
    ['pawn', 'black', 3, 6],
    ['pawn', 'black', 3, 8],
];

(async () => {
    try {
        const waitForPywebview = () => {
            return new Promise((resolve) => {
                const isReady = () =>
                    window.pywebview && window.pywebview.api && typeof window.pywebview.api.connect === 'function';
                if (isReady()) {
                    resolve();
                } else {
                    window.addEventListener('pywebviewready', () => setTimeout(() => resolve(), 100));
                    const checkInterval = setInterval(() => {
                        if (isReady()) {
                            clearInterval(checkInterval);
                            resolve();
                        }
                    }, 100);
                }
            });
        };

        await waitForPywebview();

        const userDataStr = localStorage.getItem('xiangqi_user');
        if (!userDataStr) {
            window.location.href = '../index.html';
            return;
        }
        userData = JSON.parse(userDataStr);

        const serverInfoStr = localStorage.getItem('xiangqi_server');
        if (!serverInfoStr) {
            alert('Chưa kết nối server');
            window.location.href = '../index.html';
            return;
        }

        const serverInfo = JSON.parse(serverInfoStr);

        gameController = new NetworkGameController();
        await gameController.connectToServer(serverInfo.host, serverInfo.port);

        if (userData.token && gameController.network) {
            gameController.network.sessionToken = userData.token;
        }

        await loadMatchHistory();

        setupBackButton();
    } catch (error) {
        console.error('Initialization failed:', error);
        document.getElementById('match-list').innerHTML =
            '<div class="no-matches"><p>Lỗi khởi động: ' + error.message + '</p></div>';
    }
})();

async function loadMatchHistory() {
    try {
        const matchListEl = document.getElementById('match-list');
        matchListEl.innerHTML = '<div class="loading"><div class="spinner"></div><p>Đang tải...</p></div>';

        const response = await gameController.network.getMatchHistory(50, 0);
        const matches = response.payload?.matches || [];

        displayMatchList(matches);
    } catch (error) {
        console.error('Failed to load history:', error);
        console.error('[Replay] Error details:', error.message, error.stack);
        document.getElementById('match-list').innerHTML =
            '<div class="no-matches"><p>Không thể tải lịch sử: ' + error.message + '</p></div>';
    }
}

function displayMatchList(matches) {
    const container = document.getElementById('match-list');

    if (!matches || matches.length === 0) {
        container.innerHTML = '<div class="no-matches"><p>Chưa có trận đấu nào</p></div>';
        return;
    }

    matches.sort((a, b) => {
        const timeA = !isNaN(a.ended_at) ? parseInt(a.ended_at) : new Date(a.ended_at).getTime() / 1000;
        const timeB = !isNaN(b.ended_at) ? parseInt(b.ended_at) : new Date(b.ended_at).getTime() / 1000;
        return timeB - timeA;
    });

    container.innerHTML = matches
        .map((match) => {
            const resultClass = match.result === 'win' ? 'win' : match.result === 'loss' ? 'loss' : 'draw';
            const resultText = match.result === 'win' ? 'Thắng' : match.result === 'loss' ? 'Thua' : 'Hòa';

            const colorIcon = match.my_color === 'red' ? '🔴' : '⚫';

            let date;
            const endedAt = match.ended_at;
            if (!endedAt || endedAt === 'null') {
                date = new Date();
            } else if (!isNaN(endedAt)) {
                date = new Date(parseInt(endedAt) * 1000);
            } else {
                date = new Date(endedAt);
            }

            const dateStr = isNaN(date.getTime())
                ? 'N/A'
                : date.toLocaleDateString('vi-VN', {
                      year: 'numeric',
                      month: 'short',
                      day: 'numeric',
                      hour: '2-digit',
                      minute: '2-digit',
                  });

            return `
            <div class="match-item" data-match-id="${match.match_id}">
                <div class="opponent">${colorIcon} vs ${match.opponent}</div>
                <div class="details">
                    <span>${dateStr}</span>
                    <span class="result ${resultClass}">${resultText}</span>
                </div>
            </div>
        `;
        })
        .join('');

    container.querySelectorAll('.match-item').forEach((item) => {
        item.addEventListener('click', () => {
            container.querySelectorAll('.match-item').forEach((i) => i.classList.remove('active'));
            item.classList.add('active');

            loadMatchReplay(item.dataset.matchId);
        });
    });
}

async function loadMatchReplay(matchId) {
    try {
        const boardPanel = document.getElementById('replay-board-panel');
        boardPanel.innerHTML = '<div class="loading"><div class="spinner"></div><p>Đang tải trận đấu...</p></div>';
        document.getElementById('moves-panel').style.display = 'none';

        const response = await gameController.network.getMatch(matchId);
        currentMatchData = response.payload;

        if (!currentMatchData) {
            throw new Error('Không thể tải dữ liệu trận đấu');
        }

        if (typeof currentMatchData.moves === 'string') {
            moves = JSON.parse(currentMatchData.moves);
        } else {
            moves = currentMatchData.moves || [];
        }

        currentMoveIndex = -1;
        isPlaying = false;
        if (playInterval) clearInterval(playInterval);

        createReplayUI();
    } catch (error) {
        console.error('Failed to load match:', error);
        const boardPanel = document.getElementById('replay-board-panel');
        boardPanel.innerHTML = `
            <div class="no-match-selected">
                <div class="icon"></div>
                <h2>Không thể tải trận đấu</h2>
                <p>${error.message}</p>
            </div>
        `;
    }
}

function createReplayUI() {
    const boardPanel = document.getElementById('replay-board-panel');

    const resultText =
        currentMatchData.result === 'red_win'
            ? 'Đỏ thắng'
            : currentMatchData.result === 'black_win'
              ? 'Đen thắng'
              : 'Hòa';

    boardPanel.innerHTML = `
        <div class="match-info-bar">
            <div class="player-info red">
                <div class="name">🔴 ${currentMatchData.red_user}</div>
                <div class="color">Quân Đỏ</div>
            </div>
            <div class="vs-badge">${resultText}</div>
            <div class="player-info black">
                <div class="name">⚫ ${currentMatchData.black_user}</div>
                <div class="color">Quân Đen</div>
            </div>
        </div>

        <div class="board-container">
            <div id="replay-board"></div>
        </div>

        <div class="replay-controls">
            <button class="control-btn" id="btn-first" title="Đầu tiên">⏮</button>
            <button class="control-btn" id="btn-prev" title="Nước trước">◀</button>
            <button class="control-btn play-pause" id="btn-play" title="Phát/Dừng">▶</button>
            <button class="control-btn" id="btn-next" title="Nước sau">▶</button>
            <button class="control-btn" id="btn-last" title="Cuối cùng">⏭</button>
            <input type="range" class="move-slider" id="move-slider" min="-1" max="${moves.length - 1}" value="-1">
            <div class="move-counter" id="move-counter">0 / ${moves.length}</div>
        </div>
    `;

    initializeReplayBoard();

    setupReplayControls();

    displayMovesList();
    document.getElementById('moves-panel').style.display = '';
}

function initializeReplayBoard() {
    replayBoard = new Board();
    replayBoard.initBoard(OPENING_POSITION);
    replayBoard.status = true;
    replayBoard.turn = 'red';

    replayUI = new UI('replay-board');
    replayUI.renderBoard(replayBoard.board);
}

function setupReplayControls() {
    const btnFirst = document.getElementById('btn-first');
    const btnPrev = document.getElementById('btn-prev');
    const btnPlay = document.getElementById('btn-play');
    const btnNext = document.getElementById('btn-next');
    const btnLast = document.getElementById('btn-last');
    const slider = document.getElementById('move-slider');

    if (btnFirst) btnFirst.addEventListener('click', goToFirst);
    if (btnPrev) btnPrev.addEventListener('click', goToPrev);
    if (btnPlay) btnPlay.addEventListener('click', togglePlay);
    if (btnNext) btnNext.addEventListener('click', goToNext);
    if (btnLast) btnLast.addEventListener('click', goToLast);
    if (slider)
        slider.addEventListener('input', (e) => {
            goToMove(parseInt(e.target.value));
        });
}

function goToFirst() {
    goToMove(-1);
}

function goToPrev() {
    if (currentMoveIndex >= 0) {
        goToMove(currentMoveIndex - 1);
    }
}

function goToNext() {
    if (currentMoveIndex < moves.length - 1) {
        goToMove(currentMoveIndex + 1);
    }
}

function goToLast() {
    goToMove(moves.length - 1);
}

function goToMove(index) {
    if (index < -1) index = -1;
    if (index >= moves.length) index = moves.length - 1;

    replayBoard.board = [];
    replayBoard.initBoard(OPENING_POSITION);

    for (let i = 0; i <= index && i < moves.length; i++) {
        const move = moves[i];
        try {
            const piece = replayBoard.board[move.from.row][move.from.col];
            if (piece) {
                replayBoard.board[move.to.row][move.to.col] = piece;
                replayBoard.board[move.from.row][move.from.col] = null;
            }
        } catch (e) {
            console.error('Error applying move:', move, e);
        }
    }

    currentMoveIndex = index;
    updateReplayUI();
}

function togglePlay() {
    isPlaying = !isPlaying;
    const btn = document.getElementById('btn-play');

    if (isPlaying && btn) {
        btn.textContent = '⏸';
        playInterval = setInterval(() => {
            if (currentMoveIndex < moves.length - 1) {
                goToNext();
            } else {
                isPlaying = false;
                if (btn) btn.textContent = '▶';
                if (playInterval) clearInterval(playInterval);
            }
        }, 1500);
    } else {
        if (btn) btn.textContent = '▶';
        if (playInterval) {
            clearInterval(playInterval);
            playInterval = null;
        }
    }
}

function updateReplayUI() {
    const slider = document.getElementById('move-slider');
    if (slider) slider.value = currentMoveIndex;

    const counter = document.getElementById('move-counter');
    if (counter) {
        counter.textContent = `${currentMoveIndex + 1} / ${moves.length}`;
    }

    const btnFirst = document.getElementById('btn-first');
    const btnPrev = document.getElementById('btn-prev');
    const btnNext = document.getElementById('btn-next');
    const btnLast = document.getElementById('btn-last');

    if (btnFirst) btnFirst.disabled = currentMoveIndex < 0;
    if (btnPrev) btnPrev.disabled = currentMoveIndex < 0;
    if (btnNext) btnNext.disabled = currentMoveIndex >= moves.length - 1;
    if (btnLast) btnLast.disabled = currentMoveIndex >= moves.length - 1;

    document.querySelectorAll('.move-item').forEach((item, i) => {
        item.classList.toggle('active', i === currentMoveIndex);
    });

    if (replayUI && replayBoard) {
        replayUI.renderBoard(replayBoard.board);
    }
}

function displayMovesList() {
    const container = document.getElementById('moves-list');

    if (!moves || moves.length === 0) {
        if (container) {
            container.innerHTML = '<p style="color:#666;text-align:center;">Không có nước đi</p>';
        }
        return;
    }

    if (container) {
        container.innerHTML = moves
            .map((move, i) => {
                const colorClass = move.color || (i % 2 === 0 ? 'red' : 'black');
                const colorName = colorClass === 'red' ? '🔴 Đỏ' : '⚫ Đen';

                const moveText = `(${move.from.row},${move.from.col}) → (${move.to.row},${move.to.col})`;

                return `
                <div class="move-item" data-index="${i}">
                    <span class="num">${i + 1}.</span>
                    <span class="move-text ${colorClass}">${colorName}: ${moveText}</span>
                </div>
            `;
            })
            .join('');

        container.querySelectorAll('.move-item').forEach((item) => {
            item.addEventListener('click', () => {
                const index = parseInt(item.dataset.index);
                goToMove(index);
            });
        });
    }
}

function setupBackButton() {
    const backBtn = document.getElementById('btn-back');
    if (backBtn) {
        backBtn.addEventListener('click', () => {
            if (isPlaying) togglePlay();
            window.location.href = 'lobby.html';
        });
    }
}

window.addEventListener('beforeunload', () => {
    if (playInterval) clearInterval(playInterval);
    if (gameController && gameController.network) {
        gameController.network.disconnect();
    }
});
