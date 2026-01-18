import { General, Chariot, Horse, Elephant, Advisor, Pawn, Cannon } from '../pieces/index.js';

export class Board {
    constructor() {
        this.board = [];
        this.turn = 'red';
        this.status = true;
        this.curPiece = null;
        this.turnCnt = 0;

        this.kingPos = {
            red: null,
            black: null,
        };
    }

    initBoard(situation) {
        for (let i = 0; i < 10; i++) {
            this.board[i] = new Array(9).fill(null);
        }
        this.kingPos = { red: null, black: null };
        situation.forEach((pieceInfo) => this.placePiece(pieceInfo));
    }

    placePiece(pieceInfo) {
        const [type, color, r, c] = pieceInfo;
        const row = parseInt(r);
        const col = parseInt(c);
        const PieceClass = {
            chariot: Chariot,
            horse: Horse,
            elephant: Elephant,
            advisor: Advisor,
            general: General,
            cannon: Cannon,
            pawn: Pawn,
        }[type];

        if (PieceClass) {
            const icons = {
                red: {
                    chariot: '俥',
                    horse: '傌',
                    elephant: '相',
                    advisor: '仕',
                    general: '帥',
                    cannon: '砲',
                    pawn: '兵',
                },
                black: {
                    chariot: '車',
                    horse: '馬',
                    elephant: '象',
                    advisor: '士',
                    general: '將',
                    cannon: '炮',
                    pawn: '卒',
                },
            };
            this.board[row][col] = new PieceClass(color, icons[color][type], col, row);

            if (type === 'general') {
                this.kingPos[color] = { r: row, c: col };
            }
        }
    }

    updateKingCache(piece, newRow, newCol) {
        if (piece.type === 'general') {
            this.kingPos[piece.color] = { r: newRow, c: newCol };
        }
    }

    movePiece(piece, newRow, newCol) {
        if (newRow < 0 || newRow > 9 || newCol < 0 || newCol > 8) return false;
        if (this.board[piece.row][piece.col] !== piece) return false;
        if (!piece.validateMove(newRow, newCol, this.board)) return false;
        if (this.isSuicideMove(piece, newRow, newCol)) {
            return false;
        }
        return true;
    }

    isKingInDanger(color, boardState = this.board, kingPosOverride = null) {
        const kingPos = kingPosOverride || this.kingPos[color];

        if (!kingPos) return true;

        const enemyColor = color === 'red' ? 'black' : 'red';

        for (let i = 0; i < 10; i++) {
            for (let j = 0; j < 9; j++) {
                const enemy = boardState[i][j];
                if (enemy && enemy.color === enemyColor) {
                    if (enemy.validateMove(kingPos.r, kingPos.c, boardState)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    isSuicideMove(piece, newRow, newCol) {
        const tempBoard = this.copyBoard(this.board);
        const oldRow = piece.row;
        const oldCol = piece.col;

        const clonePiece = Object.assign(Object.create(Object.getPrototypeOf(piece)), piece);
        clonePiece.row = newRow;
        clonePiece.col = newCol;

        tempBoard[oldRow][oldCol] = null;
        tempBoard[newRow][newCol] = clonePiece;

        let simKingPos = this.kingPos[piece.color];
        if (piece.type === 'general') {
            simKingPos = { r: newRow, c: newCol };
        }

        return this.isKingInDanger(piece.color, tempBoard, simKingPos);
    }

    hasValidMoves(color) {
        for (let i = 0; i < 10; i++) {
            for (let j = 0; j < 9; j++) {
                const piece = this.board[i][j];
                if (piece && piece.color === color) {
                    for (let r = 0; r < 10; r++) {
                        for (let c = 0; c < 9; c++) {
                            if (piece.validateMove(r, c, this.board)) {
                                if (!this.isSuicideMove(piece, r, c)) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    checkWinner() {
        const currentTurn = this.turn;
        if (!this.hasValidMoves(currentTurn)) {
            return currentTurn === 'red' ? 'black' : 'red';
        }
        return null;
    }

    copyBoard(original) {
        const copy = new Array(10);
        for (let i = 0; i < 10; i++) {
            copy[i] = [...original[i]];
        }
        return copy;
    }
}
