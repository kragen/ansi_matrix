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
char *allocated_labels[n_rows][n_columns];
enum { label_size = 64 }; /* max allocated size */

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

char *editable_label(int x, int y) {
  if (!allocated_labels[y][x]) {
    allocated_labels[y][x] = malloc(label_size);
    strcpy(allocated_labels[y][x], labels[y][x]);
    labels[y][x] = allocated_labels[y][x];
  }
  if (!allocated_labels[y][x]) {
    perror("malloc");
    abort();
  }
  return allocated_labels[y][x];
}

char *num_of(char *label) {
  return strpbrk(label, "0123456789");
}

void change_cell(int x, int y, char change) {
  char *lab = labels[y][x];
  if (!strlen(lab)) {
    labels[y][x] = "x";
  } else if (strcmp(lab, "x") == 0) {
    labels[y][x] = "";
  } else if (num_of(lab)) {
    char *numloc;
    int num;
    numloc = num_of(editable_label(x, y));
    num = atoi(numloc);
    num += (change == ' ' || change == '+') ? 1 : -1;
    if (num < 0) num = 0;
    sprintf(numloc, "%d", num);
  }
}

void draw_table(int x, int y) {
  int i, j;
  int column_width[n_columns];

  for (i = 0; i < n_columns; i++) {
    column_width[i] = 0;
    for (j = 0; j < n_rows; j++) {
      int len = strlen(labels[j][i]);
      if (len > column_width[i]) column_width[i] = len;
    }
  }

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
  int x = 2, y = 4;
  char c;
  int done = 0;

  my_cbreak();

  while (!done) {
    printf("\e[H\e[J");
    draw_table(x, y);
    while (!read(stdin_fd, &c, 1)) {
      usleep(20*1000); /* sort of a busywait */
    }
    switch(c) {
    case 'h': x = (x - 1 + n_columns) % n_columns; break;
    case 'j': y = (y + 1 + n_rows)    % n_rows;    break;
    case 'k': y = (y - 1 + n_rows)    % n_rows;    break;
    case 'l': x = (x + 1 + n_columns) % n_columns; break;
    case ' ': case '+': case '-': case '\b': case '\x7f':
      change_cell(x, y, c); break;
    case '\e': case 'q': case 'x': done = 1; break;
    }
  }

  restore_terminal();
  return 0;
}
