const { log } = require("console");
const express = require("express");
const app = express();
const fs = require("fs");
const path = require("path");
const uuid = require("uuid");

const jsonFilePath = path.join(__dirname, "../C/bot.json");

app.use(express.json());

let gameIds = [];
let lastProcessed = null;

function checkIsValid() {
  const data = readJsonFile();
  if (!data) return setTimeout(checkIsValid, 1000);

  const currentRequestKey = JSON.stringify(data.req) + data.isValid.toString();

  if (currentRequestKey === lastProcessed) {
    return setTimeout(checkIsValid, 1000);
  }

  if (data.req && data.isValid === true) {
    if (data.res) {
      processRes(data.res);
      lastProcessed = currentRequestKey;
    }
    else if (data.req.type === "move") {
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

app.put("/game", async (req, res) => {
  const { gameId, type, gameMode, position } = req.body;
  console.log("CHEGOU EM /game:", req.body);

  const wait = ms => new Promise(resolve => setTimeout(resolve, ms));
 
  if (!gameId || !type) {
    return res.status(400).json({ message: "gameId e type são obrigatórios" });
  }

  const cleanGameId = gameId.replace(/\D/g, '');

  const reqPayload = { type, gameId: cleanGameId, isValid: true, res: null };
  if (type === "newGame") {
    reqPayload.gameMode = gameMode ?? "1";
  } else if (type === "move" && position != null) {
    reqPayload.position = position;
  }

  if (!writeJsonFile({ req: reqPayload, isValid: true, res: null })) {
    return res.status(500).send("Erro ao criar requisição para o jogo");
  }

  let fileData;
  for (let i = 0; i < 20; i++) {
    await wait(300);
    fileData = readJsonFile();
    if (fileData?.res?.status) break;
  } 

  if (!fileData?.res) {
    console.error("Timeout aguardando resposta do C:", fileData);
    return res.status(504).send("Timeout aguardando resposta do motor");
  }

  console.log("Resposta do C:", fileData.res);
 
  const media = fileData.res.game_board_base64
    ? []
    : []; 

  const jump = fileData.res.jump;

  return res.json({ 
    isReturnData: true,
    data: { media, jump, gameId: cleanGameId }
  });
});

function readJsonFile() {
  try {
    const file = fs.readFileSync(jsonFilePath, 'utf8');
    return JSON.parse(file);
  } catch (error) {
    console.error("Error reading file:", error);
    return null;
  }
}

function writeJsonFile(obj) {
  try {
    fs.writeFileSync(jsonFilePath, JSON.stringify(obj, null, 2), 'utf8');
    return true;
  } catch (error) {
    console.error("Error writing file:", error);
    return false;
  }
}

async function pollForStatus(maxAttempts = 50, intervalMs = 300) {
  const wait = ms => new Promise(r => setTimeout(r, ms));
  for (let i = 0; i < maxAttempts; i++) {
    await wait(intervalMs);
    const fileData = readJsonFile();
    if (fileData && fileData.res && typeof fileData.res.status === 'string') {
      console.log(`⏱️  C respondeu (status='${fileData.res.status}') na tentativa ${i + 1}`);
      return fileData.res;
    }
  }
  return null;
}

async function pollComRetry(maxRetries = 3, delayRetry = 300) {
  for (let i = 0; i < maxRetries; i++) {
    const result = await pollForStatus();
    if (result) {
      return result;
    }
    console.warn(`⚠️ Tentativa ${i + 1} de ${maxRetries} falhou. Retentando em ${delayRetry}ms...`);
    await new Promise(r => setTimeout(r, delayRetry));
  }
  return null;
}

app.put('/move', async (req, res) => {
  console.log("➡️ CHEGOU EM /move:", req.body);

  const rawGameId = req.body.gameId || req.body.from;
  if (!rawGameId) {
    return res.status(400).json({ message: "gameId é obrigatório" });
  }
  const gameId = String(rawGameId).replace(/\D/g, '');
  if (!gameId) {
    return res.status(400).json({ message: "gameId deve conter ao menos um dígito" });
  }

  const position = req.body.position;
  const round = Number(req.body.round) || 1;

  if (!gameIds.includes(gameId)) {
    console.log(`🆕 Primeiro uso de gameId='${gameId}'. Enviando newGame...`);

    const newGamePayload = {
      req: {
        type:    "newGame",
        gameId:  gameId,
        gameMode: "1"
      },
      isValid: true,
      res:     null
    };

    console.log("📤 Gravando newGame em bot.json:", JSON.stringify(newGamePayload, null, 2));
    if (!writeJsonFile(newGamePayload)) {
      return res.status(500).json({ message: "Falha ao gravar newGame em bot.json" });
    }

    await new Promise(r => setTimeout(r, 100));

    const newRes = await pollComRetry();
    if (!newRes) {
      console.error("⏰ Timeout aguardando resposta do C para newGame");
      return res.status(504).json({ message: "Timeout aguardando resposta do motor para newGame" });
    }
    console.log("✅ Resposta do C ao newGame:", newRes);

    gameIds.push(gameId);
  }

  if (round === 1) {
    console.log("⏳ Esperando 500ms antes do primeiro move...");
    await new Promise(r => setTimeout(r, 500));
  }

  const movePayload = {
    req: {
      type:    "move",
      gameId:  gameId
    },
    isValid: true,
    res:     null
  };
  if (Number.isInteger(position)) {
    movePayload.req.position = position;
  }

  console.log("📤 Gravando move em bot.json:", JSON.stringify(movePayload, null, 2));
  if (!writeJsonFile(movePayload)) {
    return res.status(500).json({ message: "Falha ao gravar move em bot.json" });
  }

  const resData = await pollComRetry();
  if (!resData) {
    console.error("⏰ Timeout aguardando resposta do C para move");
    return res.status(504).json({ message: "Timeout aguardando resposta do motor para move" });
  }
  console.log("✅ Resposta completa do C ao move:", resData);

  const possible = Array.isArray(resData.possibleMoves) ? resData.possibleMoves : [];
  const list = possible.map(p => ({
    id:    String(p),
    value: `${p}\n`
  }));

  const media = resData.game_board_base64
    ? [{
        type:     "IMAGE",
        mimeType: "image/png",
        data:     resData.game_board_base64
      }]
    : [];

  const jump = resData.jump || "option:/move";

  return res.json({
    isReturnData: true,
    data: { list, jump, media }
  });
});


// Usar depois
/*app.put("/endGame", (req, res) => {
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
}); */

checkIsValid();

app.listen(3000, () => {
  console.log("Server running on port 3000");
});
