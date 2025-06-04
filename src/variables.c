#include <stdlib.h>
#include <string.h>
#include "variables.h"

static variable_t *variables = NULL;

void set_variable(const char *name, const char *value) {
    variable_t *current = variables;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            free(current->value);
            current->value = strdup(value);
            return;
        }
        current = current->next;
    }
    variable_t *new_var = malloc(sizeof(variable_t));
    if (!new_var) return;
    new_var->name = strdup(name);
    new_var->value = strdup(value);
    new_var->next = variables;
    variables = new_var;
}


const char *get_variable(const char *name) {
    variable_t *current = variables;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return "";
}

void delete_variable(const char *name) {
    variable_t *current = variables;
    variable_t *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (prev) prev->next = current->next;}}}