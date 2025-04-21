#include "main.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_wrapper.h"
#include <stdio.h> // Para stderr, fprintf, etc.

// Adicione esta função no seu código
bool check_winner(Game *game) {
    // Verifica linhas
    for (int i = 0; i < 3; i++) {
        if (game->game_board[i][0] != ' ' && 
            game->game_board[i][0] == game->game_board[i][1] && 
            game->game_board[i][1] == game->game_board[i][2]) {
            game->winning_line.start_row = i;
            game->winning_line.start_col = 0;
            game->winning_line.end_row = i;
            game->winning_line.end_col = 2;
            game->winning_line.direction = 'H';
            return true;
        }
    }

    // Verifica colunas
    for (int j = 0; j < 3; j++) {
        if (game->game_board[0][j] != ' ' && 
            game->game_board[0][j] == game->game_board[1][j] && 
            game->game_board[1][j] == game->game_board[2][j]) {
            game->winning_line.start_row = 0;
            game->winning_line.start_col = j;
            game->winning_line.end_row = 2;
            game->winning_line.end_col = j;
            game->winning_line.direction = 'V';
            return true;
        }
    }

    // Verifica diagonal principal
    if (game->game_board[0][0] != ' ' && 
        game->game_board[0][0] == game->game_board[1][1] && 
        game->game_board[1][1] == game->game_board[2][2]) {
        game->winning_line.start_row = 0;
        game->winning_line.start_col = 0;
        game->winning_line.end_row = 2;
        game->winning_line.end_col = 2;
        game->winning_line.direction = 'D';
        return true;
    }

    // Verifica diagonal secundária
    if (game->game_board[0][2] != ' ' && 
        game->game_board[0][2] == game->game_board[1][1] && 
        game->game_board[1][1] == game->game_board[2][0]) {
        game->winning_line.start_row = 0;
        game->winning_line.start_col = 2;
        game->winning_line.end_row = 2;
        game->winning_line.end_col = 0;
        game->winning_line.direction = 'D';
        return true;
    }

    return false;
}

bool is_draw(Game *game) {
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      if (game->game_board[i][j] == ' ') {
        return false;
      }
    }
  }
  return true;
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

void initialize_game_board(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->game_board[i][j] = ' ';
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

  initialize_game_board(game);

  game->player_turn = true;
}

bool make_move(Game *game, int pos) {
  int row = (pos - 1) / BOARD_SIZE;
  int col = (pos - 1) % BOARD_SIZE;

  if (game->game_board[row][col] == 'X' || game->game_board[row][col] == 'O') {
    printf("Position already occupied: %d, %d\n", row, col);
    return false;
  }

  game->game_board[row][col] = game->player_turn ? 'X' : 'O';
  draw_symbol(game, row, col);
  game->player_turn = !game->player_turn;
  return true;
}

void draw_winning_line(Game *game) {
    // Cores: verde para X (true), azul para O (false)
    unsigned char color[3];
    if (game->player_turn) {
        // O ganhou
        color[0] = 0; color[1] = 0; color[2] = 255;
    } else {
        // X ganhou
        color[0] = 0; color[1] = 255; color[2] = 0;
    }

    int thickness = 10;
    float radius = thickness / 2.0;

    int cell_width = game->width / BOARD_SIZE;
    int cell_height = game->height / BOARD_SIZE;

    // Pega os centros das células
    int x1 = game->winning_line.start_col * cell_width + cell_width / 2;
    int y1 = game->winning_line.start_row * cell_height + cell_height / 2;
    int x2 = game->winning_line.end_col * cell_width + cell_width / 2;
    int y2 = game->winning_line.end_row * cell_height + cell_height / 2;

    // Estica a linha até perto das bordas
    float offset_x = cell_width / 2.5;
    float offset_y = cell_height / 2.5;

    if (x2 > x1) x2 += offset_x; else if (x2 < x1) x2 -= offset_x;
    if (y2 > y1) y2 += offset_y; else if (y2 < y1) y2 -= offset_y;
    if (x1 > x2) x1 += offset_x; else if (x1 < x2) x1 -= offset_x;
    if (y1 > y2) y1 += offset_y; else if (y1 < y2) y1 -= offset_y;

    // Algoritmo de Bresenham
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Desenha um círculo ao redor do ponto
        for (int i = -thickness; i <= thickness; i++) {
            for (int j = -thickness; j <= thickness; j++) {
                if (i*i + j*j <= radius*radius) {
                    int px = x1 + i;
                    int py = y1 + j;
                    if (px >= 0 && px < game->width && py >= 0 && py < game->height) {
                        int pos = (py * game->width + px) * game->channels;
                        for (int c = 0; c < 3 && c < game->channels; c++) {
                            // Blend de cor (suaviza visualmente)
                            float alpha = 0.8f;
                            game->board_image[pos + c] =
                                (unsigned char)(alpha * color[c] + (1 - alpha) * game->board_image[pos + c]);
                        }
                    }
                }
            }
        }

        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}


void save_game_state(Game *game, const char *output_path) {
    // Se houver um vencedor, desenha a linha
    if (check_winner(game)) {
        draw_winning_line(game);
    }
    
    // Salva a imagem
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

char *encode_base64(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *file_data = malloc(file_size);
    if (!file_data) {
        fclose(file);
        perror("Erro ao alocar memória para o arquivo");
        return NULL;
    }

    if (fread(file_data, 1, file_size, file) != file_size) {
        perror("Erro ao ler o arquivo completamente");
        free(file_data);
        fclose(file);
        return NULL;
    }

    fclose(file);

    // + espaço extra para quebras de linha e terminador
    size_t base64_size = 4 * ((file_size + 2) / 3) + ((file_size / 48) + 1) * 2;
    char *base64_output = malloc(base64_size + 1);
    if (!base64_output) {
        free(file_data);
        perror("Erro ao alocar memória para o Base64");
        return NULL;
    }

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    int out_len = 0, tmp_len = 0;
    EVP_EncodeInit(ctx);
    EVP_EncodeUpdate(ctx, (unsigned char *)base64_output, &out_len, file_data, file_size);
    EVP_EncodeFinal(ctx, (unsigned char *)base64_output + out_len, &tmp_len);
    out_len += tmp_len;
    base64_output[out_len] = '\0';

    EVP_ENCODE_CTX_free(ctx);
    free(file_data);

    return base64_output;
}
