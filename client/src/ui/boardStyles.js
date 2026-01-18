const CELL_SIZE = 67;
const PADDING = Math.round(CELL_SIZE * 0.66);
const PIECE_SIZE = Math.round(CELL_SIZE * 0.88);
const FONT_SIZE = Math.round(PIECE_SIZE * 0.52);
const RIVER_FONT_SIZE = Math.round(CELL_SIZE * 0.64);

const GRID_WIDTH = 8 * CELL_SIZE;
const GRID_HEIGHT = 9 * CELL_SIZE;

const WRAPPER_WIDTH = GRID_WIDTH + PADDING * 2;
const WRAPPER_HEIGHT = GRID_HEIGHT + PADDING * 2;

const GRID_BOX_WIDTH = GRID_WIDTH + 2;
const GRID_BOX_HEIGHT = GRID_HEIGHT + 2;

const LAYER_WIDTH = 9 * CELL_SIZE;
const LAYER_HEIGHT = 10 * CELL_SIZE;

const LAYER_OFFSET = PADDING - CELL_SIZE / 2;

export const BOARD_DIMENSIONS = {
    CELL_SIZE,
    PADDING,
    PIECE_SIZE,
    FONT_SIZE,
    RIVER_FONT_SIZE,
    GRID_WIDTH,
    GRID_HEIGHT,
    WRAPPER_WIDTH,
    WRAPPER_HEIGHT,
    GRID_BOX_WIDTH,
    GRID_BOX_HEIGHT,
    LAYER_WIDTH,
    LAYER_HEIGHT,
    LAYER_OFFSET,
};

export function injectBoardStyles() {
    if (document.getElementById('xiangqi-renderer-styles')) return;

    const style = document.createElement('style');
    style.id = 'xiangqi-renderer-styles';
    style.textContent = generateBoardCSS();
    document.head.appendChild(style);
}

export function generateBoardCSS() {
    return `
        .game-container .board-container, 
        #xiangqi-board {
            width: auto !important; height: auto !important;
            padding: 0 !important; margin: 0 !important;
            background: transparent !important; border: none !important;
            box-shadow: none !important; display: block !important;
        }

        .board-wrapper {
            position: relative;
            width: ${WRAPPER_WIDTH}px;
            height: ${WRAPPER_HEIGHT}px;
            background: #d4a574;
            margin: 0 auto;
            border-radius: 12px;
            box-shadow: 0 15px 40px rgba(0,0,0,0.5);
            user-select: none;
            box-sizing: content-box;
            transition: transform 0.6s ease;
        }

        .xiangqi-grid {
            position: absolute;
            top: ${PADDING}px;
            left: ${PADDING}px;
            width: ${GRID_BOX_WIDTH}px;
            height: ${GRID_BOX_HEIGHT}px;
            border: 2px solid #5d4037;
            z-index: 1;
            box-sizing: border-box;
        }

        .grid-table { width: 100%; height: 100%; border-collapse: collapse; }
        .grid-table td {
            border: 1px solid #5d4037;
            width: ${CELL_SIZE}px;
            height: ${CELL_SIZE}px;
            padding: 0; box-sizing: border-box;
        }
        .grid-table .river-cell {
            border-top: none; border-bottom: none;
        }
        .grid-table .river-left {
            border-left: 1px solid #5d4037; border-right: none;
        }
        .grid-table .river-right {
            border-left: none; border-right: 1px solid #5d4037;
        }
        .grid-table .river-middle {
            border-left: none; border-right: none;
        }
        
        .palace-overlay {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            pointer-events: none;
        }
        .palace-overlay::before,
        .palace-overlay::after {
            content: '';
            position: absolute;
            width: ${2 * CELL_SIZE}px;
            height: ${2 * CELL_SIZE}px;
            border: none;
        }
        .palace-overlay::before {
            top: 0;
            left: ${3 * CELL_SIZE}px;
            background: linear-gradient(to bottom right, transparent calc(50% - 0.5px), #5d4037 calc(50% - 0.5px), #5d4037 calc(50% + 0.5px), transparent calc(50% + 0.5px)),
                        linear-gradient(to bottom left, transparent calc(50% - 0.5px), #5d4037 calc(50% - 0.5px), #5d4037 calc(50% + 0.5px), transparent calc(50% + 0.5px));
        }
        .palace-overlay::after {
            top: ${7 * CELL_SIZE}px;
            left: ${3 * CELL_SIZE}px;
            background: linear-gradient(to bottom right, transparent calc(50% - 0.5px), #5d4037 calc(50% - 0.5px), #5d4037 calc(50% + 0.5px), transparent calc(50% + 0.5px)),
                        linear-gradient(to bottom left, transparent calc(50% - 0.5px), #5d4037 calc(50% - 0.5px), #5d4037 calc(50% + 0.5px), transparent calc(50% + 0.5px));
        }

        .river-text {
            position: absolute;
            top: 50%; left: 0; width: 100%;
            transform: translateY(-50%);
            text-align: center;
            font-size: ${RIVER_FONT_SIZE}px;
            font-family: "KaiTi", "SimSun", serif;
            color: rgba(93, 64, 55, 0.4);
            pointer-events: none;
            letter-spacing: ${Math.round(CELL_SIZE * 0.6)}px;
            text-indent: ${Math.round(CELL_SIZE * 0.6)}px;
        }

        .piece-layer {
            position: absolute;
            top: ${LAYER_OFFSET}px;
            left: ${LAYER_OFFSET}px;
            width: ${LAYER_WIDTH}px; 
            height: ${LAYER_HEIGHT}px;
            display: grid;
            grid-template-columns: repeat(9, ${CELL_SIZE}px);
            grid-template-rows: repeat(10, ${CELL_SIZE}px);
            z-index: 10;
            pointer-events: none;
        }

        .piece-spot {
            width: ${CELL_SIZE}px;
            height: ${CELL_SIZE}px;
            display: flex;
            justify-content: center;
            align-items: center;
            pointer-events: auto;
            cursor: pointer;
        }

        .pieces {
            width: ${PIECE_SIZE}px;
            height: ${PIECE_SIZE}px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: ${FONT_SIZE}px;
            font-weight: bold;
            font-family: "KaiTi", "SimSun", serif;
            
            position: relative !important; 
            top: auto !important; left: auto !important; margin: 0 !important;
            
            box-shadow: 3px 5px 8px rgba(0,0,0,0.4);
            transition: transform 0.2s, box-shadow 0.2s;
            z-index: 20;
            background: #fdf5e6;
        }

        .pieces:hover {
            transform: scale(1.15);
            z-index: 30;
            cursor: pointer;
            box-shadow: 0 0 15px rgba(255, 215, 0, 0.8);
        }

        .pieces.red {
            color: #cc0000;
            border: 3px solid #cc0000;
            box-shadow: inset 0 0 8px rgba(204,0,0,0.1), 2px 4px 6px rgba(0,0,0,0.3);
        }

        .pieces.black {
            color: #000;
            border: 3px solid #000;
            box-shadow: inset 0 0 8px rgba(0,0,0,0.1), 2px 4px 6px rgba(0,0,0,0.3);
        }

        .pieces.selected {
            background-color: #a5d6a7 !important;
            box-shadow: 0 0 0 4px #4caf50, 0 5px 15px rgba(0,0,0,0.4);
            transform: scale(1.15);
        }

        .board-wrapper.flipped { transform: rotate(180deg); }
        .board-wrapper.flipped .pieces { transform: rotate(180deg); }
        .board-wrapper.flipped .river-text { transform: translateY(-50%) rotate(180deg); }
    `;
}
