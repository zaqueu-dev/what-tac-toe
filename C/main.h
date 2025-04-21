#include "stb_wrapper.h"
#include <math.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <openssl/evp.h>

#define BOARD_SIZE 3
#define CELL_SIZE 100

typedef struct {
    int start_row, start_col;  // Posição inicial da linha
    int end_row, end_col;      // Posição final da linha
    char direction;            // 'H' horizontal, 'V' vertical, 'D' diagonal
} WinningLine;

typedef struct {
  unsigned int id;
  unsigned char *board_image;
  unsigned char *original_image;
  int width, height, channels;
  char game_board[BOARD_SIZE][BOARD_SIZE];
  bool player_turn; // true para X, false para O
  WinningLine winning_line;
} Game;



void initialize_game(Game *game, const char *image_path);
void draw_symbol(Game *game, int row, int col);
bool make_move(Game *game, int position);
bool check_winner(Game *game);
void save_game_state(Game *game, const char *output_path);
void cleanup_game(Game *game);
bool is_draw(Game *game);

char *encode_base64(const char *filename);

void draw_winning_line(Game *game);

int arquivo_modificado(const char *filename, time_t *last_mod_time);
void processar_json(const char *filename);