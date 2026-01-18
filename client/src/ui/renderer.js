import { injectBoardStyles, BOARD_DIMENSIONS } from './boardStyles.js';

export class UI {
    constructor(boardContainerId = null) {
        this.boardContainerId = boardContainerId;
        this.boardContainer = boardContainerId ? document.getElementById(boardContainerId) : null;

        this.elements = {
            turnText: null,
            movesList: null,
            chessboardTable: null,
        };

        this.buttons = { newGame: null, resign: null, draw: null };

        if (!this.boardContainer) {
            this.isInitialized = false;
            return;
        }

        this.isLegacyMode = false;
        this.isInitialized = true;
        this.initModernMode();
    }

    initialize(boardContainerId) {
        if (this.isInitialized) return true;
        this.boardContainerId = boardContainerId;
        this.boardContainer = document.getElementById(boardContainerId);
        if (!this.boardContainer) throw new Error(`Container "${boardContainerId}" not found.`);

        this.isLegacyMode = false;
        this.isInitialized = true;
        this.initModernMode();
        return true;
    }

    initModernMode() {
        this.initModernBoard();
        this.bindExistingElements();
    }

    initModernBoard() {
        this.boardContainer.innerHTML = '';

        const boardWrapper = document.createElement('div');
        boardWrapper.className = 'board-wrapper';

        const boardShape = document.createElement('div');
        boardShape.className = 'xiangqi-grid';

        const gridTable = document.createElement('table');
        gridTable.className = 'grid-table';
        for (let r = 0; r < 9; r++) {
            const row = gridTable.insertRow();
            for (let c = 0; c < 8; c++) {
                const cell = row.insertCell();
                if (r === 4) {
                    if (c === 0) cell.className = 'river-cell river-left';
                    else if (c === 7) cell.className = 'river-cell river-right';
                    else cell.className = 'river-cell river-middle';
                } else {
                    cell.className = 'normal-cell';
                }
            }
        }
        boardShape.appendChild(gridTable);

        const palaceOverlay = document.createElement('div');
        palaceOverlay.className = 'palace-overlay';
        boardShape.appendChild(palaceOverlay);

        const riverText = document.createElement('div');
        riverText.className = 'river-text';
        riverText.textContent = '楚 河  漢 界';
        boardShape.appendChild(riverText);

        const chessboard = document.createElement('div');
        chessboard.id = 'chessboardContainer';
        chessboard.className = 'piece-layer';

        for (let r = 0; r < 10; r++) {
            for (let c = 0; c < 9; c++) {
                const spot = document.createElement('div');
                spot.className = 'piece-spot';
                spot.setAttribute('data-x', r);
                spot.setAttribute('data-y', c);
                chessboard.appendChild(spot);
            }
        }

        this.elements.chessboardTable = chessboard;

        boardWrapper.appendChild(boardShape);
        boardWrapper.appendChild(chessboard);
        this.boardContainer.appendChild(boardWrapper);

        this.injectStyles();
    }

    bindExistingElements() {
        this.elements.turnText = document.getElementById('turn-text');
        this.elements.movesList = document.getElementById('moves-list');
        this.buttons.resign = document.getElementById('btn-resign') || document.createElement('button');
        this.buttons.draw = document.getElementById('btn-draw-offer') || document.createElement('button');
        this.buttons.newGame = document.getElementById('btn-new-game') || document.createElement('button');
    }

    injectStyles() {
        injectBoardStyles();
    }

    createPiece(x, y, icon, color) {
        if (!this.isInitialized) return null;
        const index = x * 9 + y;
        const spot = this.elements.chessboardTable.children[index];

        if (!spot) return null;

        const div = document.createElement('div');
        div.setAttribute('data-color', color);
        div.classList.add('pieces', color);
        div.textContent = icon;

        spot.innerHTML = '';
        spot.appendChild(div);
        return div;
    }

    clearBoard() {
        if (!this.isInitialized || !this.elements.chessboardTable) return;
        const spots = this.elements.chessboardTable.children;
        for (let spot of spots) {
            spot.innerHTML = '';
        }
    }

    renderBoard(board) {
        if (!this.isInitialized) return;
        this.clearBoard();
        for (let i = 0; i < 10; i++) {
            for (let j = 0; j < 9; j++) {
                if (board[i][j]) {
                    this.createPiece(i, j, board[i][j].icon, board[i][j].color);
                }
            }
        }
    }

    updateTurn(turn) {
        if (this.elements.turnText) {
            const isRed = turn === 'red';
            this.elements.turnText.innerHTML = isRed
                ? `<span style="color:#e74c3c">🔴 Lượt Đỏ</span>`
                : `<span style="color:#2c3e50">⚫ Lượt Đen</span>`;
        }
    }

    flipBoard(isFlipped) {
        const wrapper = this.boardContainer.querySelector('.board-wrapper');
        if (wrapper) {
            if (isFlipped) wrapper.classList.add('flipped');
            else wrapper.classList.remove('flipped');
        }
    }

    movePieceDOM(fromRow, fromCol, toRow, toCol) {
        if (!this.isInitialized || !this.elements.chessboardTable) return false;

        const fromIndex = fromRow * 9 + fromCol;
        const toIndex = toRow * 9 + toCol;

        const sourceSpot = this.elements.chessboardTable.children[fromIndex];
        const targetSpot = this.elements.chessboardTable.children[toIndex];

        if (!sourceSpot || !targetSpot) return false;

        const pieceDiv = sourceSpot.querySelector('.pieces');
        if (!pieceDiv) return false;

        const targetPiece = targetSpot.querySelector('.pieces');
        const captured = !!targetPiece;

        sourceSpot.removeChild(pieceDiv);

        if (targetPiece) {
            targetSpot.removeChild(targetPiece);
        }

        targetSpot.appendChild(pieceDiv);

        this.highlightPiece(pieceDiv);

        return captured;
    }

    highlightPiece(pieceDiv) {
        if (!pieceDiv) return;

        const allPieces = this.elements.chessboardTable.querySelectorAll('.pieces');
        allPieces.forEach((p) => {
            p.style.backgroundColor = '';
            p.classList.remove('selected');
        });

        pieceDiv.style.backgroundColor = '#FAF0E6';
    }

    selectPiece(row, col) {
        if (!this.isInitialized || !this.elements.chessboardTable) return;

        this.clearSelection();

        const index = row * 9 + col;
        const spot = this.elements.chessboardTable.children[index];
        const pieceDiv = spot?.querySelector('.pieces');

        if (pieceDiv) {
            pieceDiv.classList.add('selected');
        }
    }

    clearSelection() {
        if (!this.isInitialized || !this.elements.chessboardTable) return;

        const allPieces = this.elements.chessboardTable.querySelectorAll('.pieces.selected');
        allPieces.forEach((p) => p.classList.remove('selected'));
    }

    getPieceAt(row, col) {
        if (!this.isInitialized || !this.elements.chessboardTable) return null;

        const index = row * 9 + col;
        const spot = this.elements.chessboardTable.children[index];
        return spot?.querySelector('.pieces') || null;
    }

    setFlipped(flipped) {
        if (!this.boardContainer) return;

        if (flipped) {
            this.boardContainer.classList.add('flipped');
        } else {
            this.boardContainer.classList.remove('flipped');
        }
    }

    isFlipped() {
        return this.boardContainer?.classList.contains('flipped') || false;
    }

    bindBoardClick(callback) {
        if (!this.boardContainer) {
            console.warn('[UI] Board container not found to bind events');
            return;
        }

        this.boardContainer.addEventListener('click', (e) => {
            const target = e.target.closest('.piece-spot') || e.target.closest('.pieces');
            if (!target) return;

            const spot = target.classList.contains('pieces') ? target.parentElement : target;
            if (!spot || !spot.hasAttribute('data-x')) return;

            const row = parseInt(spot.getAttribute('data-x'));
            const col = parseInt(spot.getAttribute('data-y'));

            callback({
                row,
                col,
                target: e.target,
                spot,
                event: e,
            });
        });
    }
    setPieceBackground(row, col, color) {
        const pieceDiv = this.getPieceAt(row, col);
        if (pieceDiv) {
            pieceDiv.style.backgroundColor = color;
        }
    }

    getAllPieces() {
        if (!this.elements.chessboardTable) return [];
        return this.elements.chessboardTable.querySelectorAll('.pieces');
    }
}
