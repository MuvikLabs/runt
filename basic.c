#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "runt.h"

typedef struct {
    runt_cell *cell_if;
    runt_cell *cell_else;
} runt_if_d;

static runt_int rproc_print(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    runt_print(vm, "%g\n", s->f);
    return RUNT_OK;
}

static runt_int rproc_say(runt_vm *vm, runt_ptr p)
{
    /* runt_stacklet *s = runt_pop(vm); */
    runt_stacklet *s;
    const char *str;

    runt_ppop(vm, &s);
    str = runt_to_string(s->p);
    runt_print(vm, "%s\n", str);
    return RUNT_OK;
}

static runt_int rproc_quit(runt_vm *vm, runt_ptr p)
{
    runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_OFF);
    return RUNT_OK;
}

static runt_int rproc_add(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1;
    runt_stacklet *s2;
    runt_stacklet *out;
    runt_float v1;
    runt_float v2;

    runt_int rc = RUNT_OK;

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppop(vm, &s2);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &out);
    RUNT_ERROR_CHECK(rc);

    v1 = runt_stack_float(vm, s1);
    v2 = runt_stack_float(vm, s2);
    out->f = v1 + v2;
    return rc;
}

static runt_int rproc_sub(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 - v1;
    return RUNT_OK;
}

static runt_int rproc_div(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 / v1;
    return RUNT_OK;
}

static runt_int rproc_mul(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 * v1;
    return RUNT_OK;
}

static runt_int rproc_dup(runt_vm *vm, runt_ptr p)
{
    /* TODO: how do we make this polymorphic? */
    runt_stacklet *val = NULL;
    runt_stacklet *s1 = NULL;
    runt_stacklet *s2 = NULL;

    runt_int rc; 

    rc = runt_ppop(vm, &val);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s2);
    RUNT_ERROR_CHECK(rc);

    s1->f = val->f;
    s2->f = val->f;

    return rc;
}

static runt_int rproc_dynload(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *val = runt_pop(vm);
    const char *filename = runt_to_string(val->p);
    runt_uint prev_state = runt_get_state(vm, RUNT_MODE_INTERACTIVE);
    runt_int rc = RUNT_OK;

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);

    /* remove dynload cell from cell pool */

    runt_cell_undo(vm);

    /* remove string from cell pool */

    vm->memory_pool.used = val->p.pos;

    rc = runt_load_plugin(vm, filename);

    if(rc == RUNT_NOT_OK) {
        runt_print(vm, "Error loading plugin\n");
    }

    /* mark it so it doesn't get overwritten in memory 
     * NOTE: this will cause the filepath to stay in memory
     * this will be fixed in the future for sure... 
     * is this fixed actually? */

    runt_mark_set(vm);

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, prev_state);

    return rc;
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

static int rproc_swap(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1;
    runt_stacklet *s2;
    runt_int rc;
    runt_float a, b;

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    a = s1->f;

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    b = s1->f;

    rc = runt_ppush(vm, &s1);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s2);
    RUNT_ERROR_CHECK(rc);

    s1->f = a;
    s2->f = b;

    return RUNT_OK;
}

static int rproc_drop(runt_vm *vm, runt_ptr p)
{
    runt_pop(vm);
    return RUNT_OK;
}

static int rproc_rot(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_stacklet *push[3];
    runt_int rc;
    runt_float a, b, c;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    a = s->f;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    b = s->f;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    c = s->f;

    rc = runt_ppush(vm, &push[0]);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &push[1]);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &push[2]);
    RUNT_ERROR_CHECK(rc);

    push[0]->f = b;
    push[1]->f = a;
    push[2]->f = c;
    
    return RUNT_OK;
}

static int rproc_nitems(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int rc;
    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    /* subtract one to discount the pushed value itself */
    s->f = runt_stack_pos(vm, &vm->stack) - 1;
    return RUNT_OK;
}

static int rproc_rat(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_stacklet *push[3];
    runt_int rc;
    runt_float a, b, c;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    a = s->f;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    b = s->f;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    c = s->f;

    rc = runt_ppush(vm, &push[0]);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &push[1]);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &push[2]);
    RUNT_ERROR_CHECK(rc);

    push[0]->f = a;
    push[1]->f = c;
    push[2]->f = b;
    
    return RUNT_OK;
}

static int rproc_peak(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1;
    runt_stacklet *s2;
    runt_stacklet *peak;
    runt_int rc;
    runt_int pos;

    rc = runt_ppop(vm, &s1);
    RUNT_ERROR_CHECK(rc);

    pos = s1->f;

    rc = runt_ppeakn(vm, &peak, pos);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppush(vm, &s2);
    RUNT_ERROR_CHECK(rc);

    s2->f = peak->f;
    
    return RUNT_OK;
}

static int rproc_lt(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float v1, v2;

    s = runt_pop(vm);
    v2 = s->f;
    s = runt_pop(vm);
    v1 = s->f;
    s = runt_push(vm);

    if(v1 < v2) s->f = 1.0;
    else s->f = 0.0;

    return RUNT_OK;
}

static int rproc_gt(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float v1, v2;

    s = runt_pop(vm);
    v2 = s->f;
    s = runt_pop(vm);
    v1 = s->f;
    s = runt_push(vm);

    if(v1 > v2) s->f = 1.0;
    else s->f = 0.0;

    return RUNT_OK;
}

static int rproc_eq(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float v1, v2;

    s = runt_pop(vm);
    v2 = s->f;
    s = runt_pop(vm);
    v1 = s->f;
    s = runt_push(vm);

    if(v1 == v2) s->f = 1.0;
    else s->f = 0.0;

    return RUNT_OK;
}

static int rproc_neq(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float v1, v2;

    s = runt_pop(vm);
    v2 = s->f;
    s = runt_pop(vm);
    v1 = s->f;
    s = runt_push(vm);

    if(v1 != v2) s->f = 1.0;
    else s->f = 0.0;

    return RUNT_OK;
}

static int rproc_end(runt_vm *vm, runt_ptr p)
{
    runt_set_state(vm, RUNT_MODE_END, RUNT_ON);
    return RUNT_OK;
}

static int rproc_call_copy(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    dst->fun = src->fun;
    return RUNT_OK;
}

static int rproc_call(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int rc;
    runt_uint level;
    runt_uint ppos;
    runt_int pstate;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);

    pstate = runt_get_state(vm, RUNT_MODE_END);
    runt_set_state(vm, RUNT_MODE_END, RUNT_OFF);
    ppos = vm->pos;
    vm->pos = s->f;

    vm->level++;
    level = vm->level; 
    while(runt_get_state(vm, RUNT_MODE_END) == RUNT_OFF &&
            vm->level == level) {
        rc = runt_cell_call(vm, &vm->cell_pool.cells[vm->pos - 1]);
        RUNT_ERROR_CHECK(rc); 
        vm->pos++;
    }
    vm->level--;
    vm->pos = ppos;
    runt_set_state(vm, RUNT_MODE_END, pstate);
   
    return RUNT_OK;
}

static int rproc_ex(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int rc;
    runt_uint pos;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    pos = s->f;

    runt_cell_id_exec(vm, pos);
   
    return RUNT_OK;
}

static int rproc_goto(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_uint pos;
    runt_uint jump;
    runt_int rc;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    pos = s->f;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    jump = s->f;

    if(jump && pos < vm->cell_pool.size + 1) {
        vm->pos = pos;
    }
    
    return RUNT_OK;
}

static int rproc_decr(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int rc;
        
    rc = runt_ppeak(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f -= 1;
    return RUNT_OK;
}

static int rproc_decrn(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_int pos;
    runt_stacklet *s;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    pos = s->f;
    rc = runt_ppeakn(vm, &s, pos);
    RUNT_ERROR_CHECK(rc);
    s->f -= 1;
    return RUNT_OK;
}

static int rproc_incr(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_peak(vm);
    s->f += 1;
    return RUNT_OK;
}

static int rproc_incrn(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_int pos;
    runt_stacklet *s;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    pos = s->f;
    rc = runt_ppeakn(vm, &s, pos);
    RUNT_ERROR_CHECK(rc);
    s->f += 1;
    return RUNT_OK;
}


static int rproc_rep(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    runt_uint id;
    runt_uint i, reps;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    id = s->f;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    reps = s->f;

    for(i = 0; i < reps; i++) {
        rc = runt_cell_id_exec(vm, id);
        RUNT_ERROR_CHECK(rc);
    }   
 
    return RUNT_OK;
}

static int rproc_set(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_float f;
    runt_uint id;
    runt_float *ptr;

    s = runt_pop(vm);
    id =s->f;
    s = runt_pop(vm);
    f = s->f;

    ptr = (runt_float *)vm->cell_pool.cells[id].p.ud;
    *ptr = f;

    return RUNT_OK;
}

static void parse(runt_vm *vm, char *str, size_t read)
{
    const char *code = str;
    /* runt_pmark_set(vm); */
    runt_compile(vm, code);
    /* runt_pmark_free(vm); */
}

static runt_int load_dictionary(runt_vm *vm, const char *filename)
{
    FILE *fp; 
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        runt_print(vm, "Could not open file %s\n", filename);
        return RUNT_NOT_OK;
    }

    while((read = getline(&line, &len, fp)) != -1) {
        parse(vm, line, read);
    }

    fclose(fp);
    free(line);
    return RUNT_OK;
}

static int rproc_load(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int rc;
    const char *str;
    const char *fname;
    char buf[1024];
    int pstate = RUNT_OFF;

    memset(buf, 0, 1024);
    s = runt_pop(vm);
    str = runt_to_string(s->p);
    pstate = runt_get_state(vm, RUNT_MODE_INTERACTIVE);

    sprintf(buf, "%s.rnt", str);

    /* remove load cell from cell pool */
    runt_cell_undo(vm);

    if(access(buf, F_OK) != -1) {
        fname = buf;
        /* TODO: DRY */
        /* remove string from cell pool */
        vm->memory_pool.used = s->p.pos;
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
        load_dictionary(vm, fname);
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, pstate);
        runt_mark_set(vm);
        return RUNT_OK;
    } 

    if(str[0] != '.' ) {
        /* try to find a .so native plugin */

        if(getenv("RUNT_PLUGIN_PATH") != NULL) {
            sprintf(buf, "%s/%s.so", getenv("RUNT_PLUGIN_PATH"), str);
        } else {
            /* default path */
            sprintf(buf, "/usr/local/share/runt/%s.so", str);
        }

        if(access(buf, F_OK) != -1) {
            fname = buf;
            /* TODO: DRY */
            /* remove string from cell pool */
            vm->memory_pool.used = s->p.pos;

            runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
            rc = runt_load_plugin(vm, fname);
            RUNT_ERROR_CHECK(rc);
            runt_set_state(vm, RUNT_MODE_INTERACTIVE, pstate);
            runt_mark_set(vm);
            return RUNT_OK;
        } 

        /* finally, try to find a .rnt file */
        if(getenv("RUNT_PLUGIN_PATH") != NULL) {
            sprintf(buf, "%s/%s.rnt", getenv("RUNT_PLUGIN_PATH"), str);
        } else {
            /* default path */
            sprintf(buf, "/usr/local/share/runt/%s.rnt", str);
        }

        if(access(buf, F_OK) != -1) {
            fname = buf;
            /* TODO: DRY */
            /* remove string from cell pool */
            vm->memory_pool.used = s->p.pos;
            runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
            load_dictionary(vm, fname);
            runt_set_state(vm, RUNT_MODE_INTERACTIVE, pstate);
            runt_mark_set(vm);
            return RUNT_OK;
        } 
    }

    sprintf(buf, "%s.so", str);

    if(access(buf, F_OK) != -1) {
        fname = buf;
        /* TODO: DRY */
        /* remove string from cell pool */
        vm->memory_pool.used = s->p.pos;

        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
        rc = runt_load_plugin(vm, fname);
        RUNT_ERROR_CHECK(rc);
        runt_set_state(vm, RUNT_MODE_INTERACTIVE, pstate);
        runt_mark_set(vm);
        return RUNT_OK;
    } 

    runt_print(vm, "Could not find '%s'\n", str);
    return RUNT_NOT_OK;
}

static int rproc_usage(runt_vm *vm, runt_ptr p)
{
    runt_print(vm, "Cell pool: used %d of %d cells.\n", 
            runt_cell_pool_used(vm),
            runt_cell_pool_size(vm));

    runt_print(vm, "Memory pool: used %d of %d bytes.\n", 
            runt_memory_pool_used(vm),
            runt_memory_pool_size(vm));

    runt_print(vm, "Dictionary: %d words defined.\n", 
            runt_dictionary_size(vm));
    return RUNT_OK;
}

static int rproc_mem(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f = runt_memory_pool_used(vm);
    return RUNT_OK;
}

static int rproc_cells(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f = runt_cell_pool_used(vm);
    return RUNT_OK;
}

static runt_int rproc_clear(runt_vm *vm, runt_ptr p)
{
    runt_cell_pool_clear(vm);
    runt_memory_pool_clear(vm);
    runt_close_plugins(vm);
    runt_dictionary_clear(vm);
    runt_load_stdlib(vm);
    /* make memory mark so dead cells can be cleared */
    runt_mark_set(vm);
    return RUNT_OK;
}

static runt_int rproc_bias(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_int bias;
    runt_int rc;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    bias = s->f;
    runt_stack_bias(vm, &vm->stack, bias);
    return RUNT_OK;
}

static runt_int rproc_unbias(runt_vm *vm, runt_ptr p)
{
    runt_stack_unbias(vm, &vm->stack);
    return RUNT_OK;
}

static runt_int rproc_drops(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    runt_uint n;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    n = s->f;
    runt_stack_dec_n(vm, &vm->stack, n);
    return RUNT_OK;
}

static runt_int rproc_wordlist(runt_vm *vm, runt_ptr p)
{
    runt_int w, e;
    runt_entry *entry;
    runt_int nentry = 0;
    runt_list *list = vm->dict.list;
    
    for(w = 0; w < RUNT_DICT_SIZE; w++) {
        entry = list[w].root.next;
        nentry = list[w].size;
        for(e = 0; e < nentry ; e++) {
            runt_print(vm, "%s\n", runt_to_string(entry->p));
            entry = entry->next;
        }
    }

    return RUNT_OK;
}

static runt_int rproc_rand(runt_vm *vm, runt_ptr p)
{
    runt_uint max;
    runt_stacklet *s;
    runt_int rc; 

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    max = s->f;

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f = rand() % max;

    return RUNT_OK;
}

static runt_int rproc_psize(runt_vm *vm, runt_ptr p)
{
    runt_uint id;
    runt_stacklet *s;
    runt_int rc;
    runt_cell *cell;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    id = s->f;

    rc = runt_cell_pool_get_cell(vm, id, &cell);
    RUNT_ERROR_CHECK(rc);

    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
   
    s->f = cell->psize;

    return RUNT_OK;
}

static runt_int rproc_over(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *peak;
    runt_stacklet *s;
    rc = runt_ppeakn(vm, &peak, -2);
    RUNT_ERROR_CHECK(rc);
    rc = runt_ppush(vm, &s);
    RUNT_ERROR_CHECK(rc);
    s->f = peak->f;
    return RUNT_OK;
}

static runt_int rproc_restore(runt_vm *vm, runt_ptr p)
{
    runt_int rc;
    runt_stacklet *s;
    runt_uint mem;
    runt_uint cell;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    mem = s->f;
    
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    cell = s->f;
   
    vm->cell_pool.used = cell;
    vm->memory_pool.used = mem;

    runt_cell_undo(vm);
    runt_mark_set(vm);
    return RUNT_OK;
}

static runt_int rproc_undef(runt_vm *vm, runt_ptr p)
{
    const char *str;
    runt_int rc;
    runt_stacklet *s;

    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    str = runt_to_string(s->p);

    runt_word_undefine(vm, str, strlen(str));

    return RUNT_OK;
}

static runt_int rcopy_block_begin(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    return runt_proc_begin(vm, dst);
}

static runt_int rcopy_block_end(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_int rc;
    runt_stacklet *s;
    runt_cell *proc;
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    proc = runt_to_cell(s->p);
    proc->psize--;
    return RUNT_OK;
}

runt_int runt_load_basic(runt_vm *vm)
{
    /* quit function for interactive mode */
    runt_word_define(vm, "quit", 4, rproc_quit);
    /* say: prints string */
    runt_word_define(vm, "say", 3, rproc_say);
    /* p: prints float*/
    runt_word_define(vm, "p", 1, rproc_print);
    /* arithmetic */
    runt_word_define(vm, "+", 1, rproc_add);
    runt_word_define(vm, "-", 1, rproc_sub);
    runt_word_define(vm, "*", 1, rproc_mul);
    runt_word_define(vm, "/", 1, rproc_div);

    /* stack operations */
    runt_word_define(vm, "dup", 3, rproc_dup);
    runt_word_define(vm, "swap", 4, rproc_swap);
    runt_word_define(vm, "drop", 4, rproc_drop);
    runt_word_define(vm, "peak", 4, rproc_peak);
    runt_word_define(vm, "rot", 3, rproc_rot);
    runt_word_define(vm, "n", 1, rproc_nitems);
    runt_word_define(vm, "rat", 3, rproc_rat);
    runt_word_define(vm, "drops", 5, rproc_drops);
    runt_word_define(vm, "over", 4, rproc_over);

    /* dynamic plugin loading */

    runt_word_define(vm, "dynload", 7, rproc_dynload);

    /* regular load */

    runt_word_define(vm, "load", 4, rproc_load);
   
    /* recording operations */

    runt_word_define(vm, "rec", 3, rproc_rec);
    runt_word_define_with_copy(vm, "stop", 4, vm->zproc, rproc_stop);

    /* [ and ] for rec, stop */
    runt_word_define(vm, "[", 1, rproc_rec);
    runt_word_define_with_copy(vm, "]", 1, vm->zproc, rproc_stop);

    /* conditionals */

    runt_word_define(vm, "<", 1, rproc_lt);
    runt_word_define(vm, ">", 1, rproc_gt);
    runt_word_define(vm, "=", 1, rproc_eq);
    runt_word_define(vm, "!=", 2, rproc_neq);

    /* control */
    runt_word_define(vm, "end", 3, rproc_end);
    runt_word_define_with_copy(vm, "call", 4, rproc_call, rproc_call_copy);
    runt_word_define_with_copy(vm, "goto", 4, rproc_goto, rproc_call_copy);
    runt_word_define_with_copy(vm, "ex", 2, rproc_ex, rproc_call_copy);
    runt_word_define(vm, "dec", 3, rproc_decr);
    runt_word_define(vm, "decn", 4, rproc_decrn);
    runt_word_define(vm, "inc", 3, rproc_incr);
    runt_word_define(vm, "incn", 4, rproc_incrn);
    runt_word_define(vm, "rep", 3, rproc_rep);

    /* variables */
    runt_word_define(vm, "set", 3, rproc_set);

    /* clear: clears pools and reloads basic library */
    runt_word_define(vm, "clear", 5, rproc_clear);

    /* print usage */
    runt_word_define(vm, "u", 1, rproc_usage);
    /* get memory usage */
    runt_word_define(vm, "m", 1, rproc_mem);
    /* get cell usage */
    runt_word_define(vm, "c", 1, rproc_cells);

    /* stack bias/unbias */
    runt_word_define(vm, "bias", 4, rproc_bias);
    runt_word_define(vm, "unbias", 6, rproc_unbias);
    
    /* list words in dictionary */
    runt_word_define(vm, "w", 1, rproc_wordlist);

    /* random number generator */
    srand(time(NULL));
    runt_word_define(vm, "rnd", 3, rproc_rand);

    /* proc size */
    runt_word_define(vm, "psize", 5, rproc_psize);
    
    /* restore returns the cell + memory pools to a given position */
    runt_word_define(vm, "restore", 7, rproc_restore);

    /* undefine a word in the dictionary */
    runt_word_define(vm, "undef", 5, rproc_undef);

    /* blocks */
    runt_word_define_with_copy(vm, "{", 1, vm->zproc, rcopy_block_begin);
    runt_word_define_with_copy(vm, "}", 1, vm->zproc, rcopy_block_end);

    return RUNT_OK;
}

