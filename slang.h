#ifndef SLANG_H
#define SLANG_H

#include "sobj.h"

// Load all the special forms, will shut down gc for a while.
void slang_open(obj_t *env);

// Whether the return value should be evaluated again?
bool_t slang_tailp(obj_t *val);


#endif /* SLANG_H */
