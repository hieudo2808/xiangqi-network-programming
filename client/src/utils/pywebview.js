const PYWEBVIEW_TIMEOUT = 10000;
const PYWEBVIEW_POLL_INTERVAL = 100;

function isPywebviewReady() {
    return (
        window.pywebview &&
        window.pywebview.api &&
        typeof window.pywebview.api.connect === 'function'
    );
}

export function waitForPywebview(timeout = PYWEBVIEW_TIMEOUT) {
    return new Promise((resolve, reject) => {
        if (isPywebviewReady()) {
            resolve();
            return;
        }

        let checkInterval = null;
        let timeoutId = null;

        const cleanup = () => {
            if (checkInterval) clearInterval(checkInterval);
            if (timeoutId) clearTimeout(timeoutId);
        };

        timeoutId = setTimeout(() => {
            cleanup();
            reject(new Error('pywebview not ready'));
        }, timeout);

        const onReady = () => {
            setTimeout(() => {
                if (isPywebviewReady()) {
                    cleanup();
                    resolve();
                }
            }, 100);
        };
        window.addEventListener('pywebviewready', onReady, { once: true });

        checkInterval = setInterval(() => {
            if (isPywebviewReady()) {
                cleanup();
                resolve();
            }
        }, PYWEBVIEW_POLL_INTERVAL);
    });
}

export function isInPywebview() {
    return typeof window.pywebview !== 'undefined';
}

export default waitForPywebview;
