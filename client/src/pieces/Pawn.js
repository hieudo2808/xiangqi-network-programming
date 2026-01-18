import { ChessPiece } from './ChessPiece.js';

export class Pawn extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'pawn', icon, col, row);
        this.dir = [
            [1, 0],
            [-1, 0],
            [0, 1],
            [0, -1],
        ];
    }

    validateMove(newRow, newCol, board) {
        const valid = [];

        if (this.color == 'red') {
            if (this.row <= 4) {
                valid.push(this.dir[1], this.dir[2], this.dir[3]);
            } else {
                valid.push(this.dir[1]);
            }
        } else {
            if (this.row >= 5) {
                valid.push(this.dir[0], this.dir[2], this.dir[3]);
            } else {
                valid.push(this.dir[0]);
            }
        }

        return this.checkMove(newRow, newCol, valid, board);
    }
}
