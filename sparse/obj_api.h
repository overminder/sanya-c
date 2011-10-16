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
make_string(const char *s, size_t len)
{
    obj_t *retval;
    size_t i, buf_i;
    char *buf = malloc(len);
    for (i = 0, buf_i = 0; i < len; ++i, ++buf_i) {
        if (s[i] == '\\') {
            ++i;
            switch (s[i]) {
                case 'n':
                    buf[buf_i] = '\n'; break;
                case 'r':
                    buf[buf_i] = '\r'; break;
                case 't':
                    buf[buf_i] = '\t'; break;
                default:
                    buf[buf_i] = s[i]; break;
            }
        }
        else {
            buf[buf_i] = s[i];
        }
    }
    retval = string_wrap(NULL, buf, buf_i);
    free(buf);
    return retval;
}

static obj_t *
make_fixnum(const char *s)
{
    return fixnum_wrap(NULL, atoi(s));
}

static obj_t *
make_flonum(const char *s)
{
    return flonum_wrap(NULL, atof(s));
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
