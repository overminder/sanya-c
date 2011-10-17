
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
static obj_t *lang_quasiquote(obj_t **frame, obj_t **tailp);  // Not nested

// LOL... anyway, it's usable
static obj_t *lang_lambda_syntax(obj_t **frame, obj_t **tailp);

static langdef_t specforms[] = {
    {"if", lang_if},
    {"lambda", lang_lambda},
    {"define", lang_define},
    {"set!", lang_set},
    {"begin", lang_begin},
    {"quote", lang_quote},
    {"quasiquote", lang_quasiquote},
    {"lambda-syntax", lang_lambda_syntax},

    {NULL, NULL}
};

static obj_t *symbol_unquote = NULL;
static obj_t *symbol_unquote_splicing = NULL;

void
slang_open(obj_t *env)
{
    static bool_t inited = 0;
    if (inited)
        return;
    else
        inited = 1;

    obj_t *binding;
    langdef_t *iter;
    gc_set_enabled(0);
    for (iter = specforms; iter->name; ++iter) {
        environ_bind(NULL, env, symbol_intern(NULL, iter->name),
                     specform_wrap(NULL, iter->call));
    }
    symbol_unquote = symbol_intern(NULL, "unquote");
    symbol_unquote_splicing = symbol_intern(NULL, "unquote-splicing");
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

enum quasiquote_return_flag {
    QQ_DEFAULT = 0,
    QQ_UNQUOTE = 1,
    QQ_SPLICING = 2
};

static obj_t *
expand_quasiquote(obj_t **frame, obj_t *content,
                  enum quasiquote_return_flag *flag)
{
    if (!pairp(content)) {
        return content;
    }

    // Manually compare each item with unquote/unquote-splicing
    obj_t *uq = symbol_unquote;
    obj_t *spl = symbol_unquote_splicing;
    if (pair_car(content) == uq) {
        obj_t *uq_body = pair_cadr(content);
        frame = frame_extend(frame, 1, FR_SAVE_PREV | FR_CONTINUE_ENV);
        *frame_ref(frame, 0) = uq_body;
        if (flag)
            *flag = QQ_UNQUOTE;
        return eval_frame(frame);
    }
    else if (pair_car(content) == spl) {
        obj_t *spl_body = pair_cadr(content);
        obj_t *retval;
        frame = frame_extend(frame, 1, FR_SAVE_PREV | FR_CONTINUE_ENV);
        *frame_ref(frame, 0) = spl_body;
        retval = eval_frame(frame);
        if (flag)
            *flag = QQ_SPLICING;
        return retval;
    }
    else {
        // Copy the pair content.
        content = pair_copy_list(frame, content);
        // Append a dummy header for unquote-splicing to use.
        content = pair_wrap(frame, nil_wrap(), content);

        // Mark the content.
        frame = frame_extend(frame, 1, FR_SAVE_PREV | FR_CONTINUE_ENV);
        *frame_ref(frame, 0) = content;

        // For linking unquote-splicing, we look at the next item of
        // the iterator. That's why we need a dummy header here.
        obj_t *iter, *next, *got;
        enum quasiquote_return_flag ret_flag;

        for (iter = content; pairp(iter); iter = pair_cdr(iter)) {
            // `next` will always be null or pair, since `content` is a list.
loop_begin:
            next = pair_cdr(iter);
            if (nullp(next))  // we are done.
                break;

            // XXX: this is strange. why do we need to initialize it?
            ret_flag = QQ_DEFAULT;
            got = expand_quasiquote(frame, pair_car(next), &ret_flag);
            if (ret_flag & QQ_SPLICING) {
                // Special handling for unquote-splicing
                // WARNING: messy code below!
                got = pair_copy_list(frame, got);

                if (nullp(got)) {
                    pair_set_cdr(iter, pair_cdr(next));
                }
                else {
                    pair_set_cdr(iter, got);  // iter -> got
                    while (pairp(pair_cdr(got))) {
                        got = pair_cdr(got);
                    }
                    pair_set_cdr(got, pair_cdr(next));  // got -> (next->next)
                    iter = got;  // make sure the next iteration is correct
                    goto loop_begin;  // And this...
                }
            }
            else {
                // Not unquote-splicing, easy...
                pair_set_car(next, got);
            }
        }
        if (flag)
            *flag = QQ_DEFAULT;
        return pair_cdr(content);
    }
}

static obj_t *
lang_quasiquote(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *content;
    *tailp = NULL;
    if (nullp(expr) || !nullp(pair_cdr(expr))) {
        fatal_error("quasiquote -- wrong number of argument", frame);
    }

    // Expand...
    content = pair_car(expr);
    return expand_quasiquote(frame, content, NULL);
}

static obj_t *
lang_lambda_syntax(obj_t **frame, obj_t **tailp)
{
    obj_t *expr = *frame_ref(frame, 0);
    obj_t *clos;
    *tailp = NULL;

    // LOL!!!
    clos = closure_wrap(frame, frame_env(frame),
                        pair_car(expr), pair_cdr(expr));
    SGC_ROOT1(frame, clos);
    return macro_wrap(frame, clos);
}


