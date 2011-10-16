
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

static obj_t *lib_make_vector(obj_t **frame);
static obj_t *lib_vector_length(obj_t **frame);
static obj_t *lib_vector_ref(obj_t **frame);
static obj_t *lib_vector_set(obj_t **frame);

static obj_t *lib_nullp(obj_t **frame);
static obj_t *lib_pairp(obj_t **frame);
static obj_t *lib_symbolp(obj_t **frame);
static obj_t *lib_integerp(obj_t **frame);
static obj_t *lib_vectorp(obj_t **frame);

static obj_t *lib_rtinfo(obj_t **frame);
static obj_t *lib_eval(obj_t **frame);
static obj_t *lib_apply(obj_t **frame);
static obj_t *lib_null_environ(obj_t **frame);
static obj_t *lib_report_environ(obj_t **frame);

static obj_t *lib_display(obj_t **frame);
static obj_t *lib_newline(obj_t **frame);

static procdef_t library[] = {
    // Arith
    {"+", lib_add},
    {"-", lib_minus},
    {"<", lib_lessthan},

    // Pair
    {"cons", lib_cons},
    {"car", lib_car},
    {"cdr", lib_cdr},

    // Vector
    {"make-vector", lib_make_vector},
    {"vector-length", lib_vector_length},
    {"vector-ref", lib_vector_ref},
    {"vector-set!", lib_vector_set},

    // Type predicates
    {"null?", lib_nullp},
    {"pair?", lib_pairp},
    {"symbol?", lib_symbolp},
    {"integer?", lib_integerp},
    {"vector?", lib_vectorp},

    // Runtime Reflection
    {"runtime-info", lib_rtinfo},
    {"eval", lib_eval},
    {"apply", lib_apply},
    {"scheme-report-environment", lib_report_environ},
    {"null-environment", lib_null_environ},

    // IO
    {"display", lib_display},
    {"newline", lib_newline},

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
        environ_bind(NULL, env, symbol_intern(NULL, iter->name),
                     proc_wrap(NULL, iter->func));
    }
    gc_set_enabled(1);
}

bool_t
lib_is_eval_proc(obj_t *proc)
{
    return proc->as_proc.func == lib_eval;
}

bool_t
lib_is_apply_proc(obj_t *proc)
{
    return proc->as_proc.func == lib_apply;
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
lib_make_vector(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return vector_wrap(frame,
                           fixnum_unwrap(*frame_ref(frame, 0)),
                           unspec_wrap());
    }
    else if (argc == 2) {
        return vector_wrap(frame,
                           fixnum_unwrap(*frame_ref(frame, 1)),
                           *frame_ref(frame, 0));
    }
    else {
        fatal_error("make-vector require 1 or 2 arguments");
    }
}

static obj_t *
lib_vector_length(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return fixnum_wrap(frame, vector_length(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("vector-length require 1 argument");
    }
}

static obj_t *
lib_vector_ref(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *vec;
    long idx;
    if (argc == 2) {
        vec = *frame_ref(frame, 1);
        idx = fixnum_unwrap(*frame_ref(frame, 0));
        return *vector_ref(vec, idx);
    }
    else {
        fatal_error("vector-ref require 2 arguments");
    }
}

static obj_t *
lib_vector_set(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *vec, *val;
    long idx;
    if (argc == 3) {
        vec = *frame_ref(frame, 2);
        idx = fixnum_unwrap(*frame_ref(frame, 1));
        val = *frame_ref(frame, 0);
        *vector_ref(vec, idx) = val;
        return unspec_wrap();
    }
    else {
        fatal_error("vector-set! require 3 arguments");
    }
}

static obj_t *
lib_nullp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(nullp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("null? require 1 argument");
    }
}

static obj_t *
lib_pairp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(pairp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("pair? require 1 argument");
    }
}

static obj_t *
lib_symbolp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(symbolp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("symbol? require 1 argument");
    }
}

static obj_t *
lib_integerp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(fixnump(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("integer? require 1 argument");
    }
}

static obj_t *
lib_vectorp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(vectorp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("vector? require 1 argument");
    }
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
lib_eval(obj_t **frame)
{
    fatal_error("eval is never called directly");
}

static obj_t *
lib_apply(obj_t **frame)
{
    fatal_error("apply is never called directly");
}

static obj_t *
lib_null_environ(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *prelude_env;
    if (argc == 1) {
        if (fixnum_unwrap(*frame_ref(frame, 0)) != 5) {
            fatal_error("only r5rs environ is supported");
        }
        prelude_env = frame_env(gc_get_stack_base());
        return environ_wrap(frame, prelude_env);
    }
    else {
        fatal_error("null-environment require 1 argument");
    }
}

static obj_t *
lib_report_environ(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *prelude_env;
    if (argc == 1) {
        if (fixnum_unwrap(*frame_ref(frame, 0)) != 5) {
            fatal_error("only r5rs environ is supported");
        }
        prelude_env = frame_env(gc_get_stack_base());
        return prelude_env;
    }
    else {
        fatal_error("scheme-report-environment require 1 argument");
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

static obj_t *
lib_newline(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 0) {
        puts("");
        return unspec_wrap();
    }
    else {
        fatal_error("newline require no arguments");
    }
}

