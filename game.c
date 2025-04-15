#include "main.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_wrapper.h"
#include <stdio.h> // Para stderr, fprintf, etc.

bool check_winner(Game *game) {
  // Verifica linhas e colunas
  for (int i = 0; i < BOARD_SIZE; i++) {
    // Linhas
    if (game->game_board[i][0] != ' ' &&
        game->game_board[i][0] == game->game_board[i][1] &&
        game->game_board[i][1] == game->game_board[i][2]) {
      return true;
    }
    // Colunas
    if (game->game_board[0][i] != ' ' &&
        game->game_board[0][i] == game->game_board[1][i] &&
        game->game_board[1][i] == game->game_board[2][i]) {
      return true;
    }
  }

  // Diagonais
  if (game->game_board[0][0] != ' ' &&
      game->game_board[0][0] == game->game_board[1][1] &&
      game->game_board[1][1] == game->game_board[2][2]) {
    return true;
  }

  if (game->game_board[0][2] != ' ' &&
      game->game_board[0][2] == game->game_board[1][1] &&
      game->game_board[1][1] == game->game_board[2][0]) {
    return true;
  }

  return false;
}

void cleanup_game(Game *game) { stbi_image_free(game->board_image); }

void draw_symbol(Game *game, int row, int col) {
  int center_x = col * CELL_SIZE + CELL_SIZE / 2;
  int center_y = row * CELL_SIZE + CELL_SIZE / 2;
  int size = CELL_SIZE / 3;

  unsigned char color[3] = {0};
  if (game->game_board[row][col] == 'X') {
    color[0] = 255; // Vermelho para X
  } else {
    color[2] = 255; // Azul para O
  }

  if (game->game_board[row][col] == 'X') {
    // Desenha X
    for (int i = -size; i <= size; i++) {
      for (int j = -2; j <= 2; j++) { // Espessura do X
        // Diagonal
        int x = center_x + i + j;
        int y = center_y + i;
        if (x >= 0 && x < game->width && y >= 0 && y < game->height) {
          int pos = (y * game->width + x) * game->channels;
          for (int c = 0; c < 3 && c < game->channels; c++) {
            game->board_image[pos + c] = color[c];
          }
        }

        // Diagonal /
        x = center_x + i + j;
        y = center_y - i;
        if (x >= 0 && x < game->width && y >= 0 && y < game->height) {
          int pos = (y * game->width + x) * game->channels;
          for (int c = 0; c < 3 && c < game->channels; c++) {
            game->board_image[pos + c] = color[c];
          }
        }
      }
    }
  } else {
    // Desenha O (círculo)
    for (int angle = 0; angle < 360; angle++) {
      float rad = angle * 3.14159265 / 180;
      for (int r = size - 2; r <= size + 2; r++) { // Espessura do O
        int x = center_x + cos(rad) * r;
        int y = center_y + sin(rad) * r;
        if (x >= 0 && x < game->width && y >= 0 && y < game->height) {
          int pos = (y * game->width + x) * game->channels;
          for (int c = 0; c < 3 && c < game->channels; c++) {
            game->board_image[pos + c] = color[c];
          }
        }
      }
    }
  }
}

void initialize_game(Game *game, const char *image_path) {

  game->board_image =
      stbi_load(image_path, &game->width, &game->height, &game->channels, 0);

  /* In order to read an image from the disk we’ll use the stbi_load function
     that receives as arguments the image file path, width and height of the
     image, the number of color channels and the desired number of color
     channels. The last argument of the function is useful if for example you
     want to load only the R, G, B channels from a four channel, PNG, image and
     ignore the transparency channel. If you want to load the image as is, pass
     0 as the last parameter of the load function. In case of error, the
     stbi_load function returns NULL.

     "Paul" - Solarian Programmer

  */

  if (!game->board_image) {
    fprintf(stderr, "Erro ao carregar a imagem: %s\n", stbi_failure_reason());
    exit(1);
  }

  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      game->game_board[i][j] = ' ';
    }
  }

  game->player_turn = true;
}

bool make_move(Game *game, int row, int col) {
  if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE ||
      game->game_board[row][col] != ' ') {
    return false; // Jogada inválida
  }

  game->game_board[row][col] = game->player_turn ? 'X' : 'O';
  draw_symbol(game, row, col);
  game->player_turn = !game->player_turn;
  return true;
}

void save_game_state(Game *game, const char *output_path) {
  if (!stbi_write_png(output_path, game->width, game->height, game->channels,
                      game->board_image, game->width * game->channels)) {
    fprintf(stderr, "Erro ao salvar a imagem\n");
  }
}

int arquivo_modificado(const char *filename, time_t *last_mod_time) {
  struct stat attr;
  if (stat(filename, &attr) != 0) {
    perror("Erro ao obter informações do arquivo");
    return -1;
  }

  if (difftime(attr.st_mtime, *last_mod_time) > 0) {
    *last_mod_time = attr.st_mtime;
    return 1; // Modificado
  }
  return 0; // Não modificado
}

void processar_json(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Erro ao abrir arquivo");
    return;
  }

  printf("Processando JSON...\n");
  char buffer[1024];
  while (fgets(buffer, sizeof(buffer), file)) {
    printf("%s", buffer);
  }
  printf("\n");
  fclose(file);
}