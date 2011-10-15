
#include "sgc.h"
#include "seval.h"
#include "seval_impl.h"
#include "slib.h"

enum lookup_flag {
    DONT_LOOK_OUTER,
    LOOK_OUTER
};

static obj_t *symbol_if,
             *symbol_lambda,
             *symbol_define,
             *symbol_set,
             *symbol_begin,
             *symbol_quote;

static obj_t *extend_env(obj_t **frame, obj_t *outer_env);
static obj_t *def_binding(obj_t **frame, obj_t *key, obj_t *value);
static obj_t *set_binding(obj_t **frame, obj_t *key, obj_t *value);
static obj_t *lookup_binding(obj_t **frame, obj_t *key, enum lookup_flag flag);
static obj_t *eval_symbol(obj_t **frame);
static obj_t *apply_procedure(obj_t **frame);

// Currently the environment is implemented as a list.
// Environment will point to its parent on the previous frame.

void
seval_init()
{
    static bool_t initialized = 0;
    if (initialized)
        return;
    else
        initialized = 1;

    sobj_init();

    obj_t **frame = gc_get_stack_base();

    symbol_if = symbol_intern(frame, "if");
    symbol_lambda = symbol_intern(frame, "lambda");
    symbol_define = symbol_intern(frame, "define");
    symbol_set = symbol_intern(frame, "set!");
    symbol_begin = symbol_intern(frame, "begin");
    symbol_quote = symbol_intern(frame, "quote");

    // The very first frame.
    frame = frame_extend(frame, 0, FR_CLEAR_SLOTS);
    frame_set_env(frame, extend_env(frame, nil_wrap()));
    gc_set_stack_base(frame);

    // Adding some library functions.
    slib_open(frame_env(frame));
}

// Frame layout:
// *(fp)    *(fp + 1) *(fp + 2)  ...  *(fp + size + 2)
// prev-fp   environ   arg0      arg1    argn
//
// Which is to say, if there is no size, *fp will be previous frame's prev-fp

obj_t **frame_extend(obj_t **old_frame, size_t size,
                     enum frame_extend_flag flags)
{
    long i;
    size_t real_size;
    obj_t *old_env;
    obj_t **new_frame;

    old_env = frame_env(old_frame);

    real_size = size + NB_FRAME_META_SLOTS;
    new_frame = old_frame - real_size;

    // Initialize clear every slots in the new frame to be NULL
    // Including the environment slot and the previous frame ptr slot.
    // Note that if we are extending the env, the slots will also be
    // cleared since otherwise we may end up marking garbage-collected
    // heap objects (which is extremely bad!)
    if (flags & FR_CLEAR_SLOTS || flags & FR_EXTEND_ENV) {
        for (i = 0; i < real_size; ++i) {
            new_frame[i] = NULL;
        }
    }

    // Save previous frame pointer on the new frame.
    if (flags & FR_SAVE_PREV) {
        frame_set_prev(new_frame, old_frame);
    }

    // Extend and save the previous environment pointer on the new frame.
    if (flags & FR_EXTEND_ENV) {
        frame_set_env(new_frame, extend_env(new_frame, old_env));
    }

    return new_frame;
}

obj_t **
frame_prev(obj_t **frame)
{
    return (obj_t **)frame[1];
}

void
frame_set_prev(obj_t **frame, obj_t **prev_frame)
{
    frame[1] = (obj_t *)prev_frame;
}

obj_t *
frame_env(obj_t **frame)
{
    return frame[0];
}

void
frame_set_env(obj_t **frame, obj_t *new_env)
{
    frame[0] = new_env;
}

obj_t **
frame_ref(obj_t **frame, long index)
{
    return (frame + index + NB_FRAME_META_SLOTS);
}


// For tail's purpose, eval-related things are placed here.
obj_t *
eval_frame(obj_t **frame)
{
    obj_t *self;
    obj_t *car, *cdr, *w, *x, *v;

tailcall:
    self = *frame_ref(frame, 0);
    switch (get_type(self)) {

    case TP_PAIR:
        car = pair_car(self);
        cdr = pair_cdr(self);
        if (symbolp(car)) {
            // Handling special forms
            if (car == symbol_if) {
                obj_t *pred, *todo, *otherwise;
                pred = pair_car(cdr);
                todo = pair_cadr(cdr);
                otherwise = pair_cddr(cdr);
                if (nullp(otherwise)) {
                    otherwise = unspec_wrap();
                }
                else if (!nullp(pair_cdr(otherwise))) {
                    fatal_error("if -- too many arguments");
                }
                else {
                    otherwise = pair_car(otherwise);
                }

                {
                    // start to evaluate the predicate.
                    obj_t **pred_frame = frame_extend(
                            frame, 1, FR_EXTEND_ENV | FR_SAVE_PREV);
                    *frame_ref(pred_frame, 0) = pred;
                    pred = eval_frame(pred_frame);
                }
                if (to_boolean(pred)) {
                    *frame_ref(frame, 0) = todo;
                }
                else {
                    *frame_ref(frame, 0) = otherwise;
                }
                goto tailcall;
            }
            else if (car == symbol_lambda) {
                return closure_wrap(frame, frame_env(frame),
                                    pair_car(cdr), pair_cdr(cdr));
            }
            else if (car == symbol_define) {
                w = pair_car(cdr);
                if (get_type(w) != TP_SYMBOL) {
                    fatal_error("define -- key must be symbol");
                }
                x = pair_car(pair_cdr(cdr));
                {
                    // Get the value of the expression before binding.
                    obj_t **expr_frame = frame_extend(
                            frame, 1, FR_EXTEND_ENV | FR_SAVE_PREV);
                    *frame_ref(expr_frame, 0) = x;
                    x = eval_frame(expr_frame);
                }
                def_binding(frame_prev(frame), w, x);
                return unspec_wrap();
            }
            else if (car == symbol_set) {
                w = pair_car(cdr);
                if (get_type(w) != TP_SYMBOL) {
                    fatal_error("set! -- key must be symbol");
                }
                x = pair_car(pair_cdr(cdr));
                {
                    // Get the value of the expression before binding.
                    obj_t **expr_frame = frame_extend(
                            frame, 1, FR_EXTEND_ENV | FR_SAVE_PREV);
                    *frame_ref(expr_frame, 0) = x;
                    x = eval_frame(expr_frame);
                }
                set_binding(frame_prev(frame), w, x);
                return unspec_wrap();
            }
            else if (car == symbol_begin) {
                obj_t *iter;
                for (iter = cdr; pairp(iter); iter = pair_cdr(iter)) {
                    // Eval each expression except the last.
                    if (!pairp(pair_cdr(iter))) {
                        break;
                    }
                    obj_t **expr_frame = frame_extend(frame, 1,
                            FR_SAVE_PREV | FR_EXTEND_ENV);
                    *frame_ref(expr_frame, 0) = pair_car(iter);
                    eval_frame(expr_frame);
                }
                if (nullp(iter)) {
                    // Empty (begin) expression
                    return unspec_wrap();
                }
                else if (!nullp(pair_cdr(iter))) {
                    fatal_error("begin -- not a well-formed list");
                }
                *frame_ref(frame, 0) = pair_car(iter);
                goto tailcall;
            }
            else if (car == symbol_quote) {
                if (nullp(cdr) || !nullp(pair_cdr(cdr))) {
                    fatal_error("quote -- wrong number of argument");
                }
                return pair_car(cdr);
            }
        }

        // Is normal procedure OR closure application.
        // 1. Get the callable
        // 2. Unpack and evaluate each arg and prepare for application.
        // The prepared frame will be like this:
        // [env, dumped-fp, argn, ..., arg0, callable]
        {
            obj_t *proc;
            obj_t *iter;
            obj_t *retval;
            obj_t **orig_frame = frame;
            obj_t *orig_env = frame_env(frame);
            long argc;
            long i;
            bool_t needs_eval = 1;
            {
                // Get the procedure/closure
                obj_t **proc_frame = frame_extend(frame, 1,
                        FR_SAVE_PREV | FR_EXTEND_ENV);
                *frame_ref(proc_frame, 0) = car;
                proc = eval_frame(proc_frame);

                // Check early if it's not a callable
                if (!procedurep(proc) && !closurep(proc)) {
                    fatal_error("not a callable");
                }

                // Push the callable (whatever it is)
                // on the frame, to avoid GC.
                --frame;
                *frame = proc;
// XXX
// XXX: WARNING -- Frame is UNUSABLE DURING APPLICATION PREPARATION
// XXX
            }

            // Two pass -- 1: get argc and fill the frame slots with
            // list items waiting for evaluation.
apply_reentry:
            argc = 0;
            for (iter = cdr; pairp(iter); iter = pair_cdr(iter)) {
                ++argc;
                --frame;
                *frame = pair_car(iter);
            }
            if (!nullp(iter)) {
                fatal_error("not a well-formed list");
            }

            // -- 2: move each item into evaluatin position and eval.
            // Note that we are evaluating in reversed order (right to left)
            if (needs_eval) {
                for (i = 0; i < argc; ++i) {
                    // Evaluate this argument on a new frame
                    obj_t **arg_frame = frame_extend(frame, 1, FR_CLEAR_SLOTS);
                    // Since the frame is unusable, we need to manually
                    // set its environment and prev-frame-ptr
                    frame_set_prev(arg_frame, orig_frame);
                    frame_set_env(arg_frame, orig_env);

                    *frame_ref(arg_frame, 0) = frame[i];
                    retval = eval_frame(arg_frame);

                    // Push the result back to the argument position.
                    frame[i] = retval;
                }
            }

            // Creating new frame and setting up environment.
            // Then do the actual application (two cases)
            {
                frame = frame_extend(frame, 0, FR_CLEAR_SLOTS);
                frame_set_prev(frame, orig_frame);

                if (procedurep(proc)) {
                    // Is c-procedure -- just evaluate it.
                    // Note that we will treat apply/eval differently.
                    if (lib_is_eval_proc(proc)) {
                        if (argc != 1) {
                            fatal_error("eval should have 1 argument");
                        }
                        obj_t *expr = *frame_ref(frame, 0);
                        frame = orig_frame;
                        *frame_ref(frame, 0) = expr;
                        goto tailcall;
                    }
                    else if (lib_is_apply_proc(proc)) {
                        if (argc != 2) {
                            fatal_error("apply should have 2 arguments");
                        }
                        proc = *frame_ref(frame, 1);
                        cdr = *frame_ref(frame, 0);
                        frame = orig_frame;
                        --frame;
                        *frame = proc;
                        needs_eval = 0;
                        goto apply_reentry;
                    }
                    return apply_procedure(frame);
                }
                else {
                    // Is scm-closure: (HARD PART COMES)
                    // Remember the frame layout:
                    //   [env, dumped-fp, argn, ..., arg0, callable]
                    // 1: prepare for an extended env
                    obj_t *env, *formals, *body;
                    env = extend_env(frame, closure_env(proc));
                    frame_set_env(frame, env);  // Prevent from gc

                    // 2: push bindings into it -- pos args only now.
                    // XXX: arg length check.
                    formals = closure_formals(proc);
                    for (i = argc - 1; i >= 0; --i) {
                        obj_t *binding = pair_wrap(frame,
                                                   pair_car(formals), 
                                                   *frame_ref(frame, i));
                        pair_set_car(env,
                                     pair_wrap(frame,
                                               binding,
                                               pair_car(env)));
                        formals = pair_cdr(formals);
                    }
                    // 3: make it a begin.
                    body = pair_wrap(frame, symbol_begin, closure_body(proc));

                    // 4: unwind the frame until the very beginning.
                    {
                        frame = orig_frame;
                        *frame_ref(frame, 0) = body;
                        frame_set_env(frame, env);
                        goto tailcall;
                    }
                }
            }
        }

    case TP_SYMBOL:
        return eval_symbol(frame);

    case TP_PROC:
    case TP_FIXNUM:
    case TP_FLONUM:
    case TP_CLOSURE:
        return self;

    case TP_NIL:
        fatal_error("empty application");
        break;

    case TP_VECTOR:
    case TP_BOOLEAN:
    case TP_UNSPECIFIED:
    case TP_UDATA:
        return self;
    }
    NOT_REACHED();
}

static obj_t *
extend_env(obj_t **frame, obj_t *outer_env)
{
    return pair_wrap(frame, nil_wrap(), outer_env);
}

static obj_t *
def_binding(obj_t **frame, obj_t *key, obj_t *value)
{
    obj_t *binding = lookup_binding(frame, key, DONT_LOOK_OUTER);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        frame[-1] = key;
        frame[-2] = value;
        binding = pair_wrap(frame - 2, key, value);

        frame[-1] = binding;
        pair_set_car(frame_env(frame), pair_wrap(frame - 1,
                                                 binding,
                                                 pair_car(frame_env(frame))));
    }
    return binding;
}

static obj_t *
set_binding(obj_t **frame, obj_t *key, obj_t *value)
{
    obj_t *binding = lookup_binding(frame, key, LOOK_OUTER);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        fatal_error("set! -- No such binding");
    }
    return binding;
}

static obj_t *
lookup_binding(obj_t **frame, obj_t *key, enum lookup_flag flag)
{
    obj_t *lis;
    obj_t *env;
    obj_t *binding;

    env = frame_env(frame);

    while (!nullp(env)) {
        for (lis = pair_car(env); !nullp(lis); lis = pair_cdr(lis)) {
            binding = pair_car(lis);
            if (symbol_eq(pair_car(binding), key)) {
                return binding;
            }
        }
        if (flag == LOOK_OUTER) {
            env = pair_cdr(env);
        }
        else {
            break;
        }
    }
    return NULL;
}

static obj_t *
eval_symbol(obj_t **frame)
{
    obj_t *key = *frame_ref(frame, 0);
    obj_t *binding = lookup_binding(frame, key, LOOK_OUTER);
    if (binding)
        return pair_cdr(binding);
    else
        fatal_error("unbound variable");
}

static obj_t *
apply_procedure(obj_t **frame)
{
    obj_t *proc;
    obj_t **prev_frame = frame_prev(frame);
    proc = prev_frame[-1];
    return proc_unwrap(proc)(frame);
}

