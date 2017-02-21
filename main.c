#include <stdio.h>
#include "runt.h"

static runt_int load_runt_stuff(runt_vm *vm)
{
    runt_load_stdlib(vm);

    return RUNT_OK;
}

int main(int argc, char *argv[])
{
    return irunt_begin(argc, argv, load_runt_stuff);
}
