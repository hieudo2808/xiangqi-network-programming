import { ChessPiece } from './ChessPiece.js';

export class Elephant extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'elephant', icon, col, row);
        this.dir = [
            [2, 2],
            [2, -2],
            [-2, 2],
            [-2, -2],
        ];
    }

    validateMove(newRow, newCol, board) {
        const valid = [];

        if (this.color == 'red' && newRow >= 5 && newRow <= 9) {
            valid.push(...this.dir);
        } else if (this.color == 'black' && newRow >= 0 && newRow <= 4) {
            valid.push(...this.dir);
        }

        if (!this.checkMove(newRow, newCol, valid, board)) return false;

        const rowChange = newRow - this.row;
        const colChange = newCol - this.col;

        const midRow = this.row + rowChange / 2;
        const midCol = this.col + colChange / 2;

        if (board[midRow][midCol] != null) return false;

        return true;
    }
}
