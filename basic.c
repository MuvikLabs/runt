#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "runt.h"

static runt_int rproc_print(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    printf("%g\n", s->f);
    return RUNT_OK;
}

static runt_int rproc_say(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    const char *str = runt_to_string(s->p);
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
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v1 + v2;
    return RUNT_OK;
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
    runt_stacklet *val = runt_pop(vm);
    runt_stacklet *s1 = runt_push(vm);
    runt_stacklet *s2 = runt_push(vm);

    s1->f = val->f;
    s2->f = val->f;

    return RUNT_OK;
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

    /* mark it so it doesn't get overwritten in memory 
     * NOTE: this will cause the filepath to stay in memory
     * this will be fixed in the future for sure... */

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
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    s1->f = s2->f;
    s2->f = s1->f;
    return RUNT_OK;
}

static int rproc_if_copy(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    runt_cell *cell;
    runt_cell_new(vm, &cell);
    runt_proc_begin(vm, cell);
    dst->p = runt_mk_ptr(RUNT_CELL, cell);
    dst->fun = src->fun;
    return RUNT_OK;
}

static int rproc_if(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    runt_float val = s->f;
    runt_cell *cell = runt_to_cell(p);

    if(val != 0) runt_cell_exec(vm, cell);

    return RUNT_OK;
}

static int rproc_endif_copy(runt_vm *vm, runt_cell *src, runt_cell *dst)
{
    /* TODO: this is duplicate code from rproc_end */
    runt_stacklet *s = runt_peak(vm);
    runt_cell *proc = runt_to_cell(s->p);
    runt_cell_undo(vm);
    proc->psize--;
    return runt_proc_end(vm);
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

    /* dynamic plugin loading */

    runt_word_define(vm, "dynload", 7, rproc_dynload);
   
    /* recording operations */
    runt_word_define(vm, "rec", 3, rproc_rec);
    runt_word_define_with_copy(vm, "stop", 4, vm->zproc, rproc_stop);

    /* conditionals and comparisons */
    runt_word_define_with_copy(vm, "if", 2, rproc_if, rproc_if_copy);
    runt_word_define_with_copy(vm, "endif", 5, vm->zproc, rproc_endif_copy);
    runt_word_define(vm, "<", 1, rproc_lt);
    runt_word_define(vm, ">", 1, rproc_gt);
    runt_word_define(vm, "=", 1, rproc_eq);
    runt_word_define(vm, "!=", 2, rproc_neq);

    return RUNT_OK;
}
