
#include <stdlib.h>
#include <string.h>
#include "sgc.h"
#include "sobj.h"

#define ALWAYS_COLLECT

static header_obj_t w_nil;
static header_obj_t w_true;
static header_obj_t w_false;
static header_obj_t w_unspec;
static obj_t *symbol_table = NULL;

static obj_t *default_gc_visitor(obj_t *self);
static void default_gc_finalizer(obj_t *self);
static obj_t *pair_gc_visitor(obj_t *self);

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
    gc_register_type(TP_CLOSURE, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_NIL, default_gc_visitor, default_gc_finalizer);
    //gc_register_type(TP_VECTOR, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_BOOLEAN, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_UNSPECIFIED, default_gc_visitor, default_gc_finalizer);
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
        fprintf(stream, "#<closure at %p>", NULL);
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

    case TP_UDATA:
        fprintf(stream, "#<user-data>");
        break;
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
    return self->as_fixnum.val;
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
        if (gc_is_enabled())
            SGC_ROOT2(frame_ptr, car, cdr);
        gc_collect(frame_ptr);
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
    return self->as_pair.car;
}

obj_t *
pair_cdr(obj_t *self)
{
    return self->as_pair.cdr;
}

void
pair_set_car(obj_t *self, obj_t *car)
{
    self->as_pair.car = car;
}

void
pair_set_cdr(obj_t *self, obj_t *cdr)
{
    self->as_pair.cdr = cdr;
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
    return self->as_symbol.val;
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
    return self->as_proc.func;
}

obj_t *
closure_wrap(obj_t **frame_ptr, obj_t *env, obj_t *formals, obj_t *body)
{
    obj_t *self = gc_malloc(sizeof(closure_obj_t), TP_CLOSURE);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        if (gc_is_enabled())
            SGC_ROOT3(frame_ptr, env, formals, body);
        gc_collect(frame_ptr);
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
    return self->as_closure.env;
}

obj_t *
closure_formals(obj_t *self)
{
    return self->as_closure.formals;
}

obj_t *
closure_body(obj_t *self)
{
    return self->as_closure.body;
}

// Static functions

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

