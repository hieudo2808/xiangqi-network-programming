import { ChessPiece } from './ChessPiece.js';

export class Chariot extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'chariot', icon, col, row);
        this.dir = [
            [
                [1, 0],
                [2, 0],
                [3, 0],
                [4, 0],
                [5, 0],
                [6, 0],
                [7, 0],
                [8, 0],
                [9, 0],
            ],
            [
                [-1, 0],
                [-2, 0],
                [-3, 0],
                [-4, 0],
                [-5, 0],
                [-6, 0],
                [-7, 0],
                [-8, 0],
                [-9, 0],
            ],
            [
                [0, 1],
                [0, 2],
                [0, 3],
                [0, 4],
                [0, 5],
                [0, 6],
                [0, 7],
                [0, 8],
            ],
            [
                [0, -1],
                [0, -2],
                [0, -3],
                [0, -4],
                [0, -5],
                [0, -6],
                [0, -7],
                [0, -8],
            ],
        ];
    }

    rowLimit(side, board) {
        const dirRow = side == 'up' ? 1 : 0;
        let limit = -1;

        for (let i = 0; i < 9; i++) {
            const newRow = this.row + this.dir[dirRow][i][0];
            if (newRow < 0 || newRow > 9) break;

            limit++;
            if (newRow >= 0 && newRow <= 9 && board[newRow][this.col] != null) {
                break;
            }
        }

        return limit;
    }

    colLimit(side, board) {
        const dirCol = side == 'right' ? 2 : 3;
        let limit = -1;

        for (let i = 0; i < 8; i++) {
            const newCol = this.col + this.dir[dirCol][i][1];
            if (newCol < 0 || newCol > 8) break;

            limit++;
            if (newCol >= 0 && newCol <= 8 && board[this.row][newCol] != null) {
                break;
            }
        }

        return limit;
    }

    validateMove(newRow, newCol, board) {
        const valid = [];

        const up = this.rowLimit('up', board);
        for (let i = 0; i <= up; i++) {
            valid.push(this.dir[1][i]);
        }
        const down = this.rowLimit('down', board);
        for (let i = 0; i <= down; i++) {
            valid.push(this.dir[0][i]);
        }
        const right = this.colLimit('right', board);
        for (let i = 0; i <= right; i++) {
            valid.push(this.dir[2][i]);
        }
        const left = this.colLimit('left', board);
        for (let i = 0; i <= left; i++) {
            valid.push(this.dir[3][i]);
        }

        return this.checkMove(newRow, newCol, valid, board);
    }
}
