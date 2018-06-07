#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef RUNT_PLUGINS
#include <dlfcn.h>
#endif
#include "runt.h"

#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p);
static runt_int runt_proc_link(runt_vm *vm, runt_ptr p);
static int runt_copy_float(runt_vm *vm, runt_cell *src, runt_cell *dest);
static int rproc_float(runt_vm *vm, runt_ptr p);
static int runt_copy_string(runt_vm *vm, runt_cell *src, runt_cell *dest);
static int rproc_string(runt_vm *vm, runt_ptr p);
static int rproc_begin(runt_vm *vm, runt_cell *src, runt_cell *dst);
static int rproc_end(runt_vm *vm, runt_cell *src, runt_cell *dst);
static int rproc_cptr(runt_vm *vm, runt_ptr p);
static int rproc_rec(runt_vm *vm, runt_ptr p);
static int rproc_stop(runt_vm *vm, runt_cell *src, runt_cell *dst);
static runt_int load_nothing(runt_vm *vm);

runt_int runt_init(runt_vm *vm)
{
    vm->status = 0;
    vm->zproc = runt_proc_zero;
    vm->nil = runt_mk_ptr(RUNT_NIL, NULL);

    /* init stack */

    runt_stack_init(vm, &vm->stack);

    /* init dictionary */

    runt_dictionary_init(vm);

    /* record cells being parsed by default */

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
    runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_ON);

    vm->level = 0;
    vm->pos = 0;

    /* set print output to STDERR by default */
    vm->fp = stderr;

    /* set up plugin handle list */
    runt_list_init(&vm->plugins);

    vm->nwords = 0;
    vm->loader = load_nothing;

    runt_cell_bind(vm, &vm->empty, vm->zproc);
    runt_cell_data(vm, &vm->empty, vm->nil);

    runt_print_set(vm, runt_print_default);

    /* set up destructor list */
    runt_list_init(&vm->dtors);

    return RUNT_OK;
}

runt_int runt_load_minimal(runt_vm *vm)
{
    /* create float type */
    runt_cell_malloc(vm, &vm->f_cell);
    runt_cell_bind(vm, vm->f_cell, rproc_float);

    /* create string type */
    runt_cell_malloc(vm, &vm->s_cell);
    runt_cell_bind(vm, vm->s_cell, rproc_string);

    /* ability to create procedures by default */
    runt_keyword_define_with_copy(vm, ":", 1, vm->zproc, rproc_begin, NULL);
    runt_keyword_define_with_copy(vm, ";", 1, vm->zproc, rproc_end, NULL);

    /* [ and ] for rec, stop */
    runt_keyword_define(vm, "[", 1, rproc_rec, NULL);
    runt_keyword_define_with_copy(vm, "]", 1, vm->zproc, rproc_stop, NULL);

    return RUNT_OK;
}

runt_int runt_load_stdlib(runt_vm *vm)
{
    runt_load_minimal(vm);
    /* load basic library */
    runt_load_basic(vm);
    return RUNT_OK;
}

runt_int runt_cell_pool_set(runt_vm *vm, runt_cell *cells, runt_uint size)
{
    vm->cell_pool.cells = cells;
    vm->cell_pool.size = size;
    vm->cell_pool.used = 0;
    return RUNT_OK;
}

runt_int runt_cell_pool_init(runt_vm *vm)
{
    runt_uint i;
    runt_cell *cells = vm->cell_pool.cells;

    for(i = 0; i < vm->cell_pool.size; i++) {
       runt_cell_clear(vm, &cells[i]);
    }
    vm->cell_pool.mark = 0;
    return RUNT_OK;
}

runt_int runt_memory_pool_set(runt_vm *vm, unsigned char *buf, runt_uint size)
{
    vm->memory_pool.data = buf;
    vm->memory_pool.size = size;
    vm->memory_pool.used = 0;
    vm->memory_pool.mark = 0;
    memset(buf, 0, size);
    return RUNT_OK;
}

runt_uint runt_cell_new(runt_vm *vm, runt_cell **cell)
{
    runt_uint id;
    runt_cell_pool *pool = &vm->cell_pool;
    runt_cell *proc;
    runt_stacklet *s;

    pool->used++;

    if(runt_get_state(vm, RUNT_MODE_PANIC) == RUNT_ON) {
        return 0;
    }else if(pool->used >= pool->size) {
        runt_print(vm, "Oh dear. Runt cell pool space at maximum.\n");
        runt_set_state(vm, RUNT_MODE_PANIC, RUNT_ON);
        runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_OFF);
        return 0;
    }

    if(vm->status & RUNT_MODE_PROC) {
        s = runt_peak(vm);
        proc = runt_to_cell(s->p);
        proc->psize++;
    }

    id = pool->used;
    *cell = &pool->cells[id - 1];
    (*cell)->id = id;
    return id;
}

runt_int runt_cell_malloc(runt_vm *vm, runt_cell **cell)
{
    runt_uint id;
    id = runt_malloc(vm, sizeof(runt_cell), (void **)cell);
    if(id == 0) return RUNT_NOT_OK;
    runt_cell_clear(vm, *cell);
    (*cell)->id = 0;
    return RUNT_OK;
}

runt_ptr runt_mk_ptr(runt_type type, void *ud)
{
    runt_ptr ptr;
    ptr.type = type;
    ptr.ud = ud;
    return ptr;
}

runt_int runt_cell_bind(runt_vm *vm, runt_cell *cell, runt_proc proc)
{
    cell->fun = proc;
    return RUNT_OK;
}

runt_int runt_cell_data(runt_vm *vm, runt_cell *cell, runt_ptr p)
{
    cell->p = p;
    return RUNT_OK;
}

runt_int runt_cell_call(runt_vm *vm, runt_cell *cell)
{
    return cell->fun(vm, cell->p);
}

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p)
{
    return RUNT_OK;
}

runt_int runt_proc_begin(runt_vm *vm, runt_cell *proc)
{
    runt_stacklet *s = runt_push(vm);
    runt_set_state(vm, RUNT_MODE_PROC, RUNT_ON);
    s->p = runt_mk_ptr(RUNT_CELL, (void *)proc);
    return RUNT_OK;
}

runt_int runt_proc_end(runt_vm *vm)
{
    runt_pop(vm);
    vm->status &= ~(RUNT_MODE_PROC);
    return RUNT_OK;
}

runt_int runt_cell_exec(runt_vm *vm, runt_cell *cell)
{
    runt_uint i;
    runt_int rc = RUNT_OK;

    if(cell->psize == 0) {
        return runt_cell_call(vm, cell);
    }

    for(i = 1; i <= cell->psize; i++) {
        if(cell[i].psize == 0) {
            rc = runt_cell_call(vm, &cell[i]);
        } else {
            rc = runt_cell_exec(vm, &cell[i]);
        }
        RUNT_ERROR_CHECK(rc);
    }

    return rc;
}

runt_int runt_cell_id_exec(runt_vm *vm, runt_uint id)
{
    return runt_cell_exec(vm, &vm->cell_pool.cells[id - 1]);
}

runt_int runt_cell_link(runt_vm *vm, runt_cell *src, runt_cell *dest)
{
    /* TODO: error handling for init */
    dest->fun = runt_proc_link;
    dest->p = runt_mk_ptr(RUNT_CELL, src);
    return RUNT_OK;
}

static runt_int runt_proc_link(runt_vm *vm, runt_ptr p)
{
    runt_cell *cell = runt_to_cell(p);
    return runt_cell_exec(vm, cell);
}

runt_cell * runt_to_cell(runt_ptr p)
{
    runt_cell *cell = NULL;
    /* TODO: error handling */
    if(p.type == RUNT_CELL) {
        cell = (runt_cell *)p.ud;
    } else {
        fprintf(stderr, "Not a cell!\n");
    }
    return cell;
}

runt_float * runt_to_float(runt_ptr p)
{
    runt_float *f = NULL;

    /* TODO: error handling */
    if(p.type == RUNT_FLOAT) {
        f = (runt_float *)p.ud;
    }

    return f;
}

runt_ptr runt_mk_float(runt_vm *vm, runt_float ival)
{
    runt_ptr p;
    float *val;
    val = NULL;
    p.pos = runt_malloc(vm, sizeof(runt_float), (void**)&val);
    p = runt_mk_ptr(RUNT_FLOAT, val);
    *val = ival;
    return p;
}

runt_int runt_memory_pool_get(runt_vm *vm, runt_uint id, void **ud)
{
    if(id > runt_memory_pool_size(vm)) {
        runt_print(vm, "Memory ID out of bounds!\n");
        return RUNT_NOT_OK;
    }
    if(id == 0) {
        runt_print(vm, "Memory ID has illegal value of zero.\n");
        return RUNT_NOT_OK;
    }
    *ud = &vm->memory_pool.data[id - 1];
    return RUNT_OK;
}

const char * runt_to_string(runt_ptr p)
{
    const char *str;

    /*TODO: error handling */
    if(p.type == RUNT_STRING) {
        str = p.ud;
        return str;
    } else {
        return NULL;
    }

}

runt_ptr runt_mk_string(runt_vm *vm, const char *str, runt_uint size)
{
    runt_ptr p;
    char *buf;
    runt_uint i;
    runt_uint pos;
   
    buf = NULL;
    pos = runt_malloc(vm, size + 1, (void *)&buf);
    for(i = 0; i < size; i++) {
        buf[i] = str[i];
    }

    buf[size] = 0;

    p = runt_mk_ptr(RUNT_STRING, (void *)buf);
    p.pos = pos;

    return p;
}

runt_stacklet * runt_push(runt_vm *vm)
{
    /*TODO: wrapper for ppush */
    runt_stacklet *s;
   /*TODO: error handling for stack overflows */
    if(runt_stack_pos(vm, &vm->stack) == vm->stack.size) {
        runt_print(vm, "stack overflow!\n");
        s = &vm->stack.stack[0];
        return NULL;
    }

    runt_stack_inc(vm, &vm->stack);
    s = &vm->stack.stack[runt_stack_pos(vm, &vm->stack) - 1];
    return s;
}

runt_int runt_ppush(runt_vm *vm, runt_stacklet **s)
{
   /*TODO: error handling for stack overflows */
    if(runt_stack_pos(vm, &vm->stack) == vm->stack.size) {
        runt_print(vm, "stack overflow!\n");
        *s = &vm->stack.stack[0];
        return RUNT_NOT_OK;
    }

    runt_stack_inc(vm, &vm->stack);
    *s = &vm->stack.stack[runt_stack_pos(vm, &vm->stack)- 1];
    return RUNT_OK;
}

/* peak just looks at the last item on the stack */

runt_stacklet * runt_peak(runt_vm *vm)
{
    runt_stacklet *s;
    runt_ppeak(vm, &s);
    return s;
}

runt_int runt_ppeak(runt_vm *vm, runt_stacklet **s)
{
    /*TODO: error handling for stack underflows */
    if(runt_stack_pos(vm, &vm->stack) > 0) {
        *s = &vm->stack.stack[runt_stack_pos(vm, &vm->stack) - 1];
    } else {
        runt_print(vm, "Empty stack.\n");
        *s = &vm->stack.stack[0];
        return RUNT_NOT_OK;
    }

    return RUNT_OK;
}

runt_int runt_ppeakn(runt_vm *vm, runt_stacklet **s, runt_int pos)
{
    *s = &vm->stack.stack[0];
    if(abs(pos) > runt_stack_bias_pos(vm, &vm->stack)) {
        runt_print(vm, "Invalid range %d\n", pos);
        return RUNT_NOT_OK;
    }

    if(pos >= 0) {
        *s = &vm->stack.stack[pos + vm->stack.bias];
    } else {
        *s = &vm->stack.stack[runt_stack_bias_pos(vm, &vm->stack) + pos];
    }
    return RUNT_OK;
}

/* undo the last pop */

void runt_unpop(runt_vm *vm)
{
    runt_stack_inc(vm, &vm->stack);
}

runt_float runt_stack_float(runt_vm *vm, runt_stacklet *stack)
{
    return stack->f;
}

runt_int runt_stack_pos(runt_vm *vm, runt_stack *stack)
{
    return stack->pos;
}

runt_int runt_stack_bias_pos(runt_vm *vm, runt_stack *stack)
{
    return stack->pos + stack->bias;
}

void runt_stack_bias(runt_vm *vm, runt_stack *stack, runt_int bias)
{
   stack->bias = stack->pos - bias;
}

void runt_stack_unbias(runt_vm *vm, runt_stack *stack)
{
    stack->bias = 0;
}

void runt_stack_dec(runt_vm *vm, runt_stack *stack)
{
    runt_stack_dec_n(vm, stack, 1);
}

void runt_stack_dec_n(runt_vm *vm, runt_stack *stack, runt_uint n)
{
    stack->pos -= n;
}

void runt_stack_inc(runt_vm *vm, runt_stack *stack)
{
    stack->pos++;
}

void runt_stack_init(runt_vm *vm, runt_stack *stack)
{
    stack->pos = 0;
    stack->size = RUNT_STACK_SIZE;
    stack->bias = 0;
}

runt_stacklet * runt_pop(runt_vm *vm)
{
    runt_stacklet *s = NULL;
    runt_ppop(vm, &s);
    return s;
}

runt_int  runt_ppop(runt_vm *vm, runt_stacklet **s)
{
    *s = &vm->stack.stack[0];

    if(runt_stack_pos(vm, &vm->stack) > 0) {
        runt_stack_dec(vm, &vm->stack);
        *s = &vm->stack.stack[runt_stack_pos(vm, &vm->stack)];
    } else {
        runt_print(vm, "Empty stack.\n");
        return RUNT_NOT_OK;
    }

    return RUNT_OK;
}

runt_uint runt_malloc(runt_vm *vm, size_t size, void **ud)
{

    runt_memory_pool *pool = &vm->memory_pool;
    runt_uint id = 0;

#ifdef ALIGNED_MALLOC
    if(size % 4 != 0) {
        size = ((size/4) + 1) * 4;
    }
#endif

    /* TODO: overload error handling */
    if(pool->used + size >= pool->size) {
        return 0;
    }

    *ud = (void *)&pool->data[pool->used];

    id = pool->used + 1;
    pool->used += size;

    return id;
}

runt_uint runt_entry_create(runt_vm *vm,
        runt_cell *cell,
        runt_entry **entry)
{
    runt_entry *e;
    runt_malloc(vm, sizeof(runt_entry), (void **)entry);
    e = *entry;
    e->copy = runt_cell_link;
    e->cell = cell;
    e->p = vm->nil;
    e->size = 0;
    return 0;
}

void runt_entry_set_copy_proc(runt_entry *entry, runt_copy_proc copy)
{
    entry->copy = copy;
}

runt_int runt_entry_copy(runt_vm *vm, runt_entry *entry, runt_cell *dest)
{
    return entry->copy(vm, entry->cell, dest);
}

runt_int runt_entry_exec(runt_vm *vm, runt_entry *entry)
{
    return runt_cell_exec(vm, entry->cell);
}

static runt_uint runt_hash(const char *str, runt_int size)
{
    runt_uint h = 5381;
    runt_int i = 0;

    for(i = 0; i < size; i++) {
        h = ((h << 5) + h) ^ str[0];
        h %= 0x7FFFFFFF;
    }

    return h % RUNT_DICT_SIZE;
}

runt_int runt_word(runt_vm *vm,
        const char *name,
        runt_int size,
        runt_entry *entry)
{
    runt_dict *dict;
    runt_uint pos;
    runt_list *list;

    dict = runt_dictionary_get(vm);
    pos = runt_hash(name, size);
    list = &dict->list[pos];

    entry->p = runt_mk_string(vm, name, size);
    entry->size = size;

    runt_list_append(list, entry);

    dict->nwords++;
    return RUNT_NOT_OK;
}

static runt_int runt_strncmp(runt_vm *vm, const char *str, runt_int size, runt_entry *e)
{
    if(size == e->size) {
        return strncmp(str, runt_to_string(e->p), size);
    } else {
        return 1;
    }
}

runt_int runt_word_search(runt_vm *vm,
        const char *name,
        runt_int size,
        runt_entry **entry)
{
    runt_uint pos;
    runt_list *list;
    runt_dict *dict;

    runt_uint i;
    runt_entry *ent;
    runt_entry *next;

    dict = runt_dictionary_get(vm);
    pos = runt_hash(name, size);
    list = &dict->list[pos];

    ent = runt_list_top(list);

    for(i = 0; i < list->size; i++) {
        next = ent->next;
        if(runt_strncmp(vm, name, size, ent) == 0) {
            *entry = ent;
            return RUNT_OK;
        }
        ent = next;
    }

    return RUNT_NOT_OK;
}

void runt_list_init(runt_list *lst)
{
    lst->size = 0;
}

runt_int runt_list_append(runt_list *lst, runt_entry *ent)
{
    if(lst->size == 0) {
        lst->top = ent;
        lst->last = lst->top;
    } else {
        lst->last->next = ent;
    }
    ent->prev = lst->last;
    lst->last = ent;
    lst->size++;
    ent->list = lst;
    return RUNT_OK;
}

runt_int runt_list_pop(runt_vm *vm, runt_list *lst)
{
    if(lst->size == 0) {
        runt_print(vm, "No items on the list to pop off");
        return RUNT_NOT_OK;
    } else if(lst->size == 1) {
        lst->size = 0;
        return RUNT_OK;
    }

    lst->size--;
    lst->last = lst->last->prev;
    return RUNT_OK;
}

runt_int runt_list_prepend(runt_list *lst, runt_entry *ent)
{
    if(lst->size > 0) {
        ent->next = lst->top;
    }
    lst->top = ent;
    lst->size++;
    return RUNT_OK;
}

runt_int runt_list_append_ptr(runt_vm *vm, runt_list *lst, runt_ptr p)
{
    runt_entry *entry;
    entry = NULL;
    /* make entry, using empty cell */
    runt_entry_create(vm, &vm->empty, &entry);
    /* set ptr to "string" entry in struct. works better than it looks */
    entry->p = p;

    runt_list_append(lst, entry);
    return RUNT_OK;
}

runt_int runt_list_append_cell(runt_vm *vm, runt_list *lst, runt_cell *cell)
{
    runt_entry *entry;
    entry = NULL;
    runt_entry_create(vm, cell, &entry);
    runt_list_append(lst, entry);
    return RUNT_OK;
}

runt_int runt_list_prepend_cell(runt_vm *vm, runt_list *lst, runt_cell *cell)
{
    runt_entry *entry;
    entry = NULL;
    runt_entry_create(vm, cell, &entry);
    runt_list_prepend(lst, entry);
    return RUNT_OK;
}

runt_int runt_list_size(runt_list *lst)
{
    return lst->size;
}

runt_entry * runt_list_top(runt_list *lst)
{
    return lst->top;
}

void runt_dictionary_init(runt_vm *vm)
{
    runt_dictionary_set(vm, &vm->idict);
    runt_dict_init(vm, &vm->idict);
}

void runt_dict_init(runt_vm *vm, runt_dict *dict)
{
    runt_uint i;
    dict->nwords = 0;
    for(i = 0; i < RUNT_DICT_SIZE; i++) {
        runt_list_init(&dict->list[i]);
    }
    dict->last = dict;
}

runt_int runt_dictionary_clear(runt_vm *vm)
{
    return runt_dict_clear(vm, &vm->idict);
}

runt_int runt_dict_clear(runt_vm *vm, runt_dict *dict)
{
    dict->nwords = 0;
    runt_dictionary_init(vm);
    return RUNT_OK;
}

runt_uint runt_dictionary_size(runt_vm *vm)
{
    return runt_dict_size(vm, &vm->idict);
}

runt_uint runt_dict_size(runt_vm *vm, runt_dict *dict)
{
    return dict->nwords;
}

runt_uint runt_memory_pool_size(runt_vm *vm)
{
    return vm->memory_pool.size;
}

runt_uint runt_memory_pool_used(runt_vm *vm)
{
    return vm->memory_pool.used;
}

void runt_memory_pool_clear(runt_vm *vm)
{
    vm->memory_pool.used = 0;
}

runt_uint runt_cell_pool_size(runt_vm *vm)
{
    return vm->cell_pool.size;
}

runt_uint runt_cell_pool_used(runt_vm *vm)
{
    return vm->cell_pool.used;
}

void runt_cell_pool_clear(runt_vm *vm)
{
    vm->cell_pool.used = 0;
}

runt_int runt_cell_pool_get_cell(runt_vm *vm, runt_uint id, runt_cell **cell)
{
    runt_cell_pool *cell_pool = &vm->cell_pool;
    if(id > runt_cell_pool_size(vm) || id <= 0) {
        runt_print(vm, "psize: invalid id %d\n", id);
        return RUNT_NOT_OK;
    }
    *cell = &cell_pool->cells[id - 1];
    return RUNT_OK;
}

runt_int runt_tokenize(runt_vm *vm,
        const char *str,
        runt_uint size,
        runt_uint *pos,
        runt_uint *wsize,
        runt_uint *next)
{
    runt_uint s;
    runt_uint p;
    runt_uint mode = 0;
    runt_uint stop = 0;
    char m = '"';
    *wsize = 0;

    *pos = *next;
    p = *pos;


    if(*pos > size) return RUNT_OK;

    if(*pos == size) {
        *next = size + 1;
        return RUNT_CONTINUE;
    }

    for(s = p; s < size; s++) {

        if(stop != 0) {
            break;
        }
        switch (mode) {
            case 3:
                if(str[s] == m) {
                    mode = 2;
                    *wsize = (s - *pos) + 1;
                }
                continue;
            case 2:
                if(str[s] != ' ' && str[s] != '\n') {
                    *next = s;
                    stop = 1;
                    break;
                }
                continue;
            case 1:
                if(str[s] == ' ' || str[s] == '\n') {
                    mode = 2;
                    *wsize = (s - *pos);
                }
                continue;
            case 0:
                if(str[s] == '#') return RUNT_OK;

                if(str[s] != ' ' && str[s] != '\n') {
                    if(str[s] == '\'' || str[s] == '\"') {
                        mode = 3;
                        m = str[s];
                    } else {
                        mode = 1;
                        *pos = s;
                    }
                }
                *pos = s;
                continue;
            default: break;
        }

    }
    /* if we are at the end, make next larger than the size */
    if(s == size) {
        if(mode == 1) {
            *wsize = (s - *pos);
        } if(mode == 2 && *next == size - 1) {
            return RUNT_CONTINUE;
        }
        *next = size + 1;
    }
    return RUNT_CONTINUE;
}

runt_type runt_lex(runt_vm *vm,
        const char *str,
        runt_uint pos,
        runt_uint size)
{
    runt_uint c;
    runt_int off;
    runt_int lvl;

    lvl = 0;
    off = pos + size;
    for(c = pos; c < off; c++) {
        switch(lvl) {
            case 0:
                switch(str[c]) {
                    case '0':
                        if(size > 1) {
                            lvl = 1;
                            break;
                        }
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        return RUNT_FLOAT;
                    case '-':
                        if(size == 1) {
                            return RUNT_PROC;
                        } else {
                            return RUNT_FLOAT;
                        }
                    case '"':
                    case '\'':
                        return RUNT_STRING;
                    case '_':
                        return RUNT_WORD;
                    case '\n':
                        return RUNT_NIL;
                    default:
                        return RUNT_PROC;
                }
                break;
            case 1:
                switch(str[c]) {
                    case 'x':
                        return RUNT_HEX;
                    default:
                        return RUNT_FLOAT;
                }
                break;
        }
    }

    return RUNT_NOT_OK;
}

runt_float runt_atof(const char *str, runt_uint pos, runt_uint size)
{
    runt_float total = 0.0;
    runt_int sign = 1.0;
    runt_uint i;
    runt_uint mode = 0;
    runt_float place = 1.0;
    runt_float scale = 10.0;

    runt_float num = 0.0;

    if(str[pos] == '-') {
        sign = -1;
        pos++;
        size--;
    }

    for(i = pos; i < pos + size; i++) {
        num = 0.0;
        switch(str[i]) {
            case '0':
                num = 0.0;
                break;
            case '1':
                num = 1.0;
                break;
            case '2':
                num = 2.0;
                break;
            case '3':
                num = 3.0;
                break;
            case '4':
                num = 4.0;
                break;
            case '5':
                num = 5.0;
                break;
            case '6':
                num = 6.0;
                break;
            case '7':
                num = 7.0;
                break;
            case '8':
                num = 8.0;
                break;
            case '9':
                num = 9.0;
                break;
            case '.':
                if(mode == 0) {
                    scale = 0.1;
                    mode = 1;
                    place = 1.0;
                }
                break;
        }

        if(mode == 0) {
            total *= scale;
            total += num;
        } else {
            total += num * place;
            place *= scale;
        }

    }

    return total * sign;
}

static runt_float hex2flt(runt_vm *vm, const char *str, runt_uint pos, int size)
{
    runt_uint i;
    runt_uint sz;
    const char *p;
    runt_float val;
    runt_int b;
    runt_int m;

    sz = size - 2;
    p = str + 2 + pos;
    val = 0;
    m = 1 << ((sz - 1) * 4);
    for(i = 0; i < sz; i++) {
        b = 0;
        switch(p[i]) {
            case ' ':
            case '0':
                b = 0;
                break;
            case '1':
                b = 1;
                break;
            case '2':
                b = 2;
                break;
            case '3':
                b = 3;
                break;
            case '4':
                b = 4;
                break;
            case '5':
                b = 5;
                break;
            case '6':
                b = 6;
                break;
            case '7':
                b = 7;
                break;
            case '8':
                b = 8;
                break;
            case '9':
                b = 9;
                break;
            case 'a':
                b = 10;
                break;
            case 'b':
                b = 11;
                break;
            case 'c':
                b = 12;
                break;
            case 'd':
                b = 13;
                break;
            case 'e':
                b = 14;
                break;
            case 'f':
                b = 15;
                break;
        }
        val += m * b;
        m >>= 4;
    }
    return val;
}

runt_int runt_compile(runt_vm *vm, const char *str)
{
    runt_uint size = strlen(str);
    runt_uint pos = 0;
    runt_uint word_size = 0;
    runt_uint next = 0;
    runt_stacklet *s;
    runt_cell *tmp;
    runt_entry *entry;
    runt_ptr ptr;
    runt_uint c, m;
    float val = 0.0;

    runt_int rc = RUNT_OK;

    while(runt_tokenize(vm, str, size, &pos, &word_size, &next) == RUNT_CONTINUE)
    {
        switch(runt_lex(vm, str, pos, word_size)) {
            case RUNT_FLOAT:

                val = runt_atof(str, pos, word_size);
                if(!(vm->status & RUNT_MODE_INTERACTIVE)) {
                    if(runt_cell_new(vm, &tmp) == 0) return RUNT_NOT_OK;
                    /* this needs to happen after runt_new_cell */
                    s = runt_push(vm);
                    s->f = val;
                    s->t = RUNT_FLOAT;
                    runt_copy_float(vm, vm->f_cell, tmp);
                } else {
                    s = runt_push(vm);
                    s->f = val;
                    s->t = RUNT_FLOAT;
                }

                break;
            case RUNT_HEX:
                val = hex2flt(vm, str, pos, word_size);
                if(!(vm->status & RUNT_MODE_INTERACTIVE)) {
                    if(runt_cell_new(vm, &tmp) == 0) return RUNT_NOT_OK;
                    /* this needs to happen after runt_new_cell */
                    s = runt_push(vm);
                    s->f = val;
                    runt_copy_float(vm, vm->f_cell, tmp);
                } else {
                    s = runt_push(vm);
                    s->f = val;
                }
                break;
            case RUNT_STRING:
                if(runt_cell_new(vm, &tmp) == 0) return RUNT_NOT_OK;
                s = runt_push(vm);
                ptr = runt_mk_string(vm, &str[pos + 1], word_size - 2);
                s->p = ptr;
                runt_copy_string(vm, vm->s_cell, tmp);
                if(vm->status & RUNT_MODE_INTERACTIVE) {
                    s = runt_push(vm);
                    s->p = ptr;
                    s->t = RUNT_STRING;
                }
                break;
            case RUNT_WORD:
                if(runt_word_search(vm,
                            &str[pos + 1],
                            word_size - 1,
                            &entry) == RUNT_NOT_OK) {
                    return RUNT_NOT_OK;
                }
                if(!(vm->status & RUNT_MODE_INTERACTIVE)) {
                    if(runt_cell_new(vm, &tmp) == 0) return RUNT_NOT_OK;
                    s = runt_push(vm);
                    s->f = entry->cell->id;
                    runt_copy_float(vm, vm->f_cell, tmp);
                } else {
                    s = runt_push(vm);
                    s->f = entry->cell->id;
                }
                s->p = runt_mk_cptr(vm, entry->cell);

                break;
            case RUNT_PROC:
                if(vm->status & RUNT_MODE_KEYWORD) {
                    rc = runt_word_search(vm, &str[pos], word_size, &entry);

                    s = runt_peak(vm);
                    tmp = runt_to_cell(s->p);

                    if(rc == RUNT_OK) {
                        runt_print(vm,
                            "Word '%*.*s' previously defined\n",
                            word_size, word_size, &str[pos]);

                        /* on failure, break to interactive mode */
                        runt_set_state(vm, RUNT_MODE_KEYWORD, RUNT_OFF);
                        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
                        return RUNT_NOT_OK;
                    } else {
                        c = runt_cell_pool_used(vm);
                        m = runt_memory_pool_used(vm);
                        entry = NULL;
                        runt_entry_create(vm, tmp, &entry);
                        runt_word(vm, &str[pos], word_size, entry);
                        runt_word_last_defined(vm, entry, c - 1, m);
                    }

                    runt_set_state(vm, RUNT_MODE_KEYWORD, RUNT_OFF);
                    break;
                }

                if(runt_word_search(vm, &str[pos], word_size, &entry) == RUNT_OK) {
                    if(vm->status & RUNT_MODE_INTERACTIVE) {
                        if(runt_entry_exec(vm, entry) == RUNT_NOT_OK) {
                            runt_print(vm, "Error with word '%*.*s'\n",
                                    word_size, word_size, &str[pos]);
                            return RUNT_NOT_OK;
                        }

                    } else {
                        if(runt_cell_new(vm, &tmp) == 0) return RUNT_NOT_OK;
                        runt_entry_copy(vm, entry, tmp);
                    }
                } else {
                    /*TODO: cleaner error reporting */
                    runt_print(vm, "Could not find function '%*.*s'\n",
                            word_size, word_size, str + pos);
                    return RUNT_NOT_OK;
                }
                break;

            case RUNT_NIL:
                break;
            default:
                runt_print(vm, "UNKOWN TYPE\n");
        }
    }
    return RUNT_OK;
}

static int runt_copy_float(runt_vm *vm, runt_cell *src, runt_cell *dest)
{
    runt_stacklet *s = runt_pop(vm);

    dest->fun = src->fun;
    dest->p = runt_mk_float(vm, s->f);

    return RUNT_OK;
}

static int rproc_float(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float *f;
    runt_int rc;

    f = runt_to_float(p);
    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);

    s->f = *f;
    s->t = RUNT_FLOAT;
    return RUNT_OK;
}

runt_int runt_set_state(runt_vm *vm, runt_uint mode, runt_uint state)
{
    runt_int rc = RUNT_OK;

    if(state == RUNT_ON) {
        vm->status |= mode;
    } else if(state == RUNT_OFF) {
        vm->status &= ~(mode);
    } else {
       rc = RUNT_NOT_OK;
    }

    return rc;
}

runt_uint runt_get_state(runt_vm *vm, runt_uint mode)
{
    if(vm->status & mode) {
        return RUNT_ON;
    } else {
        return RUNT_OFF;
    }
}

runt_uint runt_word_define(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc)
{
    return runt_word_define_with_copy(vm, name, size, proc, runt_cell_link);
}

runt_uint runt_word_define_with_copy(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_copy_proc copy)
{
    runt_cell *cell;
    runt_entry *entry;
    runt_uint id;
    entry = NULL;

    id = runt_cell_new(vm, &cell);
    if(id == 0) {
        return 0;
    }
    runt_cell_bind(vm, cell, proc);
    runt_entry_create(vm, cell, &entry);
    runt_entry_set_copy_proc(entry, copy);
    runt_word(vm, name, size, entry);

    return id;
}

runt_int runt_keyword_define(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_cell **pcell)
{
    return runt_keyword_define_with_copy(
        vm, name, size, proc, runt_cell_link, pcell
    );
}

runt_int runt_keyword_define_with_copy(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_proc proc,
        runt_copy_proc copy,
        runt_cell **pcell)
{
    runt_cell *cell;
    runt_entry *entry;
    runt_int rc;
    cell = NULL;
    entry = NULL;
    rc = runt_cell_malloc(vm, &cell);
    RUNT_ERROR_CHECK(rc);
    runt_cell_bind(vm, cell, proc);
    runt_entry_create(vm, cell, &entry);
    runt_entry_set_copy_proc(entry, copy);
    runt_word(vm, name, size, entry);
    if(pcell != NULL) *pcell = cell;
    return RUNT_OK;
}

static int runt_copy_string(runt_vm *vm, runt_cell *src, runt_cell *dest)
{
    runt_stacklet *s = runt_pop(vm);

    dest->fun = src->fun;
    dest->p = s->p;

    return RUNT_OK;
}

static int rproc_string(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;

    s = runt_push(vm);
    s->p = p;
    s->t = RUNT_STRING;
    return RUNT_OK;
}

static int rproc_begin(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_set_state(vm, RUNT_MODE_KEYWORD, RUNT_ON);
    return runt_proc_begin(vm, dst);
}

static int rproc_end(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_stacklet *s = runt_peak(vm);
    runt_cell *proc = runt_to_cell(s->p);
    runt_cell_undo(vm);
    proc->psize--;
    return runt_proc_end(vm);
}

runt_int runt_cell_undo(runt_vm *vm)
{
    runt_cell_pool *pool = &vm->cell_pool;
    runt_cell *cell;

    if(pool->used == 0) {
        return RUNT_NOT_OK;
    }

    pool->used--;
    /* id is N - 1 */
    cell = &pool->cells[pool->used];
    runt_cell_clear(vm, cell);

    return RUNT_OK;
}

runt_int runt_is_alive(runt_vm *vm)
{
    if(runt_get_state(vm, RUNT_MODE_RUNNING) == RUNT_ON &&
        runt_get_state(vm, RUNT_MODE_PANIC) == RUNT_OFF) {
        return RUNT_OK;
    } else {
        return RUNT_NOT_OK;
    }
}

#ifdef RUNT_PLUGINS
static runt_int rproc_close_plugin(runt_vm *vm, runt_ptr p)
{
    void *handle;
    handle = runt_to_cptr(p);
    if(handle != NULL) {
        dlclose(handle);
    }
    return RUNT_OK;
}
#endif


#ifdef RUNT_PLUGINS
/* modified from tinyscheme dynload utility */

static void make_init_fn(const char *name, char *init_fn) {
    const char *p;
    char *e;

    p=strrchr(name,'/');
    e=strrchr(name, '.');
    if(p==0) {
        p=name;
    } else {
        p++;
    }
    if(e != 0) {
        *e = 0;
    }
    strcpy(init_fn,"rplug_");
    strcat(init_fn,p);
}
#endif

runt_int runt_load_plugin(runt_vm *vm, const char *filename)
{
#ifdef RUNT_PLUGINS
    void *handle = dlopen(filename, RTLD_NOW);
    int (*fun)(runt_vm *);
    char fn_name[50];
    runt_ptr p;
    runt_cell *cell;
    runt_int rc;

    if(handle == NULL) {
        runt_print(vm, "%s\n", dlerror());
        return RUNT_NOT_OK;
    }

    make_init_fn(filename, fn_name);

    *(void **) (&fun) = dlsym(handle, fn_name);

    if(fun == NULL) {
        runt_print(vm, "Could not find loader function %s\n", fn_name);
        return RUNT_NOT_OK;
    }

    rc = fun(vm);
    RUNT_ERROR_CHECK(rc);

    /* add handle to plugin list */

    p = runt_mk_cptr(vm, handle);
    handle = runt_to_cptr(p);

    rc = runt_cell_malloc(vm, &cell);
    RUNT_ERROR_CHECK(rc);

    runt_cell_bind(vm, cell, rproc_close_plugin);
    runt_cell_data(vm, cell, p);

    return runt_list_prepend_cell(vm, &vm->plugins, cell);

#endif
    return RUNT_OK;
}

void runt_mark_set(runt_vm *vm)
{
    vm->cell_pool.mark = vm->cell_pool.used;
    vm->memory_pool.mark = vm->memory_pool.used;
}

runt_uint runt_mark_free(runt_vm *vm)
{
    vm->cell_pool.used = vm->cell_pool.mark;
    vm->memory_pool.used = vm->memory_pool.mark;
    return RUNT_OK;
}

void runt_pmark_set(runt_vm *vm)
{
    if(runt_get_state(vm, RUNT_MODE_INTERACTIVE) == RUNT_ON) {
        runt_mark_set(vm);
    } else {
        if(runt_get_state(vm, RUNT_MODE_VERBOSE) == RUNT_ON)
            runt_print(vm, "Not setting a mark!\n");
    }
}

runt_uint runt_pmark_free(runt_vm *vm)
{
    if(runt_get_state(vm, RUNT_MODE_INTERACTIVE) == RUNT_ON) {
        runt_mark_free(vm);
    } else {
        if(runt_get_state(vm, RUNT_MODE_VERBOSE) == RUNT_ON)
            runt_print(vm, "Not freeing!\n");
    }
    return RUNT_OK;
}

void runt_cell_clear(runt_vm *vm, runt_cell *cell)
{
   runt_cell_bind(vm, cell, runt_proc_zero);
   cell->p = vm->nil;
   cell->psize = 0;
}

void * runt_to_cptr(runt_ptr p)
{
    void *cptr = NULL;

    /*TODO: error handling */
    if(p.type == RUNT_CPTR) {
        cptr = p.ud;
    }

    return cptr;
}

runt_ptr runt_mk_cptr(runt_vm *vm, void *cptr)
{
    return runt_mk_ptr(RUNT_CPTR, cptr);
}

static int rproc_cptr(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_push(vm);
    s->p = p;
    return RUNT_OK;
}


runt_list * runt_to_list(runt_ptr p)
{
    runt_list *lst = NULL;
    /*TODO: error handling */
    if(p.type == RUNT_LIST) {
        lst = p.ud;
    }
    return lst;
}

runt_ptr runt_mk_list(runt_vm *vm, runt_list *lst)
{
    return runt_mk_ptr(RUNT_LIST, lst);
}

runt_int runt_mk_cptr_cell(runt_vm *vm, void *cptr)
{
    runt_cell *cell;
    if(runt_cell_new(vm, &cell) == RUNT_NOT_OK) {
        return RUNT_NOT_OK;
    }
    cell->fun = rproc_cptr;
    cell->p = runt_mk_cptr(vm, cptr);
    return RUNT_OK;
}

runt_uint runt_mk_float_cell(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_float *flt)
{
    runt_uint id = runt_word_define(vm, name, size, vm->f_cell->fun);
    runt_word_bind_ptr(vm, id, runt_mk_ptr(RUNT_FLOAT, flt));
    return id;
}

void runt_print_default(runt_vm *vm, const char *fmt, va_list ap)
{
    vfprintf(vm->fp, fmt, ap);
}

void runt_print(runt_vm *vm, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vm->print(vm, fmt, args);
    va_end(args);
}

void runt_filehandle(runt_vm *vm, FILE *handle)
{
    vm->fp = handle;
}

FILE *runt_filehandle_get(runt_vm *vm)
{
    return vm->fp;
}

runt_int runt_word_bind_ptr(runt_vm *vm, runt_uint id, runt_ptr p)
{
    runt_cell *pool = vm->cell_pool.cells;
    if(id == 0) return RUNT_NOT_OK;
    pool[id - 1].p = p;
    return RUNT_OK;
}

runt_int runt_close_plugins(runt_vm *vm)
{
    runt_uint i;
    runt_entry *ent;
    runt_list *plugins;
    runt_uint size;

    /* first, call all user-made destructor functions */

    plugins = &vm->dtors;
    size = runt_list_size(plugins);
    ent = runt_list_top(plugins);

    for(i = 0; i < size; i++) {
        runt_cell_exec(vm, ent->cell);
        ent = ent->next;
    }

    /* next, close all dll plugins */

    plugins = &vm->plugins;
    size = runt_list_size(plugins);
    ent = runt_list_top(plugins);

    for(i = 0; i < size; i++) {
        runt_cell_exec(vm, ent->cell);
        ent = ent->next;
    }
    return RUNT_OK;
}

void runt_stacklet_copy(runt_vm *vm, runt_stacklet *src, runt_stacklet *dst)
{
    dst->f = src->f;
    dst->p = src->p;
    dst->t = src->t;
}

static int rproc_rec(runt_vm *vm, runt_ptr p)
{
    return runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
}

static int rproc_stop(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_cell_undo(vm);
    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
    runt_mark_set(vm);
    return RUNT_OK;
}

runt_int runt_cell_id_get(runt_vm *vm, runt_uint id, runt_cell **cell)
{
    if(id == 0) {
        *cell = vm->f_cell;
        return RUNT_NOT_OK;
    } else  {
        *cell = &vm->cell_pool.cells[id - 1];
        return RUNT_OK;
    }
}

/* Stop Runt */
void runt_seppuku(runt_vm *vm)
{
    runt_print(vm, "Runt is committing seppuku...\n");
    runt_set_state(vm, RUNT_MODE_PANIC, RUNT_ON);
    runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_OFF);
}

runt_dict * runt_dictionary_get(runt_vm *vm)
{
    return vm->dict;
}

void runt_dictionary_set(runt_vm *vm, runt_dict *dict)
{
    dict->last = vm->dict;
    vm->dict = dict;
}

runt_dict * runt_dictionary_swap(runt_vm *vm)
{
    runt_dict *last;

    last = vm->dict->last;
    last->last = vm->dict;
    vm->dict = last;

    return last;
}

runt_uint runt_dictionary_new(runt_vm *vm, runt_dict **pdict)
{
    runt_uint rc;
    rc = runt_malloc(vm, sizeof(runt_dict), (void **)pdict);
    runt_dict_init(vm, *pdict);
    return rc;
}

size_t runt_getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

runt_int runt_add_destructor(runt_vm *vm, runt_proc proc, runt_ptr ptr)
{
    runt_cell *cell;
    runt_int rc;

    rc = runt_cell_malloc(vm, &cell);
    RUNT_ERROR_CHECK(rc);

    runt_cell_bind(vm, cell, proc);
    runt_cell_data(vm, cell, ptr);

    return runt_cell_destructor(vm, cell);
}

runt_int runt_cell_destructor(runt_vm *vm, runt_cell *cell)
{
    return runt_list_prepend_cell(vm, &vm->dtors, cell);
}

int runt_stack_dup(runt_vm *vm)
{
    runt_stacklet *val;
    runt_stacklet *s1;
    runt_stacklet *s2;

    runt_int rc;

    val = NULL;
    s1 = NULL;
    s2 = NULL;

    rc = runt_ppop(vm, &val);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s2);
    RUNT_ERROR_CHECK(rc);

    runt_stacklet_copy(vm, val, s1);
    runt_stacklet_copy(vm, val, s2);

    return rc;
}

int runt_stack_swap(runt_vm *vm)
{
    runt_stacklet *s1;
    runt_stacklet *s2;
    runt_int rc;
    runt_stacklet a, b;

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    runt_stacklet_copy(vm, s1, &a);

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    runt_stacklet_copy(vm, s1, &b);

    rc = runt_ppush(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s2);
    RUNT_ERROR_CHECK(rc);

    runt_stacklet_copy(vm, &a, s1);
    runt_stacklet_copy(vm, &b, s2);

    return RUNT_OK;
}

runt_int runt_cell_from_stack(runt_vm *vm, runt_stacklet *s, runt_cell **cell)
{
    if(s->f > 0) {
        runt_cell_id_get(vm, s->f, cell);
    } else {
        *cell = runt_to_cptr(s->p);
    }
    return RUNT_OK;
}

runt_int runt_word_last_defined(runt_vm *vm,
        runt_entry *ent,
        runt_uint c,
        runt_uint m)
{
    ent->c = c;
    ent->m = m;
    ent->last_word_defined = vm->last_word_defined;
    vm->last_word_defined = ent;
    vm->nwords++;
    return RUNT_OK;
}

runt_int runt_word_oops(runt_vm *vm)
{
    const char *name;
    runt_entry *last_word;

    if(vm->nwords == 0) {
        runt_print(vm, "No words to undefine.\n");
        return RUNT_NOT_OK;
    }

    last_word = vm->last_word_defined;
    name = runt_to_string(last_word->p);
    runt_print(vm, "Undefining word '%s'\n", name);

    runt_list_pop(vm, last_word->list);
    vm->cell_pool.used = last_word->c;
    vm->memory_pool.used = last_word->m;

    vm->last_word_defined = last_word->last_word_defined;
    vm->dict->nwords--;

    vm->nwords--;

    runt_mark_set(vm);
    return RUNT_OK;
}

static runt_int load_nothing(runt_vm *vm)
{
    return RUNT_OK;
}

runt_int runt_data(runt_vm *vm,
        const char *name,
        runt_uint size,
        runt_ptr p)
{
    runt_cell *cell;
    runt_keyword_define(vm, name, size, vm->zproc, &cell);
    runt_cell_data(vm, cell, p);
    return RUNT_OK;
}

runt_int runt_data_search(runt_vm *vm, const char *name, runt_ptr *p)
{
    runt_entry *entry;
    runt_int rc;

    rc = runt_word_search(vm, name, strlen(name), &entry);
    RUNT_ERROR_CHECK(rc);
    *p = entry->cell->p;
    return RUNT_OK;
}

void runt_print_set(runt_vm *vm, runt_printer printer)
{
    vm->print = printer;
}
