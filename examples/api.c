#include <stdio.h>
#include "runt.h"

int main()
{
   runt_vm vm;
   runt_vm_alloc(&vm, 512, RUNT_MEGABYTE * 2);
   runt_load_stdlib(&vm);
   runt_set_state(&vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
   runt_compile(&vm, "\"hello runt api!\" say");
   runt_vm_free(&vm);
}
