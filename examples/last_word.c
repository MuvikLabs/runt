#include <stdio.h>
#include "runt.h"

int main()
{
   runt_vm vm;
   runt_vm_alloc(&vm, 512, RUNT_MEGABYTE * 2);
   runt_load_stdlib(&vm);
   runt_set_state(&vm, RUNT_MODE_INTERACTIVE, RUNT_ON);
   runt_compile(&vm, "\"adding one plus one\" say");
   runt_print(&vm,
              "The last word called was '%s'\n",
              runt_last_word_called(&vm));
   runt_compile(&vm, "1 1 +");
   runt_print(&vm,
              "The last word called was '%s'\n",
              runt_last_word_called(&vm));
   runt_compile(&vm, "p");
   runt_print(&vm,
              "The last word called was '%s'\n",
              runt_last_word_called(&vm));
   runt_vm_free(&vm);
}
