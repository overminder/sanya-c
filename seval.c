
#include "sgc.h"
#include "seval.h"
#include "seval_impl.h"
#include "slib.h"
#include "slang.h"

extern void sparse_init();

static obj_t *symbol_begin;  // For lambda transform...

static obj_t *eval_symbol(obj_t **frame);
static obj_t *apply_procedure(obj_t **frame);

// Intern some symbols for later use.
void
seval_init()
{
    static bool_t initialized = 0;
    if (initialized)
        return;
    else
        initialized = 1;

    // Populate the symbol table and constants
    sobj_init();
    // Enable the parser
    sparse_init();

    obj_t **frame = gc_get_stack_base();

    symbol_begin = symbol_intern(frame, "begin");

    // The very first frame.
    frame = frame_extend(frame, 0, FR_CLEAR_SLOTS);
    frame_set_env(frame, environ_wrap(frame, nil_wrap()));
    gc_set_stack_base(frame);

    // Adding some library functions.
    slib_open(frame_env(frame));

    // And language constructs
    slang_open(frame_env(frame));
}

// Frame layout:
// *(fp)   *(fp + 1)       *(fp + 2)  ...  *(fp + size + 1)
//  env    pref-frame-ptr   arg0      arg1    argn
//
// Calling convention: caller extend frame, caller push its unextended env,
// caller may choose to keep this env (when doing define/set! things),
// @see eval_frame() for evaluation implementation
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

    if (flags & FR_EXTEND_ENV) {
        // Extend and save the previous environment pointer on the new frame.
        frame_set_env(new_frame, environ_wrap(new_frame, old_env));
    }
    else if (flags & FR_CONTINUE_ENV) {
        // Or just using the previous environment.
        frame_set_env(new_frame, old_env);
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


// main entrance for eval.
// For tail's purpose, eval-related things are all placed here.
// @see frame_extend() for frame layout information and calling convention.
obj_t *
eval_frame(obj_t **frame)
{
    obj_t *self;
    obj_t *car, *cdr, *w, *x, *v;
    // When in primitive-apply or macro expension, this will be false.
    // Otherwise, it will be set to true on normal routine.
    bool_t args_need_eval;

tailcall:
    self = *frame_ref(frame, 0);
    switch (get_type(self)) {

    case TP_PAIR:
        car = pair_car(self);
        cdr = pair_cdr(self);
        args_need_eval = 1;

        if (symbolp(car)) {
            // Handling special form / macros
            obj_t *binding = environ_lookup(frame_env(frame),
                    car, EL_LOOK_OUTER);
            if (binding) {
                obj_t *syntax = pair_cdr(binding);
                obj_t *tailp;
                obj_t *retval;
                if (specformp(syntax)) {
                    obj_t **ex_frame = frame_extend(frame, 1,
                            FR_SAVE_PREV | FR_CONTINUE_ENV);
                    *frame_ref(ex_frame, 0) = cdr;

                    // Call the special form.
                    retval = specform_unwrap(syntax)(ex_frame, &tailp);
                    if (slang_tailp(tailp)) {
                        *frame_ref(frame, 0) = retval;
                        goto tailcall;
                    }
                    else {
                        return retval;
                    }
                }
                else if (macrop(syntax)) {
                    *frame_ref(frame, 0) = macro_expand(frame, syntax, cdr);
                    goto tailcall;
                }
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
            {
                // Get the procedure/closure
                obj_t **proc_frame = frame_extend(frame, 1,
                        FR_SAVE_PREV | FR_CONTINUE_ENV);
                *frame_ref(proc_frame, 0) = car;
                proc = eval_frame(proc_frame);

                // Check early if it's not a callable
                if (!procedurep(proc) && !closurep(proc)) {
                    fatal_error("not a callable", frame);
                }

                // Push the callable (whatever it is)
                // on the frame, to avoid GC.
                --frame;
                *frame = proc;
// XXX
// XXX: WARNING -- Frame is UNUSABLE during application preparation
// XXX
            }

            // Get the argument list.
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
                fatal_error("not a well-formed list", frame);
            }

            // -- 2: move each item into evaluatin position and eval.
            // Note that we are evaluating in reversed order (right to left)
            // If we come here from the special apply proc,
            // args_need_eval will be false since they are already evaluated.
            if (args_need_eval) {
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
                // Since the library function may require the current environ
                // e.g., (load "filename.scm"), we must attach the environ.
                frame = frame_extend(frame, 0, FR_CLEAR_SLOTS);
                frame_set_env(frame, orig_env);
                frame_set_prev(frame, orig_frame);

                if (procedurep(proc)) {
                    // Is c-procedure -- just evaluate it.
                    // Note that we will treat apply/eval differently.
                    if (lib_is_eval_proc(proc)) {
                        // Special case for (eval expr env)
                        if (argc != 2) {
                            fatal_error("eval should have 2 argument",
                                        frame);
                        }
                        obj_t *expr = *frame_ref(frame, 1);
                        obj_t *environ = *frame_ref(frame, 0);
                        if (!environp(environ)) {
                            fatal_error("first argument of eval should "
                                        "be an environment", frame);
                        }
                        frame = frame_extend(orig_frame, 1,
                                FR_CLEAR_SLOTS | FR_SAVE_PREV);
                        // Must create a new frame with the environ
                        // since we should go back to the origin environ
                        // later, which could only done by creating a
                        // new frame to hold the new environ.
                        *frame_ref(frame, 0) = expr;
                        frame_set_env(frame, environ);
                        goto tailcall;
                    }
                    else if (lib_is_apply_proc(proc)) {
                        // Special case for (apply proc args)
                        if (argc != 2) {
                            fatal_error("apply should have 2 arguments",
                                        frame);
                        }
                        proc = *frame_ref(frame, 1);
                        cdr = *frame_ref(frame, 0);
                        frame = orig_frame;
                        --frame;
                        *frame = proc;
                        args_need_eval = 0;  // args are already evaluated
                        goto apply_reentry;
                    }
                    else {
                        // Ordinary procedure application
                        return apply_procedure(frame);
                    }
                }
                else {
                    // Is scm-closure: *HARD PART COMES*
                    // Remember the frame layout:
                    //   [env, dumped-fp, argn, ..., arg0, callable]
                    // 1: prepare for an extended env
                    obj_t *env, *formals, *body;
                    env = environ_wrap(frame, closure_env(proc));
                    frame_set_env(frame, env);  // Prevent from gc

                    // 2: push bindings into it -- pos args only for now.
                    // XXX: arg length check is not done.
                    formals = closure_formals(proc);
                    for (i = argc - 1; i >= 0; --i) {
                        if (symbolp(formals)) {
                            // pack things into vararg, note it's reversed
                            obj_t *vararg = nil_wrap();
                            long rev_i;
                            for (rev_i = 0; rev_i <= i; ++rev_i) {
                                obj_t **gc_frame = frame;
                                SGC_ROOT1(gc_frame, vararg);
                                vararg = pair_wrap(gc_frame,
                                                   *frame_ref(frame, rev_i),
                                                   vararg);
                            }
                            environ_bind(frame, env, formals, vararg);
                            formals = nil_wrap();  // for later to check
                            break;
                        }
                        else if (pairp(formals)) {
                            environ_bind(frame, env,
                                         pair_car(formals),
                                         *frame_ref(frame, i));
                            formals = pair_cdr(formals);
                        }
                        else {
                            fatal_error("too many args in closure application",
                                        frame);
                        }
                    }
                    if (symbolp(formals)) {
                        // Vararg with no args -- make it a empty nil.
                        environ_bind(frame, env, formals, nil_wrap());
                    }
                    else if (!nullp(formals)) {
                        // Still a pair, which means that we dont have
                        // enough arguments...
                        fatal_error("not enough args in closure application",
                                    frame);
                    }
                    // 3: make it a begin.
                    body = pair_wrap(frame, symbol_begin, closure_body(proc));

                    // 4: unwind the frame since we have finished the binding
                    // and start to evaluating the begin.
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
        {
            obj_t *got = eval_symbol(frame);
            if (syntaxp(got)) {
                fatal_error("bad syntax", frame);
            }
            else {
                return got;
            }
        }

    case TP_PROC:
    case TP_FIXNUM:
    case TP_FLONUM:
    case TP_STRING:
    case TP_CLOSURE:
        return self;

    case TP_NIL:
        fatal_error("empty application", frame);
        break;

    case TP_VECTOR:
    case TP_BOOLEAN:
    case TP_UNSPECIFIED:
    case TP_UDATA:
    case TP_EOFOBJ:
        return self;
    }
    NOT_REACHED();
}

static obj_t *
eval_symbol(obj_t **frame)
{
    obj_t *key = *frame_ref(frame, 0);
    obj_t *binding = environ_lookup(frame_env(frame), key, EL_LOOK_OUTER);
    if (binding)
        return pair_cdr(binding);
    else
        fatal_error("unbound variable", frame);
}

static obj_t *
apply_procedure(obj_t **frame)
{
    obj_t *proc;
    obj_t **prev_frame = frame_prev(frame);
    proc = prev_frame[-1];
    return proc_unwrap(proc)(frame);
}

