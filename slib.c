
#include "sgc.h"
#include "slib.h"
#include "seval.h"
#include "seval_impl.h"

typedef struct {
    const char *name;
    sobj_funcptr_t func;
} procdef_t;

// Lib declarations
static obj_t *lib_add(obj_t **frame);
static obj_t *lib_minus(obj_t **frame);
static obj_t *lib_lessthan(obj_t **frame);

static obj_t *lib_cons(obj_t **frame);
static obj_t *lib_car(obj_t **frame);
static obj_t *lib_cdr(obj_t **frame);

static obj_t *lib_rtinfo(obj_t **frame);

static obj_t *lib_display(obj_t **frame);

static procdef_t library[] = {
    // Arith
    {"+", lib_add},
    {"-", lib_minus},
    {"<", lib_lessthan},

    // Pair
    {"cons", lib_cons},
    {"car", lib_car},
    {"cdr", lib_cdr},

    // Runtime Reflection
    {"runtime-info", lib_rtinfo},

    // IO
    {"display", lib_display},

    // Sentinel
    {NULL, NULL}
};

void
slib_open(obj_t *env)
{
    obj_t *binding;
    procdef_t *iter;
    gc_set_enabled(0);
    for (iter = library; iter->name; ++iter) {
        binding = pair_wrap(NULL, symbol_intern(NULL, iter->name),
                            proc_wrap(NULL, iter->func));
        pair_set_car(env, pair_wrap(NULL, binding, pair_car(env)));
    }
    gc_set_enabled(1);
}

// Definations

static obj_t *
lib_add(obj_t **frame)
{
    LIB_PROC_HEADER();
    long retval = 0;

    for (i = argc - 1; i >= 0; --i) {
        retval += fixnum_unwrap(*frame_ref(frame, i));
    }
    return fixnum_wrap(frame, retval);
}

static obj_t *
lib_minus(obj_t **frame)
{
    LIB_PROC_HEADER();
    long retval = 0;

    if (argc == 0) {
        fatal_error("illegal (-)");
    }
    else if (argc == 1) {
        return fixnum_wrap(frame, -fixnum_unwrap(*frame_ref(frame, 0)));
    }
    else {
        retval = fixnum_unwrap(*frame_ref(frame, argc - 1));
        for (i = argc - 2; i >= 0; --i) {
            retval -= fixnum_unwrap(*frame_ref(frame, i));
        }
        return fixnum_wrap(frame, retval);
    }
}

static obj_t *
lib_lessthan(obj_t **frame)
{
    LIB_PROC_HEADER();

    if (argc == 2) {
        return boolean_wrap(fixnum_unwrap(*frame_ref(frame, 1)) <
                            fixnum_unwrap(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("< require 2 arguments");
    }
}

static obj_t *
lib_cons(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 2)
        return pair_wrap(frame, *frame_ref(frame, 1), *frame_ref(frame, 0));
    else
        fatal_error("cons require 2 arguments");
}

static obj_t *
lib_car(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1)
        return pair_car(*frame_ref(frame, 0));
    else
        fatal_error("car require 1 argument");
}

static obj_t *
lib_cdr(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1)
        return pair_cdr(*frame_ref(frame, 0));
    else
        fatal_error("cdr require 1 argument");
}

static obj_t *
lib_rtinfo(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 0) {
        gc_print_stack(frame);
        gc_print_heap();
        return unspec_wrap();
    }
    else {
        fatal_error("runtime-info require no arguments");
    }
}

static obj_t *
lib_display(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        print_repr(*frame_ref(frame, 0), stdout);
        return unspec_wrap();
    }
    else {
        fatal_error("display require 1 argument");
    }
}
