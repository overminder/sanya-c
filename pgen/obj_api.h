#ifndef OBJ_API_H
#define OBJ_API_H
#include "sgc.h"
#include "sobj.h"

static obj_t *
make_symbol(const char *s)
{
    return symbol_intern(NULL, s);
}

static obj_t *
make_fixnum(const char *s)
{
    return fixnum_wrap(NULL, atoi(s));
}

static obj_t *
make_pair(obj_t *car, obj_t *cdr)
{
    return pair_wrap(NULL, car, cdr);
}

static obj_t *
make_nil()
{
    return nil_wrap();
}


#endif /* OBJ_API_H */
