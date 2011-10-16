
#include <stdio.h>
#include <stdlib.h>
#include "rl.h"
#include "seval.h"
#include "sgc.h"

extern void sparse_do_string(char *);
extern void sparse_do_file(FILE *);
extern obj_t *sparse_get_expr();

int g_argc;
char **g_argv;

static void
repl()
{
    obj_t *expr, *retval;
    char *line;
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);
    while (1) {
        if (!(expr = sparse_get_expr())) {
            line = rl_getstr(">>> ");
            if (!line)
                break;
            sparse_do_string(line);
            if (!(expr = sparse_get_expr())) {
                continue;
            }
        }
        *frame_ref(frame, 0) = expr;
        retval = eval_frame(frame);
        if (get_type(retval) != TP_UNSPECIFIED) {
            print_repr(retval, stdout);
            puts("");
        }
        if (gc_want_collect())
            gc_collect(frame);
    }
}

static void
run_file()
{
    obj_t *expr;
    obj_t **frame = frame_extend(gc_get_stack_base(), 1,
                                 FR_SAVE_PREV | FR_EXTEND_ENV);

    FILE *fp = fopen(g_argv[1], "r");
    sparse_do_file(fp);
    fclose(fp);
    while ((expr = sparse_get_expr())) {
        *frame_ref(frame, 0) = expr;
        eval_frame(frame);
    }
}


int
main(int argc, char **argv)
{
    seval_init();

    g_argc = argc;
    g_argv = argv;

    if (argc == 1)
        repl();
    else if (argc == 2)
        run_file();
    return 0;
}

