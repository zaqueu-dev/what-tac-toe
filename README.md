Here's an updated version of the README file that includes the Node.js part:

# Tic-Tac-Toe Game Project

## Overview

This is a simple implementation of a Tic-Tac-Toe game using C programming language and Node.js. The game allows two players, 'X' and 'O', to play against each other. The game board is a 3x3 grid, and the players take turns marking a square on the grid with their symbol.

## Features

- Simple and easy-to-use game interface
- 3x3 game board for playing Tic-Tac-Toe
- Two-player game, 'X' and 'O'
- Players take turns marking a square on the grid with their symbol
- Game checks for valid moves and updates the game board accordingly
- Game checks for a winner after each move
- Node.js server for handling game requests and responses

## Requirements

- C compiler (e.g. GCC)
- C standard library (e.g. libc)
- Jansson library for JSON parsing (optional)
- Node.js (for server-side logic)
- Express.js (for server-side routing)
- UUID library (for generating unique game IDs)

## Installation

1. Clone the repository: `git clone https://github.com/your-username/tic-tac-toe.git`
2. Change into the project directory: `cd tic-tac-toe`
3. Compile the C code: `gcc -o tic-tac-toe main.c game.c -ljansson` (if using Jansson library)
4. Install Node.js dependencies: `npm install`
5. Start the Node.js server: `node index.js`

## Usage

1. Start the Node.js server: `node index.js`
2. Use a tool like `curl` to send a request to the server to start a new game: `curl -X PUT -H "Content-Type: application/json" -d '{"type": "newGame"}' http://localhost:3000/newGame`
3. The server will respond with a JSON object containing the game ID and the initial game state.
4. Players can make moves by sending a request to the server with the move details: `curl -X PUT -H "Content-Type: application/json" -d '{"type": "move", "gameId": "game123", "position": 1}' http://localhost:3000/move`
5. The server will respond with a JSON object containing the updated game state.

## JSON File Format

The game uses a JSON file to store the game state. The JSON file has the following format:

```json
{
  "req": {
    "type": "move",
    "gameId": "game",
    "position": 1
  },
  "res": {
    "status": "valid",
    "position": 1,
    "row": 0,
    "col": 0
  },
  "isValid": true
}
```

The `req` object contains the move request, with `type` set to "move", `gameId` set to the ID of the game, and `position` set to the number of the square where the player wants to place their symbol.

The `res` object contains the response from the game, with `status` set to "valid" if the move is valid, `position` set to the number of the square where the player placed their symbol, `row` set to the row number of the square, and `col` set to the column number of the square.

The `isValid` field is set to `true` if the move is valid, and `false` otherwise.

## Contributing

Contributions are welcome! If you'd like to contribute to this project, please fork the repository and submit a pull request with your changes.

## License

This project is licensed under the MIT License. See LICENSE for details.

## Acknowledgments

- Jansson library for JSON parsing
- GCC compiler for compiling the C code
- Node.js and Express.js for server-side logic
- UUID library for generating unique game IDs
