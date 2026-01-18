import { GameController } from './src/core/gameController.js';
import { OPENING_POSITION, parsePosition } from './src/core/config.js';

const initialPosition = parsePosition(OPENING_POSITION);
const game = new GameController(initialPosition);

window.addEventListener('load', () => {
    game.initListeners();
});
