#ifndef SLIB_H
#define SLIB_H

#include "sobj.h"

// will shut off gc for a while.
void slib_open(obj_t *env);
bool_t lib_is_eval_proc(obj_t *proc);
bool_t lib_is_apply_proc(obj_t *proc);

#endif /* SLIB_H */
