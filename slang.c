
#include "sgc.h"
#include "slang.h"
#include "seval.h"
#include "seval_impl.h"

typedef struct {
    const char *name;
    sobj_funcptr2_t call;
} langdef_t;

static header_obj_t _tail_token;
static obj_t *tail_token = (obj_t *)&_tail_token;

static obj_t *lang_if(obj_t **frame, obj_t **tailp);
static obj_t *lang_lambda(obj_t **frame, obj_t **tailp);
static obj_t *lang_define(obj_t **frame, obj_t **tailp);
static obj_t *lang_set(obj_t **frame, obj_t **tailp);
static obj_t *lang_begin(obj_t **frame, obj_t **tailp);
static obj_t *lang_quote(obj_t **frame, obj_t **tailp);
static obj_t *lang_lambda_syntax(obj_t **frame, obj_t **tailp);

static langdef_t specforms[] = {
    {"if", lang_if},
    {"lambda", lang_lambda},
    {"define", lang_define},
    {"set!", lang_set},
    {"begin", lang_begin},
    {"quote", lang_quote},
    {"lambda-syntax", lang_lambda_syntax},

    {NULL, NULL}
};

void
slang_open(obj_t *env)
{
    obj_t *binding;
    langdef_t *iter;
    gc_set_enabled(0);
    for (iter = specforms; iter->name; ++iter) {
        environ_bind(NULL, env, symbol_intern(NULL, iter->name),
                     specform_wrap(NULL, iter->call));
    }
    gc_set_enabled(1);
}

bool_t
slang_tailp(obj_t *val)
{
    return val == tail_token;
}

// Language defs

static obj_t *
lang_if(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *pred, *todo, *otherwise;
    *tailp = tail_token;

    pred = pair_car(expr);
    todo = pair_cadr(expr);
    otherwise = pair_cddr(expr);
    if (nullp(otherwise)) {
        otherwise = unspec_wrap();
    }
    else if (!nullp(pair_cdr(otherwise))) {
        fatal_error("if -- too many arguments", frame);
    }
    else {
        otherwise = pair_car(otherwise);
    }

    {
        // start to evaluate the predicate.
        obj_t **pred_frame = frame_extend(
                frame, 1, FR_CONTINUE_ENV | FR_SAVE_PREV);
        *frame_ref(pred_frame, 0) = pred;
        pred = eval_frame(pred_frame);
    }
    if (to_boolean(pred)) {
        return todo;
    }
    else {
        return otherwise;
    }
}

static obj_t *
lang_lambda(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    *tailp = NULL;
    return closure_wrap(frame, frame_env(frame),
                        pair_car(expr), pair_cdr(expr));
}

static obj_t *
lang_define(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *first, *name, *result;
    *tailp = NULL;
    first = pair_car(expr);
    if (symbolp(first)) {
        // Binding an expression
        // XXX: check for expr length?
        obj_t *to_eval = pair_car(pair_cdr(expr));
        // Get the value of the expression before binding.
        obj_t **expr_frame = frame_extend(
                frame, 1, FR_CONTINUE_ENV | FR_SAVE_PREV);
        *frame_ref(expr_frame, 0) = to_eval;
        result = eval_frame(expr_frame);
        name = first;
    }
    else if (pairp(first)) {
        // short hand for (define name (lambda ...))
        // x: the formals, v: the body
        obj_t *formals, *body;
        name = pair_car(first);
        formals = pair_cdr(first);
        body = pair_cdr(expr);
        result = closure_wrap(frame, frame_env(frame), formals, body);
    }
    else {
        fatal_error("define -- first argument is neither a "
                    "symbol nor a pair", frame);
    }
    environ_def(frame, frame_env(frame), name, result);
    return unspec_wrap();
}

static obj_t *
lang_set(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *first, *name, *result;
    *tailp = NULL;
    first = pair_car(expr);
    if (symbolp(first)) {
        // Binding an expression
        // XXX: check for expr length?
        obj_t *to_eval = pair_car(pair_cdr(expr));
        // Get the value of the expression before binding.
        obj_t **expr_frame = frame_extend(
                frame, 1, FR_CONTINUE_ENV | FR_SAVE_PREV);
        *frame_ref(expr_frame, 0) = to_eval;
        result = eval_frame(expr_frame);
        name = first;
    }
    else {
        fatal_error("set! -- first argument is not a symbol", frame);
    }
    environ_set(frame_env(frame), name, result);
    return unspec_wrap();
}

static obj_t *
lang_begin(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    *tailp = tail_token;
    obj_t *iter;

    for (iter = expr; pairp(iter); iter = pair_cdr(iter)) {
        // Eval each expression except the last.
        if (!pairp(pair_cdr(iter))) {
            break;
        }
        obj_t **expr_frame = frame_extend(frame, 1,
                FR_SAVE_PREV | FR_CONTINUE_ENV);
        *frame_ref(expr_frame, 0) = pair_car(iter);
        eval_frame(expr_frame);
    }
    if (nullp(iter)) {
        // Empty (begin) expression
        return unspec_wrap();
    }
    else if (!nullp(pair_cdr(iter))) {
        fatal_error("begin -- not a well-formed list", frame);
    }
    return pair_car(iter);
}

static obj_t *
lang_quote(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    *tailp = NULL;

    if (nullp(expr) || !nullp(pair_cdr(expr))) {
        fatal_error("quote -- wrong number of argument", frame);
    }
    return pair_car(expr);
}

static obj_t *
lang_lambda_syntax(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *clos;
    *tailp = NULL;

    clos = closure_wrap(frame, frame_env(frame),
                        pair_car(expr), pair_cdr(expr));
    SGC_ROOT1(frame, clos);
    return macro_wrap(frame, clos);
}


