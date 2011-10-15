
#include <stdio.h>
#include <stdlib.h>
#include "rl.h"
#include "seval.h"
#include "sgc.h"

extern obj_t *scm_parse_string(char *);

void
repl()
{
    obj_t *expr;
    char *line;
    obj_t **frame = gc_get_stack_base();

    frame = new_frame(frame, 1);

    while (1) {
        line = rl_getstr(">>> ");
        if (!line)
            break;

        expr = scm_parse_string(line);
        frame[2] = expr;
        expr = eval_frame(frame);
        print_repr(expr, stdout);
        puts("");

        if (gc_want_collect())
            gc_collect(frame);
    }
}

int
main(int argc, char **argv)
{
    seval_init();
    if (argc == 1)
        repl();
    return 0;
}

