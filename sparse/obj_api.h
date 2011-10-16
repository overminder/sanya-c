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

static obj_t *
make_true()
{
    return boolean_wrap(1);
}

static obj_t *
make_false()
{
    return boolean_wrap(0);
}

static obj_t *
make_quoted(obj_t *item)
{
    return make_pair(make_symbol("quote"), make_pair(item, make_nil()));
}

#endif /* OBJ_API_H */
