
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
static obj_t *lib_set_car(obj_t **frame);
static obj_t *lib_set_cdr(obj_t **frame);
static obj_t *lib_list2vector(obj_t **frame);

static obj_t *lib_make_vector(obj_t **frame);
static obj_t *lib_vectorp(obj_t **frame);
static obj_t *lib_vector_length(obj_t **frame);
static obj_t *lib_vector_ref(obj_t **frame);
static obj_t *lib_vector_set(obj_t **frame);
static obj_t *lib_vector2list(obj_t **frame);

static obj_t *lib_make_dict(obj_t **frame);
static obj_t *lib_dictp(obj_t **frame);
static obj_t *lib_dict_ref(obj_t **frame);
static obj_t *lib_dict_ref_default(obj_t **frame);
static obj_t *lib_dict_set(obj_t **frame);
static obj_t *lib_dict_delete(obj_t **frame);
static obj_t *lib_dict_exists(obj_t **frame);

static obj_t *lib_hash(obj_t **frame);
static obj_t *lib_symbol2string(obj_t **frame);
static obj_t *lib_string2symbol(obj_t **frame);
static obj_t *lib_gensym(obj_t **frame);

static obj_t *lib_nullp(obj_t **frame);
static obj_t *lib_pairp(obj_t **frame);
static obj_t *lib_symbolp(obj_t **frame);
static obj_t *lib_integerp(obj_t **frame);
static obj_t *lib_booleanp(obj_t **frame);
static obj_t *lib_unspecp(obj_t **frame);
static obj_t *lib_eofobjp(obj_t **frame);

static obj_t *lib_eqp(obj_t **frame);

static obj_t *lib_rtinfo(obj_t **frame);
static obj_t *lib_dogc(obj_t **frame);
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
    {"integer?", lib_integerp},
    {"+", lib_add},
    {"-", lib_minus},
    {"<", lib_lessthan},

    // Pair
    {"cons", lib_cons},
    {"pair?", lib_pairp},
    {"car", lib_car},
    {"cdr", lib_cdr},
    {"set-car!", lib_set_car},
    {"set-cdr!", lib_set_cdr},
    {"list->vector", lib_list2vector},

    // Vector
    {"make-vector", lib_make_vector},
    {"vector?", lib_vectorp},
    {"vector-length", lib_vector_length},
    {"vector-ref", lib_vector_ref},
    {"vector-set!", lib_vector_set},
    {"vector->list", lib_vector2list},

    // Hashtable
    {"make-hash", lib_make_dict},
    {"hash-table?", lib_dictp},
    {"hash-ref", lib_dict_ref},
    {"hash-ref/default", lib_dict_ref_default},
    {"hash-set!", lib_dict_set},
    {"hash-delete!", lib_dict_delete},
    {"hash-exists?", lib_dict_exists},

    // Symbol/String
    {"hash", lib_hash},
    {"symbol?", lib_symbolp},
    {"symbol->string", lib_symbol2string},
    {"string->symbol", lib_string2symbol},
    {"gensym", lib_gensym},

    // Primitive type predicates
    {"null?", lib_nullp},
    {"boolean?", lib_booleanp},
    {"eof?", lib_eofobjp},
    {"unspecified?", lib_unspecp},

    // Equality
    {"eq?", lib_eqp},

    // Runtime Reflection
    {"gc", lib_dogc},
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
        fatal_error("(load)", frame);
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
    obj_t *arg;
    double retval = 0;
    bool_t is_int = 1;

    for (i = argc - 1; i >= 0; --i) {
        arg = *frame_ref(frame, i);
        if (!fixnump(arg))
            is_int = 0;
        retval += flonum_unwrap(arg);
    }
    return is_int ? fixnum_wrap(frame, retval) : flonum_wrap(frame, retval);
}

static obj_t *
lib_minus(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *arg;
    double retval = 0;
    bool_t is_int = 1;

    if (argc == 0) {
        fatal_error("illegal (-)", frame);
    }

    arg = *frame_ref(frame, argc - 1);
    if (!fixnump(arg))
        is_int = 0;
    retval = flonum_unwrap(arg);

    if (argc == 1) {
        if (is_int)
            return fixnum_wrap(frame, -retval);
        else
            return flonum_wrap(frame, -retval);
    }
    else {
        // Argc >= 2
        for (i = argc - 2; i >= 0; --i) {
            arg = *frame_ref(frame, i);
            if (!fixnump(arg))
                is_int = 0;
            retval -= flonum_unwrap(arg);
        }
        return is_int ? fixnum_wrap(frame, retval) :
                        flonum_wrap(frame, retval);
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
        fatal_error("< require 2 arguments", frame);
    }
}

static obj_t *
lib_cons(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 2)
        return pair_wrap(frame, *frame_ref(frame, 1), *frame_ref(frame, 0));
    else
        fatal_error("cons require 2 arguments", frame);
}

static obj_t *
lib_car(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1)
        return pair_car(*frame_ref(frame, 0));
    else
        fatal_error("car require 1 argument", frame);
}

static obj_t *
lib_cdr(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1)
        return pair_cdr(*frame_ref(frame, 0));
    else
        fatal_error("cdr require 1 argument", frame);
}

static obj_t *
lib_set_car(obj_t **frame)
{
    obj_t *pair, *new_car;
    LIB_PROC_HEADER();
    if (argc == 2) {
        pair = *frame_ref(frame, 1);
        new_car = *frame_ref(frame, 0);
        pair_set_car(pair, new_car);
        return unspec_wrap();
    }
    else {
        fatal_error("set-car! require 2 arguments", frame);
    }
}

static obj_t *
lib_set_cdr(obj_t **frame)
{
    obj_t *pair, *new_cdr;
    LIB_PROC_HEADER();
    if (argc == 2) {
        pair = *frame_ref(frame, 1);
        new_cdr = *frame_ref(frame, 0);
        pair_set_cdr(pair, new_cdr);
        return unspec_wrap();
    }
    else {
        fatal_error("set-cdr! require 2 arguments", frame);
    }
}

static obj_t *
lib_list2vector(obj_t **frame)
{
    obj_t *pair, *new_cdr;
    LIB_PROC_HEADER();
    if (argc == 1) {
        return vector_from_list(frame, *frame_ref(frame, 0));
    }
    else {
        fatal_error("list->vector require 1 argument", frame);
    }
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
        fatal_error("make-vector require 1 or 2 arguments", frame);
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
        fatal_error("vector-length require 1 argument", frame);
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
        fatal_error("vector-ref require 2 arguments", frame);
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
        fatal_error("vector-set! require 3 arguments", frame);
    }
}

static obj_t *
lib_vector2list(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *vec;
    if (argc == 1) {
        vec = *frame_ref(frame, 0);
        return vector_to_list(frame, vec);
    }
    else {
        fatal_error("vector->list require 1 argument", frame);
    }
}

static obj_t *
lib_make_dict(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 0) {
        return dict_wrap(frame);
    }
    else {
        fatal_error("make-hash require 0 argument", frame);
    }
}

static obj_t *
lib_dictp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(dictp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("hash-table? require 1 argument", frame);
    }
}

static obj_t *
lib_dict_ref(obj_t **frame)
{
    obj_t *dic, *key, *got;
    LIB_PROC_HEADER();
    if (argc == 2) {
        dic = *frame_ref(frame, 1);
        key = *frame_ref(frame, 0);
        got = dict_lookup(frame, dic, key, DL_DEFAULT);
        if (!got) {
            fatal_error("hash-ref: no such key", frame);
        }
        else {
            return pair_cdr(got);
        }
    }
    else {
        fatal_error("hash-ref require 2 arguments", frame);
    }
}

static obj_t *
lib_dict_ref_default(obj_t **frame)
{
    obj_t *dic, *key, *got;
    LIB_PROC_HEADER();
    if (argc == 3) {
        dic = *frame_ref(frame, 2);
        key = *frame_ref(frame, 1);
        got = dict_lookup(frame, dic, key, DL_DEFAULT);
        if (!got) {
            return *frame_ref(frame, 0);  // The default value
        }
        else {
            return pair_cdr(got);
        }
    }
    else {
        fatal_error("hash-ref/default require 3 arguments", frame);
    }
}

static obj_t *
lib_dict_set(obj_t **frame)
{
    obj_t *dic, *key, *val, *entry;
    LIB_PROC_HEADER();
    if (argc == 3) {
        dic = *frame_ref(frame, 2);
        key = *frame_ref(frame, 1);
        val = *frame_ref(frame, 0);
        entry = dict_lookup(frame, dic, key, DL_CREATE_ON_ABSENT);
        pair_set_cdr(entry, val);
        return unspec_wrap();
    }
    else {
        fatal_error("hash-set! require 3 arguments", frame);
    }
}

static obj_t *
lib_dict_delete(obj_t **frame)
{
    obj_t *dic, *key, *got;
    LIB_PROC_HEADER();
    if (argc == 2) {
        dic = *frame_ref(frame, 1);
        key = *frame_ref(frame, 0);
        got = dict_lookup(frame, dic, key, DL_POP_IF_FOUND);
        return unspec_wrap();
    }
    else {
        fatal_error("hash-delete! require 2 arguments", frame);
    }
}

static obj_t *
lib_dict_exists(obj_t **frame)
{
    obj_t *dic, *key, *got;
    LIB_PROC_HEADER();
    if (argc == 2) {
        dic = *frame_ref(frame, 1);
        key = *frame_ref(frame, 0);
        got = dict_lookup(frame, dic, key, DL_DEFAULT);
        return boolean_wrap(got != NULL);
    }
    else {
        fatal_error("hash-exists? require 2 arguments", frame);
    }
}

static obj_t *
lib_symbol2string(obj_t **frame)
{
    LIB_PROC_HEADER();
    const char *str;
    obj_t *sym;
    if (argc == 1) {
        sym = *frame_ref(frame, 0);
        str = symbol_unwrap(sym);
        return string_wrap(frame, str, strlen(str));
    }
    else {
        fatal_error("symbol->string require 1 argument", frame);
    }
}

static obj_t *
lib_hash(obj_t **frame)
{
    LIB_PROC_HEADER();
    long hval, moder_val;
    obj_t *ob, *moder;

    if (argc == 2) {
        ob = *frame_ref(frame, 1);
        moder = *frame_ref(frame, 0);
        moder_val = fixnum_unwrap(moder);
        hval = generic_hash(ob);
        return fixnum_wrap(frame, hval % moder_val);
    }
    else {
        fatal_error("hash require 2 arguments", frame);
    }
}

static obj_t *
lib_string2symbol(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *str;
    if (argc == 1) {
        str = *frame_ref(frame, 0);
        return symbol_intern(frame, string_unwrap(str));
    }
    else {
        fatal_error("string->symbol require 1 argument", frame);
    }
}

static size_t gsym_counter = 0;

static obj_t *
lib_gensym(obj_t **frame)
{
    LIB_PROC_HEADER();
    char buf[128];
    if (argc == 0) {
        sprintf(buf, "#!@#{%ld}<>#", gsym_counter++);
        return symbol_intern(frame, buf);
    }
    else {
        fatal_error("gensym require 0 argument", frame);
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
        fatal_error("null? require 1 argument", frame);
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
        fatal_error("pair? require 1 argument", frame);
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
        fatal_error("symbol? require 1 argument", frame);
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
        fatal_error("integer? require 1 argument", frame);
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
        fatal_error("vector? require 1 argument", frame);
    }
}

static obj_t *
lib_booleanp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(booleanp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("boolean? require 1 argument", frame);
    }
}

static obj_t *
lib_unspecp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(unspecp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("unspecified? require 1 argument", frame);
    }
}

static obj_t *
lib_eofobjp(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 1) {
        return boolean_wrap(eofobjp(*frame_ref(frame, 0)));
    }
    else {
        fatal_error("eof? require 1 argument", frame);
    }
}

static obj_t *
lib_dogc(obj_t **frame)
{
    LIB_PROC_HEADER();
    if (argc == 0) {
        gc_collect(frame);
        return unspec_wrap();
    }
    else {
        fatal_error("gc require no arguments", frame);
    }
}

static obj_t *
lib_eqp(obj_t **frame)
{
    obj_t *a, *b;
    bool_t is_eq = 0;

    LIB_PROC_HEADER();
    if (argc == 2) {
        a = *frame_ref(frame, 1);
        b = *frame_ref(frame, 0);

        if (a == b) {
            return boolean_wrap(1);
        }
        if (get_type(a) != get_type(b)) {
            return boolean_wrap(0);
        }

        switch (get_type(a)) {
        case TP_PAIR:
        case TP_SYMBOL:
        case TP_PROC:
            is_eq = 0;
            break;

        case TP_FIXNUM:
            is_eq = fixnum_unwrap(a) == fixnum_unwrap(b);
            break;

        case TP_FLONUM:
        case TP_STRING:
        case TP_CLOSURE:
            is_eq = 0;
            break;

        case TP_NIL:
            NOT_REACHED();

        case TP_VECTOR:
        case TP_BOOLEAN:
            is_eq = 0;
            break;

        case TP_UNSPECIFIED:
            NOT_REACHED();
            break;

        case TP_ENVIRON:
        case TP_UDATA:
            is_eq = 0;
            break;

        case TP_EOFOBJ:
            NOT_REACHED();
            break;
        }
        return boolean_wrap(is_eq);
    }
    else {
        fatal_error("eq? require 2 arguments", frame);
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
        fatal_error("runtime-info require no arguments", frame);
    }
}

static obj_t *
lib_eval(obj_t **frame)
{
    fatal_error("eval is never called directly", frame);
}

static obj_t *
lib_apply(obj_t **frame)
{
    fatal_error("apply is never called directly", frame);
}

static obj_t *
lib_null_environ(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *prelude_env;
    if (argc == 1) {
        if (fixnum_unwrap(*frame_ref(frame, 0)) != 5) {
            fatal_error("only r5rs environ is supported", frame);
        }
        prelude_env = frame_env(gc_get_stack_base());
        return environ_wrap(frame, prelude_env);
    }
    else {
        fatal_error("null-environment require 1 argument", frame);
    }
}

static obj_t *
lib_report_environ(obj_t **frame)
{
    LIB_PROC_HEADER();
    obj_t *prelude_env;
    if (argc == 1) {
        if (fixnum_unwrap(*frame_ref(frame, 0)) != 5) {
            fatal_error("only r5rs environ is supported", frame);
        }
        prelude_env = frame_env(gc_get_stack_base());
        return prelude_env;
    }
    else {
        fatal_error("scheme-report-environment require 1 argument", frame);
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
                fatal_error("file name contains NUL char", frame);
            }
        }
        slib_primitive_load(frame, str);
        return unspec_wrap();
    }
    else {
        fatal_error("load require 1 argument", frame);
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
        fatal_error("readline-set-prompt require 1 argument", frame);
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
                return eofobj_wrap();
            if (!*line)  // empty line
                continue;
            else
                break;
        }
        expr = sparse_do_string(line);
        if (!expr) {
            fatal_error("malformed expression", frame);
        }
        return pair_car(expr);
    }
    else {
        fatal_error("read require 0 argument", frame);
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
        fatal_error("display require 1 argument", frame);
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
        fatal_error("newline require no arguments", frame);
    }
}

