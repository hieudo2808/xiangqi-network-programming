let currentProfileData = null;

export function getCurrentProfileData() {
    return currentProfileData;
}

export async function loadProfile(network, showMessage) {
    if (!network) {
        showMessage('error', 'Chưa kết nối server');
        return null;
    }

    const content = document.getElementById('profile-content');
    if (content) {
        content.innerHTML = '<div class="loading">Đang tải...</div>';
    }

    try {
        const response = await network.getProfile();
        if (response.success && response.payload) {
            currentProfileData = response.payload.profile || response.payload;
            displayProfile(currentProfileData);
            return currentProfileData;
        } else {
            if (content) {
                content.innerHTML = '<div class="error">Không thể tải thông tin</div>';
            }
            return null;
        }
    } catch (error) {
        console.error('Failed to load profile:', error);
        if (content) {
            content.innerHTML = '<div class="error">Lỗi kết nối</div>';
        }
        return null;
    }
}

export function displayProfile(profile) {
    const container = document.getElementById('profile-content');
    if (!container) return;

    const firstLetter = profile.username ? profile.username.charAt(0).toUpperCase() : '?';

    container.innerHTML = `
        <div class="profile-header">
            <div class="profile-avatar">${firstLetter}</div>
            <div class="profile-username">${profile.username}</div>
            <div class="profile-id">ID: ${profile.user_id}</div>
            <div class="profile-rank">${profile.rank_title}</div>
            <div class="profile-rating-big">Rating: ${profile.rating}</div>
        </div>
        
        <div class="profile-stats">
            <div class="stat-box">
                <div class="stat-value wins">${profile.wins}</div>
                <div class="stat-label">Thắng</div>
            </div>
            <div class="stat-box">
                <div class="stat-value losses">${profile.losses}</div>
                <div class="stat-label">Thua</div>
            </div>
            <div class="stat-box">
                <div class="stat-value draws">${profile.draws}</div>
                <div class="stat-label">Hòa</div>
            </div>
        </div>
        
        <div class="profile-winrate">
            <div>Tổng số trận: <strong>${profile.total_matches}</strong></div>
            <div class="winrate-bar">
                <div class="winrate-fill" style="width: ${profile.win_rate}%"></div>
            </div>
            <div>Tỷ lệ thắng: <strong>${profile.win_rate.toFixed(1)}%</strong></div>
        </div>
        
        <div class="profile-joined">
            Tham gia: ${profile.created_at || 'N/A'}
        </div>
    `;
}

export function copyUserId(showMessage) {
    if (currentProfileData && currentProfileData.user_id) {
        navigator.clipboard
            .writeText(currentProfileData.user_id.toString())
            .then(() => {
                showMessage('success', 'Đã copy ID: ' + currentProfileData.user_id);
            })
            .catch(() => {
                const textArea = document.createElement('textarea');
                textArea.value = currentProfileData.user_id.toString();
                document.body.appendChild(textArea);
                textArea.select();
                document.execCommand('copy');
                document.body.removeChild(textArea);
                showMessage('success', 'Đã copy ID: ' + currentProfileData.user_id);
            });
    } else {
        showMessage('error', 'Không có dữ liệu ID');
    }
}

export async function loadLeaderboard(gameController) {
    try {
        if (gameController && gameController.network) {
            const leaderboard = await gameController.getLeaderboard();
            displayLeaderboard(leaderboard || []);
            return leaderboard;
        } else {
            console.warn('⚠️ gameController or network not ready');
            return null;
        }
    } catch (error) {
        console.error('Failed to load leaderboard:', error);
        return null;
    }
}

export function displayLeaderboard(leaderboard) {
    const tbody = document.getElementById('leaderboard-body');
    if (!tbody) return;
    tbody.innerHTML = '';

    leaderboard.slice(0, 5).forEach((player, index) => {
        const rank = index + 1;
        let rankBadge = `<span class="rank-badge">${rank}</span>`;

        if (rank === 1) rankBadge = `<span class="rank-badge gold">👑</span>`;
        else if (rank === 2) rankBadge = `<span class="rank-badge silver">🥈</span>`;
        else if (rank === 3) rankBadge = `<span class="rank-badge bronze">🥉</span>`;

        const totalGames = player.wins + player.losses + player.draws;
        const winRate = totalGames > 0 ? ((player.wins / totalGames) * 100).toFixed(1) : '0.0';

        tbody.innerHTML += `
            <tr>
                <td>${rankBadge}</td>
                <td><strong>${player.username}</strong></td>
                <td><strong>${player.rating}</strong></td>
                <td>${totalGames}</td>
                <td>${winRate}%</td>
            </tr>
        `;
    });
}

export async function refreshLiveMatches(network, showMessage) {
    if (!network) {
        showMessage('error', 'Chưa kết nối server');
        return;
    }

    const container = document.getElementById('live-matches-container');
    container.innerHTML = '<div class="no-matches"><p>Đang tải...</p></div>';

    try {
        const response = await network.getLiveMatches();
        const matches = response.payload?.matches || [];
        displayLiveMatches(matches);
        return matches;
    } catch (error) {
        container.innerHTML = '<div class="no-matches"><p>Không thể tải danh sách trận đấu</p></div>';
        return null;
    }
}

/**
 * Display live matches in UI
 */
export function displayLiveMatches(matches) {
    const container = document.getElementById('live-matches-container');
    if (!container) return;

    if (!matches || matches.length === 0) {
        container.innerHTML =
            '<div class="no-matches"><p>Không có trận đấu nào đang diễn ra</p><p>Hãy quay lại sau!</p></div>';
        return;
    }

    let html = '';
    for (const match of matches) {
        const elapsed = Math.floor((Date.now() / 1000 - match.started_at) / 60);

        html += `
            <div class="live-match-item">
                <div class="live-match-info">
                    <div class="live-match-players">
                        <span class="player-name">${match.red_player || 'Red'}</span>
                        <span class="vs">VS</span>
                        <span class="player-name">${match.black_player || 'Black'}</span>
                    </div>
                    <div class="live-match-time">${elapsed} phút</div>
                </div>
                <button class="btn-spectate" onclick="window.spectateMatch('${match.match_id}')">
                    👁️ Xem
                </button>
            </div>
        `;
    }

    container.innerHTML = html;
}
