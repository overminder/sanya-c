#ifndef SOBJ_H
#define SOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <setjmp.h>

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

#define TP_STRING       6
#define TP_CLOSURE      7
#define TP_NIL          8
#define TP_VECTOR       9
#define TP_BOOLEAN      10

#define TP_UNSPECIFIED  11
#define TP_ENVIRON      12
#define TP_EOFOBJ       13
#define TP_DICT         14
#define TP_SPECFORM     15

#define TP_MACRO        16
#define TP_ECONT        17
#define TP_UDATA        18
#define TP_MAX          TP_UDATA

typedef struct obj_t obj_t;

typedef struct {
    long val;
} fixnum_obj_t;

typedef struct {
    double val;
} flonum_obj_t;

typedef struct {
    size_t length;
    char val[1];
} string_obj_t;

typedef struct {
    obj_t *car;
    obj_t *cdr;
} pair_obj_t;

typedef struct {
    long hash;
    char val[1];
} symbol_obj_t;

// For primitive procedures
typedef obj_t * (*sobj_funcptr_t) (obj_t **);
// For special forms
typedef obj_t * (*sobj_funcptr2_t) (obj_t **, obj_t **);

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

typedef pair_obj_t environ_obj_t;

typedef struct {
    uint32_t nb_items;
    uint32_t hash_mask;
    obj_t *vec;
} dict_obj_t;

typedef struct {
    sobj_funcptr2_t call;
} specform_obj_t;

typedef struct {
    obj_t *rules;  // rule implemented as closure
} macro_obj_t;

// Escaping continuation
typedef struct {
    void *env; 
    obj_t *aux;
} econt_obj_t;

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
        string_obj_t as_string;
        proc_obj_t as_proc;
        closure_obj_t as_closure;
        vector_obj_t as_vector;
        environ_obj_t as_environ;
        dict_obj_t as_dict;
        specform_obj_t as_specform;
        macro_obj_t as_macro;
        econt_obj_t as_econt;
    };
};

// initializer
void sobj_init();

// Generic
type_t get_type(obj_t *self);
const char *get_typename(obj_t *self);
bool_t to_boolean(obj_t *self);
void print_repr(obj_t *self, FILE *stream);
long generic_hash(obj_t *self);
bool_t generic_eq(obj_t *a, obj_t *b);

// Type predicates, most of them are around the corresponding type's decl
bool_t nullp(obj_t *self);
bool_t booleanp(obj_t *self);
bool_t unspecp(obj_t *self);
bool_t eofobjp(obj_t *self);

// Simple debug func
// @see NOT_REACHED
void fatal_error(const char *msg, obj_t **frame) __attribute__((noreturn));

// Consts
obj_t *nil_wrap();
obj_t *boolean_wrap(bool_t bval);
obj_t *unspec_wrap();
obj_t *eofobj_wrap();

// Fixnum
obj_t *fixnum_wrap(obj_t **frame, long ival);
bool_t fixnump(obj_t *self);
long fixnum_unwrap(obj_t *self);

// Flonum, note that value of fixnum will also be accessable from flonum_uw
obj_t *flonum_wrap(obj_t **frame, double dval);
bool_t flonump(obj_t *self);
double flonum_unwrap(obj_t *self);

// Pair
obj_t *pair_wrap(obj_t **frame, obj_t *car, obj_t *cdr);
bool_t pairp(obj_t *self);
obj_t *pair_copy_list(obj_t **frame, obj_t *self);
obj_t *pair_car(obj_t *self);
obj_t *pair_cdr(obj_t *self);
void pair_set_car(obj_t *self, obj_t *car);
void pair_set_cdr(obj_t *self, obj_t *cdr);

// Symbol
bool_t symbolp(obj_t *self);
obj_t *symbol_intern(obj_t **frame, const char *sval);
const char *symbol_unwrap(obj_t *self);
long symbol_hash(obj_t *self);
bool_t symbol_eq(obj_t *self, obj_t *other);

// String
obj_t *string_wrap(obj_t **frame, const char *sval, size_t len);
bool_t stringp(obj_t *self);
const char *string_unwrap(obj_t *self);
size_t string_length(obj_t *self);
bool_t string_eq(obj_t *self, obj_t *other);

// Proc
obj_t *proc_wrap(obj_t **frame, sobj_funcptr_t func);
bool_t procedurep(obj_t *self);
sobj_funcptr_t proc_unwrap(obj_t *self);

// Closure
obj_t *closure_wrap(obj_t **frame, obj_t *env, obj_t *formals, obj_t *body);
bool_t closurep(obj_t *self);
obj_t *closure_env(obj_t *self);
obj_t *closure_formals(obj_t *self);
obj_t *closure_body(obj_t *self);

// Vector
obj_t *vector_wrap(obj_t **frame, size_t nb_alloc, obj_t *fill);
bool_t vectorp(obj_t *self);
obj_t *vector_from_list(obj_t **frame, obj_t *lis);
obj_t **vector_ref(obj_t *self, long index);
size_t vector_length(obj_t *self);
obj_t *vector_to_list(obj_t **frame, obj_t *self);

// Environment
enum environ_lookup_flag {
    EL_DONT_LOOK_OUTER,
    EL_LOOK_OUTER
};
// Implementation details:
//   Currently the environment is implemented as a list.
//   e.g., (env . outer-env)
//   Where env is ((key . value) (key2 . value2) ...)
obj_t *environ_wrap(obj_t **frame, obj_t *outer);
bool_t environp(obj_t *self);
obj_t *environ_set(obj_t *self, obj_t *key, obj_t *val);
obj_t *environ_lookup(obj_t *self, obj_t *key, enum environ_lookup_flag);
obj_t *environ_def(obj_t **frame, obj_t *self, obj_t *key, obj_t *value);
obj_t *environ_bind(obj_t **frame, obj_t *self, obj_t *key, obj_t *value);

// *Hashtables*, which indeed increased the lookup speed by 20%.
enum dict_lookup_flag {
    DL_DEFAULT = 0,
    DL_CREATE_ON_ABSENT = 1,
    DL_POP_IF_FOUND = 2
};

// By default the initial size is 8.
obj_t *dict_wrap(obj_t **frame);
bool_t dictp(obj_t *self);
size_t dict_size(obj_t *self);
// If not found, it will return NULL.
obj_t *dict_lookup(obj_t **frame, obj_t *self,
                   obj_t *key, enum dict_lookup_flag);
// Get a list of keys
obj_t *dict_get_keys(obj_t **frame, obj_t *self);

// Language construct and macro
bool_t syntaxp(obj_t *self);

obj_t *specform_wrap(obj_t **frame, sobj_funcptr2_t call);
bool_t specformp(obj_t *self);
sobj_funcptr2_t specform_unwrap(obj_t *self);

// Macros
obj_t *macro_wrap(obj_t **frame, obj_t *rule);
bool_t macrop(obj_t *self);
obj_t *macro_expand(obj_t **frame, obj_t *self, obj_t *args);

// Continuations (escaping...)
obj_t *econt_wrap(obj_t **frame, void *env);
bool_t continuationp(obj_t *self);
bool_t econtp(obj_t *self);
jmp_buf *econt_getenv(obj_t *self);
void econt_clear(obj_t *self);
obj_t *econt_getaux(obj_t *self);
void econt_putaux(obj_t *self, obj_t *thing);

#endif /* SOBJ_H */
