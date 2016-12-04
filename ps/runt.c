#include <soundpipe.h>
#include <sporth.h>
#include <stdlib.h>
#include <string.h>


/* You'll need this header file. It can be found in the Sporth codebase */
#include "scheme-private.h"
#include "runt.h"

typedef struct {
    runt_vm *vm;
    runt_cell *cell;
} ugen_data;

plumber_data * scheme_plumber(scheme *sc);
/* macros needed for car/cdr operations */
#define car(p) ((p)->_object._cons._car)
#define cdr(p) ((p)->_object._cons._cdr)

static pointer runtspace(scheme *sc, pointer args) 
{

    plumber_data *pd = scheme_plumber(sc);
    runt_vm *vm = malloc(sizeof(runt_vm));
    runt_cell *cells;
    unsigned char *mem;
    long cell_size;
    long mem_size;
    const char *name;
    char buf[1024];

    name = string_value(car(args));
    args = cdr(args);

    cell_size = ivalue(car(args));
    args = cdr(args);

    mem_size = ivalue(car(args));
    mem_size *= RUNT_KILOBYTE;
    args = cdr(args);

    cells = malloc(sizeof(runt_cell) * cell_size);
    mem = malloc(mem_size);

    runt_init(vm);
    runt_cell_pool_set(vm, cells, (runt_uint)cell_size);
    runt_cell_pool_init(vm);
    runt_memory_pool_set(vm, mem, (runt_uint)mem_size);

    runt_load_stdlib(vm);

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);

    plumber_ftmap_add_userdata(pd, name, (void *)vm);
    sprintf(buf, "%s_mem", name);
    plumber_ftmap_add_userdata(pd, buf, (void *)mem);
    sprintf(buf, "%s_buf", name);
    plumber_ftmap_add_userdata(pd, buf, (void *)cells);

    return sc->NIL;
}

static pointer get_runt_ptr(scheme *sc, pointer args) 
{

    plumber_data *pd = scheme_plumber(sc);
    runt_vm *vm;
    const char *name;

    name = string_value(car(args));

    if(plumber_ftmap_search_userdata(pd, name, (void **)&vm) != PLUMBER_OK)
    {
        fprintf(stderr, "Could not find runtspace named %s\n", name);
        return sc->NIL;
    }

    return mk_cptr(sc, (void **)&vm);
}

static pointer ps_runt_compile(scheme *sc, pointer args) 
{
    runt_vm *vm;
    const char *code;

    vm = (runt_vm *) string_value(car(args));
    args = cdr(args);
    code = string_value(car(args));

    runt_pmark_set(vm);
    runt_compile(vm, code);
    runt_pmark_free(vm);

    return sc->NIL;

}

static pointer ps_runt_rec(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_OFF);

    return sc->NIL;
}

static pointer ps_runt_stop(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));

    runt_set_state(vm, RUNT_MODE_INTERACTIVE, RUNT_ON);

    return sc->NIL;
}

static pointer ps_runt_cpool_used(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));


    return mk_integer(sc, (long )runt_cell_pool_used(vm));
}

static pointer ps_runt_cpool_size(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));

    return mk_integer(sc, (long )runt_cell_pool_size(vm));
}

static pointer ps_runt_mpool_used(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));


    return mk_integer(sc, (long )runt_memory_pool_used(vm));
}

static pointer ps_runt_mpool_size(scheme *sc, pointer args) 
{
    runt_vm *vm;

    vm = (runt_vm *) string_value(car(args));

    return mk_integer(sc, (long )runt_memory_pool_size(vm));
}

static pointer ps_runt_float(scheme *sc, pointer args) 
{
    runt_vm *vm;
    SPFLOAT *ptr;
    const char *name;
    runt_uint size;

    vm = (runt_vm *) string_value(car(args));
    args = cdr(args);
    name = string_value(car(args));
    size = strlen(name);
    args = cdr(args);
    ptr = (SPFLOAT *) string_value(car(args));

    runt_mk_float_cell(vm, name, size, ptr);

    return sc->NIL;
}

static int run_ugen(plumber_data *pd, sporth_stack *stack, void **ud)
{
    ugen_data *ug;

    switch(pd->mode) {
        case PLUMBER_CREATE:
            ug = (ugen_data *)*ud;
            runt_cell_exec(ug->vm, ug->cell);
            break;
        case PLUMBER_INIT:
            ug = (ugen_data *)*ud;
            runt_cell_exec(ug->vm, ug->cell);
            break;
        case PLUMBER_COMPUTE:
            ug = (ugen_data *)*ud;
            runt_cell_exec(ug->vm, ug->cell);
            break;
        case PLUMBER_DESTROY:
            break;
    }

    return PLUMBER_OK;
}

static pointer ps_bind_function(scheme *sc, pointer args) 
{
    const char *func;
    const char *proc;
    runt_vm *vm;
    ugen_data *ug;
    plumber_data *pd;
    runt_entry *entry;

    pd = scheme_plumber(sc);
    ug = malloc(sizeof(ugen_data));
    vm = (runt_vm *) string_value(car(args));
    args = cdr(args);
    proc = string_value(car(args));
    args = cdr(args);
    func = string_value(car(args));

    runt_word_search(vm, proc, strlen(proc), &entry); 

    ug->vm = vm;
    ug->cell = entry->cell;

    plumber_ftmap_add_function(pd, func, run_ugen, ug);

    return sc->NIL;
}

static void sporth_define(plumber_data *pd, 
        runt_vm *vm, 
        const char *word,
        runt_uint size,
        runt_proc proc,
        runt_ptr p) 
{
    runt_uint word_id;
    word_id = runt_word_define(vm, word, size, proc);
    runt_word_bind_ptr(vm, word_id, p);
}

static int rproc_sporth_push(runt_vm *vm, runt_ptr p)
{
    plumber_data *pd;
    runt_stacklet *s1;

    pd = runt_to_cptr(p);
    s1 = runt_pop(vm);
  
    sporth_stack_push_float(&pd->sporth.stack, s1->f);

    return RUNT_OK;
}

static int rproc_sporth_pop(runt_vm *vm, runt_ptr p)
{
    plumber_data *pd;
    runt_stacklet *s1;

    pd = runt_to_cptr(p);
    s1 = runt_push(vm);
  
    s1->f = sporth_stack_pop_float(&pd->sporth.stack);

    return RUNT_OK;
}

static pointer ps_sporth_dictionary(scheme *sc, pointer args) 
{
    runt_vm *vm;
    plumber_data *pd;
    runt_ptr p;
    vm = (runt_vm *) string_value(car(args));
    pd = scheme_plumber(sc);
    p = runt_mk_cptr(vm, pd);

    sporth_define(pd, vm, "push", 4, rproc_sporth_push, p);
    sporth_define(pd, vm, "pop", 3, rproc_sporth_pop, p);
    return sc->NIL;
}

void init_runt(scheme *sc) 
{
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-alloc"), 
        mk_foreign_func(sc, runtspace));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-vm"), 
        mk_foreign_func(sc, get_runt_ptr));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-compile"), 
        mk_foreign_func(sc, ps_runt_compile));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-rec"), 
        mk_foreign_func(sc, ps_runt_rec));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-stop"), 
        mk_foreign_func(sc, ps_runt_stop));
    
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-cpool-used"), 
        mk_foreign_func(sc, ps_runt_cpool_used));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-cpool-size"), 
        mk_foreign_func(sc, ps_runt_cpool_size));
    
    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-mpool-used"), 
        mk_foreign_func(sc, ps_runt_mpool_used));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-mpool-size"), 
        mk_foreign_func(sc, ps_runt_mpool_size));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-float"), 
        mk_foreign_func(sc, ps_runt_float));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-bind"), 
        mk_foreign_func(sc, ps_bind_function));

    scheme_define(sc, sc->global_env, 
        mk_symbol(sc, "runt-sporth-dictionary"), 
        mk_foreign_func(sc, ps_sporth_dictionary));
}
