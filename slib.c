
#include "rl.h"  // for read.
#include "sgc.h"
#include "slib.h"
#include "seval.h"
#include "seval_impl.h"

// From sparse/scm_*
extern obj_t *sparse_do_string(const char *);
extern obj_t *sparse_do_file(FILE *);

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

static obj_t *lib_load(obj_t **frame);

static obj_t *lib_rl_setprompt(obj_t **frame);
static obj_t *lib_read(obj_t **frame);
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

    // Extension
    {"load", lib_load},

    // IO
    {"readline-set-prompt", lib_rl_setprompt},
    {"read", lib_read},
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

static
void execute_expr_list(obj_t **frame, obj_t *prog)
{
    obj_t *expr;
    obj_t **prog_frame, **run_frame;

    prog_frame = frame_extend(frame, 1,
                              FR_SAVE_PREV | FR_CONTINUE_ENV);

    for (expr = prog; !nullp(expr); expr = pair_cdr(expr)) {
        *frame_ref(prog_frame, 0) = expr;
        run_frame = frame_extend(prog_frame, 1,
                                 FR_SAVE_PREV | FR_CONTINUE_ENV);
        *frame_ref(run_frame, 0) = pair_car(expr);
        eval_frame(run_frame);
    }
}

void
slib_primitive_load(obj_t **frame, const char *file_name)
{
    FILE *fp;
    obj_t *prog;

    fp = fopen(file_name, "r");
    if (!fp) {
        perror(file_name);
        fatal_error("(load)");
    }
    prog = sparse_do_file(fp);
    fclose(fp);

    execute_expr_list(frame, prog);
}

void
slib_primitive_load_string(obj_t **frame, const char *expr_str)
{
    execute_expr_list(frame, sparse_do_string(expr_str));
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
lib_load(obj_t **frame)
{
    // Search path?
    obj_t *file_name;
    size_t len;
    const char *str;
    FILE *fp;

    LIB_PROC_HEADER();
    if (argc == 1) {
        file_name = *frame_ref(frame, 0);
        str = string_unwrap(file_name);
        // Check if there is null chars
        for (i = 0, len = string_length(file_name); i < len; ++i) {
            if (str[i] == '\0') {
                fatal_error("file name contains NUL char");
            }
        }
        slib_primitive_load(frame, str);
        return unspec_wrap();
    }
    else {
        fatal_error("load require 1 argument");
    }
}

// Used in read and readline-set-prompt
static char *lib_rl_prompt = NULL;

static obj_t *
lib_rl_setprompt(obj_t **frame)
{
    obj_t *prompt;
    const char *buf;
    LIB_PROC_HEADER();
    if (argc == 1) {
        if (lib_rl_prompt) {
            free(lib_rl_prompt);
            lib_rl_prompt = NULL;
        }
        prompt = *frame_ref(frame, 0);
        buf = string_unwrap(prompt);
        i = string_length(prompt);
        lib_rl_prompt = malloc(i + 1);
        memcpy(lib_rl_prompt, buf, i + 1);
        return unspec_wrap();
    }
    else {
        fatal_error("readline-set-prompt require 1 argument");
    }
}

static obj_t *
lib_read(obj_t **frame)
{
    char *line;
    obj_t *expr;
    LIB_PROC_HEADER();
    if (argc == 0) {
        while (1) {
            // environment Hook
            line = rl_getstr(lib_rl_prompt ? lib_rl_prompt : "");
            if (!line)  // EOF
                return symbol_intern(frame, "#EOF");
            if (!*line)  // empty line
                continue;
            else
                break;
        }
        expr = sparse_do_string(line);
        if (!expr) {
            fatal_error("malformed expression");
        }
        return pair_car(expr);
    }
    else {
        fatal_error("read require 0 argument");
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

