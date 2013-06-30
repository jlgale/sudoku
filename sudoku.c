#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>

#define EMPTY 0
#define BOARDSIZE 9
#define GRIDSIZE 3

#define UNUSED __attribute__((unused))

typedef struct pos
{
    int x, y;
} pos;

static pos
make_pos(int x, int y)
{
    pos p = { .x = x, .y = y };
    return p;
}

typedef struct board
{
    char squares[BOARDSIZE][BOARDSIZE];
} board;

static char
board_deref(board *b, pos p)
{
    return b->squares[p.y][p.x];
}

static void
board_set(board *b, pos p, int i)
{
    b->squares[p.y][p.x] = i;
}

/* numset is a bitmask set of numbers, 1-9 */
typedef struct numset
{
    unsigned numbers;
} numset;

static unsigned
numset_mask(int i)
{
    return 1 << (i - 1);
}

static numset
numset_empty(void)
{
    numset empty = { 0 };
    return empty;
}

static numset UNUSED
numset_one(int i)
{
    numset n = { numset_mask(i) };
    return n;
}

static numset
numset_add(numset n, int i)
{
    n.numbers |= numset_mask(i);
    return n;
}

static numset
numset_remove(numset n, int i)
{
    n.numbers &= ~numset_mask(i);
    return n;
}

static numset
all_numbers(void)
{
    numset all = numset_empty();
    for (int i = 0; i < BOARDSIZE; ++i)
        all = numset_add(all, i + 1);
  return all;
}

static bool
numset_contains(numset n, int i)
{
    return numset_mask(i) & n.numbers;
}

static int
numset_elements(numset n)
{
    int c = 0;
    while (n.numbers) {
        if (n.numbers & 1)
            c = c + 1;
        n.numbers >>= 1;
    }
    return c;
}

static bool UNUSED
numset_is_empty(numset n)
{
    return n.numbers == 0;
}

/* Iterate over each number, _i, in the given numset _n. */
#define numset_foreach(_i, _n)			\
    for(int _i = numset_next(_n, 0); _i; _i = numset_next(_n, _i))

static int
numset_next(numset n, int i)
{
    while (i < BOARDSIZE) {
        if (n.numbers & (1 << i))
            return i + 1;
        i++;
    }
    return 0;
}

static void
numset_print(FILE *f, numset n)
{
    bool first = true;
    putc('(', f);
    numset_foreach(i, n) {
        if (!first)
            putc(' ', f);
        first    = false;
        putc('0' + i, f);
    }
    putc(')', f);
}

static void UNUSED
numset_print_dbg(numset n)
{
    numset_print(stdout, n);
    putc('\n', stdout);
    fflush(stdout);
}

/* Iterate over each row position, _p, in the row of the given position _from. */
#define row_foreach(_p, _from)						\
    for (pos _p = { .x = 0, .y = (_from).y }; _p.x < BOARDSIZE; ++_p.x)

/* Iterate over each column position, _p, in the column of the position _from. */
#define col_foreach(_p, _from)		\
    for (pos _p = { .x = (_from).x, .y = 0 }; _p.y < BOARDSIZE; ++_p.y)

/* Iterate over each grid square position, _p, in the grid of the
 * position _from. */
#define grid_foreach(_p, _from, _step, _size)                           \
    for (pos _h = grid_home((_from), (_size)),                          \
             _p = grid_home((_from), (_size));                          \
         _p.y < _h.y + (_size);                                         \
         _p = grid_next(_p, (_step), (_size)))

static pos
grid_home(pos p, int gridsize)
{
    p.x -= p.x % gridsize;
    p.y -= p.y % gridsize;
    return p;
}

static pos
grid_next(pos p, int gridstep, int gridsize)
{
    p.x += gridstep;
    if (p.x % gridsize == 0) {
        p.x -= gridsize;
        p.y += gridstep;
    }
    return p;
}

static void
possibilities_each(board *b, pos p, numset *n)
{
    char v = board_deref(b, p);
    if (v != EMPTY)
        *n = numset_remove(*n, v);
}

/* For the given square, return a set of all possible values. */
static numset
possibilities(board *b, pos p)
{
    numset n = all_numbers();
    row_foreach(r, p)
        possibilities_each(b, r, &n);
    col_foreach(c, p)
        possibilities_each(b, c, &n);
    grid_foreach(g, p, 1, GRIDSIZE)
        possibilities_each(b, g, &n);
    return n;
}

/* Find the most constrained square on the board. */
static bool
board_next(board *b, pos *next_out, numset *choices_out)
{
    pos next;
    numset choices = numset_empty();
    int count = -1;

    for (int x = 0; x < BOARDSIZE; ++x) {
        for (int y = 0; y < BOARDSIZE; ++y) {
            pos p = make_pos(x, y);
            if (board_deref(b, p) == EMPTY) {
                numset n = possibilities(b, p);
                int i = numset_elements(choices);
                if (count < 0 || i < count) {
                    count = i;
                    next = p;
                    choices = n;
                }
                if (i == 0)
                    goto done;
            }
        }
    }
    if (count < 0)
        return false;
 done:
    *next_out = next;
    *choices_out = choices;
    return true;
}

/* Try to solve the sudoku board, returns true if successful. */
static bool
sudoku(board *b)
{
    pos n;
    numset choices;
    if (!board_next(b, &n, &choices))
        return true;
    numset_foreach(c, choices) {
        board_set(b, n, c);
        if (sudoku(b))
            return true;
    }
    board_set(b, n, EMPTY);
    return false;
}

static bool
sudoku_check_one(board *b, numset *n, pos p)
{
    char v = board_deref(b, p);
    if (v != EMPTY) {
        if (numset_contains(*n, v))
            return false;
        *n = numset_add(*n, v);
    }
    return true;
}

static bool
sudoku_check(board *b)
{
    row_foreach(r, make_pos(0, 0)) {
        numset n = numset_empty();
        col_foreach(c, r) {
            if (!sudoku_check_one(b, &n, c))
                return false;
        }
    }
    col_foreach(c, make_pos(0, 0)) {
        numset n = numset_empty();
        row_foreach(r, c)
            if (!sudoku_check_one(b, &n, r))
                return false;
    }
    grid_foreach(g, make_pos(0, 0), GRIDSIZE, BOARDSIZE) {
        numset n = numset_empty();
        grid_foreach(e, g, 1, GRIDSIZE)
            if (!sudoku_check_one(b, &n, e))
                return false;
    }
    return true;
}

static char
square_parse(char a)
{
    if (a >= '1' && a <= '9')
        return a - '0';
    return EMPTY;
}

static int
line_len(char *a)
{
    int n = strlen(a);
    while (isspace(a[n - 1])) n--;
    return n;
}

static char *
eat_whitespace(char *a)
{
    while (isspace(*a)) a++;
    return a;
}

static bool
read_sdm(board *b, FILE *f)
{
    char buf[BOARDSIZE * BOARDSIZE + 2 /* CRLF */ + 1];
    if (fgets(buf, sizeof(buf), f) == NULL)
        return false; /* read error */
    char *p = eat_whitespace(buf);
    int n = line_len(p);
    if (n != BOARDSIZE * BOARDSIZE)
        return false;

    for (int x = 0; x < BOARDSIZE; ++x)
        for (int y = 0; y < BOARDSIZE; ++y)
            b->squares[y][x] = square_parse(p[y * BOARDSIZE + x]);
    return true;
}

static void
write_sdm(board *b, FILE *f)
{
    for (int y = 0; y < BOARDSIZE; ++y) {
        for (int x = 0; x < BOARDSIZE; ++x) {
            int v = b->squares[y][x];
            if (v == EMPTY)
                fputc('.', f);
            else
                fputc('0' + v, f);
        }
    }
    fputc('\n', f);
    fflush(f);
}

static bool
read_sdk(board *b, FILE *f)
{
    for (int y = 0; y < BOARDSIZE; ++y) {
        char buf[1024];
        char *p;
        do {
            if (!fgets(buf, sizeof(buf), f))
                return false;
            p = eat_whitespace(buf);
        } while (*p == '\0' || *p == '#');
        int n = line_len(p);
        if (n != BOARDSIZE)
            return false;
        for (int x = 0; x < BOARDSIZE; ++x)
            b->squares[y][x] = square_parse(p[x]);
    }
    return true;
}

static void
write_sdk(board *b, FILE *f)
{
    for (int y = 0; y < BOARDSIZE; ++y) {
        for (int x = 0; x < BOARDSIZE; ++x) {
            int v = b->squares[y][x];
            if (v == EMPTY)
                fputc('.', f);
            else
                fputc('0' + v, f);
        }
        fputc('\n', f);
    }
    fflush(f);
}

typedef bool (*read_func)(board *b, FILE *f);
typedef void (*write_func)(board *b, FILE *f);

typedef struct filetype *filetype;
static struct filetype {
    char *format;
    read_func read;
    write_func write;
} io_handlers[] = { { "sdk", read_sdk, write_sdk },
                    { "sdm", read_sdm, write_sdm },
                    { 0, 0, 0}};

static filetype
get_filetype(char *extension)
{
    for (int i = 0; io_handlers[i].format; ++i) {
        if (strcmp(extension, io_handlers[i].format) == 0)
            return io_handlers + i;
    }
    fprintf(stderr, "unrecognized file format: %s", extension);
    exit(1);
}

static filetype g_read_type = NULL;
static filetype g_write_type = NULL;
static filetype default_filetype = &io_handlers[0];
static bool verbose = false;
static bool noop = false;
static bool check = false;

static filetype
select_filetype(char *filename)
{
    char *p = rindex(filename, '.');
    return p ? get_filetype(p + 1) : default_filetype;
}

static void
do_file(filetype outtype, char *outname, FILE *out,
        filetype intype, char *inname, FILE *in)
{
    struct board b;
    if (verbose)
        printf("reading from %s in .%s format\n", inname, intype->format);
    while (intype->read(&b, in)) {
        if (check && !sudoku_check(&b)) {
            fprintf(stderr, "%s: invalid game:\n", outname);
            outtype->write(&b, stderr);
            exit(1);
        }
        if (!noop) {
            if (!sudoku(&b)) {
                fprintf(stderr, "no solution!?\n");
                continue;
            }
            if (check && !sudoku_check(&b))
                fprintf(stderr, "invalid solution!?\n");
        }
        if (verbose)
            printf("writing to %s in .%s format\n", outname, outtype->format);
        outtype->write(&b, out);
    }
}

static void
usage(char *argv0)
{
    fprintf(stderr,
            "Usage: %s -cvn [-o file] [-O fmt] [-f fmt] [files...]\n",
            argv0);
}


int main(int argc, char **argv)
{
    FILE *out = NULL;
    char *outfile = NULL;
    filetype outtype = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "cnvho:f:O:")) != -1) {
        switch (opt) {
        case 'n':
            noop = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 'c':
            check = true;
            break;
        case 'f':
            g_read_type = get_filetype(optarg);
            break;
        case 'O':
            g_write_type = get_filetype(optarg);
            break;
        case 'o':
            outfile = optarg;
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        default:
            usage(argv[0]);
            exit(1);
        }
    }
    if (outfile) {
        out = fopen(outfile, "w");
        if (out == NULL) {
            perror(outfile);
            exit(1);
        }
        outtype = g_write_type ?: select_filetype(outfile);
    } else {
        out = stdout;
        outfile = "<stdout>";
        outtype = g_write_type ?: default_filetype;
    }
    if (optind >= argc) {
        do_file(outtype, outfile, out,
                g_read_type ?: default_filetype, "<stdin>", stdin);
    } else {
        for (int i = optind; i < argc; ++i) {
            char *infile = argv[i];
            FILE *in = fopen(infile, "r");
            if (in == NULL) {
                perror(infile);
                exit(1);
            }
            do_file(outtype, outfile, out,
                    g_read_type ?: select_filetype(infile), infile, in);
        }
    }
    return 0;
}
