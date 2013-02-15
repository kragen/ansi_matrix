/* ASCII interface for bytebeat generator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "avr_bytebeat_interp.h"
#include "sound.h"

/* First, display a table and use cursor keys to highlight different cells. */

enum { n_columns = 11, n_rows = 8 };

char *original_labels[n_rows][n_columns] = {
  {"160<<6",    "",   "",   "",   "x",  "",   "",   "",   "",   "",   ""},
  {"t<<0",      "x",  "",   "",   "x",  "x",  "",   "",   "",   "",   "x"},
  {"t<<8",      "",   "x",  "",   "" ,  "",   "",   "",   "",   "x",  "x"},
  {"t<<3",      "",   "",   "",   "",   "",   "",   "",   "",   "",   ""},
  {"xa^xb^xc",  "",   "",   "",   "",   "",   "",   "",   "",   "",   ""},
  {"pa*pb*pc",  "",   "",   "",   "",   "",   "",   "",   "",   "",   ""},
  {"sa+sb+sc",  "",   "",   "",   "",   "",   "",   "",   "x",  "",   ""},
  {"-",         "sa",  "sb",  "sc",  "pa",  "pb",  "pc",  "xa",  "xb",  "xc",  "audio<<8"},
};
char *labels[n_rows][n_columns];
enum { label_size = 64 }; /* max allocated size */

#define LEN(x) (sizeof(x)/sizeof((x)[0]))

void dump_configuration() {
  int i;
  printf("constant = %d\r\n", (int)configuration.constant);
  printf("shift1 = %d\r\n", configuration.shift1);
  printf("shift2 = %d\r\n", configuration.shift2);
  printf("shift3 = %d\r\n", configuration.shift3);
  printf("audioshift = %d\r\n", configuration.audioshift);
  printf("t = %d\r\n", (int)configuration.t);
  printf("columns = {");
  for (i = 0; i < LEN(configuration.columns); i++) {
    if (i) printf(", ");
    printf("0x%x", configuration.columns[i]);
  }
  printf("}\r\n");
}

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
  new_termios.c_cc[VMIN] = 0;
  new_termios.c_cc[VTIME] = 0;
  if (tcsetattr(stdin_fd, TCSANOW, &new_termios) < 0) {
    perror("tcsetattr");
    exit(1);
  }
}

void restore_terminal() {
  tcsetattr(stdin_fd, TCSANOW, &old_termios);
}


void initialize_labels() {
  int i, j;
  for (i = 0; i < n_rows; i++) {
    for (j = 0; j < n_columns; j++) {
      labels[i][j] = malloc(label_size);
      if (!labels[i][j]) {
        perror("malloc");
        abort();
      }
      strcpy(labels[i][j], original_labels[i][j]);
    }
  }
}

char *num_of(char *label) {
  return strpbrk(label, "0123456789");
}

void update_number(int x, int y, int num) {
  if (x == 0) {
    if (y == 0) {
      configuration.constant = num << 6;
    } else if (y == 1) {
      configuration.shift1 = num;
    } else if (y == 2) {
      configuration.shift2 = num;
    } else if (y == 3) {
      configuration.shift3 = num;
    }
  } else if (x == n_columns - 1 && y == n_rows - 1) {
    configuration.audioshift = num;
  }
}

/* This is stupid! */
void update_numbers() {
  int i, j;
  for (i = 0; i < n_columns; i++) {
    for (j = 0; j < n_rows; j++) {
      if (num_of(labels[j][i])) {
	update_number(i, j, atoi(num_of(labels[j][i])));
      }
    }
  }
}

void update_columns() {
  int i, j, column;
  for (i = 1; i < n_columns; i++) {
    column = 0;
    for (j = n_rows-2; j >= 0; j--) {
      column <<= 1;
      if (strlen(labels[j][i])) column++;
    }
    configuration.columns[i-1] = column;
  }
}

void change_cell(int x, int y, char change) {
  char *lab = labels[y][x];
  if (!strlen(lab)) {
    labels[y][x] = "x"; /* XXX memory leak, and below */
    update_columns();
  } else if (strcmp(lab, "x") == 0) {
    labels[y][x] = "";
    update_columns();
  } else if (num_of(lab)) {
    char *numloc;
    char *tail;                /* stuff after number */
    int num;
    numloc = num_of(labels[y][x]);
    num = atoi(numloc);
    strtol(num_of(original_labels[y][x]), &tail, 10);
    num += (change == ' ' || change == '+') ? 1 : -1;
    if (num < 0) num = 0;
    sprintf(numloc, "%d%s", num, tail);
    update_number(x, y, num);
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

void dump_ops() {
  int i;
  char buf[64];
  for (i = 0; i < n_ops(); i++) {
    disassemble_op(i, buf);
    printf("%s\r\n", buf);
  }
}

struct timeval last_samples;

enum { samples_per_second = 8000 };

void spew_audio() {
  struct timeval now, elapsed, tmp;
  int n_samples;
  gettimeofday(&now, NULL);
  timersub(&now, &last_samples, &elapsed);
  n_samples = (elapsed.tv_sec * 1000000 + 
               elapsed.tv_usec +
               500000) * samples_per_second / 1000000;
  //fprintf(stderr, "%d.%06d elapsed, %d samples\r\n", (int)elapsed.tv_sec, (int)elapsed.tv_usec, n_samples);
  n_samples = 4 * (n_samples / 4);  /* 4 samples per buf */
  /* Now adjust last_samples to account for the samples we're about to emit */
  elapsed.tv_sec = 0;
  elapsed.tv_usec = n_samples * 1000000 / samples_per_second;
  /* Not sure timeradd is safe with aliased arguments. */
  timeradd(&last_samples, &elapsed, &tmp);
  last_samples = tmp;

  /* Now finally emit the samples */
  for (; n_samples > 0; n_samples -= 4) {
    sample *data = interpret_ops();
    unsigned char buf[4];
    int i;
    //printf("data is %04x %04x %04x %04x\r\n", data[0], data[1], data[2], data[3]);
    for (i = 0; i < 4; i++) buf[i] = data[i];
    u8_audio_write(buf, 4);
    configuration.t += 4;
  }
}

int main() {
  int x = 10, y = 3;
  char c;
  int done = 0;
  int debug = 0;

  initialize_labels();
  update_numbers();
  update_columns();
  gettimeofday(&last_samples, NULL);
  my_cbreak();

  while (!done) {
    printf("\e[H\e[J");
    draw_table(x, y);
    compile_matrix();

    if (debug) {
      dump_configuration();
      dump_ops();
    }

    while (!read(stdin_fd, &c, 1)) {
      spew_audio();
      usleep(20*1000); /* sort of a busywait */
    }
    switch(c) {
    case 'h': x = (x - 1 + n_columns) % n_columns; break;
    case 'j': y = (y + 1 + n_rows)    % n_rows;    break;
    case 'k': y = (y - 1 + n_rows)    % n_rows;    break;
    case 'l': x = (x + 1 + n_columns) % n_columns; break;
    case 'd': debug = !debug; break;
    case ' ': case '+': case '-': case '\b': case '\x7f':
      change_cell(x, y, c); break;
    case '\e': case 'q': case 'x': done = 1; break;
    }
  }

  restore_terminal();
  return 0;
}
