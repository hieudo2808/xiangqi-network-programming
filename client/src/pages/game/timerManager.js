let timerInterval = null;
let playerTime = 600;
let opponentTime = 600;
let redTimeMs = 600000;
let blackTimeMs = 600000;
let isGameActive = true;
let matchStartTime = Date.now();

let onTimeoutCallback = null;
let getIsMyTurnCallback = null;
let getIsSpectatorCallback = null;
let getBoardTurnCallback = null;
let getMatchDataCallback = null;

export function initTimerManager(options = {}) {
    playerTime = Math.ceil((options.initialTimeMs || 600000) / 1000);
    opponentTime = Math.ceil((options.initialTimeMs || 600000) / 1000);
    redTimeMs = options.initialTimeMs || 600000;
    blackTimeMs = options.initialTimeMs || 600000;
    isGameActive = true;
    matchStartTime = Date.now();

    onTimeoutCallback = options.onTimeout || null;
    getIsMyTurnCallback = options.getIsMyTurn || (() => false);
    getIsSpectatorCallback = options.getIsSpectator || (() => false);
    getBoardTurnCallback = options.getBoardTurn || (() => 'red');
    getMatchDataCallback = options.getMatchData || (() => null);
}

export function getPlayerTime() {
    return playerTime;
}

export function getOpponentTime() {
    return opponentTime;
}

export function getMatchStartTime() {
    return matchStartTime;
}

export function setGameActive(active) {
    isGameActive = active;
    if (!active && timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
}

export function syncTimerFromServer(redMs, blackMs) {
    redTimeMs = redMs;
    blackTimeMs = blackMs;

    const matchData = getMatchDataCallback?.();
    const isSpectator = getIsSpectatorCallback?.();

    if (isSpectator) {
        playerTime = Math.ceil(redMs / 1000);
        opponentTime = Math.ceil(blackMs / 1000);
    } else if (matchData && matchData.your_color === 'red') {
        playerTime = Math.ceil(redMs / 1000);
        opponentTime = Math.ceil(blackMs / 1000);
    } else {
        playerTime = Math.ceil(blackMs / 1000);
        opponentTime = Math.ceil(redMs / 1000);
    }

    updateTimerDisplay('player-timer', playerTime);
    updateTimerDisplay('opponent-timer', opponentTime);
}

export function startTimer() {
    if (timerInterval) clearInterval(timerInterval);

    matchStartTime = Date.now();

    timerInterval = setInterval(() => {
        if (!isGameActive) {
            clearInterval(timerInterval);
            return;
        }

        const isSpectator = getIsSpectatorCallback?.();
        const isMyTurn = getIsMyTurnCallback?.();
        const boardTurn = getBoardTurnCallback?.();

        if (isSpectator) {
            if (boardTurn === 'red') {
                playerTime--;
                updateTimerDisplay('player-timer', playerTime);
                if (playerTime <= 30 && playerTime > 0) {
                    document.getElementById('player-timer')?.classList.add('warning');
                }
                if (playerTime <= 0) {
                    handleTimeout('player');
                }
            } else {
                opponentTime--;
                updateTimerDisplay('opponent-timer', opponentTime);
                if (opponentTime <= 30 && opponentTime > 0) {
                    document.getElementById('opponent-timer')?.classList.add('warning');
                }
                if (opponentTime <= 0) {
                    handleTimeout('opponent');
                }
            }
        } else if (isMyTurn) {
            playerTime--;
            updateTimerDisplay('player-timer', playerTime);

            if (playerTime <= 30 && playerTime > 0) {
                document.getElementById('player-timer')?.classList.add('warning');
            }
            if (playerTime <= 0) {
                handleTimeout('player');
            }
        } else {
            opponentTime--;
            updateTimerDisplay('opponent-timer', opponentTime);
        }

        if (playerTime <= 0 || opponentTime <= 0) {
            clearInterval(timerInterval);
        }
    }, 1000);
}

export function stopTimer() {
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
}

async function handleTimeout(who) {
    if (onTimeoutCallback) {
        await onTimeoutCallback(who);
    }
}

export function updateTimerDisplay(id, seconds) {
    const el = document.getElementById(id);
    if (!el) return;

    if (seconds < 0) seconds = 0;
    const m = Math.floor(seconds / 60)
        .toString()
        .padStart(2, '0');
    const s = (seconds % 60).toString().padStart(2, '0');
    el.textContent = `${m}:${s}`;
    el.classList.toggle('warning', seconds < 30);
}

export function setupPeriodicSync(gameController, matchId, isSpectateMode, intervalMs = 30000) {
    return setInterval(async () => {
        if (!isGameActive || !gameController.network || isSpectateMode) return;

        try {
            const response = await gameController.network.getTimer(matchId);
            if (response.success && response.payload?.timer) {
                const timer = response.payload.timer;
                syncTimerFromServer(timer.red_time_ms, timer.black_time_ms);
            }
        } catch (e) {
            console.warn('[Timer] Sync failed:', e);
        }
    }, intervalMs);
}

export function getMatchDurationString() {
    const matchDuration = Math.floor((Date.now() - matchStartTime) / 1000);
    const minutes = Math.floor(matchDuration / 60);
    const seconds = matchDuration % 60;
    return `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
}
