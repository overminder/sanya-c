#ifndef SOBJ_H
#define SOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// Simple debug macro
// @see fatal_error
#define NOT_REACHED() \
    fprintf(stderr, "FATAL: NOT REACHED %s:%s:%d\n", \
            __FILE__, __func__, __LINE__); \
    exit(1)

typedef uint8_t type_t;
typedef uint8_t bool_t;

#define TP_PAIR         1
#define TP_SYMBOL       2
#define TP_PROC         3
#define TP_FIXNUM       4
#define TP_FLONUM       5
#define TP_CLOSURE      6
#define TP_NIL          7
#define TP_VECTOR       8
#define TP_BOOLEAN      9
#define TP_UNSPECIFIED  10
#define TP_ENVIRON      11
#define TP_UDATA        12
#define TP_MAX          TP_UDATA

typedef struct obj_t obj_t;

typedef struct {
    long val;
} fixnum_obj_t;

typedef struct {
    double val;
} flonum_obj_t;

typedef struct {
    obj_t *car;
    obj_t *cdr;
} pair_obj_t;

typedef pair_obj_t environ_obj_t;

typedef struct {
    long hash;
    char val[1];
} symbol_obj_t;

typedef obj_t * (*sobj_funcptr_t) (obj_t **);

typedef struct {
    sobj_funcptr_t func;
} proc_obj_t;

typedef struct {
    obj_t *env;
    obj_t *formals;
    obj_t *body;
} closure_obj_t;

typedef struct {
    size_t nb_alloc;
    obj_t *data[1];
} vector_obj_t;

// 16-byte for each object...
#define OB_HEADER \
    obj_t * gc_next; \
    bool_t gc_marked; \
    type_t ob_type; \
    bool_t ob_allocated; \
    bool_t ob_bar; \
    uint32_t ob_size

typedef struct {
    OB_HEADER;
} header_obj_t;

struct obj_t {
    OB_HEADER;
    union {
        fixnum_obj_t as_fixnum;
        flonum_obj_t as_flonum;
        pair_obj_t as_pair;
        symbol_obj_t as_symbol;
        proc_obj_t as_proc;
        closure_obj_t as_closure;
        vector_obj_t as_vector;
        environ_obj_t as_environ;
    };
};

// initializer
void sobj_init();

// Generic
type_t get_type(obj_t *self);
const char *get_typename(obj_t *self);
bool_t to_boolean(obj_t *self);
void print_repr(obj_t *self, FILE *stream);

// Type predicates
bool_t nullp(obj_t *self);
bool_t pairp(obj_t *self);
bool_t symbolp(obj_t *self);
bool_t fixnump(obj_t *self);
bool_t procedurep(obj_t *self);
bool_t closurep(obj_t *self);
bool_t vectorp(obj_t *self);
bool_t environp(obj_t *self);

// Simple debug func
// @see NOT_REACHED
void fatal_error(const char *msg) __attribute__((noreturn));

// Consts
obj_t *nil_wrap();
obj_t *boolean_wrap(bool_t bval);
obj_t *unspec_wrap();

// Fixnum
obj_t *fixnum_wrap(obj_t **frame_ptr, long ival);
long fixnum_unwrap(obj_t *self);

// Flonum
obj_t *flonum_wrap(obj_t **frame_ptr, double dval);
double flonum_unwrap(obj_t *self);

// Pair
obj_t *pair_wrap(obj_t **frame_ptr, obj_t *car, obj_t *cdr);
obj_t *pair_car(obj_t *self);
obj_t *pair_cdr(obj_t *self);
void pair_set_car(obj_t *self, obj_t *car);
void pair_set_cdr(obj_t *self, obj_t *cdr);

// Symbol
const char *symbol_unwrap(obj_t *self);
long symbol_hash(obj_t *self);
obj_t *symbol_intern(obj_t **frame_ptr, const char *sval);
bool_t symbol_eq(obj_t *self, obj_t *other);

// Proc
obj_t *proc_wrap(obj_t **frame_ptr, sobj_funcptr_t func);
sobj_funcptr_t proc_unwrap(obj_t *self);

// Closure
obj_t *closure_wrap(obj_t **frame_ptr, obj_t *env, obj_t *formals, obj_t *body);
obj_t *closure_env(obj_t *self);
obj_t *closure_formals(obj_t *self);
obj_t *closure_body(obj_t *self);

// Vector
obj_t *vector_wrap(obj_t **frame_ptr, size_t nb_alloc, obj_t *fill);
obj_t **vector_ref(obj_t *self, long index);
size_t vector_length(obj_t *self);

// Environment
enum environ_lookup_flag {
    EL_DONT_LOOK_OUTER,
    EL_LOOK_OUTER
};
// Implementation details:
//   Currently the environment is implemented as a list.
//   e.g., (env . outer-env)
//   Where env is ((key . value) (key2 . value2) ...)
obj_t *environ_wrap(obj_t **frame_ptr, obj_t *outer);
obj_t *environ_set(obj_t *self, obj_t *key, obj_t *val);
obj_t *environ_lookup(obj_t *self, obj_t *key, enum environ_lookup_flag);
obj_t *environ_def(obj_t **frame, obj_t *self, obj_t *key, obj_t *value);
obj_t *environ_bind(obj_t **frame, obj_t *self, obj_t *key, obj_t *value);

#endif /* SOBJ_H */
