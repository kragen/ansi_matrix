typedef short sample;
typedef sample buf[4];
typedef unsigned char row_id;

// We only have 7 rows, so a char is enough to hold one bit for each, an
// entire column.  x & (1<<n) will be true if row n is set in x.
typedef unsigned char column;
enum { matrix_rows = 7, matrix_cols = 10 };

extern struct bytebeat_interp_configuration {
  sample constant;
  char shift1, shift2, shift3;  // three left shifts for t
  char audioshift;              // right shift for audio output
  column columns[matrix_cols];
  sample t;
} configuration;

void compile_matrix(void);
sample *interpret_ops(void);
