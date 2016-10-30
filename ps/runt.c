#include <soundpipe.h>
#include <sporth.h>
#include <stdlib.h>


/* You'll need this header file. It can be found in the Sporth codebase */
#include "scheme-private.h"
#include "runt.h"

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

static pointer ps_runt_ptr(scheme *sc, pointer args) 
{
    runt_vm *vm;
    void *ptr;

    vm = (runt_vm *) string_value(car(args));
    args = cdr(args);
    ptr = (runt_vm *) string_value(car(args));
    runt_mk_cptr_cell(vm, ptr);
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
        mk_symbol(sc, "runt-ptr"), 
        mk_foreign_func(sc, ps_runt_ptr));
}
