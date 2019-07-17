#include <stdio.h>
#include "../runt.h"

void dict_add_value(runt_dict *d, const char *str, int val)
{
    /* TODO: implement me! */
}

void dict_get_value(runt_dict *d, const char *str, int *val)
{
    /* TODO: implement me! */
}

int main(int argc, char *argv[])
{
    runt_dict dict;
    int val;
    runt_dict_init(NULL, &dict);
    dict_add_value(&dict, "foo", 12345);
    val = 0;
    dict_get_value(&dict, "foo", &val);
    printf("the retrieved foo value is %d\n", val);
    return 0;
}
