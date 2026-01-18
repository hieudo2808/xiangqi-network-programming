export class ChessPiece {
    constructor(color, type, icon, col, row) {
        this.color = color;
        this.icon = icon;
        this.type = type;
        this.col = col;
        this.row = row;
    }

    isFriendly(newRow, newCol, board) {
        const unknown = board[newRow][newCol];
        if (unknown != null && unknown.color == this.color) {
            return true;
        }
        return false;
    }

    checkMove(newRow, newCol, valid, board) {
        if (this.isFriendly(newRow, newCol, board)) return false;

        const rowChange = newRow - this.row;
        const colChange = newCol - this.col;

        return valid.some(([validRowChange, validColChange]) => {
            return rowChange === validRowChange && colChange === validColChange;
        });
    }

    validateMove(newRow, newCol, board) {
        throw new Error('validateMove() must be implemented by subclass');
    }
}
