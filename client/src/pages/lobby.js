import NetworkGameController from '../core/networkGameController.js';
import { waitForPywebview } from '../utils/pywebview.js';
import * as roomManager from './lobby/roomManager.js';
import * as matchmaking from './lobby/matchmakingManager.js';
import * as profile from './lobby/profileManager.js';

let isConnected = false;

window.openModal = function (modalId) {
    document.getElementById(modalId)?.classList.add('active');
};

window.closeModal = function (modalId) {
    document.getElementById(modalId)?.classList.remove('active');
};

function showMessage(type, text) {
    const msgDiv = document.getElementById('status-message');
    if (msgDiv) {
        msgDiv.className = `status-message ${type}`;
        msgDiv.textContent = text;
        msgDiv.style.display = 'block';

        setTimeout(() => {
            msgDiv.style.display = 'none';
        }, 5000);
    }
}

window.refreshRoomList = async function () {
    await roomManager.refreshRoomList(window.gameController?.network, showMessage);
};

window.filterRoomList = function () {
    roomManager.filterRoomList();
};

window.refreshLiveMatches = async function () {
    await profile.refreshLiveMatches(window.gameController?.network, showMessage);
};

window.joinRoom = async function (roomCode, hasPassword) {
    if (hasPassword) {
        document.getElementById('join-room-code').value = roomCode;
        closeModal('room-list-modal');
        openModal('join-password-modal');
    } else {
        await roomManager.joinRoom(window.gameController?.network, roomCode, '', showMessage, openModal, closeModal);
    }
};

window.leaveRoom = async function () {
    await roomManager.leaveRoom(window.gameController?.network, showMessage, closeModal);
};

window.copyUserId = function () {
    profile.copyUserId(showMessage);
};

async function loadProfile() {
    await profile.loadProfile(window.gameController?.network, showMessage);
}

function handleMenuAction(action) {
    switch (action) {
        case 'create-room':
            openModal('create-room-modal');
            break;
        case 'room-list':
            openModal('room-list-modal');
            window.refreshRoomList();
            break;
        case 'history':
            window.location.href = 'replay.html';
            break;
        case 'spectate':
            openModal('live-matches-modal');
            window.refreshLiveMatches();
            break;
        case 'profile':
            openModal('profile-modal');
            loadProfile();
            break;
        default:
            showMessage('info', 'Chức năng đang phát triển');
    }
}

function setupNetworkListeners(gameController) {
    if (!gameController.network) return;

    gameController.network.on('reconnected', () => {
        const userData = localStorage.getItem('xiangqi_user');
        if (userData) {
            try {
                const user = JSON.parse(userData);
                if (user.token) {
                    gameController.token = user.token;
                    gameController.network.sessionToken = user.token;
                }
            } catch (e) {
                console.warn('[Lobby] Failed to refresh token:', e);
            }
        }
    });

    gameController.network.on('room_guest_joined', (msg) => {
        roomManager.handleGuestJoined(msg.payload, showMessage);
    });

    gameController.network.on('room_guest_left', (msg) => {
        roomManager.handleGuestLeft(showMessage);
    });

    gameController.network.on('room_closed', (msg) => {
        roomManager.handleRoomClosed(showMessage, closeModal);
    });

    gameController.network.on('rooms_update', (msg) => {
        if (document.getElementById('room-list-modal')?.classList.contains('active')) {
            roomManager.displayRoomList(msg.payload || []);
        }
    });

    gameController.network.on('match_found', (msg) => {
        const currentRoom = roomManager.getCurrentRoom();
        if (currentRoom) {
            localStorage.setItem('current_match', JSON.stringify(msg.payload));
            closeModal('waiting-room-modal');
            roomManager.clearRoom();
            setTimeout(() => {
                window.location.href = 'game.html';
            }, 300);
        }
    });
}

const btnFindMatch = document.getElementById('btn-find-match');
if (btnFindMatch) {
    btnFindMatch.disabled = true;
    btnFindMatch.textContent = 'Đang kết nối...';
}

(async () => {
    await waitForPywebview();

    const userDataStr = localStorage.getItem('xiangqi_user');
    if (!userDataStr) {
        window.location.href = 'login.html';
        return;
    }

    const userData = JSON.parse(userDataStr);
    document.getElementById('user-name').textContent = userData.username;
    document.getElementById('user-rating').textContent = `Rating: ${userData.rating || 1500}`;
    document.getElementById('user-avatar').textContent = userData.username.charAt(0).toUpperCase();
    let gameController = window.gameController;

    if (!gameController || !gameController.network) {
        const serverInfoStr = localStorage.getItem('xiangqi_server');
        if (!serverInfoStr) {
            showMessage('error', 'Chưa kết nối server. Vui lòng đăng nhập lại.');
            setTimeout(() => {
                window.location.href = 'login.html';
            }, 2000);
            return;
        }

        const serverInfo = JSON.parse(serverInfoStr);
        gameController = new NetworkGameController();

        try {
            const connected = await gameController.connectToServer(serverInfo.host, serverInfo.port);
            if (!connected) {
                throw new Error('Không thể kết nối lại server');
            }

            const freshUserData = JSON.parse(localStorage.getItem('xiangqi_user') || '{}');
            if (freshUserData.token) {
                gameController.token = freshUserData.token;
                if (gameController.network) {
                    gameController.network.sessionToken = freshUserData.token;
                }
            }

            window.gameController = gameController;
            isConnected = true;

            if (btnFindMatch) {
                btnFindMatch.disabled = false;
                btnFindMatch.textContent = '⚡ Tìm Trận';
            }
        } catch (error) {
            console.error('Reconnection failed:', error);
            showMessage('error', 'Mất kết nối. Đang thử lại...');
            if (btnFindMatch) btnFindMatch.textContent = 'Lỗi kết nối';
            setTimeout(() => window.location.reload(), 3000);
            return;
        }
    } else {
        isConnected = true;
        if (btnFindMatch) {
            btnFindMatch.disabled = false;
            btnFindMatch.textContent = '⚡ Tìm Trận';
        }
    }

    matchmaking.setupMatchFoundCallback(gameController);

    document.querySelectorAll('.menu-btn').forEach((btn) => {
        btn.addEventListener('click', () => {
            const action = btn.dataset.action;
            handleMenuAction(action);
        });
    });

    if (btnFindMatch) {
        btnFindMatch.addEventListener('click', async () => {
            await matchmaking.startMatchSearch(gameController, showMessage);
        });
    }

    document.getElementById('btn-cancel-search')?.addEventListener('click', async () => {
        await matchmaking.cancelMatchSearch(gameController, showMessage);
    });
    document.getElementById('btn-start-game')?.addEventListener('click', () => {
        window.location.href = 'game.html';
    });

    document.getElementById('btn-logout')?.addEventListener('click', () => {
        if (confirm('Bạn có chắc muốn đăng xuất?')) {
            localStorage.removeItem('xiangqi_user');
            localStorage.removeItem('current_match');
            window.location.href = 'login.html';
        }
    });

    document.getElementById('create-room-form')?.addEventListener('submit', async (e) => {
        e.preventDefault();

        const password = document.getElementById('room-password').value;
        await roomManager.createRoom(
            window.gameController?.network,
            password,
            true,
            showMessage,
            openModal,
            closeModal,
        );
        document.getElementById('room-password').value = '';
    });

    document.getElementById('join-password-form')?.addEventListener('submit', async (e) => {
        e.preventDefault();

        const roomCode = document.getElementById('join-room-code').value;
        const password = document.getElementById('join-room-password').value;

        await roomManager.joinRoom(
            window.gameController?.network,
            roomCode,
            password,
            showMessage,
            openModal,
            closeModal,
        );
        document.getElementById('join-room-password').value = '';
    });

    document.getElementById('btn-start-room-game')?.addEventListener('click', async () => {
        await roomManager.startRoomGame(window.gameController?.network, showMessage);
    });
    setupNetworkListeners(gameController);
    setTimeout(() => profile.loadLeaderboard(gameController), 500);
})();
