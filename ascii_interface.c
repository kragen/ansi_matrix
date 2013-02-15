/* ASCII interface for bytebeat generator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* First, display a table and use cursor keys to highlight different cells. */

enum { n_columns = 11, n_rows = 8 };

char *labels[n_rows][n_columns] = {
  {"42",        "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"t<<3",      "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"t<<5",      "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"t<<7",      "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"xa^xb^xc",  "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"pa*pb*pc",  "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"sa+sb+sc",  "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x",   "x"},
  {"-",         "sa",  "sb",  "sc",  "pa",  "pb",  "pc",  "xa",  "xb",  "xc",  "audio<<9"},
};
int column_width[n_columns];

enum { stdin_fd = 0 };
struct termios old_termios, new_termios;
void my_cbreak() { /* because curses makes me curse */
  if (tcgetattr(stdin_fd, &old_termios) < 0) {
    perror("tcgetattr");
    exit(1);
  }

  new_termios = old_termios;
  new_termios.c_iflag &= ~(INLCR | ICRNL | IXON | IXOFF);
  new_termios.c_oflag &= ~(OPOST);
  new_termios.c_lflag &= ~(ECHO | ICANON);
  if (tcsetattr(stdin_fd, TCSANOW, &new_termios) < 0) {
    perror("tcsetattr");
    exit(1);
  }
}

void restore_terminal() {
  tcsetattr(stdin_fd, TCSANOW, &old_termios);
}

void draw_table(int x, int y) {
  int i, j;
  for (i = 0; i < n_rows; i++) {
    for (j = 0; j < n_columns; j++) {
      char *after = "";
      if (i == y && j == x) {
	printf("\e[47m");
	after = "\e[0m";
      }
      printf("%*s %s", column_width[j], labels[i][j], after);
    }
    printf("\r\n");
  }
}

int main() {
  int i, j, x = 2, y = 4;
  char c;
  int done = 0;
  for (i = 0; i < n_columns; i++) {
    column_width[i] = 0;
    for (j = 0; j < n_rows; j++) {
      int len = strlen(labels[j][i]);
      if (len > column_width[i]) column_width[i] = len;
    }
  }

  my_cbreak();

  while (!done) {
    printf("\e[H\e[J");
    draw_table(x, y);
    while (!read(stdin_fd, &c, 1));
    switch(c) {
    case 'h': x = (x - 1 + n_columns) % n_columns; break;
    case 'j': y = (y + 1 + n_rows)    % n_rows;    break;
    case 'k': y = (y - 1 + n_rows)    % n_rows;    break;
    case 'l': x = (x + 1 + n_columns) % n_columns; break;
    case '\e': case 'q': case 'x': done = 1; break;
    }
  }

  restore_terminal();
  return 0;
}
