#include "main.h"
#include <jansson.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))
#define MAX_GAMES 10

typedef struct {
  char game_id[64];
  Game game;
} GameEntry;

GameEntry games[MAX_GAMES];
int game_count = 0;

static time_t last_modified = 0;

Game *get_or_create_game(const char *id, const char *image_path) {
  for (int i = 0; i < game_count; ++i) {
    if (strcmp(games[i].game_id, id) == 0) {
      return &games[i].game;
    }
  }

  if (game_count >= MAX_GAMES)
    return NULL;

  strcpy(games[game_count].game_id, id);
  initialize_game(&games[game_count].game, image_path);
  return &games[game_count++].game;
}

bool can_c_operate(json_t *root) {
  json_t *canCdo = json_object_get(root, "isValid");
  return canCdo && json_is_true(canCdo);
}

void set_can_cdo(json_t *root, bool value) {
  json_object_set_new(root, "isValid", json_boolean(value));
}

void process_move(json_t *req, json_t *res, Game *game) {
  const char *type = json_string_value(json_object_get(req, "type"));
  const char *game_id = json_string_value(json_object_get(req, "gameId"));
  int position = json_integer_value(json_object_get(req, "position"));

  // Verifica primeiro se a posição é válida
  if (position < 1 || position > 9) {
    json_object_set_new(res, "status", json_string("invalid"));
    json_object_set_new(res, "message", json_string("Invalid position"));
    return;
  }

  // Se já há um vencedor, não permitir mais jogadas
  if (check_winner(game)) {
    json_object_set_new(res, "status", json_string("finished"));
    json_object_set_new(res, "message", json_string("O jogo já terminou."));
    return;
  }

  // Verifica se a posição já está ocupada
  int row = (position - 1) / 3;
  int col = (position - 1) % 3;
  if (game->game_board[row][col] != ' ') {
    json_object_set_new(res, "status", json_string("invalid"));
    json_object_set_new(res, "message", json_string("Posição já ocupada"));
    return;
  }

  // Tenta fazer o movimento
  bool move_valid = make_move(game, position);
  if (!move_valid) {
    json_object_set_new(res, "status", json_string("invalid"));
    json_object_set_new(res, "message", json_string("Movimento inválido"));
    return;
  }

  json_object_set_new(res, "status", json_string("valid"));
  json_object_set_new(res, "position", json_integer(position));
  json_object_set_new(res, "row", json_integer(row));
  json_object_set_new(res, "col", json_integer(col));

  // Salva a imagem do estado atual do jogo (com base no game_board)
  char game_image[128];
  snprintf(game_image, sizeof(game_image), "games/%s.png", game_id);
  save_game_state(game, game_image);

  // Converte a imagem para Base64
  char *base64_image = encode_base64(game_image);
  if (base64_image) {
    json_t *encoded_str = json_string(base64_image);  // cria json string
    json_object_set_new(res, "game_board_base64", encoded_str); // seta no objeto
    free(base64_image); // agora é seguro liberar
  } else {
    json_object_set_new(res, "game_board_base64", json_string("Erro ao gerar Base64"));
  }


  // Verifica se há um vencedor
  if (check_winner(game)) {
    char winner_str[2] = {game->player_turn ? 'O' : 'X', '\0'};
    json_object_set_new(res, "game_state", json_string("finished"));
    json_object_set_new(res, "winner", json_string(winner_str));

    // Linha vencedora
    json_t *winning_line = json_object();
    json_object_set_new(winning_line, "start_row", json_integer(game->winning_line.start_row));
    json_object_set_new(winning_line, "start_col", json_integer(game->winning_line.start_col));
    json_object_set_new(winning_line, "end_row", json_integer(game->winning_line.end_row));
    json_object_set_new(winning_line, "end_col", json_integer(game->winning_line.end_col));
    json_object_set_new(winning_line, "direction", json_string(&game->winning_line.direction));

    json_object_set_new(res, "winning_line", winning_line);
  }
}


void process_json(const char *filename) {
  struct stat file_stat;
  if (stat(filename, &file_stat) != 0) {
    perror("stat");
    return;
  }

  if (file_stat.st_mtime <= last_modified) {
    return;
  }

  json_error_t error;
  json_t *root;
  int attempts = 0;

  while ((root = json_load_file(filename, 0, &error)) == NULL && attempts < 5) {
    usleep(100000);
    attempts++;
  }

  if (!root) {
    fprintf(stderr, "Erro ao ler JSON: %s\n", error.text);
    return;
  }

  json_t *valid_flag = json_object_get(root, "isValid");
  if (!json_is_true(valid_flag)) {
    json_decref(root);
    return;
  }

  json_t *req = json_object_get(root, "req");
  if (!req || json_object_size(req) == 0) {
    json_decref(root);
    return;
  }

  const char *type = json_string_value(json_object_get(req, "type"));
  const char *game_id = json_string_value(json_object_get(req, "gameId"));
  json_t *res = json_object();

  if (!type || !game_id) {
    json_object_set_new(res, "status", json_string("error"));
    json_object_set_new(res, "message", json_string("type ou gameId ausente"));
  } else if (strcmp(type, "move") == 0) {
    char game_image[128];
    snprintf(game_image, sizeof(game_image), "games/%s.png", game_id);
    Game *game = get_or_create_game(game_id, game_image);
    if (!game) {
      json_object_set_new(res, "status", json_string("error"));
      json_object_set_new(res, "message",
                          json_string("Limite de jogos atingido"));
    } else {
      process_move(req, res, game);
      save_game_state(game, game_image);
    }
  } else if (strcmp(type, "newGame") == 0) {
    char new_image[128];
    snprintf(new_image, sizeof(new_image), "games/%s.png", game_id);

    // Copiar board.png para <gameId>.png
    FILE *src = fopen("board.png", "rb");
    FILE *dst = fopen(new_image, "wb");
    if (src && dst) {
      char buf[4096];
      size_t bytes;
      while ((bytes = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, bytes, dst);
      }
    }
    if (src)
      fclose(src);
    if (dst)
      fclose(dst);

    Game *game = get_or_create_game(game_id, new_image);
    if (!game) {
      json_object_set_new(res, "status", json_string("error"));
      json_object_set_new(res, "message",
                          json_string("Limite de jogos atingido"));
    } else {
      json_object_set_new(res, "status", json_string("ok"));
      json_object_set_new(res, "message", json_string("Novo jogo criado"));
    }
  } else if (strcmp(type, "endGame") == 0) {
    const char *game_id = json_string_value(json_object_get(req, "gameId"));
    Game *game = NULL;
    for (int i = 0; i < game_count; ++i) {
      if (strcmp(games[i].game_id, game_id) == 0) {
        game = &games[i].game;
        break;
      }
    }

    if (game) {
      // Delete the board image
      char game_image[128];
      snprintf(game_image, sizeof(game_image), "games/%s.png", game_id);
      if (remove(game_image) != 0) {
        perror("Error deleting game image");
      }

      // Remove the game object from the array
      for (int i = 0; i < game_count; ++i) {
        if (strcmp(games[i].game_id, game_id) == 0) {
          cleanup_game(&games[i].game);
          memmove(&games[i], &games[i + 1], (game_count - i - 1) * sizeof(GameEntry));
          game_count--;
          break;
        }
      }

      // Set the response
      json_object_set_new(res, "status", json_string("ok"));
      json_object_set_new(res, "message", json_string("Game deleted successfully"));
    } else {
      json_object_set_new(res, "status", json_string("error"));
      json_object_set_new(res, "message", json_string("Game not found"));
    }
  }

  json_object_set_new(root, "res", res);
  set_can_cdo(root, false);

  if (json_dump_file(root, filename, JSON_INDENT(2))) {
    fprintf(stderr, "Erro ao salvar JSON\n");
  }

  last_modified = file_stat.st_mtime;
  json_decref(root);
}

int main() {
  const char *filename = "bot.json";
  int fd = inotify_init();

  if (fd == -1) {
    perror("inotify_init");
    exit(EXIT_FAILURE);
  }

  int wd = inotify_add_watch(fd, filename, IN_MODIFY);
  if (wd == -1) {
    perror("inotify_add_watch");
    close(fd);
    exit(EXIT_FAILURE);
  }

  printf("Monitorando %s...\n", filename);

  char buffer[BUF_LEN];
  while (1) {
    int length = read(fd, buffer, BUF_LEN);
    if (length < 0) {
      perror("read");
      break;
    }

    struct inotify_event *event = (struct inotify_event *)buffer;
    if (event->mask & IN_MODIFY) {
      printf("Arquivo modificado. Verificando...\n");
      process_json(filename);
    }

    usleep(50000);
  }

  inotify_rm_watch(fd, wd);
  close(fd);
  return 0;
}


