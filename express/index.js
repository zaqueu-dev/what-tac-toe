const express = require("express");
const app = express();
const fs = require("fs");
const path = require("path");
const uuid = require("uuid");

const jsonFilePath = path.join(__dirname, "../C/bot.json");

app.use(express.json());

let gameIds = [];
let lastProcessed = null;

function readJsonFile() {
  try {
    const file = fs.readFileSync(jsonFilePath);
    return JSON.parse(file);
  } catch (error) {
    console.error("Error reading file:", error);
    return null;
  }
}

function writeJsonFile(data) {
  try {
    fs.writeFileSync(jsonFilePath, JSON.stringify(data, null, 2));
    return true;
  } catch (error) {
    console.error("Error writing file:", error);
    return false;
  }
}

function checkIsValid() {
  const data = readJsonFile();
  if (!data) return setTimeout(checkIsValid, 1000);

  // Cria uma chave única para identificar a requisição atual
  const currentRequestKey = JSON.stringify(data.req) + data.isValid.toString();

  // Se já processamos esta requisição, ignora
  if (currentRequestKey === lastProcessed) {
    return setTimeout(checkIsValid, 1000);
  }

  if (data.req && data.isValid === true) {
    // Se for uma resposta (res) e não uma requisição (req)
    if (data.res) {
      processRes(data.res);
      // Marca como processado
      lastProcessed = currentRequestKey;
    }
    // Se for uma requisição que ainda não foi processada pelo C
    else if (data.req.type === "move") {
      // Aguarda o C processar (não faz nada aqui)
    }
  }

  setTimeout(checkIsValid, 1000);
}

function processRes(res) {
  if (!res) return;

  if (
    res.status === "ok" ||
    res.status === "valid" ||
    res.status === "ongoing"
  ) {
    console.log("Success:", res.message || "Operation completed successfully");
  } else if (res.status === "invalid") {
    console.log(
      "Invalid move:",
      res.message || "Position already taken or invalid"
    );
  } else if (res.status === "error") {
    console.error("Error:", res.message || "An error occurred");
  }
}

app.put("/newGame", (req, res) => {
  const gameId = "game" + (gameIds.length + 1); // Simplificado para exemplo
  gameIds.push(gameId);
  const gameMode = req.query.gameMode;

  const data = {
    req: {
      type: "newGame",
      gameId,
      gameMode,
    },
    isValid: true,
    res: null,
  };

  if (writeJsonFile(data)) {
    res.send(`New game created with id ${gameId} and game mode ${gameMode}!`);
  } else {
    res.status(500).send("Error creating new game");
  }
});

app.put("/move", (req, res) => {
  const { gameId, position } = req.body;

  if (!gameId || !position) {
    return res.status(400).send("Game ID and position are required");
  }

  const data = {
    req: {
      type: "move",
      gameId,
      position,
    },
    isValid: true,
    res: null,
  };

  if (writeJsonFile(data)) {
    res.send(`Move requested for game ${gameId} at position ${position}`);
  } else {
    res.status(500).send("Error processing move");
  }
});

app.put("/endGame", (req, res) => {
  const { gameId } = req.body;

  const data = {
    req: {
      type: "endGame",
      gameId,
    },
    isValid: true,
    res: null,
  };

  if (writeJsonFile(data)) {
    res.send(`Game ${gameId} ended successfully`);
  } else {
    res.status(500).send("Error ending game");
  }
});

// Inicia o loop de verificação
checkIsValid();

app.listen(3000, () => {
  console.log("Server running on port 3000");
});
