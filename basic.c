#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "runt.h"

typedef struct {
    runt_cell *cell_if;
    runt_cell *cell_else;
} runt_if_d;

static runt_int rproc_print(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    printf("%g\n", s->f);
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
    runt_print(vm, "Recording.\n");
    return runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
}

static int rproc_stop(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_print(vm, "Stopping.\n");
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
    s->f = vm->stack.pos - 1; 
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
    rc = runt_ppop(vm, &s);
    RUNT_ERROR_CHECK(rc);
    runt_set_state(vm, RUNT_MODE_END, RUNT_OFF);
    vm->pos = s->f;
    while(runt_get_state(vm, RUNT_MODE_END) == RUNT_OFF) {
        rc = runt_cell_call(vm, &vm->cell_pool.cells[vm->pos - 1]);
        RUNT_ERROR_CHECK(rc); 
        vm->pos++;
    }
   
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

static int rproc_incr(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_peak(vm);
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
        runt_cell_id_exec(vm, id);
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

    free(line);
    return RUNT_OK;
}

static int rproc_load(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
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

    sprintf(buf, "./%s.so", str);

    if(access(buf, F_OK) != -1) {
        fname = buf;
        /* TODO: DRY */
        /* remove string from cell pool */
        vm->memory_pool.used = s->p.pos;

        runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);
        runt_load_plugin(vm, fname);
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

static runt_int rproc_clear(runt_vm *vm, runt_ptr p)
{
    runt_cell_pool_clear(vm);
    runt_memory_pool_clear(vm);
    runt_dictionary_clear(vm);
    runt_load_stdlib(vm);
    /* make memory mark so dead cells can be cleared */
    runt_mark_set(vm);
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

    /* dynamic plugin loading */

    runt_word_define(vm, "dynload", 7, rproc_dynload);

    /* regular load */

    runt_word_define(vm, "load", 4, rproc_load);
   
    /* recording operations */

    runt_word_define(vm, "rec", 3, rproc_rec);
    runt_word_define_with_copy(vm, "stop", 4, vm->zproc, rproc_stop);

    /* conditionals */

    runt_word_define(vm, "<", 1, rproc_lt);
    runt_word_define(vm, ">", 1, rproc_gt);
    runt_word_define(vm, "=", 1, rproc_eq);
    runt_word_define(vm, "!=", 2, rproc_neq);

    /* control */
    runt_word_define(vm, "end", 3, rproc_end);
    runt_word_define_with_copy(vm, "call", 4, rproc_call, rproc_call_copy);
    runt_word_define_with_copy(vm, "goto", 4, rproc_goto, rproc_call_copy);
    runt_word_define(vm, "dec", 3, rproc_decr);
    runt_word_define(vm, "inc", 3, rproc_incr);
    runt_word_define(vm, "rep", 3, rproc_rep);

    /* variables */
    runt_word_define(vm, "set", 3, rproc_set);

    /* clear: clears pools and reloads basic library */
    runt_word_define(vm, "clear", 5, rproc_clear);

    /* get usage */
    runt_word_define(vm, "u", 1, rproc_usage);

    return RUNT_OK;
}

