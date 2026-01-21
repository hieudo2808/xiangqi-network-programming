export function startMessagePolling(bridge, intervalMs = 50) {
    if (bridge._messagePollInterval) return;

    bridge._messagePollInterval = setInterval(() => {
        pollMessages(bridge);
    }, intervalMs);
}

export function stopMessagePolling(bridge) {
    if (bridge._messagePollInterval) {
        clearInterval(bridge._messagePollInterval);
        bridge._messagePollInterval = null;
    }
}

async function pollMessages(bridge) {
    if (!window.pywebview?.api) return;

    try {
        const messages = await window.pywebview.api.get_messages();
        if (messages && Array.isArray(messages) && messages.length > 0) {
            messages.forEach((message) => {
                bridge.handleMessage(message);
            });
        }
    } catch (error) {
        console.error('[ConnectionManager] Poll error:', error);
    }
}

export function startHeartbeat(bridge) {
    stopHeartbeat(bridge);
    bridge._lastPongTime = Date.now();

    bridge._heartbeatInterval = setInterval(() => {
        if (!bridge.connected) return;

        const timeSinceLastPong = Date.now() - bridge._lastPongTime;
        if (timeSinceLastPong > bridge._heartbeatTimeout) {
            console.warn('[Heartbeat] No pong in', timeSinceLastPong, 'ms - server may be down');
            return;
        }

        bridge.send('ping', {}).catch((err) => {
            console.warn('[Heartbeat] Ping failed:', err.message);
        });
    }, bridge._heartbeatPingInterval);

    console.log('[Heartbeat] Started (interval:', bridge._heartbeatPingInterval, 'ms)');
}

export function stopHeartbeat(bridge) {
    if (bridge._heartbeatInterval) {
        clearInterval(bridge._heartbeatInterval);
        bridge._heartbeatInterval = null;
    }
}

export function handlePongResponse(bridge) {
    bridge._lastPongTime = Date.now();
}

export function handleConnectionStatus(bridge, payload) {
    const status = payload.status;
    console.log('[ConnectionManager] Status:', status, payload);

    if (status === 'reconnecting') {
        bridge.connected = false;
        showReconnectOverlay(payload.attempt || 0, payload.remaining || 300);
    } else if (status === 'reconnected') {
        bridge.connected = true;
        hideReconnectOverlay();
        startHeartbeat(bridge);

        reloginAfterReconnect(bridge).then((success) => {
            if (success) {
                console.log('[ConnectionManager] Re-login successful');
                bridge.emit('reconnected', payload);
            } else {
                console.warn('[ConnectionManager] Re-login failed');
                alert('Phiên đăng nhập đã hết hạn. Vui lòng đăng nhập lại.');
                localStorage.removeItem('user_data');
                localStorage.removeItem('match_data');
                window.location.href = '../index.html';
            }
        });
    } else if (status === 'reconnect_failed') {
        bridge.connected = false;
        hideReconnectOverlay();
        bridge.emit('reconnect_failed', payload);
        alert('Không thể kết nối lại server. Vui lòng thử lại sau.');
        window.location.href = '../index.html';
    }
}

export async function reloginAfterReconnect(bridge) {
    try {
        const existingToken = bridge.sessionToken;
        const userData = localStorage.getItem('xiangqi_user');
        const storedToken = userData ? JSON.parse(userData).token : null;
        const tokenToValidate = existingToken || storedToken;

        if (tokenToValidate) {
            console.log('[ConnectionManager] Validating existing token...');
            try {
                const validateRes = await bridge.sendAndWait(
                    'validate_token',
                    { token: tokenToValidate },
                    'validate_token',
                );

                if (validateRes.success && validateRes.payload?.valid) {
                    console.log('[ConnectionManager] Token still valid!');
                    bridge.sessionToken = tokenToValidate;
                    return true;
                }
            } catch (e) {
                console.warn('[ConnectionManager] Token validation failed:', e.message);
            }
        }

        const savedCreds = localStorage.getItem('_auth_creds');
        if (!savedCreds) {
            console.warn('[ConnectionManager] No saved credentials');
            return false;
        }

        const creds = JSON.parse(savedCreds);
        const response = await bridge.sendAndWait(
            'login',
            {
                username: creds.username,
                password: creds.hashedPassword,
            },
            'login',
        );

        if (response.success && response.payload?.token) {
            bridge.sessionToken = response.payload.token;

            if (userData) {
                try {
                    const user = JSON.parse(userData);
                    user.token = response.payload.token;
                    localStorage.setItem('xiangqi_user', JSON.stringify(user));
                } catch (e) {
                    console.warn('[ConnectionManager] Failed to update user data:', e);
                }
            }
            return true;
        }
        return false;
    } catch (error) {
        console.error('[ConnectionManager] Re-login failed:', error);
        return false;
    }
}

export function showReconnectOverlay(attempt, remaining) {
    let overlay = document.getElementById('reconnect-overlay');
    if (!overlay) {
        overlay = document.createElement('div');
        overlay.id = 'reconnect-overlay';
        overlay.innerHTML = `
            <div style="background: rgba(0,0,0,0.9); position: fixed; top: 0; left: 0; right: 0; bottom: 0; 
                        display: flex; align-items: center; justify-content: center; z-index: 10000;">
                <div style="background: #1a1a2e; padding: 40px; border-radius: 16px; text-align: center; 
                            border: 2px solid #4a5568; max-width: 400px;">
                    <div style="font-size: 48px; margin-bottom: 20px;">🔄</div>
                    <h2 style="color: #fff; margin-bottom: 15px;">Mất kết nối</h2>
                    <p style="color: #a0aec0; margin-bottom: 10px;">Đang kết nối lại server...</p>
                    <p id="reconnect-status" style="color: #fbbf24; font-size: 14px;">Lần thử: ${attempt} | Còn ${remaining}s</p>
                    <div style="margin-top: 20px;">
                        <div style="width: 200px; height: 4px; background: #2d3748; border-radius: 2px; margin: 0 auto;">
                            <div id="reconnect-progress" style="width: 100%; height: 100%; background: #3b82f6; border-radius: 2px; transition: width 1s;"></div>
                        </div>
                    </div>
                </div>
            </div>
        `;
        document.body.appendChild(overlay);
    } else {
        const statusEl = document.getElementById('reconnect-status');
        const progressEl = document.getElementById('reconnect-progress');
        if (statusEl) statusEl.textContent = `Lần thử: ${attempt} | Còn ${remaining}s`;
        if (progressEl) progressEl.style.width = `${(remaining / 300) * 100}%`;
    }
}

export function hideReconnectOverlay() {
    const overlay = document.getElementById('reconnect-overlay');
    if (overlay) overlay.remove();
}

export async function connect(bridge, host, port) {
    return new Promise((resolve, reject) => {
        try {
            if (!window.pywebview?.api) {
                reject(new Error('pywebview API not available'));
                return;
            }

            window.pywebview.api
                .connect(host, port)
                .then(async (result) => {
                    console.log('Connect result:', result);
                    if (result.success) {
                        bridge.connected = true;
                        bridge.emit('connected');
                        startMessagePolling(bridge);
                        startHeartbeat(bridge);

                        const success = await reloginAfterReconnect(bridge);
                        if (success) {
                            console.log('[ConnectionManager] Auto re-authenticated');
                        }

                        resolve();
                    } else {
                        reject(new Error(result.message || 'Connection failed'));
                    }
                })
                .catch((err) => reject(err));
        } catch (error) {
            reject(error);
        }
    });
}

export async function disconnect(bridge) {
    if (!bridge.connected) return;

    try {
        if (window.pywebview?.api) {
            await window.pywebview.api.disconnect();
        }
    } catch (error) {
        console.error('[ConnectionManager] Disconnect error:', error);
    }

    stopMessagePolling(bridge);
    stopHeartbeat(bridge);
    bridge.connected = false;
    bridge.emit('disconnected');
}
