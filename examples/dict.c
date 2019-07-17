/* how to use the runt dictionary without the VM */
/* things currently do not work */

#include <stdio.h>
#include <string.h>
#include "../runt.h"

int main(int argc, char *argv[])
{
    runt_dict dict;
    runt_entry ent;
    runt_entry *e;
    const char *name;
    int rc;

    name = "foo";
    runt_dict_init(NULL, &dict);

    /* zeroing out instead of initialization */
    memset(&ent, 0, sizeof(runt_entry));

    /* manually store the string value */
    ent.p = runt_mk_ptr(RUNT_STRING, (void *)name);

    runt_word_dict(NULL, "foo", 3, &ent, &dict);

    /* shouldn't crash, but we haven't gotten here yet */
    rc = runt_word_search_dict(NULL, "foo", 3, &e, &dict);

    if(rc != RUNT_OK) {
        printf("Could not find word.\n");
    } else {
        printf("Found word '%s'.\n", runt_to_string(e->p));
    }

    return 0;
}
