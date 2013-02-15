/*

This interprets a bytebeat expression entered in the form of a matrix:

42        x   x   x   x   x   x   x   x   x   x
t<<3      x   x   x   x   x   x   x   x   x   x
t<<5      x   x   x   x   x   x   x   x   x   x
t<<7      x   x   x   x   x   x   x   x   x   x
xa^xb^xc  x   x   x   x   x   x   x   x   x   x
pa*pb*pc  x   x   x   x   x   x   x   x   x   x
sa+sb+sc  x   x   x   x   x   x   x   x   x   x
-         sa  sb  sc  pa  pb  pc  xa  xb  xc  audio<<9

All the numbers are changeable by twisting knobs, and the xes are
toggleable buttons which connect the given source (on the left) to the
given sink (on the bottom).

*/


/*

I was thinking of using some kind of stack-based bytecode for the
interpretation itself, using buffers of four samples to amortize the
dispatch overhead over several samples without using too much memory.

So what's my strategy for interpreting the matrix?

I need to do some kind of topological sort, I think, to turn the matrix into
bytecode. The straightforward approach there is recursion, with some kind of
flag on the matrix rows to prevent infinite recursive loops. But I need some
kind of approach to deal with rows used in more than one place. Can I just
use dup?  Probably the simplest thing to do is to use variables.

But in that case, I don't need stack-based bytecode at all; the bytecode to
compute any particular column will look like

    fetchrow 3; fetchrow 5; and; fetchrow 7; and;
    fetchrow 2;
    sum;
    const -1;
    sum;
    storerow 1;

That is, every fetchrow operation has a necessary "and" following it. In the
above, this is not quite obvious because the first item in the column
may not; but if we stick a const -1 before it, it can.

So our "stack" becomes rather limited! We have two items: the AND of the
current column (which begins as zero and then we fetchrow stuff and AND it
in) and the value we're computing for the row. That means we don't really
need a stack at all, just an accumulator, with the operations:

- accumulator := -1 (possibly implicit)
- accumulator &= row[n]
- row[n] := accumulator
- row[n] += accumulator
- row[n] *= accumulator
- row[n] ^= accumulator

plus the initialization of the rows that aren't computed from columns.

We still probably want to sequence the column computation for reasons of
topological sorting.
 
*/

#include "avr_bytebeat_interp.h"

/* A little analysis of and_row's assembly listing suggests that it spends about:
 * - 20 cycles entering the function and setting up Z
 * - 6 cycles accessing memory for each of the four samples
 * - 2 cycles performing a logical AND on each of the four samples
 * - 2 cycles returning
 * for a total of 54 cycles in order to compute eight bytewise ANDs,
 * almost 7 cycles per AND or 14 cycles per sample.
 * If I expanded bufs to 8 samples, then it would be 86 cycles,
 * almost 6 cycles per AND or 11 cycles per sample.
 * This seems like a good ballpark for the interpretation overhead I can expect:
 * 6 or 7 times.
 * With a single-sample buf, it would be 30 cycles to do two bytewise ANDs,
 * 15 cycles per AND or 30 cycles per sample, twice as bad.
 *
 * add_row, similarly, has a prologue with 9 instructions (of which
 * four, one two-cycle, are repeated 3 times, for a total of 5 + 3*4 =
 * 17 cycles, 8 straight-line instructions per sample, and a ret.
 *
 * Oh, wait, RET takes 4 cycles, not 2 like I thought.  
 * And lds, ldd, st, and std take 2 cycles, not 1 like I thought.  
 * So those 8 instructions in the inner loop actually cost 
 * 14 cycles, not 8!  That brings the total to 80+ cycles, 10x overhead.
 * (Or maybe a little more.)
 */

static buf rows[matrix_rows];
static buf accumulator;

static void and_row(row_id row_num) {
  accumulator[0] &= rows[row_num][0];
  accumulator[1] &= rows[row_num][1];
  accumulator[2] &= rows[row_num][2];
  accumulator[3] &= rows[row_num][3];
}

static void clear_accumulator(row_id unused) {
  accumulator[0] = -1;
  accumulator[1] = -1;
  accumulator[2] = -1;
  accumulator[3] = -1;
}

#define row_op(op) do {                         \
    rows[row_num][0] op accumulator[0];         \
    rows[row_num][1] op accumulator[1];         \
    rows[row_num][2] op accumulator[2];         \
    rows[row_num][3] op accumulator[3];         \
  } while (0)

static void set_row(row_id row_num) {
  row_op(=);
}

static void add_row(row_id row_num) {
  row_op(+=);
}

static void mul_row(row_id row_num) {
  row_op(*=);
}

static void xor_row(row_id row_num) {
  row_op(^=);
}

struct bytebeat_interp_configuration configuration;

enum { max_ops = matrix_rows * matrix_cols + matrix_cols };
static void (*op_table[])(row_id) = {
  and_row, clear_accumulator, set_row, add_row, mul_row, xor_row
};
typedef enum {
  op_and = 0, op_clear,       op_set,  op_add,  op_mul,  op_xor
} opcode;

/* We pack an opcode and a row_id into a single byte.  Hope we don't
   ever have more than 16 of either. */
typedef unsigned char bytecode;
inline static opcode get_opcode(bytecode x) { return  x & 0x0f; }
inline static row_id get_row_id(bytecode x) { return (x & 0xf0) >> 4; }
inline static bytecode make_bytecode(opcode a, row_id b) { return b << 4 | a; }
static bytecode ops[max_ops];
static int ops_length;

enum {
  const_row_id = 0,
  shift1_row_id = 1,
  shift2_row_id = 2,
  shift3_row_id = 3,
  xor_row_id = 4,
  product_row_id = 5,
  sum_row_id = 6,

  first_sum_col_id = 0,
  first_product_col_id = 3,
  first_xor_col_id = 6,
  audio_col_id = 9,
};

static inline void populate_t_row(row_id row_num, char shift) {
  register sample t = configuration.t;
  rows[row_num][0] = t++ >> shift;
  rows[row_num][1] = t++ >> shift;
  rows[row_num][2] = t++ >> shift;
  rows[row_num][3] = t   >> shift;
}

sample *interpret_ops() {
  unsigned char op;             /* max_ops < 256 */

  /* XXX maybe only do these if they're needed? */

  rows[const_row_id][0] = configuration.constant;
  rows[const_row_id][1] = configuration.constant;
  rows[const_row_id][2] = configuration.constant;
  rows[const_row_id][3] = configuration.constant;

  populate_t_row(shift1_row_id, configuration.shift1);
  populate_t_row(shift2_row_id, configuration.shift2);
  populate_t_row(shift3_row_id, configuration.shift3);
    
  /* The body of this loop: subi sbci mov mov ld (6);
   * mov ldi andi andi (5); 
   * lsl rol subi sbci ld ld mov swap andi icall (14);
   * subi (1); mov ldi lds lds cp cpc brlt (11);
   * total of 37 cycles in the body, 
   * another 4x overhead to be added to the 10x in each function,
   * for a total of 14x.
   * Plus there's the overhead of the rest of this function,
   * but that's distributed over all the ops.
   * 14x overhead means that instead of having 2000
   * instructions available per sample, you effectively have
   * 142, or 71 basic ops.  
   * It should be barely possible to exceed this with this matrix.
   */
  for (op = 0; op < ops_length; op++) {
    op_table[get_opcode(ops[op])](get_row_id(ops[op]));
  }

  accumulator[0] >>= configuration.audioshift;
  accumulator[1] >>= configuration.audioshift;
  accumulator[2] >>= configuration.audioshift;
  accumulator[3] >>= configuration.audioshift;

  return accumulator;
}

// Tell us which rows we've already started computing.
static column already_started_computing;

static void compile(opcode op, row_id arg) {
  ops[ops_length++] = make_bytecode(op, arg);
}

static void compile_column(unsigned char col);

static void compile_row(row_id row) {
  if (row == xor_row_id) {
    compile_column(first_xor_col_id);
    compile(op_set, xor_row_id);
    compile_column(first_xor_col_id + 1);
    compile(op_xor, xor_row_id);
    compile_column(first_xor_col_id + 2);
    compile(op_xor, xor_row_id);
  } else if (row == sum_row_id) {
    compile_column(first_sum_col_id);
    compile(op_set, sum_row_id);
    compile_column(first_sum_col_id + 1);
    compile(op_add, sum_row_id);
    compile_column(first_sum_col_id + 2);
    compile(op_add, sum_row_id);
  } else if (row == product_row_id) {
    compile_column(first_product_col_id);
    compile(op_set, product_row_id);
    compile_column(first_product_col_id + 1);
    compile(op_mul, product_row_id);
    compile_column(first_product_col_id + 2);
    compile(op_mul, product_row_id);
  }
}

static void compile_column(unsigned char col) {
  row_id i;

  for (i = 0; i < matrix_cols; i++) {
    if ((configuration.columns[col] & 1 << i) && 
        !(already_started_computing & 1 << i)) {
      already_started_computing |= 1 << i;
      compile_row(i);
    }
  }

  compile(op_clear, 0);         /* unused argument */
  for (i = 0; i < matrix_cols; i++) {
    if (configuration.columns[col] & 1 << i) {
      compile(op_and, i);
    }
  }
}

void compile_matrix() {
  ops_length = 0;
  already_started_computing =
    1 << const_row_id |
    1 << shift1_row_id |
    1 << shift2_row_id |
    1 << shift3_row_id;
    
  compile_column(audio_col_id);
}
