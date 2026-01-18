import { ChessPiece } from './ChessPiece.js';

export class Cannon extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'cannon', icon, col, row);
    }

    validateMove(newRow, newCol, board) {
        if (this.isFriendly(newRow, newCol, board)) return false;
        if (this.row !== newRow && this.col !== newCol) return false;

        let obstacles = 0;

        if (this.row === newRow) {
            const start = Math.min(this.col, newCol) + 1;
            const end = Math.max(this.col, newCol);
            for (let c = start; c < end; c++) {
                if (board[this.row][c] !== null) {
                    obstacles++;
                }
            }
        } else {
            const start = Math.min(this.row, newRow) + 1;
            const end = Math.max(this.row, newRow);
            for (let r = start; r < end; r++) {
                if (board[r][this.col] !== null) {
                    obstacles++;
                }
            }
        }

        const targetPiece = board[newRow][newCol];

        if (targetPiece === null) {
            return obstacles === 0;
        } else {
            return obstacles === 1;
        }
    }
}
