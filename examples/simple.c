#include <stdio.h>
#include <stdlib.h>
#include <runt.h>

typedef struct {
    int counter;
} user_data;

static runt_int runt_test(runt_vm *vm, runt_ptr p)
{
    runt_print(vm, "Hello Runt!\n");
    return RUNT_OK;
}

static runt_int status(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    ud = runt_to_cptr(p); runt_print(vm, "The counter is at %d\n", ud->counter);
    return RUNT_OK;
}

static runt_int tick_up(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    ud = runt_to_cptr(p);
    ud->counter++;
    return RUNT_OK;
}

static void custom_keyword_define(
    runt_vm *vm, 
    const char *str,
    runt_int size,
    runt_proc proc,
    runt_ptr p
)
{
    runt_cell *cell;
    runt_keyword_define(vm, str, size, proc, &cell);
    runt_cell_data(vm, cell, p);
}

static runt_int dtor(runt_vm *vm, runt_ptr p)
{
    user_data *ud;
    ud = runt_to_cptr(p);
    runt_print(vm, "Cleaning up. No ceremonies are necessary.\n");
    free(ud);
    return RUNT_OK;
}

static runt_int loader(runt_vm *vm)
{
    runt_load_stdlib(vm);
    runt_ptr p;
    user_data *ud;

    ud = malloc(sizeof(user_data));
    ud->counter = 0;
    p = runt_mk_cptr(vm, ud);
    runt_print(vm, "Starting simple library.\n");
    custom_keyword_define(vm, "runt_test", 9, runt_test, p);
    custom_keyword_define(vm, "status", 6, status, p);
    custom_keyword_define(vm, "tick_up", 7, tick_up, p);
    runt_add_destructor(vm, dtor, p);
    return runt_is_alive(vm);
}

int main(int argc, char *argv[])
{
    return irunt_begin(argc, argv, loader);
}
