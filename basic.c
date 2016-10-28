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

static int rproc_say(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s = runt_pop(vm);
    const char *str = runt_to_string(s->p);
    fprintf(stdout, "%s\n", str);
    return RUNT_OK;
}

static int rproc_quit(runt_vm *vm, runt_ptr p)
{
    runt_set_state(vm, RUNT_MODE_RUNNING, RUNT_OFF);
    return RUNT_OK;
}

static int rproc_add(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v1 + v2;
    return RUNT_OK;
}

static int rproc_sub(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 - v1;
    return RUNT_OK;
}

static int rproc_div(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 / v1;
    return RUNT_OK;
}

static int rproc_mul(runt_vm *vm, runt_ptr p)
{
    runt_stacklet *s1 = runt_pop(vm);
    runt_stacklet *s2 = runt_pop(vm);
    runt_stacklet *out = runt_push(vm);

    runt_float v1 = s1->f;
    runt_float v2 = s2->f;
    out->f = v2 * v1;
    return RUNT_OK;
}

runt_int rproc_dup(runt_vm *vm, runt_ptr p)
{
    /* TODO: how do we make this polymorphic? */
    runt_stacklet *val = runt_pop(vm);
    runt_stacklet *s1 = runt_push(vm);
    runt_stacklet *s2 = runt_push(vm);

    s1->f = val->f;
    s2->f = val->f;

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
    return RUNT_OK;
}
