import { Board } from './board.js';
import { UI } from '../ui/renderer.js';

export class GameController {
    constructor(initialPositionOrContainerId, initialPosition = null) {
        this.chessboard = new Board();
        this.eventListeners = new Map();
        this.ui = null;

        let boardContainerId = null;

        if (typeof initialPositionOrContainerId === 'string') {
            boardContainerId = initialPositionOrContainerId;
            this.initialPosition = initialPosition;
        } else if (Array.isArray(initialPositionOrContainerId)) {
            this.initialPosition = initialPositionOrContainerId;
        } else {
            this.initialPosition = initialPosition;
        }

        this.boardContainerId = boardContainerId;
        this.boundChoosePiece = (e) => {
            const spot = e.target.closest('.piece-spot') || e.target.parentNode;
            if (spot && spot.hasAttribute('data-x')) {
                const row = parseInt(spot.getAttribute('data-x'));
                const col = parseInt(spot.getAttribute('data-y'));
                this.choosePiece(row, col);
                e.stopPropagation();
            }
        };
        this.boundCancelPiece = (e) => {
            const spot = e.target.closest('.piece-spot') || e.target.parentNode;
            if (spot && spot.hasAttribute('data-x')) {
                const row = parseInt(spot.getAttribute('data-x'));
                const col = parseInt(spot.getAttribute('data-y'));
                this.cancelPiece(row, col);
                e.stopPropagation();
            }
        };
        this.chessboard.initBoard(this.initialPosition);

        if (boardContainerId) {
            try {
                this.ui = new UI(boardContainerId);
                this.ui.renderBoard(this.chessboard.board);
                this.bindEvents();
            } catch (error) {
                console.error('[GameController] Failed to initialize UI:', error.message);
                this.ui = null;
            }
        }
    }

    initUI(boardContainerId) {
        if (this.ui && this.ui.isInitialized) {
            return true;
        }

        try {
            this.boardContainerId = boardContainerId;

            if (this.ui && !this.ui.isInitialized) {
                this.ui.initialize(boardContainerId);
            } else {
                this.ui = new UI(boardContainerId);
            }

            this.ui.renderBoard(this.chessboard.board);
            this.bindEvents();
            return true;
        } catch (error) {
            console.error('[GameController] Failed to initialize UI:', error.message);
            return false;
        }
    }

    reset() {
        this.stack = [];

        if (this.initialPosition) {
            this.chessboard.initBoard(this.initialPosition);
        } else {
            this.chessboard.initBoard([]);
        }

        this.chessboard.turn = 'red';
        this.chessboard.status = true;
        this.chessboard.curPiece = null;
        this.chessboard.turnCnt = 0;

        if (this.ui && typeof this.ui.clearBoard === 'function') {
            this.ui.clearBoard();
            this.ui.renderBoard(this.chessboard.board);
        }
    }

    bindEvents() {
        if (this.ui.buttons.newGame) {
            this.ui.buttons.newGame.addEventListener('click', () => this.handleNewGame());
        }
        if (this.ui.buttons.resign) {
            this.ui.buttons.resign.addEventListener('click', () => this.handleResign());
        }

        this.ui.bindBoardClick(({ row, col, event }) => {
            this.handleBoardClick({
                row,
                col,
                stopPropagation: () => event.stopPropagation(),
            });
        });
    }

    handleNewGame() {
        location.reload();
    }

    handleResign() {
        this.chessboard.status = false;
    }

    handleBoardClick({ row, col, stopPropagation }) {
        if (!this.chessboard.status) {
            if (stopPropagation) stopPropagation();
            return;
        }

        const x = row;
        const y = col;

        if (!this.chessboard.curPiece) {
            const piece = this.chessboard.board[x][y];
            if (piece && piece.color === this.chessboard.turn) {
                this.chessboard.curPiece = piece;
                this.ui.renderBoard(this.chessboard.board);
                this.ui.selectPiece(x, y);
            }
        } else {
            if (this.chessboard.curPiece.row === x && this.chessboard.curPiece.col === y) {
                this.chessboard.curPiece = null;
                this.ui.renderBoard(this.chessboard.board);
                return;
            }

            const res = this.chessboard.movePiece(this.chessboard.curPiece, x, y);
            if (res) {
                this.executeMove(x, y);
            } else {
                const targetPiece = this.chessboard.board[x][y];
                if (targetPiece && targetPiece.color === this.chessboard.turn) {
                    this.chessboard.curPiece = targetPiece;
                    this.ui.renderBoard(this.chessboard.board);
                    this.ui.selectPiece(x, y);
                }
            }
        }

        if (stopPropagation) stopPropagation();
    }

    executeMove(newRow, newCol) {
        const curRow = this.chessboard.curPiece.row;
        const curCol = this.chessboard.curPiece.col;

        const moveInfo = {
            from: { row: curRow, col: curCol },
            to: { row: newRow, col: newCol },
            piece: this.chessboard.curPiece,
            color: this.chessboard.turn,
        };

        this.chessboard.board[curRow][curCol] = null;
        this.chessboard.board[newRow][newCol] = this.chessboard.curPiece;

        this.chessboard.updateKingCache(this.chessboard.curPiece, newRow, newCol);

        if (this.ui && this.ui.movePieceDOM) {
            const captured = this.ui.movePieceDOM(curRow, curCol, newRow, newCol);
            if (captured) {
                moveInfo.captured = true;
            }
        }

        this.switchTurn();

        this.chessboard.curPiece.row = newRow;
        this.chessboard.curPiece.col = newCol;
        this.chessboard.curPiece = null;

        this.checkGameStatus();
        this.emit('move-made', moveInfo);

        if (this.onMoveMade) {
            this.onMoveMade(moveInfo);
        }

        return true;
    }

    switchTurn() {
        this.chessboard.turn = this.chessboard.turn === 'red' ? 'black' : 'red';
        if (this.ui && this.ui.updateTurn) {
            this.ui.updateTurn(this.chessboard.turn);
        }
    }

    checkGameStatus() {
        const currentTurn = this.chessboard.turn;

        let isCheck = false;
        if (typeof this.chessboard.isKingInDanger === 'function') {
            isCheck = this.chessboard.isKingInDanger(currentTurn);
        }

        if (this.ui && this.ui.elements.turnText) {
            if (isCheck) {
                const label = currentTurn === 'red' ? 'Đỏ' : 'Đen';
                const colorStyle = currentTurn === 'red' ? '#e74c3c' : '#2c3e50';
                this.ui.elements.turnText.innerHTML = `<span style="color:${colorStyle}">Lượt ${label} (ĐANG BỊ CHIẾU!)</span>`;
            } else {
                this.ui.updateTurn(currentTurn);
            }
        }

        let winner = null;
        if (typeof this.chessboard.checkWinner === 'function') {
            winner = this.chessboard.checkWinner();
        }

        if (winner) {
            this.chessboard.status = false;

            const reason = isCheck ? 'checkmate' : 'stalemate';

            this.emit('game-over', {
                winner: winner,
                reason: reason,
            });
        }
    }

    choosePiece(row, col) {
        if (!this.chessboard.status) {
            return;
        }

        const piece = this.chessboard.board[row][col];
        if (!piece || this.chessboard.turn !== piece.color) {
            return;
        }

        this.chessboard.curPiece = piece;
        this.ui.setPieceBackground(row, col, '#B0E0E6');
    }

    cancelPiece(row, col) {
        const piece = this.chessboard.board[row][col];

        if (this.chessboard.status && this.chessboard.curPiece && this.chessboard.curPiece === piece) {
            this.ui.setPieceBackground(row, col, '#FAF0E6');
            this.chessboard.curPiece = null;
        }
    }

    on(event, callback) {
        if (!this.eventListeners.has(event)) {
            this.eventListeners.set(event, []);
        }
        this.eventListeners.get(event).push(callback);
    }

    off(event, callback) {
        if (!this.eventListeners.has(event)) return;
        const listeners = this.eventListeners.get(event);
        const index = listeners.indexOf(callback);
        if (index > -1) {
            listeners.splice(index, 1);
        }
    }

    emit(event, ...args) {
        if (!this.eventListeners.has(event)) return;
        this.eventListeners.get(event).forEach((callback) => {
            try {
                callback(...args);
            } catch (e) {
                console.error(`Error in event handler for ${event}:`, e);
            }
        });
    }

    setupBoard(options = {}) {
        const { flipped = false } = options;
        this.isFlipped = flipped;
        this.reset();

        if (flipped && this.ui) {
            this.flipBoard();
        }

        return true;
    }

    flipBoard() {
        this.isFlipped = !this.isFlipped;
        this.ui.setFlipped(this.isFlipped);
    }

    validateMove(from, to) {
        if (!this.chessboard || !this.chessboard.board) {
            return false;
        }

        const piece = this.chessboard.board[from.row][from.col];
        if (!piece) {
            return false;
        }

        if (piece.color !== this.chessboard.turn) {
            return false;
        }

        if (typeof piece.canMove === 'function') {
            return piece.canMove(to.row, to.col, this.chessboard.board);
        }

        return true;
    }
}
