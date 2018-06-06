#ifndef RUNT_H
#define RUNT_H

#define RUNT_KILOBYTE 1000
#define RUNT_MEGABYTE (RUNT_KILOBYTE * 1000)
#define RUNT_GIGABYTE (RUNT_MEGABYTE * 1000)
#define RUNT_STACK_SIZE 32
#define RUNT_MODE_PROC 1
#define RUNT_MODE_INTERACTIVE 2
#define RUNT_MODE_KEYWORD 4
#define RUNT_MODE_RUNNING 8
#define RUNT_MODE_VERBOSE 16
#define RUNT_MODE_PANIC 32
#define RUNT_MODE_END 64

#define RUNT_DICT_SIZE 128

#define RUNT_ERROR_CHECK(A) if(A == RUNT_NOT_OK) return RUNT_NOT_OK;

#include <stdarg.h> /* a not-so-obvious header to include */

enum {
RUNT_NOT_OK = 0,
RUNT_OK,
RUNT_CONTINUE,
RUNT_ON,
RUNT_OFF,
RUNT_NIL = 0,
RUNT_FLOAT,
RUNT_STRING,
RUNT_CPTR,
RUNT_WORD,
RUNT_PROC,
RUNT_CELL,
RUNT_LIST,
RUNT_HEX,
RUNT_PLUGIN,
RUNT_END = 100 /* custom types can be numbered starting after RUNT_END */
};

typedef struct runt_vm runt_vm;
typedef struct runt_entry runt_entry;

typedef int runt_int;
typedef float runt_float;
typedef int runt_type;
typedef unsigned int runt_uint;

typedef struct {
    runt_type type;
    void *ud;
    runt_uint pos;
} runt_ptr;

typedef runt_int (*runt_proc)(runt_vm *, runt_ptr);
typedef void (*runt_printer)(runt_vm *, const char *fmt, va_list);

typedef struct {
    runt_proc fun;
    runt_ptr p;
    runt_uint psize;
    runt_uint id;
} runt_cell;

typedef runt_int (*runt_copy_proc)(runt_vm *, runt_cell *, runt_cell *);

typedef struct {
    runt_type t;
    runt_ptr p;
    runt_float f;
} runt_stacklet;

typedef struct {
    runt_stacklet stack[RUNT_STACK_SIZE];
    runt_int pos;
    runt_int size;
    runt_int bias;
} runt_stack;

typedef struct runt_list {
    runt_int size;
    runt_entry *top;
    runt_entry *last;
} runt_list;

struct runt_entry {
    runt_cell *cell;
    runt_copy_proc copy;
    runt_ptr p;
    runt_uint size;
    runt_uint c;
    runt_uint m;
    runt_list *list;
    runt_entry *next;
    runt_entry *prev;
    runt_entry *last_word_defined;
};

typedef struct runt_dict {
    runt_int nwords;
    runt_list list[RUNT_DICT_SIZE];
    struct runt_dict *last;
} runt_dict;

typedef struct {
    unsigned char *data;
    runt_uint size;
    runt_uint used;
    runt_uint mark;
} runt_memory_pool;

typedef struct {
    runt_cell *cells;
    runt_uint size;
    runt_uint used;
    runt_uint mark;
} runt_cell_pool;

struct runt_vm {
    runt_stack stack;
    runt_cell_pool cell_pool;
    runt_memory_pool memory_pool;
    runt_dict *dict;
    runt_dict idict;

    runt_uint status;
    /* runt_cell *proc; */

    runt_proc zproc;
    runt_ptr nil;

    /* the base cell for floats */
    runt_cell *f_cell;
    /* the base cell for strings */
    runt_cell *s_cell;

    /* a cell that does nothing */
    runt_cell empty;

    /* recursion level */
    runt_uint level;
    /* cell position */
    runt_uint pos;

    /* list of plugin handles */
    runt_list plugins;

    /* what to write output to */
    FILE *fp;

    /* command line arguments */
    char **argv;
    int argc;
    int argpos;

    runt_entry *last_word_defined;
    runt_uint nwords;

    runt_int (*loader)(runt_vm*);
    runt_printer print;

    /* list of destructors */
    runt_list dtors;
};

/* Main */
runt_int runt_init(runt_vm *vm);
runt_int runt_load_minimal(runt_vm *vm);
runt_int runt_load_stdlib(runt_vm *vm);
void runt_seppuku(runt_vm *vm);
runt_int runt_is_alive(runt_vm *vm);
runt_int runt_load_plugin(runt_vm *vm, const char *filename);
runt_int runt_close_plugins(runt_vm *vm);
runt_int runt_add_destructor(runt_vm *vm, runt_proc proc, runt_ptr ptr);
void runt_print(runt_vm *vm, const char *fmt, ...);
void runt_filehandle(runt_vm *vm, FILE *handle);
FILE *runt_filehandle_get(runt_vm *vm);
void runt_print_default(runt_vm *vm, const char *fmt, va_list ap);
void runt_print_set(runt_vm *vm, runt_printer printer);

/* Pools */

runt_int runt_cell_pool_set(runt_vm *vm, runt_cell *cells, runt_uint size);
runt_int runt_cell_pool_init(runt_vm *vm);

runt_uint runt_cell_pool_size(runt_vm *vm);
runt_uint runt_cell_pool_used(runt_vm *vm);
void runt_cell_pool_clear(runt_vm *vm);

void runt_cell_pool_list(runt_vm *vm);

runt_int runt_cell_pool_get_cell(runt_vm *vm, runt_uint id, runt_cell **cell);

runt_int runt_memory_pool_set(runt_vm *vm, unsigned char *buf, runt_uint size);
runt_uint runt_malloc(runt_vm *vm, size_t size, void **ud);
runt_int runt_memory_pool_get(runt_vm *vm, runt_uint id, void **ud);

runt_uint runt_memory_pool_size(runt_vm *vm);
runt_uint runt_memory_pool_used(runt_vm *vm);
void runt_memory_pool_clear(runt_vm *vm);

void runt_mark_set(runt_vm *vm);
void runt_pmark_set(runt_vm *vm);
runt_uint runt_mark_free(runt_vm *vm);
runt_uint runt_pmark_free(runt_vm *vm);

/* Cell Operations */
runt_uint runt_cell_new(runt_vm *vm, runt_cell **cell);
runt_int runt_cell_malloc(runt_vm *vm, runt_cell **cell);
runt_int runt_cell_link(runt_vm *vm, runt_cell *src, runt_cell *dest);
runt_int runt_cell_bind(runt_vm *vm, runt_cell *cell, runt_proc proc);
runt_int runt_cell_data(runt_vm *vm, runt_cell *cell, runt_ptr p);
runt_int runt_cell_call(runt_vm *vm, runt_cell *cell);
runt_int runt_cell_exec(runt_vm *vm, runt_cell *cell);
runt_int runt_cell_id_exec(runt_vm *vm, runt_uint id);
/*TODO: rename to runt_cell_pool_undo */
runt_int runt_cell_undo(runt_vm *vm);
void runt_cell_clear(runt_vm *vm, runt_cell *cell);
runt_int runt_cell_get_psize(runt_vm *vm, runt_cell *cell);
runt_int runt_cell_id_get(runt_vm *vm, runt_uint id, runt_cell **cell);
runt_int runt_cell_destructor(runt_vm *vm, runt_cell *cell);
runt_int runt_cell_from_stack(runt_vm *vm, runt_stacklet *s, runt_cell **cell);

/* Stack Operations */

runt_stacklet * runt_pop(runt_vm *vm);
runt_int  runt_ppop(runt_vm *vm, runt_stacklet **s);
runt_stacklet * runt_push(runt_vm *vm);
runt_int runt_ppush(runt_vm *vm, runt_stacklet **s);
runt_stacklet * runt_peak(runt_vm *vm);
runt_int runt_ppeak(runt_vm *vm, runt_stacklet **s);
runt_int runt_ppeakn(runt_vm *vm, runt_stacklet **s, runt_int pos);
void runt_unpop(runt_vm *vm);
runt_float runt_stack_float(runt_vm *vm, runt_stacklet *stack);
runt_int runt_stack_bias_pos(runt_vm *vm, runt_stack *stack);
runt_int runt_stack_pos(runt_vm *vm, runt_stack *stack);
void runt_stack_dec(runt_vm *vm, runt_stack *stack);
void runt_stack_dec_n(runt_vm *vm, runt_stack *stack, runt_uint n);
void runt_stack_inc(runt_vm *vm, runt_stack *stack);
void runt_stack_init(runt_vm *vm, runt_stack *stack);
void runt_stack_bias(runt_vm *vm, runt_stack *stack, runt_int bias);
void runt_stack_unbias(runt_vm *vm, runt_stack *stack);
void runt_stacklet_copy(runt_vm *vm, runt_stacklet *src, runt_stacklet *dst);
int runt_stack_dup(runt_vm *vm);
int runt_stack_swap(runt_vm *vm);


/* Pointers */

/* TODO: reconsider naming conventions for conversion functions */

runt_ptr runt_mk_ptr(runt_type type, void *ud);
runt_int runt_ref_to_cptr(runt_vm *vm, runt_uint ref, void **ud);
runt_cell * runt_to_cell(runt_ptr p);
runt_float * runt_to_float(runt_ptr p);
runt_ptr runt_mk_float(runt_vm *vm, runt_float ival);
const char * runt_to_string(runt_ptr p);
runt_ptr runt_mk_string(runt_vm *vm, const char *str, runt_uint size);
void * runt_to_cptr(runt_ptr p);
runt_ptr runt_mk_cptr(runt_vm *vm, void *cptr);
runt_list * runt_to_list(runt_ptr p);
runt_ptr runt_mk_list(runt_vm *vm, runt_list *lst);

runt_int runt_mk_cptr_cell(runt_vm *vm, void *cptr);
runt_uint runt_mk_float_cell(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_float *flt);

/* Dictionary */

runt_uint runt_entry_create(runt_vm *vm, runt_cell *cell, runt_entry **entry);

void runt_entry_set_copy_proc(runt_entry *entry, runt_copy_proc copy);

runt_int runt_entry_copy(runt_vm *vm, runt_entry *entry, runt_cell *dest);
runt_int runt_entry_exec(runt_vm *vm, runt_entry *entry);

runt_int runt_word(runt_vm *vm,
        const char *name,
        runt_int size,
        runt_entry *entry);

runt_int runt_word_search(runt_vm *vm,
        const char *name,
        runt_int size,
        runt_entry **entry);

runt_int runt_word_oops(runt_vm *vm);

void runt_list_init(runt_list *lst);
runt_int runt_list_append(runt_list *lst, runt_entry *ent);
runt_int runt_list_prepend(runt_list *lst, runt_entry *ent);
runt_int runt_list_append_ptr(runt_vm *vm, runt_list *lst, runt_ptr p);
runt_int runt_list_append_cell(runt_vm *vm, runt_list *lst, runt_cell *cell);
runt_int runt_list_prepend_cell(runt_vm *vm, runt_list *lst, runt_cell *cell);
runt_int runt_list_size(runt_list *lst);
runt_entry * runt_list_top(runt_list *lst);
runt_int runt_list_pop(runt_vm *vm, runt_list *lst);

void runt_dictionary_init(runt_vm *vm);
void runt_dict_init(runt_vm *vm, runt_dict *dict);
runt_dict * runt_dictionary_get(runt_vm *vm);
void runt_dictionary_set(runt_vm *vm, runt_dict *dict);

runt_int runt_dictionary_clear(runt_vm *vm);
runt_int runt_dict_clear(runt_vm *vm, runt_dict *dict);

runt_uint runt_dictionary_size(runt_vm *vm);
runt_uint runt_dict_size(runt_vm *vm, runt_dict *dict);

runt_dict * runt_dictionary_swap(runt_vm *vm);

runt_uint runt_dictionary_new(runt_vm *vm, runt_dict **pdict);

runt_uint runt_word_define(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc);

runt_int runt_word_last_defined(runt_vm *vm,
        runt_entry *ent,
        runt_uint c,
        runt_uint m);

runt_uint runt_word_define_with_copy(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_copy_proc copy);

runt_int runt_keyword_define(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_cell **pcell);

runt_int runt_keyword_define_with_copy(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_copy_proc copy,
        runt_cell **pcell);

runt_int runt_data(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_ptr p);

runt_int runt_data_search(runt_vm *vm, const char *name, runt_ptr *p);

runt_int runt_word_bind_ptr(runt_vm *vm, runt_uint id, runt_ptr p);

runt_int runt_parse_file(runt_vm *vm, const char *filename);

runt_int runt_parse_filehandle(runt_vm *vm, FILE *fp);


/* Lexing, Parsing, and Compiling */

runt_int runt_compile(runt_vm *vm, const char *str);

runt_int runt_tokenize(runt_vm *vm,
        const char *str,
        runt_uint size,
        runt_uint *pos,
        runt_uint *wsize,
        runt_uint *next);

runt_type runt_lex(runt_vm *vm,
        const char *str,
        runt_uint pos,
        runt_uint size);

runt_float runt_atof(const char *str, runt_uint pos, runt_uint size);

runt_int runt_set_state(runt_vm *vm, runt_uint mode, runt_uint state);
runt_uint runt_get_state(runt_vm *vm, runt_uint mode);

/* Procedures */

runt_int runt_proc_begin(runt_vm *vm, runt_cell *proc);
runt_int runt_proc_end(runt_vm *vm);

/* Standard Library Procedures */

runt_int runt_load_basic(runt_vm *vm);

runt_int irunt_begin(int argc, char *argv[], runt_int (*loader)(runt_vm *));

size_t runt_getline(char **lineptr, size_t *n, FILE *stream);
#endif
