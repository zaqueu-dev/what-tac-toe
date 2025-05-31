// main.c

#include "main.h"
#include <jansson.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>

#define MAX_GAMES 10
#define POLL_INTERVAL_USEC 200000  // 200 ms

typedef struct {
  char game_id[64];
  Game game;
} GameEntry;

static GameEntry games[MAX_GAMES];
static int game_count = 0;
static time_t last_modified = 0;

//--- forward declarations from game.c ---
bool check_winner(Game *game);
bool is_draw(Game *game);
void save_game_state(Game *game, const char *output_path);
char *encode_base64(const char *filename);
bool make_move(Game *game, int pos);
//-----------------------------------------

/// apaga todos os .png em games/ usando stat() em vez de d_type
void clear_game_images(const char *folder) {
  DIR *dir = opendir(folder);
  if (!dir) return;

  struct dirent *entry;
  char filepath[512];
  struct stat st;

  while ((entry = readdir(dir))) {
    snprintf(filepath, sizeof(filepath), "%s/%s", folder, entry->d_name);
    if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
      const char *ext = strrchr(entry->d_name, '.');
      if (ext && strcmp(ext, ".png") == 0) {
        remove(filepath);
      }
    }
  }
  closedir(dir);
}

/// reseta bot.json para estado inicial
void reset_bot_json(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) return;
  const char *content =
    "{\n"
    "  \"req\": null,\n"
    "  \"isValid\": false,\n"
    "  \"res\": null\n"
    "}";
  fputs(content, f);
  fclose(f);
}

/// encontra ou cria uma entrada de jogo em memória
Game *get_or_create_game(const char *id, const char *image_path) {
  for (int i = 0; i < game_count; ++i) {
    if (strcmp(games[i].game_id, id) == 0)
      return &games[i].game;
  }
  if (game_count >= MAX_GAMES) return NULL;
  strcpy(games[game_count].game_id, id);
  initialize_game(&games[game_count].game, image_path);
  return &games[game_count++].game;
}

/// verifica se o JSON pode ser processado
bool can_c_operate(json_t *root) {
  json_t *f = json_object_get(root, "isValid");
  return f && json_is_true(f);
}

void set_can_cdo(json_t *root, bool v) {
  json_object_set_new(root, "isValid", json_boolean(v));
}

/// escolhe movimento aleatório do computador
static int choose_computer_move(Game *game) {
  int avail[9], cnt = 0;
  for (int pos = 1; pos <= 9; pos++) {
    int r = (pos-1)/3, c = (pos-1)%3;
    if (game->game_board[r][c] == ' ')
      avail[cnt++] = pos;
  }
  if (cnt == 0) return -1;
  return avail[rand() % cnt];
}

void process_move(json_t *req, json_t *res, Game *game) {
  char img_path[128];

  json_t *pos_obj = json_object_get(req, "position");
  bool has_position = pos_obj && json_is_integer(pos_obj);
  int position = has_position ? (int)json_integer_value(pos_obj) : -1;

  if (has_position) {
    int row = (position-1)/3, col = (position-1)%3;
    if (game->game_board[row][col] == ' ') {
      game->game_board[row][col] = 'X';
      draw_symbol(game, row, col);
      json_object_set_new(res, "status",   json_string("valid"));
      json_object_set_new(res, "position", json_integer(position));
    } else {
      json_object_set_new(res, "status",  json_string("invalid"));
      json_object_set_new(res, "message", json_string("Posição já ocupada"));
      return;
    }

    snprintf(img_path, sizeof(img_path), "games/%s.png",
             json_string_value(json_object_get(req, "gameId")));
    save_game_state(game, img_path);

    if (!check_winner(game) && !is_draw(game)) {
      int compPos = choose_computer_move(game);
      printf("🔄 Computador escolheu a posição %d\n", compPos);
      if (compPos != -1) {
        int crow = (compPos-1)/3, ccol = (compPos-1)%3;
        game->game_board[crow][ccol] = 'O';
        draw_symbol(game, crow, ccol);
        json_object_set_new(res, "computerPosition", json_integer(compPos));
      }
    }

    save_game_state(game, img_path);

  } else {
    json_object_set_new(res, "status",  json_string("ok"));
    json_object_set_new(res, "message", json_string("Estado atual do jogo."));
    snprintf(img_path, sizeof(img_path), "games/%s.png",
             json_string_value(json_object_get(req, "gameId")));
  }

  // 5) Codifica imagem em Base64
  char *b64 = encode_base64(img_path);
  json_object_set_new(res, "game_board_base64",
                     json_string(b64 ? b64 : "Erro ao gerar Base64"));
  free(b64);

  // 6) Lista de movimentos restantes
  json_t *possible = json_array();
  for (int pos = 1; pos <= 9; pos++) {
    int r = (pos-1)/3, c = (pos-1)%3;
    if (game->game_board[r][c] == ' ')
      json_array_append_new(possible, json_integer(pos));
  }

  // 7) Jump e limpeza de possibleMoves em vitória/empate
  if (check_winner(game)) {
    json_object_set_new(res, "jump", json_string("info:/victory"));
    json_array_clear(possible);
  }
  else if (json_array_size(possible) == 0) {
    json_object_set_new(res, "jump", json_string("info:/draw"));
    json_array_clear(possible);
  }
  else {
    json_object_set_new(res, "jump", json_string("option:/move"));
  }

  // define possibleMoves no JSON de resposta
  json_object_set_new(res, "possibleMoves", possible);
}

void process_json(const char *filename) {
  struct stat st;
  if (stat(filename, &st) != 0) return;
  if (st.st_mtime <= last_modified) return;
  last_modified = st.st_mtime;

  json_error_t err;
  json_t *root = json_load_file(filename, 0, &err);
  if (!root) return;
  if (!can_c_operate(root)) { json_decref(root); return; }

  json_t *req = json_object_get(root, "req");
  json_t *res = json_object();
  const char *type    = json_string_value(json_object_get(req, "type"));
  const char *game_id = json_string_value(json_object_get(req, "gameId"));

  if (!type || !game_id) {
    json_object_set_new(res, "status",  json_string("error"));
    json_object_set_new(res, "message", json_string("type ou gameId ausente"));
  }
  else if (!strcmp(type, "move")) {
    Game *g = get_or_create_game(game_id, "games/board.png");
    if (!g) {
      json_object_set_new(res,"status",  json_string("error"));
      json_object_set_new(res,"message", json_string("Limite de jogos atingido"));
    } else {
      process_move(req, res, g);
    }
  }
  else if (!strcmp(type, "newGame")) {
    // copia tabuleiro inicial
    char img_path[128];
    snprintf(img_path, sizeof(img_path), "games/%s.png", game_id);
    FILE *src = fopen("board.png","rb");
    FILE *dst = fopen(img_path,"wb");
    if (src && dst) {
      char buf[4096];
      size_t n;
      while ((n = fread(buf,1,sizeof(buf),src)) > 0) {
        fwrite(buf,1,n,dst);
      }
    }
    if (src) fclose(src);
    if (dst) fclose(dst);

    Game *g = get_or_create_game(game_id, img_path);
    json_object_set_new(res, "status",
      json_string(g ? "ok" : "error"));
    json_object_set_new(res, "message",
      json_string(g ? "Novo jogo criado" : "Limite de jogos atingido"));

    char *b64 = encode_base64(img_path);
    json_object_set_new(res, "game_board_base64",
      json_string(b64 ? b64 : "Erro"));
    free(b64);

    json_object_set_new(res, "jump", json_string("option:/move"));
  }
  // ... outros tipos (endGame, etc.) podem ser adicionados aqui ...

  json_object_set_new(root, "res", res);
  set_can_cdo(root, false);
  json_dump_file(root, filename, JSON_INDENT(2));
  json_decref(root);
}

int main() {
  srand((unsigned)time(NULL));   // seed para o computador
  const char *filename = "bot.json";

  // → Reinicia o estado do jogo ao subir
  reset_bot_json(filename);
  clear_game_images("games");
  printf("🔁 Estado reiniciado: bot.json limpo e imagens removidas\n");

  // loop de polling
  while (1) {
    printf(">>> C: Monitorando %s\n", filename);
    process_json(filename);
    usleep(POLL_INTERVAL_USEC);
  }

  return 0;
}