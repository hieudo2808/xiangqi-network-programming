import { ChessPiece } from './ChessPiece.js';

export class Advisor extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'advisor', icon, col, row);
        this.dir = [
            [1, 1],
            [1, -1],
            [-1, 1],
            [-1, -1],
        ];
    }

    validateMove(newRow, newCol, board) {
        const valid = [];

        if (newCol >= 3 && newCol <= 5) {
            if (this.color == 'red' && newRow >= 7 && newRow <= 9) {
                valid.push(...this.dir);
            }
            if (this.color == 'black' && newRow >= 0 && newRow <= 2) {
                valid.push(...this.dir);
            }
        }

        return this.checkMove(newRow, newCol, valid, board);
    }
}
