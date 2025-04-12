#include "main.h"

int main() {

  Game game;
  initialize_game(&game, "/home/zaqueu/Code/C/projeto_antonio/board.png");

  printf("Jogo da Velha - X começa\n");

  while (true) {
    // Exibe o tabuleiro atual
    save_game_state(&game, "/home/zaqueu/Code/C/projeto_antonio/board.png");
    printf(
        "Tabuleiro salvo em '/home/zaqueu/Code/C/projeto_antonio/board.png'\n");

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

  return 0;
}