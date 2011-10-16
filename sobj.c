
#include <stdlib.h>
#include <string.h>
#include "sgc.h"
#include "sobj.h"

//#define ALWAYS_COLLECT

static header_obj_t w_nil;
static header_obj_t w_true;
static header_obj_t w_false;
static header_obj_t w_unspec;
static obj_t *symbol_table = NULL;

static obj_t *default_gc_visitor(obj_t *self);
static void default_gc_finalizer(obj_t *self);
static obj_t *pair_gc_visitor(obj_t *self);
static obj_t *closure_gc_visitor(obj_t *self);

void
sobj_init()
{
    static bool_t initialized = 0;
    if (initialized)
        return;

    initialized = 1;

    w_nil.ob_type = TP_NIL;
    w_true.ob_type = TP_BOOLEAN;
    w_false.ob_type = TP_BOOLEAN;
    w_unspec.ob_type = TP_UNSPECIFIED;

    gc_register_type(TP_PAIR, pair_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_SYMBOL, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_PROC, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_FIXNUM, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_FLONUM, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_CLOSURE, closure_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_NIL, default_gc_visitor, default_gc_finalizer);
    //gc_register_type(TP_VECTOR, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_BOOLEAN, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_UNSPECIFIED, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_ENVIRON, pair_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_UDATA, default_gc_visitor, default_gc_finalizer);

    // Symbol table
    sgc_init();
    symbol_table = pair_wrap(gc_get_stack_base(), nil_wrap(), nil_wrap());
    gc_incr_stack_base(-1);
    *gc_get_stack_base() = symbol_table;
}

type_t
get_type(obj_t *self)
{
    return self->ob_type;
}

const char *
get_typename(obj_t *self)
{
    switch (get_type(self)) {
    case TP_PAIR: return "pair";
    case TP_SYMBOL: return "symbol";
    case TP_PROC: return "procedure";
    case TP_FIXNUM: return "fixnum";
    case TP_FLONUM: return "flonum";
    case TP_CLOSURE: return "closure";
    case TP_NIL: return "nil";
    case TP_VECTOR: return "vector";
    case TP_BOOLEAN: return "boolean";
    case TP_UNSPECIFIED: return "unspecified";
    case TP_UDATA: return "udata";
    }
    NOT_REACHED();
}

bool_t to_boolean(obj_t *self)
{
    if (self == (obj_t *)&w_nil || self == (obj_t *)&w_false)
        return 0;
    else
        return 1;
}

void
print_repr(obj_t *self, FILE *stream)
{
    obj_t *w;
    switch (get_type(self)) {

    case TP_PAIR:
        fprintf(stream, "(");
        print_repr(pair_car(self), stream);
        self = pair_cdr(self);

        while (get_type(self) == TP_PAIR) {
            fprintf(stream, " ");
            print_repr(pair_car(self), stream);
            self = pair_cdr(self);
        }
        if (get_type(self) != TP_NIL) {
            fprintf(stream, " . ");
            print_repr(self, stream);
        }
        fprintf(stream, ")");
        break;

    case TP_SYMBOL:
        fprintf(stream, "%s", symbol_unwrap(self));
        break;

    case TP_PROC:
        fprintf(stream, "#<procedure at %p>", proc_unwrap(self));
        break;

    case TP_FIXNUM:
        fprintf(stream, "%ld", fixnum_unwrap(self));
        break;

    case TP_FLONUM:
        fprintf(stream, "%g", flonum_unwrap(self));
        break;

    case TP_CLOSURE:
        fprintf(stream, "#<closure env=%p", closure_env(self));
        fprintf(stream, " formals=");
        print_repr(closure_formals(self), stream);
        fprintf(stream, " body=%p", closure_body(self));
        fprintf(stream, ">");
        break;

    case TP_NIL:
        fprintf(stream, "()");
        break;

    case TP_VECTOR:
        fprintf(stream, "#()");
        break;

    case TP_BOOLEAN:
        fprintf(stream, to_boolean(self) ? "#t" : "#f");
        break;

    case TP_UNSPECIFIED:
        fprintf(stream, "#<unspecified>");
        break;

    case TP_ENVIRON:
        fprintf(stream, "#<environ at %p", self);
        if (!nullp(self->as_environ.cdr)) {
            fprintf(stream, " outer=%p", self->as_environ.cdr);
        }
        fprintf(stream, ">");
        break;

    case TP_UDATA:
        fprintf(stream, "#<user-data>");
        break;

    default:
        NOT_REACHED();
    }
}

bool_t
nullp(obj_t *self)
{
    return self == nil_wrap();
}

bool_t
pairp(obj_t *self)
{
    return get_type(self) == TP_PAIR;
}

bool_t
symbolp(obj_t *self)
{
    return get_type(self) == TP_SYMBOL;
}

bool_t
fixnump(obj_t *self)
{
    return get_type(self) == TP_FIXNUM;
}

bool_t
procedurep(obj_t *self)
{
    return get_type(self) == TP_PROC;
}

bool_t
closurep(obj_t *self)
{
    return get_type(self) == TP_CLOSURE;
}

void
fatal_error(const char *msg)
{
    fprintf(stderr, "FATAL -- %s\n", msg);
    exit(1);
}

// Object implementations

obj_t *
nil_wrap()
{
    return (obj_t *)&w_nil;
}

obj_t *
boolean_wrap(bool_t bval)
{
    return (obj_t *)(bval ? &w_true : &w_false);
}

obj_t *
unspec_wrap()
{
    return (obj_t *)&w_unspec;
}


// Fixnum
obj_t *
fixnum_wrap(obj_t **frame_ptr, long ival)
{
    obj_t *self = gc_malloc(sizeof(fixnum_obj_t), TP_FIXNUM);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame_ptr);
        self = gc_malloc(sizeof(fixnum_obj_t), TP_FIXNUM);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_fixnum.val = ival;
    return self;
}

long
fixnum_unwrap(obj_t *self)
{
    if (fixnump(self))
        return self->as_fixnum.val;
    else
        fatal_error("not a fixnum");
}

// Flonum
obj_t *
flonum_wrap(obj_t **frame_ptr, double dval)
{
    obj_t *self = gc_malloc(sizeof(flonum_obj_t), TP_FLONUM);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame_ptr);
        self = gc_malloc(sizeof(flonum_obj_t), TP_FLONUM);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_flonum.val = dval;
    return self;
}

double
flonum_unwrap(obj_t *self)
{
    return self->as_flonum.val;
}

// Pair
obj_t *
pair_wrap(obj_t **frame_ptr, obj_t *car, obj_t *cdr)
{
    obj_t *self = gc_malloc(sizeof(pair_obj_t), TP_PAIR);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        if (gc_is_enabled()) {
            SGC_ROOT2(frame_ptr, car, cdr);
            gc_collect(frame_ptr);
        }
        self = gc_malloc(sizeof(pair_obj_t), TP_PAIR);
#ifndef ALWAYS_COLLECT
    }
#endif
    pair_set_car(self, car);
    pair_set_cdr(self, cdr);
    return self;
}

obj_t *
pair_car(obj_t *self)
{
    if (pairp(self))
        return self->as_pair.car;
    else
        fatal_error("not a pair");
}

obj_t *
pair_cdr(obj_t *self)
{
    if (pairp(self))
        return self->as_pair.cdr;
    else
        fatal_error("not a pair");
}

void
pair_set_car(obj_t *self, obj_t *car)
{
    if (pairp(self))
        self->as_pair.car = car;
    else
        fatal_error("not a pair");
}

void
pair_set_cdr(obj_t *self, obj_t *cdr)
{
    if (pairp(self))
        self->as_pair.cdr = cdr;
    else
        fatal_error("not a pair");
}

// Symbol
static obj_t *
symbol_wrap(obj_t **frame_ptr, const char *sval)
{
    size_t slen = strlen(sval);
    obj_t *self = gc_malloc(sizeof(symbol_obj_t) + slen, TP_SYMBOL);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame_ptr);
        self = gc_malloc(sizeof(symbol_obj_t) + slen, TP_SYMBOL);
#ifndef ALWAYS_COLLECT
    }
#endif
    memcpy(self->as_symbol.val, sval, slen + 1);
    self->as_symbol.hash = -1;
    return self;
}

const char *
symbol_unwrap(obj_t *self)
{
    if (symbolp(self))
        return self->as_symbol.val;
    else
        fatal_error("not a symbol");
}

static long
string_hash(const char *sval, size_t length)
{
    long res = length;
    size_t iter;
    for (iter = 0; iter < length; ++iter) {
        res ^= sval[iter] << (iter % 56);
    }
    if (res == -1)
        res = -2;
    return res;
}

long
symbol_hash(obj_t *self)
{
    const char *sval;
    long hash = self->as_symbol.hash;
    if (hash == -1) {
        sval = symbol_unwrap(self);
        hash = string_hash(sval, strlen(sval));
        self->as_symbol.hash = hash;
    }
    return hash;
}

obj_t *
symbol_intern(obj_t **frame_ptr, const char *sval)
{
    long hash;
    obj_t *item, *symb, *symb_tab_ptr = pair_car(symbol_table);

    hash = string_hash(sval, strlen(sval));
    for (item = symb_tab_ptr; item != nil_wrap(); item = pair_cdr(item)) {
        symb = pair_car(item);
        if (symbol_hash(symb) == hash &&
                strcmp(symbol_unwrap(symb), sval) == 0) {
            return symb;
        }
    }
    symb = symbol_wrap(frame_ptr, sval);
    symb_tab_ptr = pair_wrap(frame_ptr, symb, symb_tab_ptr);
    pair_set_car(symbol_table, symb_tab_ptr);
    return symb;
}

bool_t
symbol_eq(obj_t *self, obj_t *other)
{
    if (self == other)
        return 1;
    else
        return 0;
}

// Proc
obj_t *
proc_wrap(obj_t **frame_ptr, sobj_funcptr_t func)
{
    obj_t *self = gc_malloc(sizeof(proc_obj_t), TP_PROC);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame_ptr);
        self = gc_malloc(sizeof(proc_obj_t), TP_PROC);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_proc.func = func;
    return self;
}

sobj_funcptr_t
proc_unwrap(obj_t *self)
{
    if (procedurep(self))
        return self->as_proc.func;
    else
        fatal_error("not a procedure");
}

obj_t *
closure_wrap(obj_t **frame_ptr, obj_t *env, obj_t *formals, obj_t *body)
{
    obj_t *self = gc_malloc(sizeof(closure_obj_t), TP_CLOSURE);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        if (gc_is_enabled()) {
            SGC_ROOT3(frame_ptr, env, formals, body);
            gc_collect(frame_ptr);
        }
        self = gc_malloc(sizeof(closure_obj_t), TP_CLOSURE);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_closure.env = env;
    self->as_closure.formals = formals;
    self->as_closure.body = body;
    return self;
}

obj_t *
closure_env(obj_t *self)
{
    if (closurep(self))
        return self->as_closure.env;
    else
        fatal_error("not a closure");
}

obj_t *
closure_formals(obj_t *self)
{
    if (closurep(self))
        return self->as_closure.formals;
    else
        fatal_error("not a closure");
}

obj_t *
closure_body(obj_t *self)
{
    if (closurep(self))
        return self->as_closure.body;
    else
        fatal_error("not a closure");
}

// Accessor macros for environ
#define ENV_CAR(self) (self->as_environ.car)
#define ENV_CDR(self) (self->as_environ.cdr)

obj_t *
environ_wrap(obj_t **frame_ptr, obj_t *outer)
{
    obj_t *self = gc_malloc(sizeof(environ_obj_t), TP_ENVIRON);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        if (gc_is_enabled()) {
            SGC_ROOT1(frame_ptr, outer);
            gc_collect(frame_ptr);
        }
        self = gc_malloc(sizeof(environ_obj_t), TP_ENVIRON);
#ifndef ALWAYS_COLLECT
    }
#endif
    ENV_CAR(self) = nil_wrap();
    ENV_CDR(self) = outer;
    return self;
}

obj_t *
environ_set(obj_t *self, obj_t *key, obj_t *value)
{
    obj_t *binding = environ_lookup(self, key, EL_LOOK_OUTER);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        fatal_error("set! -- No such binding");
    }
    return binding;
}

obj_t *
environ_lookup(obj_t *self, obj_t *key, enum environ_lookup_flag flag)
{
    obj_t *lis;  // One particular cons list in the current scanning env self
    obj_t *binding;

    while (!nullp(self)) {
        for (lis = ENV_CAR(self); !nullp(lis); lis = pair_cdr(lis)) {
            binding = pair_car(lis);
            if (symbol_eq(pair_car(binding), key)) {
                return binding;
            }
        }
        if (flag == EL_LOOK_OUTER) {
            self = ENV_CDR(self);
        }
        else {
            break;
        }
    }
    return NULL;
}

obj_t *
environ_def(obj_t **frame, obj_t *self, obj_t *key, obj_t *value)
{
    obj_t *binding = environ_lookup(self, key, EL_DONT_LOOK_OUTER);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        binding = environ_bind(frame, self, key, value);
    }
    return binding;
}

obj_t *
environ_bind(obj_t **frame, obj_t *self, obj_t *key, obj_t *value)
{
    obj_t *binding;

    if (gc_is_enabled()) {
        frame[-1] = key;
        frame[-2] = value;
    }
    binding = pair_wrap(frame - 2, key, value);

    if (gc_is_enabled()) {
        frame[-1] = binding;
    }
    ENV_CAR(self) = pair_wrap(frame - 1, binding, ENV_CAR(self));
    return binding;
}

// Static utilities

static obj_t *
default_gc_visitor(obj_t *self)
{
    return NULL;
}

static void
default_gc_finalizer(obj_t *self)
{
    free(self);
}

static obj_t *
pair_gc_visitor(obj_t *self)
{
    gc_mark(pair_car(self));
    return pair_cdr(self);
}

static obj_t *
closure_gc_visitor(obj_t *self)
{
    gc_mark(closure_env(self));
    gc_mark(closure_formals(self));
    return closure_body(self);
}

