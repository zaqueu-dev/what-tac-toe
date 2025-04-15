#include "stb_wrapper.h"
#include <math.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BOARD_SIZE 3
#define CELL_SIZE 100

typedef struct {
  unsigned int id;
  unsigned char *board_image;
  unsigned char *original_image;
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

int arquivo_modificado(const char *filename, time_t *last_mod_time);
void processar_json(const char *filename);