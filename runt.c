#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "runt.h"

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p);
static runt_int runt_proc_link(runt_vm *vm, runt_ptr p);
static int runt_copy_float(runt_vm *vm, runt_cell *src, runt_cell *dest);
static int rproc_float(runt_vm *vm, runt_ptr p);
static int runt_copy_string(runt_vm *vm, runt_cell *src, runt_cell *dest);
static int rproc_string(runt_vm *vm, runt_ptr p);
static int rproc_begin(runt_vm *vm, runt_cell *src, runt_cell *dst);
static int rproc_end(runt_vm *vm, runt_cell *src, runt_cell *dst);
static int rproc_cptr(runt_vm *vm, runt_ptr p);

runt_int runt_init(runt_vm *vm)
{
    vm->status = 0;
    vm->zproc = runt_proc_zero;
    vm->nil = runt_mk_ptr(RUNT_NIL, NULL);

    /* init stack */

    vm->stack.pos = 0;
    vm->stack.size = RUNT_STACK_SIZE;

    /* init dictionary */

    runt_dictionary_init(vm);

    /* record cells being parsed by default */

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
    runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_ON);

    vm->level = 0;

    return RUNT_OK;
}

runt_int runt_load_stdlib(runt_vm *vm)
{
    /* create float type */
    runt_cell_new(vm, &vm->f_cell);
    runt_cell_bind(vm, vm->f_cell, rproc_float);

    /* create string type */
    runt_cell_new(vm, &vm->s_cell);
    runt_cell_bind(vm, vm->s_cell, rproc_string);
   
    /* ability to create procedures by default */
    runt_word_define_with_copy(vm, ":", 1, vm->zproc, rproc_begin);
    runt_word_define_with_copy(vm, ";", 1, vm->zproc, rproc_end);

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
    if(pool->used >= pool->size) {
        return 0;
    }
    pool->used++;
    if(vm->status & RUNT_MODE_PROC) vm->proc->psize++;
    id = pool->used;
    *cell = &pool->cells[id - 1];
    return id;
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
    cell->p = vm->nil;
    return RUNT_OK;
}

runt_int runt_call(runt_vm *vm, runt_cell *cell)
{
    return cell->fun(vm, cell->p);
}

static runt_int runt_proc_zero(runt_vm *vm, runt_ptr p)
{
    return RUNT_OK;
}

runt_int runt_proc_begin(runt_vm *vm, runt_cell *proc)
{
    vm->status |= RUNT_MODE_PROC;
    vm->proc = proc;
    return RUNT_OK;
}

runt_int runt_proc_end(runt_vm *vm)
{
    vm->status &= ~(RUNT_MODE_PROC);
    return RUNT_OK;
}

runt_int runt_cell_exec(runt_vm *vm, runt_cell *cell)
{
    runt_uint i;
    runt_int rc = RUNT_OK;
  
    if(cell->psize == 1) {
        return runt_call(vm, cell);
    }

    for(i = 1; i < cell->psize; i++) {
        if(cell[i].psize == 1) {
            rc = runt_call(vm, &cell[i]);
        } else {
            vm->level++;
            rc = runt_cell_exec(vm, &cell[i]); 
            vm->level--;
        }
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
    }
    return cell;
}

runt_float * runt_to_float(runt_ptr p)
{
    runt_float *f;

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
    p.pos = runt_malloc(vm, sizeof(runt_float), (void**)&val);
    p = runt_mk_ptr(RUNT_FLOAT, val);
    *val = ival;
    return p;
}

const char * runt_to_string(runt_ptr p)
{
    const char *str;

    /*TODO: error handling */
    if(p.type == RUNT_STRING) {
        str = p.ud;
    }

    return str;
}

runt_ptr runt_mk_string(runt_vm *vm, const char *str, runt_uint size)
{
    runt_ptr p;
    char *buf;
    runt_uint i;
    runt_uint pos = runt_malloc(vm, size + 1, (void *)&buf);

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
    /*TODO: error handling for stack overflows */
    vm->stack.pos++;
    return &vm->stack.stack[vm->stack.pos - 1];
}

runt_stacklet * runt_pop(runt_vm *vm)
{
    /*TODO: error handling for stack underflows */
    if(vm->stack.pos > 0) {
        vm->stack.pos--;
        return &vm->stack.stack[vm->stack.pos];
    }
    
    return &vm->stack.stack[0];
}

runt_uint runt_malloc(runt_vm *vm, size_t size, void **ud)
{

    runt_memory_pool *pool = &vm->memory_pool;
    runt_uint id = 0;

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
    e->str = vm->nil;
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
    vm->level = 0;
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
    runt_uint pos = runt_hash(name, size);
    runt_list *list = &vm->dict.list[pos]; 

    entry->str = runt_mk_string(vm, name, size);

    runt_list_append(list, entry);

    return RUNT_NOT_OK;
}

static runt_int runt_strncmp(const char *str, runt_ptr ptr, runt_int n)
{
    return strncmp(str, runt_to_string(ptr), n);
}

runt_int runt_word_search(runt_vm *vm, 
        const char *name, 
        runt_int size,
        runt_entry **entry)
{
    runt_uint pos = runt_hash(name, size);
    runt_list *list = &vm->dict.list[pos]; 

    runt_uint i;
    runt_entry *ent = list->root.next;
    runt_entry *next;


    for(i = 0; i < list->size; i++) {
        next = ent->next;
        if(runt_strncmp(name, ent->str, size) == 0) {
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
    lst->last = &lst->root;
}

runt_int runt_list_append(runt_list *lst, runt_entry *ent)
{
    lst->last->next = ent;
    lst->last = ent;
    lst->size++;
    return RUNT_OK;
}

void runt_dictionary_init(runt_vm *vm)
{
    runt_uint i;
    runt_dict *dict = &vm->dict;
    for(i = 0; i < RUNT_DICT_SIZE; i++) {
        runt_list_init(&dict->list[i]);
    }
}

runt_uint runt_memory_pool_size(runt_vm *vm)
{
    return vm->memory_pool.size;
}

runt_uint runt_memory_pool_used(runt_vm *vm)
{
    return vm->memory_pool.used;
}

runt_uint runt_cell_pool_size(runt_vm *vm)
{
    return vm->cell_pool.size;
}

runt_uint runt_cell_pool_used(runt_vm *vm)
{
    return vm->cell_pool.used;
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
                if(str[s] != ' ' && str[s] != '\n') {
                    if(str[s] == '\'' || str[s] == '\"') {
                        mode = 3;
                        m = str[s];
                    } else {
                        mode = 1;
                        *pos = s;
                    }
                }
                continue;
            default: break;
        }

    }

    /* if we are at the end, make next larger than the size */
    if(s == size) {
        *next = size + 1;
        if(mode == 1) {
            *wsize = (s - *pos);
        }
    }
    return RUNT_CONTINUE;
}

runt_type runt_lex(runt_vm *vm, 
        const char *str,
        runt_uint pos,
        runt_uint size)
{
    runt_uint c;

    for(c = pos; c < pos + size; c++) {
        switch(str[c]) {
            case '0':
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
            default:
                return RUNT_PROC;
        }
    }

    return RUNT_NIL;
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

runt_int runt_compile(runt_vm *vm, const char *str)
{
    runt_uint size = strlen(str);
    runt_uint pos = 0;
    runt_uint word_size = 0;
    runt_uint next = 0;
    runt_stacklet *s;
    runt_cell *tmp;
    runt_entry *entry;

    float val = 0.0;
    while(runt_tokenize(vm, str, size, &pos, &word_size, &next) == RUNT_CONTINUE) 
    {
        switch(runt_lex(vm, str, pos, word_size)) {
            case RUNT_FLOAT:
                val = runt_atof(str, pos, word_size);
                s = runt_push(vm);
                s->f = val;
                if(!(vm->status & RUNT_MODE_INTERACTIVE)) {
                    runt_cell_new(vm, &tmp);
                    runt_copy_float(vm, vm->f_cell, tmp);
                }
                break;
            case RUNT_STRING:

                s = runt_push(vm);
                s->p = runt_mk_string(vm, &str[pos + 1], word_size - 2);
                runt_cell_new(vm, &tmp);
                runt_copy_string(vm, vm->s_cell, tmp);

                break;
            case RUNT_WORD:
                s = runt_push(vm);
                s->p = runt_mk_string(vm, &str[pos + 1], word_size - 1);
                runt_cell_new(vm, &tmp);
                runt_copy_string(vm, vm->s_cell, tmp);
                break;
            case RUNT_PROC:

                if(vm->status & RUNT_MODE_KEYWORD) {
                    runt_entry_create(vm, vm->proc, &entry);
                    runt_word(vm, &str[pos], word_size, entry);
                    runt_set_state(vm, RUNT_MODE_KEYWORD, RUNT_OFF);
                    break;
                } 

                if(runt_word_search(vm, &str[pos], word_size, &entry) == RUNT_OK) {
                    if(vm->status & RUNT_MODE_INTERACTIVE) {
                        runt_entry_exec(vm, entry);
                    } else {
                        runt_cell_new(vm, &tmp);
                        runt_entry_copy(vm, entry, tmp);
                    }
                } else {
                    /*TODO: cleaner error reporting */
                    runt_print(vm, "Error: could not find function '%*.*s'\n",
                            word_size, word_size, str + pos);
                }
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

    f = runt_to_float(p);
    s = runt_push(vm);
    s->f = *f;
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

    id = runt_cell_new(vm, &cell);
    runt_cell_bind(vm, cell, proc);
    runt_entry_create(vm, cell, &entry);
    runt_entry_set_copy_proc(entry, copy);
    runt_word(vm, name, size, entry);

    return id;
}

static int runt_copy_string(runt_vm *vm, runt_cell *src, runt_cell *dest)
{
    runt_stacklet *s = runt_pop(vm);

    const char *str = runt_to_string(s->p);

    dest->fun = src->fun;
    dest->p = runt_mk_string(vm, str, strlen(str));

    return RUNT_OK;
}

static int rproc_string(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;

    s = runt_push(vm);
    s->p = p;
    return RUNT_OK;
}

static int rproc_begin(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_set_state(vm, RUNT_MODE_KEYWORD, RUNT_ON);
    return runt_proc_begin(vm, dst);
}

static int rproc_end(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    /* TODO: make this work someday.. */
    runt_cell_undo(vm);
    vm->proc->psize--;
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
    if(vm->status & RUNT_MODE_RUNNING) {
        return RUNT_OK;
    } else {
        return RUNT_NOT_OK;
    }
}

runt_int runt_load_plugin(runt_vm *vm, const char *filename)
{
    void *handle = dlopen(filename, RTLD_NOW);
    void (*fun)(runt_vm *);

    if(handle == NULL) {
        dlerror();
        return RUNT_NOT_OK;
    }

    *(void **) (&fun) = dlsym(handle, "runt_plugin_init");

    fun(vm);
    
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
   cell->psize = 1;
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

void runt_print(runt_vm *vm, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
