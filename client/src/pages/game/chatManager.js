let gameController = null;
let currentUsername = '';

export function initChatManager(controller, username) {
    gameController = controller;
    currentUsername = username;

    const btnSend = document.getElementById('btn-send-chat');
    const inputChat = document.getElementById('chat-input');

    if (btnSend) {
        btnSend.addEventListener('click', sendChat);
    }

    if (inputChat) {
        inputChat.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') sendChat();
        });
    }
}

export async function sendChat() {
    const input = document.getElementById('chat-input');
    if (!input) return;

    const msg = input.value.trim();
    if (!msg) return;

    try {
        await gameController.sendChatMessage(msg);
        addChatMessage(currentUsername, msg, true);
        input.value = '';
    } catch (e) {
        console.error('Lỗi gửi tin nhắn:', e);
    }
}

export function addChatMessage(senderName, msg, isMe) {
    const container = document.getElementById('chat-messages');
    if (!container) return;

    if (container.children.length > 0 && container.children[0].innerText === 'Trò chuyện với đối thủ') {
        container.innerHTML = '';
    }

    const msgWrapper = document.createElement('div');
    msgWrapper.className = `chat-message ${isMe ? 'me' : 'opponent'}`;

    const senderDiv = document.createElement('div');
    senderDiv.className = 'message-sender';
    senderDiv.textContent = senderName;

    const bubbleDiv = document.createElement('div');
    bubbleDiv.className = 'message-bubble';
    bubbleDiv.textContent = msg;

    msgWrapper.appendChild(senderDiv);
    msgWrapper.appendChild(bubbleDiv);
    container.appendChild(msgWrapper);

    container.scrollTop = container.scrollHeight;
}

export function setupChatListener(controller) {
    if (!controller.network) return;

    controller.network.on('chat_message', (data) => {
        const payload = data.payload || data;
        const senderName = payload.sender_name || payload.username || 'Opponent';
        const message = payload.message || payload.msg || '';

        if (message) {
            addChatMessage(senderName, message, false);
        }
    });
}
