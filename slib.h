#ifndef SLIB_H
#define SLIB_H

#include "sobj.h"

// will shut off gc for a while.
void slib_open(obj_t *env);
bool_t lib_is_eval_proc(obj_t *proc);
bool_t lib_is_apply_proc(obj_t *proc);

void slib_primitive_load(obj_t **frame, const char *file_name);
void slib_primitive_load_string(obj_t **frame, const char *expr_str);

#endif /* SLIB_H */
