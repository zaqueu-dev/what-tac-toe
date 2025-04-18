const express = require("express");
const app = express();
const fs = require("fs");
const path = require("path");
const uuid = require("uuid");

const jsonFilePath = path.join(__dirname, "../C/bot.json");

app.use(express.json());

let gameIds = [];

function readJsonFile() {
  try {
    const file = fs.readFileSync(jsonFilePath);
    return JSON.parse(file);
  } catch (error) {
    console.error("Error reading file:", error);
    return {};
  }
}

function writeJsonFile(data) {
  try {
    fs.writeFileSync(jsonFilePath, JSON.stringify(data, null, 2));
  } catch (error) {
    console.error("Error writing file:", error);
  }
}

function checkIsValid() {
  const data = readJsonFile();
  if (data.req && data.req.type === "move" && data.isValid !== true) {
    processRes(data.res);
    data.isValid = true;
    writeJsonFile(data);
  }
  setTimeout(checkIsValid, 1000);
}

function processRes(res) {
  if (res && res.status === "ok") {
    console.log(res.message);
  } else if (res) {
    console.error("Error:", res.message);
  }
}

app.use(express.json());

app.get("/newGame", (req, res) => {
  const gameId = uuid.v4();
  gameIds.push(gameId);
  const gameMode = req.query.gameMode;
  const data = readJsonFile();
  data.req = {
    type: "newGame",
    gameId: "game5",
    gameMode,
  };
  data.isValid = true;
  writeJsonFile(data);

  res.send(`New game requested with id ${gameId} and game mode ${gameMode}!`);
});

app.post("/move", (req, res) => {
  const gameId = req.body.gameId;
  const position = req.body.position;
  const data = readJsonFile();
  data.req = {
    type: "move",
    gameId,
    position,
  };
  data.isValid = true;
  writeJsonFile(data);
  res.send(`Move requested for game ${gameId} at position ${position}!`);
});

checkIsValid();

app.listen(3000, () => {
  console.log("Example app listening on port 3000!");
});
