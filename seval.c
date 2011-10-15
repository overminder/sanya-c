
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
             *symbol_set;

static bool_t special_form_p(obj_t *self);
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

    // The very first frame.
    NEW_FRAME(frame, 0);
    FRAME_ENV() = extend_env(frame, nil_wrap());
    FRAME_DUMP() = NULL;
    gc_set_stack_base(frame);

    // Adding some library functions.
    slib_open(FRAME_ENV());
}

// Frame layout:
// *(fp)    *(fp + 1) *(fp + 2)  ...  *(fp + size + 2)
// prev-fp   environ   arg0      arg1    argn
//
// Which is to say, if there is no size, *fp will be previous frame's prev-fp
obj_t **
new_frame(obj_t **old_frame, size_t size)
{
    size_t i;
    obj_t **new_frame;
    obj_t *prev_env;
    {
        USING_FRAME(old_frame);
        prev_env = FRAME_ENV();
    }

    new_frame = old_frame - (NB_FRAME_META_SLOTS + size);
    USING_FRAME(new_frame);

    // the environ
    FRAME_ENV() = extend_env(new_frame, prev_env);
    // the previous frame pointer, to be handled specifically in gc...
    // @see sgc:gc_collect()
    FRAME_DUMP() = (obj_t *)old_frame;

    return new_frame;
}

// For tail's purpose, eval-related things are placed here.
obj_t *
eval_frame(obj_t **frame)
{
    obj_t *self;
    obj_t *car, *cdr, *w, *x, *v;
    USING_FRAME(frame);

tailcall:
    self = FRAME_SLOT(0);
    switch (get_type(self)) {

    case TP_PAIR:
        car = pair_car(self);
        cdr = pair_cdr(self);
        if (special_form_p(car)) {
            if (car == symbol_if) {
                fatal_error("if");
            }
            else if (car == symbol_lambda) {
                fatal_error("lambda");
            }
            else if (car == symbol_define) {
                w = pair_car(cdr);
                if (get_type(w) != TP_SYMBOL) {
                    fatal_error("define -- key must be symbol");
                }
                x = pair_car(pair_cdr(cdr));
                {
                    NEW_FRAME(frame, 1);
                    FRAME_SLOT(0) = x;
                    x = eval_frame(frame);
                    DESTROY_FRAME(frame, 1);
                }
                def_binding(frame, w, x);
                return unspec_wrap();
            }
            else if (car == symbol_set) {
                w = pair_car(cdr);
                if (get_type(w) != TP_SYMBOL) {
                    fatal_error("set! -- key must be symbol");
                }
                x = pair_car(pair_cdr(cdr));
                {
                    NEW_FRAME(frame, 1);
                    FRAME_SLOT(0) = x;
                    x = eval_frame(frame);
                    DESTROY_FRAME(frame, 1);
                }
                set_binding(frame, w, x);
                return unspec_wrap();
            }
        }
        else {
            // Is normal procedure OR closure application.
            // 1. Get the callable
            // 2. Unpack and evaluate each arg and prepare for application.
            // The prepared frame will be like this:
            // [env, dumped-fp, argn, ..., arg0, callable]
            obj_t *proc;
            obj_t **orig_frame = frame;
            obj_t *iter;
            obj_t *retval;
            long argc;
            {
                // Get the procedure/closure
                NEW_FRAME(frame, 1);
                FRAME_SLOT(0) = car;
                proc = eval_frame(frame);
                DESTROY_FRAME(frame, 1);

                // Check early if it's not a callable
                if (!procedurep(proc) && !closurep(proc)) {
                    fatal_error("not a callable");
                }

                // Push the callable (whatever it is)
                // on the frame, to avoid GC.
                --frame;
                *frame = proc;
            }
            // Two pass -- 1: get argc and clean the lower frame slots
            argc = 0;
            for (iter = cdr; pairp(iter); iter = pair_cdr(iter)) {
                ++argc;
                --frame;
                *frame = pair_car(iter);
            }
            if (!nullp(iter)) {
                fatal_error("not a well-formed list");
            }

            long i;
            // -- 2: unpack and eval.
            for (i = 0; i < argc; ++i) {
                // Evaluate this argument on a new frame
                NEW_FRAME(frame, 1);
                FRAME_ENV() = orig_frame[0];
                FRAME_SLOT(0) = FRAME_SLOT(i + 1);  // i + 1: the (n - i)th arg
                retval = eval_frame(frame);

                // Push the result back to the argument position.
                FRAME_SLOT(i + 1) = retval;

                // Unwind this temporary frame
                DESTROY_FRAME(frame, 1);
            }

            // Creating new frame and setting up environment.
            // Then do the actual application (two cases)
            // If it's
            {
                frame -= 2;
                USING_FRAME(frame);
                FRAME_DUMP() = (obj_t *)orig_frame;
                if (procedurep(proc)) {
                    // Is c-procedure: push an empty env (or just no env?)
                    FRAME_ENV() = extend_env(frame, nil_wrap());
                    return apply_procedure(frame);
                }
                else {
                    // Is scm-closure: push an extended env
                    FRAME_ENV() = extend_env(frame, closure_env(proc));
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

static bool_t
special_form_p(obj_t *self)
{
    return (self == symbol_if || self == symbol_lambda ||
            self == symbol_define || self == symbol_set);
}

static obj_t *
extend_env(obj_t **frame, obj_t *outer_env)
{
    return pair_wrap(frame, nil_wrap(), outer_env);
}

static obj_t *
def_binding(obj_t **frame, obj_t *key, obj_t *value)
{
    USING_FRAME(frame);

    obj_t *binding = lookup_binding(frame, key, DONT_LOOK_OUTER);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        frame[-1] = key;
        frame[-2] = value;
        binding = pair_wrap(frame - 2, key, value);

        frame[-1] = binding;
        pair_set_car(FRAME_ENV(), pair_wrap(frame - 1,
                                            binding,
                                            pair_car(FRAME_ENV())));
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
    {
        USING_FRAME(frame);
        env = FRAME_ENV();
    }

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
    USING_FRAME(frame);
    obj_t *key = FRAME_SLOT(0);
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
    USING_FRAME(frame);
    obj_t **prev_frame = (obj_t **)FRAME_DUMP();
    proc = prev_frame[-1];
    return proc_unwrap(proc)(frame);
}

