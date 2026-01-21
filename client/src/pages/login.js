import NetworkGameController from '../core/networkGameController.js';
import { waitForPywebview } from '../utils/pywebview.js';

let gameController = null;

document.querySelectorAll('.tab-btn').forEach((btn) => {
    btn.addEventListener('click', () => {
        const tabName = btn.dataset.tab;

        document.querySelectorAll('.tab-btn').forEach((b) => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach((c) => c.classList.remove('active'));

        btn.classList.add('active');
        document.getElementById(`tab-${tabName}`).classList.add('active');
    });
});

function updateConnectionStatus(status, message) {
    const indicator = document.getElementById('conn-indicator');
    const statusDiv = document.getElementById('connection-status');

    indicator.className = `connection-indicator ${status}`;
    statusDiv.textContent = message;
    statusDiv.className = `status-message ${
        status === 'connected' ? 'success' : status === 'connecting' ? 'info' : 'error'
    }`;
}

document.getElementById('btn-connect').addEventListener('click', async () => {
    let host = document.getElementById('server-host').value.trim();
    host = host.replace(/^(tcp|ws|wss|http|https):\/\//i, '');
    const port = document.getElementById('server-port').value;
    const btn = document.getElementById('btn-connect');

    if (!host || !port) {
        updateConnectionStatus('error', 'Vui lòng nhập đầy đủ thông tin!');
        return;
    }

    btn.disabled = true;
    btn.textContent = 'Đang kết nối...';
    updateConnectionStatus('connecting', 'Đang kết nối...');

    try {
        await waitForPywebview();

        gameController = new NetworkGameController();
        const success = await gameController.connectToServer(host, parseInt(port));

        if (success) {
            updateConnectionStatus('connected', 'Kết nối thành công!');
            btn.textContent = 'Đã kết nối';
            document.getElementById('auth-forms').style.display = 'block';

            localStorage.setItem(
                'xiangqi_server',
                JSON.stringify({
                    host: host,
                    port: parseInt(port),
                }),
            );

            window.gameController = gameController;
        } else {
            throw new Error('Không thể kết nối');
        }
    } catch (error) {
        updateConnectionStatus('error', `Lỗi: ${error.message}`);
        btn.disabled = false;
        btn.textContent = 'Kết Nối';
    }
});

document.getElementById('btn-login').addEventListener('click', async () => {
    const username = document.getElementById('login-username').value.trim();
    const password = document.getElementById('login-password').value;
    const statusDiv = document.getElementById('auth-status');
    const btn = document.getElementById('btn-login');

    if (!username || !password) {
        statusDiv.textContent = 'Vui lòng nhập đầy đủ thông tin!';
        statusDiv.className = 'status-message error';
        return;
    }

    btn.disabled = true;
    btn.textContent = 'Đang đăng nhập...';
    statusDiv.textContent = 'Đang xử lý...';
    statusDiv.className = 'status-message info';

    try {
        const response = await gameController.login(username, password);

        statusDiv.textContent = 'Đăng nhập thành công! Đang chuyển...';
        statusDiv.className = 'status-message success';

        const userData = {
            username: response.username,
            token: response.token,
            rating: response.rating,
            user_id: response.user_id,
        };
        localStorage.setItem('xiangqi_user', JSON.stringify(userData));

        try {
            if (window.gameController && window.gameController.network) {
                window.gameController.token = response.token;
                window.gameController.network.sessionToken = response.token;
            }
        } catch (e) {}

        setTimeout(() => {
            window.location.href = 'pages/lobby.html';
        }, 1000);
    } catch (error) {
        statusDiv.textContent = `${error.message || 'Đăng nhập thất bại'}`;
        statusDiv.className = 'status-message error';
        btn.disabled = false;
        btn.textContent = 'Đăng Nhập';
    }
});

document.getElementById('btn-register').addEventListener('click', async () => {
    const username = document.getElementById('register-username').value.trim();
    const email = document.getElementById('register-email').value.trim();
    const password = document.getElementById('register-password').value;
    const confirm = document.getElementById('register-confirm').value;
    const statusDiv = document.getElementById('auth-status');
    const btn = document.getElementById('btn-register');

    if (!username || !email || !password || !confirm) {
        statusDiv.textContent = 'Vui lòng nhập đầy đủ thông tin!';
        statusDiv.className = 'status-message error';
        return;
    }

    if (username.length < 3 || username.length > 20) {
        statusDiv.textContent = 'Username phải từ 3-20 ký tự!';
        statusDiv.className = 'status-message error';
        return;
    }

    if (password.length < 6) {
        statusDiv.textContent = 'Mật khẩu phải ít nhất 6 ký tự!';
        statusDiv.className = 'status-message error';
        return;
    }

    if (password !== confirm) {
        statusDiv.textContent = 'Mật khẩu xác nhận không khớp!';
        statusDiv.className = 'status-message error';
        return;
    }

    btn.disabled = true;
    btn.textContent = 'Đang đăng ký...';
    statusDiv.textContent = 'Đang xử lý...';
    statusDiv.className = 'status-message info';

    try {
        const response = await gameController.register(username, email, password);

        statusDiv.textContent = 'Đăng ký thành công! Vui lòng đăng nhập.';
        statusDiv.className = 'status-message success';

        setTimeout(() => {
            document.querySelector('.tab-btn[data-tab="login"]').click();
            document.getElementById('login-username').value = username;
        }, 1500);
    } catch (error) {
        statusDiv.textContent = `${error.message || 'Đăng ký thất bại'}`;
        statusDiv.className = 'status-message error';
    } finally {
        btn.disabled = false;
        btn.textContent = 'Đăng Ký';
    }
});

document.querySelectorAll('#tab-login input').forEach((input) => {
    input.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') document.getElementById('btn-login').click();
    });
});

document.querySelectorAll('#tab-register input').forEach((input) => {
    input.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') document.getElementById('btn-register').click();
    });
});
