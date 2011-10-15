#ifndef SEVAL_IMPL_H
#define SEVAL_IMPL_H

#define pair_caar(o) (pair_car(pair_car(o)))
#define pair_cadr(o) (pair_car(pair_cdr(o)))
#define pair_cddr(o) (pair_cdr(pair_cdr(o)))

#define pair_caaar(o) (pair_car(pair_caar(o)))
#define pair_caadr(o) (pair_car(pair_cadr(o)))
#define pair_caddr(o) (pair_car(pair_cddr(o)))
#define pair_cdddr(o) (pair_cdr(pair_cddr(o)))

// How many slots of meta data do we store on the frame?
// We have one slot for environment and one for saved frame pointer.
#define NB_FRAME_META_SLOTS 2

#define LIB_PROC_HEADER() \
    obj_t **prev_frame = frame_prev(frame); \
    long argc = prev_frame - frame - NB_FRAME_META_SLOTS - 1; \
    long i

#endif /* SEVAL_IMPL_H */
