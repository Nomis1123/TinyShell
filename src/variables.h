#ifndef __VARIABLES_H__
#define __VARIABLES_H__

#include <stddef.h>

typedef struct variable {
    char *name;
    char *value;
    struct variable *next;
} variable_t;

void set_variable(const char *name, const char *value);