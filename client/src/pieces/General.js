import { ChessPiece } from './ChessPiece.js';

export class General extends ChessPiece {
    constructor(color, icon, col, row) {
        super(color, 'general', icon, col, row);
        this.dir = [
            [1, 0],
            [-1, 0],
            [0, 1],
            [0, -1],
            [0, 0],
        ];
    }

    findColTgt(color, board) {
        if (color == 'black') {
            for (let i = this.row + 1; i <= 9; i++) {
                if (board[i][this.col] == null) continue;
                if (board[i][this.col].type != 'general') break;

                return [i - this.row, 0];
            }
        } else {
            for (let i = this.row - 1; i >= 0; i--) {
                if (board[i][this.col] == null) continue;
                if (board[i][this.col].type != 'general') break;

                return [i - this.row, 0];
            }
        }

        return null;
    }

    validateMove(newRow, newCol, board) {
        const valid = [];

        if (this.col != 3) valid.push(this.dir[3]);
        if (this.col != 5) valid.push(this.dir[2]);
        if (this.color == 'black') {
            if (this.row != 0) valid.push(this.dir[1]);
            if (this.row != 2) valid.push(this.dir[0]);
            const tgt = this.findColTgt('black', board);
            if (tgt != null) {
                valid.push(tgt);
                this.dir[4] = tgt;
            } else {
                this.dir[4] = [0, 0];
            }
        } else {
            if (this.row != 7) valid.push(this.dir[1]);
            if (this.row != 9) valid.push(this.dir[0]);
            const tgt = this.findColTgt('red', board);
            if (tgt != null) {
                valid.push(tgt);
                this.dir[4] = tgt;
            } else {
                this.dir[4] = [0, 0];
            }
        }

        return this.checkMove(newRow, newCol, valid, board);
    }
}
