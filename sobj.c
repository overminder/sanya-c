
#include <stdlib.h>
#include <string.h>
#include "sgc.h"
#include "sobj.h"

// Uncomment this when testing collector.
//#define ALWAYS_COLLECT

static header_obj_t w_nil;
static header_obj_t w_true;
static header_obj_t w_false;
static header_obj_t w_unspec;
static header_obj_t w_eofobj;
static obj_t *symbol_table = NULL;

static obj_t *default_gc_visitor(obj_t *self);
static void default_gc_finalizer(obj_t *self);
static obj_t *pair_gc_visitor(obj_t *self);
static obj_t *closure_gc_visitor(obj_t *self);
static obj_t *vector_gc_visitor(obj_t *self);
static obj_t *environ_gc_visitor(obj_t *self);
static obj_t *dict_gc_visitor(obj_t *self);

// Initialize gc visitors and finalizers for each primitive type.
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
    w_eofobj.ob_type = TP_EOFOBJ;

    gc_register_type(TP_PAIR, pair_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_SYMBOL, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_PROC, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_FIXNUM, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_FLONUM, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_STRING, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_CLOSURE, closure_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_NIL, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_VECTOR, vector_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_BOOLEAN, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_UNSPECIFIED, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_ENVIRON, environ_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_EOFOBJ, default_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_DICT, dict_gc_visitor, default_gc_finalizer);
    gc_register_type(TP_UDATA, default_gc_visitor, default_gc_finalizer);

    // Symbol table
    sgc_init();
    symbol_table = dict_wrap(gc_get_stack_base());
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
        case TP_EOFOBJ: return "eof";
    }
    NOT_REACHED();
}

bool_t to_boolean(obj_t *self)
{
    if (self == (obj_t *)&w_false)
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

    case TP_FLONUM: {
        // So as to print out a .0 everytime.
        double dval = flonum_unwrap(self);
        fprintf(stream, "%g", dval);
        if (dval == (long)dval)
            fprintf(stream, ".0");
        break;
    }

    case TP_STRING:
        fwrite(string_unwrap(self), 1, string_length(self), stream);
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
        fprintf(stream, "#(");
        if (vector_length(self) != 0) {
            size_t i, len;
            print_repr(*vector_ref(self, 0), stream);
            for (i = 1, len = vector_length(self); i < len; ++i) {
                fprintf(stream, " ");
                print_repr(*vector_ref(self, i), stream);
            }
        }
        fprintf(stream, ")");
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

    case TP_DICT:
        fprintf(stream, "#<hash-table (%d/%ld)>",
                self->as_dict.nb_items, vector_length(self->as_dict.vec));
        break;

    case TP_UDATA:
        fprintf(stream, "#<user-data>");
        break;

    case TP_EOFOBJ:
        fprintf(stream, "#EOF");
        break;

    default:
        NOT_REACHED();
    }
}

long
generic_hash(obj_t *self)
{
    union {
        long lval;
        double dval;
    } double2long;
    long hval;

    switch (get_type(self)) {
    case TP_PAIR:
        hval = (long)self;
        break;

    case TP_SYMBOL:
        hval = symbol_hash(self);
        break;

    case TP_PROC:
        hval = (long)self;
        break;

    case TP_FIXNUM:
        hval = fixnum_unwrap(self);
        break;

    case TP_FLONUM:
        double2long.dval = flonum_unwrap(self);
        hval = double2long.lval;
        break;

    case TP_STRING:
    case TP_CLOSURE:
    case TP_NIL:
    case TP_VECTOR:
    case TP_BOOLEAN:
    case TP_UNSPECIFIED:
    case TP_ENVIRON:
    case TP_UDATA:
    case TP_EOFOBJ:
        hval = (long)self;
        break;
    default:
        NOT_REACHED();
    }
    return hval;
}

bool_t
nullp(obj_t *self)
{
    return get_type(self) == TP_NIL;
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
flonump(obj_t *self)
{
    return get_type(self) == TP_FLONUM;
}

bool_t
stringp(obj_t *self)
{
    return get_type(self) == TP_STRING;
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

bool_t
vectorp(obj_t *self)
{
    return get_type(self) == TP_VECTOR;
}

bool_t
booleanp(obj_t *self)
{
    return get_type(self) == TP_BOOLEAN;
}

bool_t
unspecp(obj_t *self)
{
    return get_type(self) == TP_UNSPECIFIED;
}

bool_t
environp(obj_t *self)
{
    return get_type(self) == TP_ENVIRON;
}

bool_t
eofobjp(obj_t *self)
{
    return get_type(self) == TP_EOFOBJ;
}


void
fatal_error(const char *msg, obj_t **frame)
{
    fprintf(stderr, "FATAL -- %s\n", msg);
    if (frame)
        gc_stack_trace_lite(frame);
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

obj_t *
eofobj_wrap()
{
    return (obj_t *)&w_eofobj;
}

// Fixnum
obj_t *
fixnum_wrap(obj_t **frame, long ival)
{
    obj_t *self = gc_malloc(sizeof(fixnum_obj_t), TP_FIXNUM);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(fixnum_obj_t), TP_FIXNUM);
        if (!self)
            fatal_error("out of memory", frame);
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
        fatal_error("not a fixnum", NULL);
}

// Flonum
obj_t *
flonum_wrap(obj_t **frame, double dval)
{
    obj_t *self = gc_malloc(sizeof(flonum_obj_t), TP_FLONUM);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(flonum_obj_t), TP_FLONUM);
        if (!self)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_flonum.val = dval;
    return self;
}

double
flonum_unwrap(obj_t *self)
{
    if (flonump(self))
        return self->as_flonum.val;
    else if (fixnump(self))
        return fixnum_unwrap(self);
    else
        fatal_error("not a number", NULL);
}

// Pair
obj_t *
pair_wrap(obj_t **frame, obj_t *car, obj_t *cdr)
{
    obj_t *self = gc_malloc(sizeof(pair_obj_t), TP_PAIR);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        SGC_ROOT2(frame, car, cdr);
        gc_collect(frame);
        self = gc_malloc(sizeof(pair_obj_t), TP_PAIR);
        if (!self)
            fatal_error("out of memory", frame);
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
        fatal_error("not a pair", NULL);
}

obj_t *
pair_cdr(obj_t *self)
{
    if (pairp(self))
        return self->as_pair.cdr;
    else
        fatal_error("not a pair", NULL);
}

void
pair_set_car(obj_t *self, obj_t *car)
{
    if (pairp(self))
        self->as_pair.car = car;
    else
        fatal_error("not a pair", NULL);
}

void
pair_set_cdr(obj_t *self, obj_t *cdr)
{
    if (pairp(self))
        self->as_pair.cdr = cdr;
    else
        fatal_error("not a pair", NULL);
}

// Symbol
static obj_t *
symbol_wrap(obj_t **frame, const char *sval)
{
    size_t slen = strlen(sval);
    obj_t *self = gc_malloc(sizeof(symbol_obj_t) + slen, TP_SYMBOL);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(symbol_obj_t) + slen, TP_SYMBOL);
        if (!self)
            fatal_error("out of memory", NULL);
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
        fatal_error("not a symbol", NULL);
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
symbol_intern(obj_t **frame, const char *sval)
{
    obj_t *symb = symbol_wrap(frame, sval);
    obj_t *got = dict_lookup(frame, symbol_table, symb, DL_CREATE_ON_ABSENT);
    symb = pair_car(got);
    return symb;
}

bool_t
symbol_eq(obj_t *self, obj_t *other)
{
    if (self == other)
        return 1;
    if (symbol_hash(self) != symbol_hash(other))
        return 0;
    
    const char *s_self, *s_other;
    size_t i, len_self, len_other;

    s_self = symbol_unwrap(self);
    s_other = symbol_unwrap(other);
    len_self = strlen(s_self);
    len_other = strlen(s_other);
    if (len_self != len_other)
        return 0;

    for (i = 0; i < len_self; ++i) {
        if (s_self[i] != s_other[i])
            return 0;
    }

    return 1;
}

obj_t *
string_wrap(obj_t **frame, const char *sval, size_t len)
{
    obj_t *self = gc_malloc(sizeof(string_obj_t) + len, TP_STRING);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(string_obj_t) + len, TP_STRING);
        if (!self)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    memcpy(self->as_string.val, sval, len);
    self->as_string.val[len] = '\0';
    self->as_string.length = len;
    return self;
}

const char *
string_unwrap(obj_t *self)
{
    return self->as_string.val;
}

size_t
string_length(obj_t *self)
{
    return self->as_string.length;
}

bool_t
string_eq(obj_t *self, obj_t *other)
{
    size_t i, len;

    if ((len = string_length(self)) != string_length(other))
        return 0;
    
    for (i = 0; i < len; ++i) {
        if (self->as_string.val[i] != other->as_string.val[i])
            return 0;
    }
    return 1;
}

// Proc
obj_t *
proc_wrap(obj_t **frame, sobj_funcptr_t func)
{
    obj_t *self = gc_malloc(sizeof(proc_obj_t), TP_PROC);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(proc_obj_t), TP_PROC);
        if (!self)
            fatal_error("out of memory", frame);
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
        fatal_error("not a procedure", NULL);
}

obj_t *
closure_wrap(obj_t **frame, obj_t *env, obj_t *formals, obj_t *body)
{
    obj_t *self = gc_malloc(sizeof(closure_obj_t), TP_CLOSURE);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        SGC_ROOT3(frame, env, formals, body);
        gc_collect(frame);
        self = gc_malloc(sizeof(closure_obj_t), TP_CLOSURE);
        if (!self)
            fatal_error("out of memory", frame);
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
        fatal_error("not a closure", NULL);
}

obj_t *
closure_formals(obj_t *self)
{
    if (closurep(self))
        return self->as_closure.formals;
    else
        fatal_error("not a closure", NULL);
}

obj_t *
closure_body(obj_t *self)
{
    if (closurep(self))
        return self->as_closure.body;
    else
        fatal_error("not a closure", NULL);
}

obj_t *
vector_wrap(obj_t **frame, size_t nb_alloc, obj_t *fill)
{
    obj_t *self;
    size_t i;
    self = gc_malloc(sizeof(vector_obj_t) +
                     sizeof(obj_t *) * nb_alloc, TP_VECTOR);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        SGC_ROOT1(frame, fill);
        gc_collect(frame);
        self = gc_malloc(sizeof(vector_obj_t) +
                         sizeof(obj_t *) * nb_alloc, TP_VECTOR);
        if (!self)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    self->as_vector.nb_alloc = nb_alloc;
    for (i = 0; i < nb_alloc; ++i) {
        self->as_vector.data[i] = fill;
    }
    return self;
}

obj_t *
vector_from_list(obj_t **frame, obj_t *lis)
{
    // First pass -- Calculate the length
    size_t list_len = 0;
    obj_t *iter = lis;

    while (pairp(iter)) {
        ++list_len;
        iter = pair_cdr(iter);
    }
    if (!nullp(iter)) {
        fatal_error("list->vector -- not a well-formed list", frame);
    }

    // Second pass -- make a list and do copy
    SGC_ROOT1(frame, lis);
    obj_t *self = vector_wrap(frame, list_len, nil_wrap());
    size_t i = 0;
    for (; i < list_len; ++i, lis = pair_cdr(lis)) {
        *vector_ref(self, i) = pair_car(lis);
    }
    return self;
}


obj_t **
vector_ref(obj_t *self, long index)
{
    return self->as_vector.data + index;
}

size_t
vector_length(obj_t *self)
{
    return self->as_vector.nb_alloc;
}

obj_t *
vector_to_list(obj_t **frame, obj_t *self)
{
    obj_t *lis = nil_wrap();
    long i = vector_length(self) - 1;
    --frame;
    for (; i >= 0; --i) {
        *frame = lis;
        lis = pair_wrap(frame, *vector_ref(self, i), lis);
    }
    return lis;
}

// Accessor macros for environ
#define ENV_CAR(self) (self->as_environ.car)
#define ENV_CDR(self) (self->as_environ.cdr)

obj_t *
environ_wrap(obj_t **frame, obj_t *outer)
{
    obj_t *self = gc_malloc(sizeof(environ_obj_t), TP_ENVIRON);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        SGC_ROOT1(frame, outer);
        gc_collect(frame);
        self = gc_malloc(sizeof(environ_obj_t), TP_ENVIRON);
        if (!self)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    SGC_ROOT2(frame, self, outer);
    ENV_CAR(self) = NULL;
    ENV_CDR(self) = NULL;

    ENV_CAR(self) = dict_wrap(frame);
    ENV_CDR(self) = outer;
    return self;
}

obj_t *
environ_set(obj_t *self, obj_t *key, obj_t *value)
{
    obj_t *binding = dict_lookup(NULL, self, key, DL_DEFAULT);
    if (binding) {
        pair_set_cdr(binding, value);
    }
    else {
        fatal_error("set! -- No such binding", NULL);
    }
    return binding;
}

obj_t *
environ_lookup(obj_t *self, obj_t *key, enum environ_lookup_flag flag)
{
    obj_t *binding;

    while (!nullp(self)) {
        binding = dict_lookup(NULL, ENV_CAR(self), key, DL_DEFAULT);
        if (binding)
            return binding;

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

    SGC_ROOT1(frame, value);
    binding = dict_lookup(frame, ENV_CAR(self), key, DL_CREATE_ON_ABSENT);
    pair_set_cdr(binding, value);

    return binding;
}

#define DICT_INIT_SIZE 8

obj_t *
dict_wrap(obj_t **frame)
{
    obj_t *self = gc_malloc(sizeof(dict_obj_t), TP_DICT);
#ifndef ALWAYS_COLLECT
    if (!self) {
#endif
        gc_collect(frame);
        self = gc_malloc(sizeof(dict_obj_t), TP_DICT);
        if (!self)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    SGC_ROOT1(frame, self);
    self->as_dict.nb_items = 0;
    self->as_dict.hash_mask = DICT_INIT_SIZE - 1;
    self->as_dict.vec = NULL;
    self->as_dict.vec = vector_wrap(frame, DICT_INIT_SIZE, nil_wrap());
    return self;
}

bool_t
dictp(obj_t *self)
{
    return get_type(self) == TP_DICT;
}

size_t
dict_size(obj_t *self)
{
    return self->as_dict.nb_items;
}

static void
dict_rehash(obj_t **frame, obj_t *self, size_t target_size)
{
    // Firstly create a new dict
    obj_t *ndict = gc_malloc(sizeof(dict_obj_t), TP_DICT);
#ifndef ALWAYS_COLLECT
    if (!ndict) {
#endif
        SGC_ROOT1(frame, ndict);
        gc_collect(frame);
        ndict = gc_malloc(sizeof(dict_obj_t), TP_DICT);
        if (!ndict)
            fatal_error("out of memory", frame);
#ifndef ALWAYS_COLLECT
    }
#endif
    SGC_ROOT2(frame, self, ndict);
    ndict->as_dict.vec = vector_wrap(frame, target_size, nil_wrap());
    // Other fields are not important

    // Then rehash each item in the self.
    obj_t *self_vec, *new_vec;
    size_t vec_i, vec_len, hashval, hash_mask, new_bucket;
    obj_t *entry_list, *entry, *key;
    obj_t **new_entry_head;

    hash_mask = target_size - 1;
    self_vec = self->as_dict.vec;
    new_vec = ndict->as_dict.vec;
    for (vec_i = 0, vec_len = vector_length(self_vec);
            vec_i < vec_len; ++vec_i) {
        for (entry_list = *vector_ref(self_vec, vec_i);
                !nullp(entry_list); entry_list = pair_cdr(entry_list)) {
            entry = pair_car(entry_list);
            key = pair_car(entry);

            hashval = symbol_hash(key);
            new_bucket = hashval & hash_mask;

            new_entry_head = vector_ref(new_vec, new_bucket);
            *new_entry_head = pair_wrap(frame, entry, *new_entry_head);
        }
    }
    self->as_dict.vec = new_vec;
    self->as_dict.hash_mask = hash_mask;
}

obj_t *
dict_lookup(obj_t **frame, obj_t *self, obj_t *key, enum dict_lookup_flag fl)
{
    obj_t **entry_ref;
    obj_t *entry_list, *entry, *prev_entry_list;
    size_t hashval, index;

    hashval = symbol_hash(key);
    index = hashval & self->as_dict.hash_mask;
    entry_ref = vector_ref(self->as_dict.vec, index);

    prev_entry_list = NULL;
    entry_list = *entry_ref;
    while (!nullp(entry_list)) {
        entry = pair_car(entry_list);
        if (symbol_eq(pair_car(entry), key)) {
            goto found_entry;
        }
        prev_entry_list = entry_list;
        entry_list = pair_cdr(entry_list);
    }

// Not found
    if (fl & DL_CREATE_ON_ABSENT) {
        SGC_ROOT1(frame, key);
        entry_list = *entry_ref;
        entry = pair_wrap(frame, key, nil_wrap());
        *entry_ref = pair_wrap(frame, entry, entry_list);

        self->as_dict.nb_items += 1;
        // :: Insert code here to rehash and expand the vector.
        if (self->as_dict.nb_items > 1.5 * vector_length(self->as_dict.vec)) {
            dict_rehash(frame, self, (1 + self->as_dict.hash_mask) * 2);
        }

        return entry;
    }
    else {
        return NULL;
    }

found_entry:
    if (fl & DL_POP_IF_FOUND) {
        if (prev_entry_list) {
            pair_set_cdr(prev_entry_list, pair_cdr(entry_list));
        }
        else {
            // The found entry is the first entry.
            *entry_ref = pair_cdr(entry_list);
        }
        self->as_dict.nb_items -= 1;
        // :: Insert code here to rehash and shrink the vector.
        if (self->as_dict.nb_items < 0.3 * vector_length(self->as_dict.vec) &&
                vector_length(self->as_dict.vec) > DICT_INIT_SIZE * 2) {
            dict_rehash(frame, self, (1 + self->as_dict.hash_mask) / 2);
        }
    }
    return entry;
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

static obj_t *
vector_gc_visitor(obj_t *self)
{
    size_t i, len;
    for (i = 0, len = vector_length(self); i < len; ++i) {
        gc_mark(*vector_ref(self, i));
    }
    return NULL;
}

static obj_t *
environ_gc_visitor(obj_t *self)
{
    gc_mark(ENV_CAR(self));
    return ENV_CDR(self);
}

static obj_t *
dict_gc_visitor(obj_t *self)
{
    return self->as_dict.vec;
}

