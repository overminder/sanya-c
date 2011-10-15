
#define NB_FRAME_META_SLOTS 2

#define USING_FRAME(frame) \
    obj_t **_ob_stack = frame + NB_FRAME_META_SLOTS

#define NEW_FRAME(frame, size) \
    frame = new_frame(frame, size); \
    USING_FRAME(frame)

#define DESTROY_FRAME(frame, size) \
    frame += size + NB_FRAME_META_SLOTS;

#define FRAME_SLOT(n)       (_ob_stack[n])
#define FRAME_ENV()         (_ob_stack[-2])
#define FRAME_DUMP()        (_ob_stack[-1])

#define PROC_ARGC() \
    (((obj_t **)FRAME_DUMP() - 1) - &FRAME_SLOT(0))

#define LIB_PROC_HEADER() \
    USING_FRAME(frame); \
    size_t argc = PROC_ARGC(); \
    long i;

