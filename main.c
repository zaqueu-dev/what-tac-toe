#include "main.h"
#include <jansson.h>
#include <sys/inotify.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

void process_move(json_t *req) {
  const char *request = json_object_get(req, "type");
  printf("Requisição: %s\n", request);
}

void process_json(const char *filename) {
  json_error_t error;
  json_t *root = json_load_file(filename, 0, &error);

  if (!root) {
    fprintf(stderr, "Erro ao carregar o arquivo JSON: %s\n", error.text);
    return;
  }

  json_t *req = json_object_get(root, "req");
  printf("req: %s\n", req);
  if (req) {
    process_move(req);
    Game game;
    initialize_game(&game, "/home/zaqueu/Code/C/projeto_antonio/board.png");

    printf("Jogo da Velha - X começa\n");

    while (true) {
      // Exibe o tabuleiro atual
      save_game_state(&game, "/home/zaqueu/Code/C/projeto_antonio/board.png");
      printf("Tabuleiro salvo em "
             "'/home/zaqueu/Code/C/projeto_antonio/board.png'\n");

      // Verifica se há vencedor
      if (check_winner(&game)) {
        printf("Jogador %c venceu!\n", game.player_turn ? 'O' : 'X');

        break;
      }

      // Verifica empate
      bool is_draw = true;
      for (int i = 0; i < BOARD_SIZE && is_draw; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
          if (game.game_board[i][j] == ' ') {
            is_draw = false;
            break;
          }
        }
      }

      if (is_draw) {
        printf("Empate!\n");
        break;
      }

      // Obtém jogada do usuário
      int row, col;
      printf("Jogador %c, digite linha e coluna (0-2): ",
             game.player_turn ? 'X' : 'O');
      scanf("%d %d", &row, &col);

      // Processa jogada
      if (!make_move(&game, row, col)) {
        printf("Jogada inválida! Tente novamente.\n");
      }
    }

    cleanup_game(&game);

    json_t *res = json_object();
    json_object_set_new(res, "status", json_string("processed"));
    json_object_set(root, "res", res);
    json_object_del(root, "req");

    if (json_dump_file(root, filename, JSON_INDENT(2))) {
      fprintf(stderr, "Erro ao salvar JSON\n");
    }
  }

  json_decref(root);
}

int main() {
  const char *filename = "bot.json";
  int fd = inotify_init();
  int wd = inotify_add_watch(fd, filename, IN_MODIFY);

  if (wd == -1) {
    perror("inotify_add_watch");
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

    for (int i = 0; i < length;) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->mask & IN_MODIFY) {
        printf("Arquivo modificado. Processando...\n");
        process_json(filename);
      }
      i += EVENT_SIZE + event->len;
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);

  return 0;
}