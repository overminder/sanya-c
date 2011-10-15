#ifndef SEVAL_H
#define SEVAL_H

#include "sobj.h"

void seval_init();

obj_t **new_frame(obj_t **old_frame, size_t size);
obj_t *eval_frame(obj_t **frame);

#endif /* SEVAL_H */
