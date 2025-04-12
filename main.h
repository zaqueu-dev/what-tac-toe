#include "stb_wrapper.h"
#include <math.h>
#include <stdbool.h>

#define BOARD_SIZE 3
#define CELL_SIZE 100

typedef struct {
  unsigned char *board_image;
  int width, height, channels;
  char game_board[BOARD_SIZE][BOARD_SIZE];
  bool player_turn; // true para X, false para O
} Game;

void initialize_game(Game *game, const char *image_path);
void draw_symbol(Game *game, int row, int col);
bool make_move(Game *game, int row, int col);
bool check_winner(Game *game);
void save_game_state(Game *game, const char *output_path);
void cleanup_game(Game *game);