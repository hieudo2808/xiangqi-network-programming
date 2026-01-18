import { ChessPiece } from './ChessPiece.js';

export class Horse extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'horse', icon, col, row);
        this.dir = [
            [2, 1],
            [2, -1],
            [-2, 1],
            [-2, -1],
            [1, 2],
            [-1, 2],
            [1, -2],
            [-1, -2],
        ];
    }

    validateMove(newRow, newCol, board) {
        const valid = [...this.dir];

        if (!this.checkMove(newRow, newCol, valid, board)) return false;

        const rowChange = newRow - this.row;
        const colChange = newCol - this.col;

        const legRow = Math.abs(rowChange) == 2 ? this.row + rowChange / 2 : this.row;
        const legCol = Math.abs(colChange) == 2 ? this.col + colChange / 2 : this.col;

        if (board[legRow][legCol] != null) return false;

        return true;
    }
}
