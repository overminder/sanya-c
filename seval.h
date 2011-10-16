#ifndef SEVAL_H
#define SEVAL_H

#include "sobj.h"

enum frame_extend_flag {
    FR_DEFAULT = 0,
    FR_CLEAR_SLOTS = 1,
    FR_SAVE_PREV = 2,
    FR_EXTEND_ENV = 4,
    FR_CONTINUE_ENV = 8
};

void seval_init();

obj_t **frame_extend(obj_t **old_frame, size_t size,
                     enum frame_extend_flag flags);
obj_t **frame_prev(obj_t **frame);
void frame_set_prev(obj_t **frame, obj_t **prev_frame);
obj_t *frame_env(obj_t **frame);
void frame_set_env(obj_t **frame, obj_t *new_env);
obj_t **frame_ref(obj_t **frame, long index);

obj_t *eval_frame(obj_t **frame);

#endif /* SEVAL_H */
