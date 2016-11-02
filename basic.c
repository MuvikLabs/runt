#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

static int rproc_drop(runt_vm *vm, runt_ptr p)
{
    runt_pop(vm);
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
    runt_stacklet *s = runt_pop(vm);
    runt_set_state(vm, RUNT_MODE_END, RUNT_OFF);
    vm->pos = s->f;
    while(runt_get_state(vm, RUNT_MODE_END) == RUNT_OFF) {
        runt_cell_call(vm, &vm->cell_pool.cells[vm->pos - 1]);
        vm->pos++;
    }
    
    return RUNT_OK;
}

static int rproc_goto(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s;
    runt_uint pos;
    runt_uint jump;

    s = runt_pop(vm);
    pos = s->f;

    s = runt_pop(vm);
    jump = s->f;

    if(jump && pos < vm->cell_pool.size + 1) {
        vm->pos = pos;
    }
    
    return RUNT_OK;
}

static int rproc_decr(runt_vm *vm, runt_ptr p)
{
    /*TODO: replace with peak */
    runt_stacklet *s = runt_pop(vm);
    runt_float f = s->f;
    s = runt_push(vm);
    s->f = f - 1;
    return RUNT_OK;
}

static int rproc_incr(runt_vm *vm, runt_ptr p)
{
    /*TODO: replace with peak */
    runt_stacklet *s = runt_pop(vm);
    runt_float f = s->f;
    s = runt_push(vm);
    s->f = f + 1;
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

    /* dynamic plugin loading */

    runt_word_define(vm, "dynload", 7, rproc_dynload);
   
    /* recording operations */

    runt_word_define(vm, "rec", 3, rproc_rec);
    runt_word_define_with_copy(vm, "stop", 4, vm->zproc, rproc_stop);

    /* conditionals */

    runt_word_define(vm, "<", 1, rproc_lt);
    runt_word_define(vm, ">", 1, rproc_gt);
    runt_word_define(vm, "=", 1, rproc_eq);
    runt_word_define(vm, "!=", 2, rproc_neq);

    /* things related to goto */
    runt_word_define(vm, "end", 3, rproc_end);
    runt_word_define_with_copy(vm, "call", 4, rproc_call, rproc_call_copy);
    runt_word_define_with_copy(vm, "goto", 4, rproc_goto, rproc_call_copy);
    runt_word_define(vm, "dec", 3, rproc_decr);
    runt_word_define(vm, "inc", 3, rproc_incr);

    /* variables */
    runt_word_define(vm, "set", 3, rproc_set);

    return RUNT_OK;
}
