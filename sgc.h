#ifndef SGC_H
#define SGC_H

#include <inttypes.h>

#include "sobj.h"

#define SGC_ROOT1(frame_ptr, o) \
    do { \
        --frame_ptr; \
        *frame_ptr = o; \
    } while (0)
#define SGC_ROOT2(frame_ptr, o1, o2) \
    do { \
        SGC_ROOT1(frame_ptr, o1); \
        SGC_ROOT1(frame_ptr, o2); \
    } while (0)
#define SGC_ROOT3(frame_ptr, o1, o2, o3) \
    do { \
        SGC_ROOT2(frame_ptr, o1, o2); \
        SGC_ROOT1(frame_ptr, o3); \
    } while (0)

typedef obj_t * (*gc_visitor_t) (obj_t *);
typedef void (*gc_finalizer_t) (obj_t *);

void sgc_init();

void gc_set_enabled(bool_t);
bool_t gc_is_enabled();

obj_t *gc_malloc(size_t size, type_t ob_type);
void gc_register_type(type_t tp_index, gc_visitor_t v, gc_finalizer_t fini);
size_t gc_mark(obj_t *self);
size_t gc_collect(obj_t **frame_ptr);
bool_t gc_want_collect();

obj_t **gc_get_stack_base();
void gc_set_stack_base(obj_t **new_sp);
void gc_incr_stack_base(long nb_incr);

void gc_print_heap();
void gc_print_stack(obj_t **frame);

#endif /* SGC_H */
