let currentRoom = null;
let isHost = false;

export function getCurrentRoom() {
    return currentRoom;
}

export function isRoomHost() {
    return isHost;
}

export function setCurrentRoom(roomCode, asHost = false) {
    currentRoom = roomCode;
    isHost = asHost;
}

export function clearRoom() {
    currentRoom = null;
    isHost = false;
}

export async function refreshRoomList(network, showMessage) {
    if (!network) {
        showMessage('error', 'Chưa kết nối server');
        return;
    }

    const container = document.getElementById('room-list-container');
    container.innerHTML = '<div class="no-rooms"><p>Đang tải...</p></div>';

    try {
        const response = await network.getRooms();
        const rooms = response.payload?.rooms || [];
        window._cachedRooms = rooms;
        displayRoomList(rooms);
    } catch (error) {
        container.innerHTML = '<div class="no-rooms"><p>Không thể tải danh sách phòng</p></div>';
    }
}

export function filterRoomList() {
    const searchInput = document.getElementById('room-search-input');
    const searchTerm = searchInput.value.toLowerCase().trim();
    const rooms = window._cachedRooms || [];

    if (!searchTerm) {
        displayRoomList(rooms);
        return;
    }

    const filteredRooms = rooms.filter(
        (room) =>
            room.room_code?.toLowerCase().includes(searchTerm) || room.host_name?.toLowerCase().includes(searchTerm),
    );

    displayRoomList(filteredRooms);
}

/**
 * Display room list in container
 */
export function displayRoomList(rooms) {
    const container = document.getElementById('room-list-container');

    if (!rooms || rooms.length === 0) {
        container.innerHTML = `
            <div class="no-rooms">
                <p>Chưa có phòng nào</p>
                <p>Hãy tạo phòng mới!</p>
            </div>`;
        return;
    }

    let html = '';
    for (const room of rooms) {
        const hasPassword = room.has_password;
        const statusClass = room.is_full ? 'full' : 'open';
        const statusText = room.is_full ? 'Đầy' : 'Mở';
        const lockIcon = hasPassword ? '🔒' : '';

        html += `
            <div class="room-item ${room.is_full ? 'disabled' : ''}" 
                 onclick="${room.is_full ? '' : `window.joinRoom('${room.room_code}', ${hasPassword})`}">
                <div class="room-code">${lockIcon} ${room.room_code}</div>
                <div class="room-host">Host: ${room.host_name}</div>
                <div class="room-rating">Rating: ${room.host_rating || 1500}</div>
                <div class="room-status ${statusClass}">${statusText}</div>
            </div>
        `;
    }

    container.innerHTML = html;
}

export async function createRoom(network, password, rated, showMessage, openModal, closeModal) {
    if (!network) {
        showMessage('error', 'Chưa kết nối server');
        return null;
    }

    try {
        const response = await network.createRoom('', password, rated);
        const roomCode = response.payload.room_code;

        setCurrentRoom(roomCode, true);
        updateWaitingRoomUI(true);

        closeModal('create-room-modal');
        openModal('waiting-room-modal');
        showMessage('success', 'Phòng đã được tạo!');

        return roomCode;
    } catch (error) {
        showMessage('error', `Không thể tạo phòng: ${error.message}`);
        return null;
    }
}

export async function joinRoom(network, roomCode, password, showMessage, openModal, closeModal) {
    try {
        const response = await network.joinRoom(roomCode, password);

        if (response.success) {
            setCurrentRoom(roomCode, false);

            const hostInfo = response.payload || {};
            updateWaitingRoomUI(false, hostInfo);

            closeModal('join-password-modal');
            closeModal('room-list-modal');
            openModal('waiting-room-modal');

            showMessage('success', 'Đã tham gia phòng!');
            return true;
        } else {
            showMessage('error', response.message || 'Không thể tham gia phòng');
            return false;
        }
    } catch (error) {
        showMessage('error', `Lỗi: ${error.message}`);
        return false;
    }
}

export async function leaveRoom(network, showMessage, closeModal) {
    if (!currentRoom) {
        closeModal('waiting-room-modal');
        return;
    }

    try {
        await network.leaveRoom(currentRoom);
    } catch (e) {
        console.warn('Leave room error:', e);
    }

    clearRoom();
    closeModal('waiting-room-modal');
    showMessage('info', 'Đã rời phòng');
}

export function updateWaitingRoomUI(asHost, hostInfo = null) {
    const userData = JSON.parse(localStorage.getItem('xiangqi_user') || '{}');

    document.getElementById('waiting-room-code').textContent = currentRoom;

    if (asHost) {
        document.getElementById('host-name').textContent = userData.username || 'You';
        document.getElementById('host-rating').textContent = `${userData.rating || 1500}`;
        document.getElementById('guest-name').innerHTML = '<span class="waiting-spinner"></span>Đang chờ...';
        document.getElementById('guest-rating').textContent = '';
        document.getElementById('guest-slot').classList.remove('filled');
        document.getElementById('guest-slot').classList.add('empty');
        document.getElementById('waiting-room-status').innerHTML = '<p>Đang chờ đối thủ tham gia...</p>';
        document.getElementById('btn-start-room-game').disabled = true;
        document.getElementById('btn-start-room-game').style.display = '';
    } else {
        document.getElementById('host-name').textContent = hostInfo.host_name || 'Host';
        document.getElementById('host-rating').textContent = `${hostInfo.host_rating || 1500}`;
        document.getElementById('guest-name').textContent = userData.username || 'You';
        document.getElementById('guest-rating').textContent = `${userData.rating || 1500}`;
        document.getElementById('guest-slot').classList.add('filled');
        document.getElementById('guest-slot').classList.remove('empty');
        document.getElementById('waiting-room-status').innerHTML = '<p>Đang chờ host bắt đầu game...</p>';
        document.getElementById('btn-start-room-game').style.display = 'none';
    }
}

export function handleGuestJoined(guest, showMessage) {
    document.getElementById('guest-name').textContent = guest.guest_name || 'Guest';
    document.getElementById('guest-rating').textContent = `${guest.guest_rating || 1500}`;
    document.getElementById('guest-slot').classList.add('filled');
    document.getElementById('guest-slot').classList.remove('empty');
    document.getElementById('waiting-room-status').innerHTML = '<p>Đối thủ đã tham gia! Sẵn sàng bắt đầu.</p>';
    document.getElementById('btn-start-room-game').disabled = false;
    showMessage('success', `${guest.guest_name} đã tham gia phòng!`);
}

export function handleGuestLeft(showMessage) {
    document.getElementById('guest-name').innerHTML = '<span class="waiting-spinner"></span>Đang chờ...';
    document.getElementById('guest-rating').textContent = '';
    document.getElementById('guest-slot').classList.remove('filled');
    document.getElementById('guest-slot').classList.add('empty');
    document.getElementById('waiting-room-status').innerHTML = '<p>Đang chờ đối thủ tham gia...</p>';
    document.getElementById('btn-start-room-game').disabled = true;
    showMessage('info', 'Đối thủ đã rời phòng');
}

export function handleRoomClosed(showMessage, closeModal) {
    clearRoom();
    closeModal('waiting-room-modal');
    showMessage('info', 'Host đã đóng phòng');
}

export async function startRoomGame(network, showMessage) {
    if (!currentRoom || !isHost) return;

    try {
        network.startRoomGame(currentRoom);
    } catch (error) {
        showMessage('error', `Không thể bắt đầu: ${error.message}`);
    }
}
